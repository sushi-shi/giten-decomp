"""C source written-cast census using the target-C compile database.

`giten verify c-casts` parses every C translation unit from units.toml with
libclang, deduplicates explicit C casts by spelling location, and retains the
source/target types and all unit contexts at that location. It does not change
source or claim that a lower cast count proves a better model. The default
report is derived under build/gen; `--max` supplies an explicit ratchet.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
from ctypes import byref, c_uint
from dataclasses import asdict, dataclass
import hashlib
import json
from pathlib import Path

from giten.core.paths import BUILD, REPO
from giten.manifest import units
from giten.verify.constants import _flags, _require_cl_mode, _source_path

CDB = BUILD / "clangd/compile_commands.json"
REPORT = BUILD / "gen/c_casts.tsv"
COVERAGE = BUILD / "gen/c_casts_coverage.json"
_SOURCE_SUFFIXES = {".c", ".h", ".inc"}


@dataclass(frozen=True, order=True)
class Location:
    file: str
    line: int
    column: int
    offset: int


@dataclass(frozen=True, order=True)
class TypeDescription:
    spelling: str
    canonical: str
    kind: str
    size: int


@dataclass(frozen=True, order=True)
class Context:
    unit: str
    function: str


@dataclass(frozen=True)
class CastSite:
    location: Location
    category: str
    source_types: tuple[TypeDescription, ...]
    target_types: tuple[TypeDescription, ...]
    contexts: tuple[Context, ...]
    line_text: str


@dataclass(frozen=True)
class Audit:
    units: int
    expanded_casts: int
    external_casts: int
    sites: tuple[CastSite, ...]
    headers_seen: tuple[str, ...]
    uncovered_headers: tuple[str, ...]


def _source_files(repo: Path) -> tuple[Path, ...]:
    return tuple(sorted(
        path for root in (repo / "src", repo / "include") if root.is_dir()
        for path in root.rglob("*")
        if path.is_file() and path.suffix in _SOURCE_SUFFIXES
    ))


def _hashes(repo: Path) -> dict[str, str]:
    return {str(path.relative_to(repo)): hashlib.sha256(path.read_bytes()).hexdigest()
            for path in _source_files(repo)}


def _relative(filename: str, line: int, column: int, offset: int,
              repo: Path) -> Location | None:
    if not filename:
        return None
    try:
        rel = Path(filename).resolve().relative_to(repo)
    except ValueError:
        return None
    return Location(str(rel), line, column, offset)


def _spelling_location(location, repo: Path, cidx) -> Location | None:
    file_pointer = cidx.c_object_p()
    line, column, offset = c_uint(), c_uint(), c_uint()
    cidx.conf.lib.clang_getSpellingLocation(
        location, byref(file_pointer), byref(line), byref(column), byref(offset))
    filename = str(cidx.File(file_pointer)) if file_pointer else ""
    return _relative(filename, line.value, column.value, offset.value, repo)


def _describe(typ) -> TypeDescription:
    canonical = typ.get_canonical()
    try:
        size = canonical.get_size()
    except Exception:  # incomplete SDK types report no size
        size = -1
    return TypeDescription(typ.spelling, canonical.spelling,
                           str(canonical.kind).rsplit(".", 1)[-1].lower(), size)


def _selected_entries(cdb: Path, repo: Path) -> tuple[tuple[str, dict], ...]:
    if not cdb.is_file():
        raise FileNotFoundError(
            f"{cdb}: no compile database; run `python3 -m giten.graph.compdb`")
    c_units = [unit for unit in units() if Path(unit["source"]).suffix == ".c"]
    expected = {str((repo / unit["source"]).resolve()): unit["unit"]
                for unit in c_units}
    if len(expected) != len(c_units):
        raise RuntimeError("multiple C manifest units share a source; "
                           "the cast census needs each unit's compile context")
    actual = {str(path.resolve()) for path in (repo / "src").rglob("*.c")}
    stray = sorted(actual - set(expected))
    if stray:
        raise RuntimeError("C source absent from config/units.toml: "
                           + ", ".join(str(Path(p).relative_to(repo)) for p in stray[:8]))
    selected: list[tuple[str, dict]] = []
    seen: set[str] = set()
    stale: list[str] = []
    for entry in json.loads(cdb.read_text()):
        path = _source_path(entry, repo)
        key = str(path)
        try:
            rel = path.relative_to(repo)
        except ValueError:
            continue
        if not rel.parts or rel.parts[0] != "src" or path.suffix != ".c":
            continue
        if key not in expected:
            stale.append(key)
            continue
        if key in seen:
            raise RuntimeError(f"duplicate C compile database entry: {rel}")
        selected.append((expected[key], entry))
        seen.add(key)
    missing = sorted(set(expected) - seen)
    stale.sort()
    if missing or stale:
        detail = ", ".join(str(Path(path).relative_to(repo))
                           for path in (missing + stale)[:8])
        raise RuntimeError(f"compile database misses {len(missing)} C unit(s) "
                           f"and lists {len(stale)} stale C unit(s): {detail}; "
                           "run `python3 -m giten.graph.compdb`")
    return tuple(selected)


def collect(*, cdb: Path = CDB, repo: Path = REPO) -> Audit:
    """Parse every target-C unit; abort on incomplete coverage or diagnostics."""
    import clang.cindex as cidx

    repo = repo.resolve()
    selected = _selected_entries(cdb, repo)
    before = _hashes(repo)
    texts = {rel: (repo / rel).read_text(errors="replace").splitlines()
             for rel in before}
    casts: dict[Location, dict[str, set]] = defaultdict(
        lambda: {"categories": set(), "sources": set(),
                 "targets": set(), "contexts": set()})
    headers: set[str] = set()
    expanded = external = 0
    for unit, entry in selected:
        path = _source_path(entry, repo)
        if not path.is_file():
            raise RuntimeError(f"{path}: source listed in compile database is missing")
        flags = _flags(entry)
        _require_cl_mode(flags)
        try:
            tu = cidx.Index.create().parse(
                str(path), args=flags,
                options=cidx.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD)
        except cidx.TranslationUnitLoadError as exc:
            raise RuntimeError(f"{path}: libclang could not load C unit: {exc}") from exc
        errors = [str(d) for d in tu.diagnostics
                  if d.severity >= cidx.Diagnostic.Error]
        if errors:
            raise RuntimeError(f"{path}: target-C parse error: {errors[0]}")
        for inclusion in tu.get_includes():
            if inclusion.include is None:
                continue
            try:
                rel = Path(str(inclusion.include)).resolve().relative_to(repo)
            except ValueError:
                continue
            if rel.parts[0] in {"src", "include"} and rel.suffix in {".h", ".inc"}:
                headers.add(str(rel))

        def walk(node, function: str = "") -> None:
            nonlocal expanded, external
            if node.kind != cidx.CursorKind.TRANSLATION_UNIT and node.location.file:
                here = _relative(str(node.location.file), node.location.line,
                                 node.location.column, node.location.offset, repo)
                if here is None:
                    return
            if node.kind == cidx.CursorKind.FUNCTION_DECL:
                function = node.spelling
            if node.kind == cidx.CursorKind.CSTYLE_CAST_EXPR:
                expanded += 1
                origin = _spelling_location(node.location, repo, cidx)
                if origin is None:
                    external += 1
                else:
                    children = [child for child in node.get_children()
                                if child.kind.is_expression()]
                    operand = children[-1] if children else None
                    while operand is not None and operand.kind in {
                            cidx.CursorKind.UNEXPOSED_EXPR,
                            cidx.CursorKind.PAREN_EXPR}:
                        nested = [child for child in operand.get_children()
                                  if child.kind.is_expression()]
                        if len(nested) != 1:
                            break
                        operand = nested[0]
                    source = (_describe(operand.type) if operand is not None
                              else TypeDescription("", "", "invalid", -1))
                    target = _describe(node.type)
                    category = ("pointer" if node.type.get_canonical().kind
                                == cidx.TypeKind.POINTER else "scalar")
                    row = casts[origin]
                    row["categories"].add(category)
                    row["sources"].add(source)
                    row["targets"].add(target)
                    row["contexts"].add(Context(unit, function))
            for child in node.get_children():
                walk(child, function)

        walk(tu.cursor)
    if _hashes(repo) != before:
        raise RuntimeError("project source changed during cast audit; rerun on a stable tree")
    sites = []
    for loc, row in sorted(casts.items()):
        if loc.file not in texts:
            raise RuntimeError(f"cast spelling outside project C source: {loc.file}")
        lines = texts[loc.file]
        line_text = lines[loc.line - 1].strip() if 0 < loc.line <= len(lines) else ""
        categories = row["categories"]
        sites.append(CastSite(
            loc, next(iter(categories)) if len(categories) == 1 else "mixed",
            tuple(sorted(row["sources"])), tuple(sorted(row["targets"])),
            tuple(sorted(row["contexts"])), line_text))
    all_headers = {rel for rel in before if rel.endswith((".h", ".inc"))}
    return Audit(len(selected), expanded, external, tuple(sites),
                 tuple(sorted(headers)), tuple(sorted(all_headers - headers)))


def _write_report(audit: Audit, path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = ["file\tline\tcolumn\toffset\tcategory\tsource_types\t"
             "target_types\tcontexts\tline_text"]
    for site in audit.sites:
        loc = site.location
        sources = json.dumps([asdict(t) for t in site.source_types], ensure_ascii=False)
        targets = json.dumps([asdict(t) for t in site.target_types], ensure_ascii=False)
        contexts = json.dumps([asdict(c) for c in site.contexts], ensure_ascii=False)
        lines.append(f"{loc.file}\t{loc.line}\t{loc.column}\t{loc.offset}\t"
                     f"{site.category}\t{sources}\t{targets}\t{contexts}\t"
                     f"{site.line_text.replace(chr(9), ' ')}")
    path.write_text("\n".join(lines) + "\n")
    COVERAGE.write_text(json.dumps({
        "units": audit.units,
        "expanded_casts": audit.expanded_casts,
        "external_casts": audit.external_casts,
        "visited_project_headers": audit.headers_seen,
        "unvisited_project_headers": audit.uncovered_headers,
    }, ensure_ascii=False, indent=2) + "\n")


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="giten verify c-casts", description=__doc__)
    ap.add_argument("--kind", choices=("all", "pointer", "scalar", "mixed"),
                    default="all", help="filter written sites by target category")
    ap.add_argument("--scope", choices=("all", "source", "header"), default="all",
                    help="filter sites to C source or project headers")
    ap.add_argument("--path", action="append", default=[],
                    help="filter written sites by source path substring")
    ap.add_argument("--list", action="store_true", help="list each written site")
    ap.add_argument("--json", action="store_true", help="emit the full audit as JSON")
    ap.add_argument("--max", type=int, help="fail if the filtered site count exceeds N")
    ap.add_argument("--no-report", action="store_true",
                    help="skip writing the derived TSV and coverage reports")
    args = ap.parse_args(argv)
    try:
        audit = collect()
    except (FileNotFoundError, RuntimeError, ValueError) as exc:
        print(f"[c-casts] FATAL: {exc}")
        return 2
    sites = tuple(site for site in audit.sites
                  if (args.kind == "all" or site.category == args.kind)
                  and (args.scope == "all"
                       or (args.scope == "source" and site.location.file.endswith(".c"))
                       or (args.scope == "header" and
                           site.location.file.endswith((".h", ".inc"))))
                  and (not args.path or any(term in site.location.file
                                            for term in args.path)))
    if not args.no_report:
        _write_report(audit, REPORT)
    if args.json:
        payload = asdict(audit)
        payload["sites"] = [asdict(site) for site in sites]
        payload["selected_sites"] = len(sites)
        print(json.dumps(payload, ensure_ascii=False, indent=2))
    else:
        print(f"[c-casts] {len(audit.sites)} written site(s): "
              f"{sum(s.category == 'pointer' for s in audit.sites)} pointer, "
              f"{sum(s.category == 'scalar' for s in audit.sites)} scalar, "
              f"{sum(s.category == 'mixed' for s in audit.sites)} mixed; "
              f"{len(sites)} selected")
        print(f"[c-casts] {audit.units} C unit(s), {audit.expanded_casts} AST "
              f"expansion(s), {audit.external_casts} external cast(s), "
              f"{len(audit.headers_seen)} visited project header(s), "
              f"{len(audit.uncovered_headers)} unvisited header(s)")
        if args.list:
            for site in sites:
                loc = site.location
                src = ", ".join(t.spelling for t in site.source_types)
                dst = ", ".join(t.spelling for t in site.target_types)
                print(f"{loc.file}:{loc.line}:{loc.column}\t{site.category}\t"
                      f"{src} -> {dst}\t{site.line_text}")
        if not args.no_report:
            print(f"[c-casts] reports: {REPORT.relative_to(REPO)}, "
                  f"{COVERAGE.relative_to(REPO)}")
    if args.max is not None and len(sites) > args.max:
        print(f"[c-casts] {len(sites)} exceeds --max {args.max}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
