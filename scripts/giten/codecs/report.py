"""Strict three-way result comparison with per-family coverage and counterexamples."""
from collections import Counter
import json
from pathlib import Path
import struct

FAMILIES = {1: 'crypt-record', 2: 'bmp-surface', 3: 'dib-surface', 4: 'mids-convert',
            5: 'area-map', 6: 'item-record', 7: 'bitmap-seek', 8: 'mids-load', 9: 'wave-copy', 10: 'raw-record', 11: 'text-token'}


def read_exact(file, size):
    data = file.read(size)
    if len(data) != size:
        raise ValueError(f'{file.name}: truncated results, expected {size}, got {len(data)} bytes')
    return data


def result(file):
    equal, size = struct.unpack('<II', read_exact(file, 8))
    if equal not in (0, 1) or size > 4 * 1024 * 1024:
        raise ValueError(f'{file.name}: invalid result header')
    return equal, read_exact(file, size)


def compare(out: Path):
    cases = json.loads((out / 'cases.json').read_text())
    summary = {family: {'cases': 0, 'candidate_disagreements': 0, 'rust_disagreements': 0}
               for family in FAMILIES.values()}
    differences = []
    with (out / 'retail.bin').open('rb') as retail, (out / 'rust.bin').open('rb') as rust:
        header = struct.pack('<II', 0x53455247, len(cases))
        for file in (retail, rust):
            if read_exact(file, 8) != header:
                raise ValueError(f'{file.name}: result count/magic does not match cases')
        for index, case in enumerate(cases):
            candidate_equal, expected = result(retail)
            candidate = expected if candidate_equal else result(retail)[1]
            _, actual = result(rust)
            counts = summary[FAMILIES[case['kind']]]
            counts['cases'] += 1
            counts['candidate_disagreements'] += not candidate_equal
            counts['rust_disagreements'] += expected != actual
            if not candidate_equal or expected != actual:
                first = next((i for i, (a, b) in enumerate(zip(expected, actual)) if a != b),
                             min(len(expected), len(actual)))
                differences.append({'index': index, **case, 'candidate_equal': bool(candidate_equal),
                                    'rust_equal': expected == actual, 'first_rust_difference': first,
                                    'retail_bytes': expected[first:first + 16].hex(),
                                    'rust_bytes': actual[first:first + 16].hex(),
                                    'first_candidate_difference': next((i for i, (a, b) in
                                        enumerate(zip(expected, candidate)) if a != b),
                                        min(len(expected), len(candidate))) if not candidate_equal else None})
                if len(differences) <= 20:
                    (out / f'mismatch-{index}-retail.bin').write_bytes(expected)
                    (out / f'mismatch-{index}-rust.bin').write_bytes(actual)
                    (out / f'mismatch-{index}-candidate.bin').write_bytes(candidate)
        for file in (retail, rust):
            if file.read(1):
                raise ValueError(f'{file.name}: trailing results')
    report = {'input_kinds': dict(Counter(
                'synthetic' if case['name'].startswith('control/') else
                'transformed' if 'removed-stream-ids' in case['name'] or 'unreferenced-as' in case['name'] else
                'original-resource' for case in cases)), 'families': summary, 'differences': differences, 'jobs': len(cases),
              'inventory': dict(Counter(r['family'] for r in json.loads((out / 'inventory.json').read_text())))}
    (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    for family, counts in summary.items():
        print(f'{family:18} {counts["cases"]:6} cases  candidate: {counts["candidate_disagreements"]:5}  Rust: {counts["rust_disagreements"]:5} differences')
    print(f'{len(differences)} differing jobs; report: {out / "report.json"}')
    return bool(differences) or any(not counts["cases"] for counts in summary.values())


if __name__ == '__main__':
    import sys
    raise SystemExit(compare(Path(sys.argv[1])))
