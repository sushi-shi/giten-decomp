"""Resumable unattended searches over the current near-exact MAX population.

Run in an isolated, clean worktree. Exact outputs are review candidates, never
source edits or ledger updates. status.json and per-target logs survive restarts.
"""
from __future__ import annotations

import argparse
from collections import Counter
import fcntl
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import time

from giten.permute.campaign import classified_candidates
from giten.permute.tu_state_noise import load_units, project_root
from giten.permute.upstream import write_checkpoint
from giten.verify.baseline import load as load_baseline


def population(bank, units, minimum):
    rows = []
    for (unit, symbol), entry in bank.items():
        if not minimum <= entry['best'] < 100 or entry['addr'] is None:
            continue
        rows.append({
            'unit': unit, 'symbol': symbol, 'rva': hex(entry['addr']),
            'max': entry['best'], 'hist': entry['hist'], 'source_hash': entry['fp'],
            'source': str(units.get(unit, {}).get('source', '')),
            'route': 'pending', 'state': 'pending', 'runs': [],
        })
    return sorted(rows, key=lambda row: (-row['max'], int(row['rva'], 0)))


def search_route(row, candidate):
    if row['hist'] >= 100:
        return 'history-recovery'
    if not candidate:
        return 'not-in-live-inventory'
    if candidate['classification'] != 'regalloc':
        return 'review-' + candidate['classification']
    return 'random' if Path(row['source']).suffix.lower() == '.c' else 'variants'


def route_population(rows, candidates):
    # Ledger and inventory format the same address with different zero padding.
    by_rva = {int(row['rva'], 0): row for row in candidates}
    for row in rows:
        candidate = by_rva.get(int(row['rva'], 0))
        row['classification'] = candidate['classification'] if candidate else 'unavailable'
        row['route'] = search_route(row, candidate)
        if row['route'] not in ('random', 'variants'):
            row['state'] = 'needs-review'


