"""Run original-resource differential codecs (retail x candidate COFF x no_std Rust).

    giten codecs --disc build/codecs/DDSWIN.BIN

The disc is read in place. Resources and generated results stay under build/codecs.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

from giten.core.paths import REPO, BUILD
from giten.codecs.build import build, UNITS
from giten.codecs.corpus import prepare
from giten.codecs.report import compare
from giten.tool.wine import winepath


def sha256(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--disc', type=Path, required=True, help='original MODE1/2352 DDSWIN.BIN')
    args = parser.parse_args(argv)
    if not args.disc.is_file():
        parser.error(f'disc not found: {args.disc}')
    if not os.environ.get('MSVC_DIR') or not os.environ.get('GITEN_RETAIL_EXE'):
        parser.error('run inside nix develop')
    out = BUILD / 'codecs'
    # A failed rerun must not leave a previous successful report in place.
    out.mkdir(parents=True, exist_ok=True)
    (out / 'report.json').unlink(missing_ok=True)
    exe = build()
    retail = Path(os.environ['GITEN_RETAIL_EXE'])
    prepare(args.disc, retail, out)
    provenance = {
        'git_head': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=REPO, text=True).strip(),
        'retail_sha256': sha256(retail), 'relocated_image_sha256': sha256(BUILD / 'exe/DDS.EXE'),
        'disc_sha256': sha256(args.disc), 'jobs_sha256': sha256(out / 'jobs.bin'),
        'objects': {unit: sha256(BUILD / f'objdiff/base/{unit}.obj') for unit in UNITS},
        'source_files': {str(p.relative_to(REPO)): sha256(p) for root in
                         (REPO / 'src', REPO / 'include', REPO / 'scripts/giten/codecs', REPO / 'tools/giten-codec')
                         for p in sorted(root.rglob('*')) if p.is_file() and
                         p.suffix in ('.c', '.cpp', '.h', '.py', '.rs', '.toml', '.lock')},
        'scope': 'memory/API seams; no game startup, physical rendering, or audio playback',
    }
    env = {**os.environ, 'WINEDEBUG': '-all', 'CARGO_TARGET_DIR': str(out / 'rust')}
    with (out / 'retail.log').open('w') as log:
        result = subprocess.run(['wine', str(exe), winepath(BUILD / 'exe/DDS.EXE'),
                                 winepath(out / 'jobs.bin'), winepath(out / 'retail.bin')],
                                cwd=REPO, env=env, stdout=log, stderr=subprocess.STDOUT, timeout=600)
    if result.returncode not in (0, 1):
        raise RuntimeError(f'retail harness failed ({result.returncode}); see {out / "retail.log"}')
    with (out / 'rust.log').open('w') as log:
        subprocess.run(['cargo', 'run', '--offline', '--release', '--manifest-path',
                        str(REPO / 'tools/giten-codec/Cargo.toml'), '--features', 'oracle',
                        '--bin', 'codec-oracle', '--', str(out / 'jobs.bin'), str(out / 'rust.bin')],
                       cwd=REPO, env=env, stdout=log, stderr=subprocess.STDOUT, check=True, timeout=600)
    provenance['harness_sha256'] = sha256(exe)
    provenance['rust_oracle_sha256'] = sha256(out / 'rust/release/codec-oracle')
    (out / 'provenance.json').write_text(json.dumps(provenance, indent=2) + '\n')
    return int(compare(out))


if __name__ == '__main__':
    raise SystemExit(main())
