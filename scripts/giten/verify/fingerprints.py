"""giten.verify.fingerprints - per-function source fingerprints (src_hash).

A function's src_hash is the `ast:` fingerprint of ITS OWN definition
(giten.verify.astprint): the libclang AST as the retail compile sees it, with
enum constants and constant expressions reduced to their values. Renaming a
constant to an enumerator or macro of the same value, retyping through a
typedef of the same type, or editing comments and layout is not an edit.
Hashes of another domain (the older raw-text range hashes) are never compared
against it: a domain change is re-banked, not counted as an edit. Hashing a whole .cpp is too coarse - editing one function would reset
the high-water of every sibling in the unit, hiding collateral regressions.

Bridge from the report's MANGLED names to clangd's source-level names:
  * C++ (?...):  llvm-undname -> "Class::Method" -> clangd qualified symbol.
  * C   (_name): strip the cdecl/stdcall decoration (leading `_`, trailing
                 `@N`) -> clangd source name.
A function clangd cannot resolve gets NO entry, and the fingerprinter falls
back to the unit's whole-.cpp hash, tagged `cpp:` = "no per-function
fingerprint available". Header-owned definitions are resolved separately from
libclang spelling extents, including immutable SDK headers. An unresolved
fallback remains UNKNOWN; the MAX gate conservatively treats a changed
fallback as an edit, while banking never lowers MAX on that evidence alone.

The cache is DERIVED, incremental (units whose source, transitive local
includes or clang compilation context changed get re-parsed), and lives in
build/gen/ (this slice's scratch). These inputs only invalidate the cache;
the per-function AST still decides whether a function changed. The old
pipeline's cache at build/clangd/func_fingerprints.tsv is read as a SEED
when the new one is absent. Rows without input hashes are reparsed without
changing the function fingerprint domain or banked src_hash provenance.
"""

from __future__ import annotations

import hashlib
import re
import subprocess
from pathlib import Path

from giten.core.paths import BUILD, REPO

CACHE = BUILD / "gen/func_fingerprints.tsv"
SEED = BUILD / "clangd/func_fingerprints.tsv"   # old pipeline's cache (read-only)

FALLBACK = "cpp:"  # marks a fingerprint we could NOT resolve per-function


def is_fallback(h: str) -> bool:
    return h.startswith(FALLBACK)


def domain(fp: str) -> str:
    """The hash domain: the prefix before ':' (`ast`, `header`, `cpp`), or
    `text` for the older bare raw-text range hashes."""
    return fp.split(":", 1)[0] if ":" in fp else "text"


def real_edit(prev_fp: str, cur_fp: str) -> bool:
    """True only when BOTH sides are real (non-fallback) fingerprints of the
    SAME domain that differ - a genuine source edit, not a domain change."""
    return not is_fallback(prev_fp) and not is_fallback(cur_fp) \
        and domain(prev_fp) == domain(cur_fp) and prev_fp != cur_fp


def cpp_hash(source: str) -> str:
    """12-hex sha1 of a unit's whole source file (the unit-level fallback)."""
    p = REPO / source
    return hashlib.sha1(p.read_bytes()).hexdigest()[:12] if p.is_file() \
        else "nosrc"


def _sha12(text: str) -> str:
    return hashlib.sha1(text.encode("utf-8", "replace")).hexdigest()[:12]


def input_hasher():
    """Hash the build's include closure and clang context, memoized per run.

    SDK headers are pinned by the toolchain paths in the compilation database,
    just as they are for the build's include scanner. Old cache rows without
    this hash must be regenerated, even when their source hash still agrees.
    """
    from giten.graph.scan import Scanner
    from giten.verify.astprint import CDB

    scan = Scanner()
    context = hashlib.sha1(CDB.read_bytes()).hexdigest() if CDB.is_file() else "nocdb"
    hashes: dict[str, str] = {}

    def inputs_of(source: str) -> str:
        parts = [context]
        for path in sorted({source, *scan.headers(source)}):
            if path not in hashes:
                hashes[path] = cpp_hash(path)
            parts.extend((path, hashes[path]))
        return _sha12("\n".join(parts))

    return inputs_of


def unit_sources() -> dict[str, str]:
    from giten import manifest
    return {u["unit"]: u.get("source", "") for u in manifest.units()}


def unit_mangled() -> dict[str, set]:
    """unit -> {mangled names}, from the Model (the one rva/unit/name join)."""
    from giten.model import resolve
    out: dict[str, set] = {}
    for b in resolve().functions:
        if b.name and b.unit:
            out.setdefault(b.unit, set()).add(b.name)
    return out