def collect_run(directory, route):
    path = directory / ('campaign.json' if route == 'random' else 'results.json')
    if not path.is_file():
        return {}
    document = json.loads(path.read_text())
    if route == 'random':
        return {'results': str(path), **document.get('totals', {}),
                'exact': document.get('exact'),
                'best_score': max((r['best']['score'] for r in document['rounds']
                                   if r.get('best')), default=None)}
    scored = [r for r in document['results'] if r.get('score') is not None]
    baseline = document['source_sha256']
    return {'results': str(path), 'attempted': document['attempted_variant_count'],
            'unique_scored_mutations': len({r['source_sha256'] for r in scored} - {baseline}),
            'distinct_target_states': len({r['state_id'] for r in scored}),
            'elapsed_seconds': document['elapsed_seconds'],
            'exact': document.get('exact_source'),
            'best_score': (document.get('best') or {}).get('score')}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--minimum', type=float, default=99.0)
    parser.add_argument('--passes', type=int, default=3)
    parser.add_argument('--scored', type=int, default=10000)
    parser.add_argument('--rounds', type=int, default=96)
    parser.add_argument('--trials', type=int, default=256)
    parser.add_argument('--chain-depth', type=int, default=8)
    parser.add_argument('--jobs', type=int, default=16)
    parser.add_argument('--frontier', type=int, default=4)
    parser.add_argument('--seed', type=lambda value: int(value, 0), default=0x20260928)
    parser.add_argument('--upstream', type=Path, default=os.environ.get('GITEN_DECOMP_PERMUTER'))
    parser.add_argument('--plan-only', action='store_true')
    parser.add_argument('--status', action='store_true')
    args = parser.parse_args(argv)
    root = project_root()
    output = (root / args.output).resolve()
    # Artifacts and recovery actions are confined to ignored build storage.
    if not output.is_relative_to(root / 'build'):
        parser.error('--output must be inside this worktree\'s build directory')
    if args.status:
        print((output / 'status.json').read_text())
        return 0
    if not 0 <= args.minimum < 100 or min(args.passes, args.scored, args.rounds,
                                         args.trials, args.chain_depth, args.jobs,
                                         args.frontier) < 1:
        parser.error('invalid population or search bounds')
    if not args.upstream:
        parser.error('use nix develop or --upstream')
    output.mkdir(parents=True, exist_ok=True)
    lock = (output / '.lock').open('a')
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        parser.error('this mining harness is already running')
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
    if subprocess.check_output(['git', 'status', '--porcelain'], cwd=root, text=True).strip():
        parser.error('use a clean isolated worktree; finish or remove interrupted source probes first')
    settings = {key: getattr(args, key) for key in (
        'minimum', 'passes', 'scored', 'rounds', 'trials', 'chain_depth', 'jobs', 'frontier', 'seed')}
    settings['upstream'] = str(args.upstream.resolve())
    status_path = output / 'status.json'
    if status_path.exists():
        document = json.loads(status_path.read_text())
        if document['revision'] != revision or document['configuration'] != settings:
            parser.error('resume requires the same revision and configuration; use a new output otherwise')
    else:
        document = {'schema': 1, 'revision': revision, 'configuration': settings,
                    'created_at': time.time(), 'phase': 'build', 'targets': []}

    stopped = False
    child = None

    def interrupt(_signum, _frame):
        nonlocal stopped
        stopped = True
        if child and child.poll() is None:
            child.terminate()

    signal.signal(signal.SIGTERM, interrupt)
    signal.signal(signal.SIGINT, interrupt)

    def save():
        document['updated_at'] = time.time()
        document['pid'] = os.getpid()
        document['counts'] = dict(Counter(row['state'] for row in document['targets']))
        write_checkpoint(status_path, document)

    def run(command, log, on_progress=None):
        nonlocal child, stopped
        print('running:', ' '.join(command), flush=True)
        with log.open('a') as stream:
            child = subprocess.Popen(command, cwd=root, stdout=stream,
                                     stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
            while child.poll() is None:
                if (output / 'STOP').exists() and not stopped:
                    interrupt(signal.SIGTERM, None)
                if on_progress:
                    on_progress()
                time.sleep(2)
            code = child.returncode
            child = None
            return code

    save()
    if not document['targets'] and document['phase'] in ('interrupted', 'build-failed'):
        document['phase'] = 'build'
    if document['phase'] == 'build':
        code = run([sys.executable, '-m', 'giten', 'build', 'compare'], output / 'build.log')
        if stopped or code:
            document['phase'] = 'interrupted' if stopped else 'build-failed'
            save()
            return code or 130
        document['targets'] = population(load_baseline(), load_units(root), args.minimum)
        rvas = {int(row['rva'], 0) for row in document['targets']}
        candidates = classified_candidates(rvas=rvas) if rvas else []
        write_checkpoint(output / 'candidates.json', candidates)
        route_population(document['targets'], candidates)
        document['phase'] = 'ready'
        save()
    if args.plan_only:
        print(json.dumps(document['counts']), flush=True)
        return 0

    document['phase'] = 'running'
    save()
    for pass_number in range(1, args.passes + 1):
        for row in document['targets']:
            if row['route'] not in ('random', 'variants') or row['state'] in ('exact-candidate', 'error'):
                continue
            completed = next((entry for entry in row['runs'] if entry['pass'] == pass_number), None)
            if completed and completed['state'] == 'complete':
                continue
            if stopped or (output / 'STOP').exists():
                document['phase'] = 'interrupted'
                save()
                return 130
            if subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip() != revision:
                document['phase'] = 'inputs-changed'
                save()
                return 2
            stem = f"{row['unit']}-{int(row['rva'], 0):06x}"
            target = output / stem / f'pass-{pass_number:02d}'
            target.parent.mkdir(parents=True, exist_ok=True)
            seed = args.seed ^ int(row['rva'], 0) ^ (pass_number << 32)
            command = [sys.executable, '-m', 'giten', 'permute', row['route'],
                       row['source'], row['rva'], '--jobs', str(args.jobs),
                       '--frontier', str(args.frontier)]
            if row['route'] == 'random':
                command += ['--upstream', settings['upstream'], '--rounds', str(args.rounds),
                            '--trials', str(args.trials), '--scored', str(args.scored),
                            '--chain-depth', str(args.chain_depth), '--seed', str(seed),
                            '--stop-on-exact', '--output', str(target)]
                if target.exists():
                    command += ['--resume']
            else:
                if target.exists():
                    target.rename(target.with_name(target.name + f'.interrupted-{time.time_ns()}'))
                command += ['--min-depth', '0', '--max-depth', str(min(pass_number + 1, 3)),
                            '--state-trials', '32', '--state-seed', str(seed),
                            '--helper-name-count', '8', '--limit', str(args.scored),
                            '--wall-time-seconds', '14400', '--run',
                            '-o', str(target.with_suffix('.manifest.json')),
                            '--batch-output', str(target)]
            entry = completed or {'pass': pass_number, 'state': 'running'}
            if not completed:
                row['runs'].append(entry)
            row['state'] = 'running'
            document['active'] = {'rva': row['rva'], 'symbol': row['symbol'], 'pass': pass_number,
                                  'log': str(target.with_suffix('.log'))}

            def progress():
                try:
                    entry.update(collect_run(target, row['route']))
                except (OSError, ValueError, KeyError):
                    pass
                save()

            save()
            code = run(command, target.with_suffix('.log'), progress)
            progress()
            entry['exit_code'] = code
            entry['state'] = 'interrupted' if stopped else 'complete'
            if entry.get('exact'):
                row['state'] = 'exact-candidate'
            elif stopped:
                row['state'] = 'pending'
            elif code not in (0, 1):
                row['state'] = 'error'
            else:
                row['state'] = 'searched'
            save()
    document['phase'] = 'interrupted' if stopped else 'complete'
    document.pop('active', None)
    save()
    return 130 if stopped else 0


if __name__ == '__main__':
    raise SystemExit(main())
