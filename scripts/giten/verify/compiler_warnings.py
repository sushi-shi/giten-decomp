"""Census written MSVC 5 warning locations in every configured unit.

`giten verify compiler-warnings` compiles the exact units.toml profiles into
disposable objects, captures cl.exe diagnostics, and writes derived reports
under build/gen. It does not install objects, compare, link, or enforce a
warning count. Every configured unit must compile before a report is published.
VC5 usually reports lines without columns; repeated diagnostics on one line
keep their compiler-order occurrence number in the TSV.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import io
import json
import os
import re
import tempfile
from collections import Counter
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path

from giten.core.paths import BUILD, CONFIG, REPO, dxsdk_dir, msvc_dir
from giten.graph.cc import coff_defect
from giten.graph.emit import load_units
from giten.tool import ToolError, cl
from giten.tool.wine import verify_prefix


REPORT = BUILD / "gen/compiler_warnings.tsv"
COVERAGE = BUILD / "gen/compiler_warnings_coverage.json"
_DIAGNOSTIC = re.compile(
    r"^(.+?)\((\d+)(?:,(\d+))?\)\s*:\s*"
    r"(warning|error|fatal error)\s+([A-Z]\d+)\s*:\s*(.*)$", re.I)
_CODED_DIAGNOSTIC = re.compile(
    r"\b(?:warning|error|fatal error)\s+[A-Z]\d+\s*:", re.I)
_DRIVE_PATH = re.compile(r"^[A-Za-z]:/")


@dataclass(frozen=True)
class Warning:
    unit: str
    file: str
    line: int
    column: int | None
    code: str
    occurrence: int
    message: str
    source_line: str


def _source_snapshot() -> dict[str, str]:
    paths = [CONFIG / "units.toml"]
    for root in (REPO / "src", REPO / "include"):
        if not root.is_dir():
            raise RuntimeError(f"missing source tree: {root}")
        paths.extend(path for path in root.rglob("*") if path.is_file())
    return {str(path.relative_to(REPO)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in sorted(paths)}


def _diagnostic_path(raw: str) -> tuple[str, Path | None]:
    spelling = raw.replace("\\", "/")
    if spelling[:3].lower() == "z:/":
        path = Path(spelling[2:])
    elif _DRIVE_PATH.match(spelling):
        return spelling, None
    else:
        path = REPO / spelling
    path = path.resolve()
    roots = (("", REPO), ("@msvc/", msvc_dir()),
             ("@dxsdk/", dxsdk_dir()))
    for prefix, root in roots:
        try:
            relative = path.relative_to(root.resolve())
        except ValueError:
            continue
        return prefix + relative.as_posix(), path if not prefix else None
    return path.as_posix(), None


def _source_line(path: Path | None, number: int) -> str:
    if path is None:
        return ""
    if not path.is_file():
        raise RuntimeError(f"compiler cited missing file: {path}")
    lines = path.read_bytes().splitlines()
    if number < 1 or number > len(lines):
        raise RuntimeError(f"compiler cited invalid line {path}:{number}")
    return lines[number - 1].decode("latin1").strip()


def _parse(unit: str, output: str) -> list[Warning]:
    warnings = []
    seen = Counter()
    for text in output.splitlines():
        match = _DIAGNOSTIC.match(text.strip())
        if match is None:
            if _CODED_DIAGNOSTIC.search(text):
                raise RuntimeError(f"{unit}: unparsed compiler diagnostic: {text}")
            continue
        raw_path, line, column, severity, code, message = match.groups()
        if severity.lower() != "warning":
            raise RuntimeError(f"{unit}: compiler {severity} {code}: {message}")
        filename, path = _diagnostic_path(raw_path)
        number = int(line)
        key = filename, number, code.upper()
        seen[key] += 1
        warnings.append(Warning(
            unit, filename, number, int(column) if column else None,
            code.upper(), seen[key], message.strip(), _source_line(path, number)))
    return warnings


def _compile(unit: dict, scratch: Path) -> tuple[str, list[Warning]]:
    source = (REPO / unit["source"]).resolve()
    try:
        source.relative_to(REPO)
    except ValueError as exc:
        raise RuntimeError(f"{unit['unit']}: source escapes repository") from exc
    if not source.is_file():
        raise RuntimeError(f"{unit['unit']}: missing source {source}")
    object_path = scratch / f"{unit['unit']}.obj"
    output = cl.compile(source, object_path, unit["cflags"], strict_status=True)
    if not object_path.is_file():
        raise RuntimeError(f"{unit['unit']}: cl produced no object")
    defect = coff_defect(object_path.read_bytes())
    if defect is not None:
        raise RuntimeError(f"{unit['unit']}: incomplete compiler object: {defect}")
    return unit["unit"], _parse(unit["unit"], output)


def _tsv(warnings: list[Warning]) -> str:
    stream = io.StringIO()
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(("unit", "file", "line", "column", "code", "occurrence",
                     "message", "source_line"))
    for row in warnings:
        writer.writerow((row.unit, row.file, row.line,
                         row.column if row.column is not None else "", row.code,
                         row.occurrence, row.message, row.source_line))
    return stream.getvalue()


def _publish(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", newline="",
                                     dir=path.parent, prefix=f".{path.name}.",
                                     delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(content)
    try:
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def scan(*, jobs: int = 4) -> tuple[list[Warning], dict]:
    REPORT.unlink(missing_ok=True)
    COVERAGE.unlink(missing_ok=True)
    verify_prefix()
    _, units = load_units()
    before = _source_snapshot()
    results = {}
    errors = []
    (BUILD / "gen").mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="compiler_warnings_",
                                     dir=BUILD / "gen") as temporary:
        with ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
            futures = {pool.submit(_compile, unit, Path(temporary)): unit["unit"]
                       for unit in units}
            for future in as_completed(futures):
                unit_name = futures[future]
                try:
                    name, warnings = future.result()
                    results[name] = warnings
                except (ToolError, OSError, RuntimeError) as exc:
                    errors.append(f"{unit_name}: {exc}")
    after = _source_snapshot()
    if before != after:
        errors.append("source tree or units.toml changed during warning census; rerun")
    expected = {unit["unit"] for unit in units}
    if set(results) != expected:
        errors.append(f"configured-unit coverage {len(results)}/{len(units)}")
    if errors:
        raise RuntimeError("\n".join(sorted(errors)))
    ordered = [warning for unit in units for warning in results[unit["unit"]]]
    coverage = {
        "complete": True,
        "configured_units": len(units),
        "compiled_units": len(results),
        "warnings": len(ordered),
        "external_warnings": sum(not warning.file.startswith(
            ("src/", "include/", "vendor/")) for warning in ordered),
        "warning_codes": dict(sorted(Counter(
            warning.code for warning in ordered).items())),
        "msvc_dir": str(msvc_dir().resolve()),
        "dxsdk_dir": str(dxsdk_dir().resolve()),
        "source_snapshot_sha256": hashlib.sha256(
            json.dumps(before, sort_keys=True).encode()).hexdigest(),
        "units": [{"unit": unit["unit"], "source": unit["source"],
                   "profile": unit["flags"], "flags": unit["cflags"],
                   "warnings": len(results[unit["unit"]])} for unit in units],
    }
    try:
        _publish(REPORT, _tsv(ordered))
        _publish(COVERAGE, json.dumps(coverage, indent=2) + "\n")
    except OSError:
        REPORT.unlink(missing_ok=True)
        COVERAGE.unlink(missing_ok=True)
        raise
    return ordered, coverage


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=4,
                        help="concurrent compiler invocations (default: 4)")
    parser.add_argument("--list", metavar="FILTER", nargs="?", const="",
                        help="print warning sites; filter by file, unit or code")
    args = parser.parse_args(argv)
    try:
        warnings, coverage = scan(jobs=args.jobs)
    except (ToolError, OSError, RuntimeError, SystemExit) as exc:
        print(f"[compiler-warnings] FATAL: {exc}")
        return 2
    if args.list is not None:
        for row in warnings:
            if not args.list or any(args.list in value for value in
                                    (row.file, row.unit, row.code)):
                print(f"{row.file}:{row.line} [{row.unit}] {row.code} "
                      f"#{row.occurrence}: {row.message}")
    codes = Counter(row.code for row in warnings)
    summary = ", ".join(f"{code}={count}" for code, count in sorted(codes.items()))
    print(f"[compiler-warnings] {coverage['compiled_units']}/"
          f"{coverage['configured_units']} units; {len(warnings)} warning(s)"
          f"{': ' + summary if summary else ''}")
    print(f"[compiler-warnings] {REPORT.relative_to(REPO)}")
    print(f"[compiler-warnings] {COVERAGE.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