# --------------------------------------------------------------------------- #
# cache I/O                                                                   #
# --------------------------------------------------------------------------- #
def load_cache() -> tuple[dict[str, dict], dict[tuple[str, str], str]]:
    """(units{unit: {cpp_hash, source, inputs_hash}}, funcs{(unit, mangled): src_hash})."""
    units: dict[str, dict] = {}
    funcs: dict[tuple[str, str], str] = {}
    path = CACHE if CACHE.is_file() else SEED
    if not path.is_file():
        return units, funcs
    section = None
    for line in path.read_text().splitlines():
        if line.startswith("# [units]"):
            section = "u"
            continue
        if line.startswith("# [functions]"):
            section = "f"
            continue
        if not line or line.startswith("#"):
            continue
        c = line.split("\t")
        if section == "u" and len(c) >= 3:
            units[c[0]] = {"cpp_hash": c[1], "source": c[2],
                           "inputs_hash": c[3] if len(c) >= 4 else ""}
        elif section == "f" and len(c) >= 3:
            funcs[(c[0], c[1])] = c[2]
    return units, funcs


def write_cache(units: dict[str, dict],
                funcs: dict[tuple[str, str], str]) -> None:
    CACHE.parent.mkdir(parents=True, exist_ok=True)
    out = ["# func source fingerprints - generated by giten.verify."
           "fingerprints.\n",
           "# Derived cache (gitignored). Consumed by giten.verify.\n",
           "# [units]\tunit\tcpp_hash\tsource\tinputs_hash\n"]
    for u in sorted(units):
        out.append(f"{u}\t{units[u]['cpp_hash']}\t{units[u]['source']}\t"
                   f"{units[u]['inputs_hash']}\n")
    out.append("# [functions]\tunit\tmangled\tsrc_hash\n")
    for k in sorted(funcs):
        out.append(f"{k[0]}\t{k[1]}\t{funcs[k]}\n")
    CACHE.write_text("".join(out))


# --------------------------------------------------------------------------- #
# the fingerprinter (what bank/check consume)                                 #
# --------------------------------------------------------------------------- #
def header_fingerprints(source: str) -> dict[str, str]:
    """Canonical external-header bodies, independent of the emitting TU.

    Resolve on each fingerprinter invocation rather than caching by the .cpp
    hash: an external header can change while its including TU does not.
    Ambiguous definitions and bodies owned by the repository remain unresolved.
    """
    from giten.core.msvc_names import func
    from giten.tool import clang

    path = (REPO / source).resolve()
    if path.suffix != ".cpp" or not path.is_file():
        return {}
    definitions = clang.function_definition_extents(
        str(path), clang.compdb().get(str(path))) or {}
    out: dict[str, str] = {}
    repo = REPO.resolve()
    for mangled, extents in definitions.items():
        unique = {(e["file"], e["start"], e["end"]) for e in extents}
        if len(unique) != 1:
            continue
        file, start, end = next(iter(unique))
        header = Path(file).resolve()
        if header.is_relative_to(repo) or header.suffix.lower() not in {".h", ".hpp", ".inl"}:
            continue
        try:
            payload = header.read_bytes()
        except OSError:
            continue
        if not (0 <= start < end <= len(payload)):
            continue
        out[func(mangled, decorated=True)] = "header:" + hashlib.sha1(
            payload[start:end]).hexdigest()[:12]
    return out


def fingerprinter():
    """Return (fp, cpp_of, stale): fp(unit, mangled) -> CURRENT fingerprint.

    The cache's per-function range hash when the cache is fresh for the unit
    AND has the function; otherwise a FALLBACK-tagged whole-.cpp hash. `stale`
    collects units whose cache is stale/absent (the dangerous fallback).
    """
    sources = unit_sources()
    cache_units, cache_funcs = load_cache()
    cur_cpp: dict[str, str] = {}
    inputs_of = input_hasher()
    fresh: dict[str, bool] = {}

    def cpp_of(unit: str) -> str:
        if unit not in cur_cpp:
            cur_cpp[unit] = cpp_hash(sources.get(unit, ""))
        return cur_cpp[unit]

    stale: set = set()
    headers: dict[str, dict[str, str]] = {}

    def fp(unit: str, mangled: str) -> str:
        h = cpp_of(unit)
        cu = cache_units.get(unit)
        if unit not in fresh:
            fresh[unit] = bool(cu and cu.get("cpp_hash") == h
                               and cu.get("inputs_hash") == inputs_of(sources.get(unit, "")))
        if fresh[unit]:
            real = cache_funcs.get((unit, mangled))
            if real is not None:
                return real                     # real per-function range hash
            if unit not in headers:
                headers[unit] = header_fingerprints(sources.get(unit, ""))
            return headers[unit].get(mangled, FALLBACK + h)
        stale.add(unit)                         # unit's cache stale/absent
        return FALLBACK + h

    return fp, cpp_of, stale


# --------------------------------------------------------------------------- #
# mangled -> qualified source name bridge                                     #
# --------------------------------------------------------------------------- #
def demangle_map(names: set) -> dict[str, str]:
    """C++ mangled -> 'Class::Method' qualified key, one batched llvm-undname."""
    cpp = sorted(n for n in names if n.startswith("?"))
    if not cpp:
        return {}
    # NOTE the trailing newline: without it llvm-undname drops the final name.
    proc = subprocess.run(["llvm-undname"], input="\n".join(cpp) + "\n",
                          capture_output=True, text=True)
    out: dict[str, str] = {}
    for block in proc.stdout.split("\n\n"):
        lines = block.splitlines()
        if len(lines) >= 2 and lines[0] in names:
            q = _qualified_of(lines[1])
            if q:
                out[lines[0]] = q
    return out


def _qualified_of(demangled: str) -> str | None:
    """'public: int __thiscall CFileIO::Open(char const *,...)' -> 'CFileIO::Open'."""
    # A function-pointer return type wraps the declared name in parentheses:
    #   void (__cdecl * __thiscall C::Get(...))(char *, int)
    # The first `(` therefore belongs to the return type, not the parameter
    # list.  Prefer the qualified identifier which is itself followed by `(`.
    wrapped = re.search(
        r"(?<![\w:])((?:~?[A-Za-z_]\w*::)+~?[A-Za-z_]\w*)\s*\(",
        demangled)
    if wrapped:
        return wrapped.group(1)
    head = demangled.split("(", 1)[0].strip()    # drop the parameter list
    if not head:
        return None
    tok = head.split()[-1]                       # name follows retty/callconv
    if "`" in tok or "'" in tok:                 # compiler-generated -> no source
        return None
    return tok


def _candidates(mangled: str) -> list:
    """Source-name candidates for a C symbol: undo the cdecl/stdcall
    decoration (leading `_`, trailing `@N` stdcall arg-byte count)."""
    cands = []
    for n in (mangled, mangled.split("@", 1)[0]):
        cands += [n, n[1:] if n.startswith("_") else n, n.lstrip("_")]
    return list(dict.fromkeys(cands))


# --------------------------------------------------------------------------- #
# regenerate (incremental)                                                    #
# --------------------------------------------------------------------------- #
def regenerate(force_all: bool = False, verbose: bool = False) -> int:
    sources = unit_sources()
    umang = unit_mangled()
    cache_units, cache_funcs = load_cache()

    cur_cpp: dict[str, str] = {}
    cur_inputs: dict[str, str] = {}
    inputs_of = input_hasher()
    todo: list[str] = []
    for unit, source in sources.items():
        if not source:
            continue
        cur_cpp[unit] = cpp_hash(source)
        cur_inputs[unit] = inputs_of(source)
        if force_all or cache_units.get(unit, {}).get("inputs_hash") != cur_inputs[unit]:
            todo.append(unit)

    new_units: dict[str, dict] = {}
    new_funcs: dict[tuple[str, str], str] = {}
    for unit, source in sources.items():           # carry unchanged units over
        if unit not in cur_cpp or unit in todo:
            continue
        new_units[unit] = {"cpp_hash": cur_cpp[unit], "source": source,
                           "inputs_hash": cur_inputs[unit]}
        for k, v in cache_funcs.items():
            if k[0] == unit:
                new_funcs[k] = v

    if todo:
        for unit in todo:
            new_units[unit] = {"cpp_hash": cur_cpp[unit],
                               "source": sources[unit],
                               "inputs_hash": cur_inputs[unit]}
        allm: set = set()
        for u in todo:
            allm |= umang.get(u, set())
        if allm:
            from giten.verify.astprint import unit_fingerprints
            m2q = demangle_map(allm)
            for unit in todo:
                qhash = unit_fingerprints(sources[unit])
                n = 0
                for m in umang.get(unit, set()):
                    q = m2q.get(m) if m.startswith("?") else next(
                        (c for c in _candidates(m) if c in qhash), None)
                    if q and q in qhash:
                        new_funcs[(unit, m)] = qhash[q]
                        n += 1
                if verbose:
                    print(f"  {unit}: {n}/{len(umang.get(unit, set()))} "
                          f"fingerprinted ({len(qhash)} defs in the AST)")

    write_cache(new_units, new_funcs)
    print(f"func fingerprints: {len(new_funcs)} functions  "
          f"({len(todo)} unit(s) re-parsed, {len(new_units) - len(todo)} cached)")
    return 0


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="giten verify fingerprints", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true",
                    help="re-parse every unit (ignore the cache)")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="name each unit as it is re-parsed")
    a = ap.parse_args(argv)
    return regenerate(force_all=a.all, verbose=a.verbose)
