"""giten.graph.link - PHASE 2: base objs -> candidate DDS.EXE + .map.

    python3 -m giten.graph.link [--out E] [--objs-dir D] [--res R] [--order F]

Graph phase 2, opt-in (`ninja candidate` / `giten link`): the genuine VC5
pinned link.exe (5.10.7303) run under wine over our
base objects. The deliverable is the `.map`: every function's link-assigned
RVA and its source object, which cross-referenced with the retail RVAs is what
recovers the original build order (intra-TU order = source-definition order,
cross-TU = object link order).

**There is no `/FORCE`, and it must never come back.** It was scaffolding for
the partial-reconstruction era; the tree links with ZERO unresolved externals
and ZERO duplicate symbols, so the link is an ORACLE. `/FORCE` would re-swallow
exactly the defects this phase exists to catch - an unresolved extern (a
fabricated name, a body homed nowhere) and an LNK2005/LNK4006 duplicate (a
symbol the CRT owns that we also define, or one global defined in two TUs). A
link failure here is a FINDING: read the LNK codes and fix the source.

Libraries. The objects cl emits already carry the CRT in their `.drectve`
directives (`/ML` writes `-defaultlib:LIBC` + `-defaultlib:OLDNAMES`), so we do
NOT pass `/NODEFAULTLIB` and let them fire - what the devs' link did. The Win32,
WINMM and DirectX 6 import libs declare themselves nowhere (the SDK ships no
`#pragma comment(lib)`) and are named explicitly, corroborated by retail's own
import table. giten.graph.implib would synthesise any import lib the toolchain
lacks; for DDS.EXE the toolchain has all eight.

Link mode. Retail is a NON-incremental /FIXED link: no E9 thunk band, the import
tables merged into .rdata (no .idata), no .reloc and IMAGE_FILE_RELOCS_STRIPPED
set (docs/linker-flags.md). The prior image is still dropped first so a link's
output never depends on what happened to be left on disk.
"""

from __future__ import annotations

import collections
import re
import struct
import sys
from datetime import datetime, timezone
from pathlib import Path

from giten.core.paths import BUILD, REPO, RETAIL
from giten.tool import ToolError
from giten.tool.wine import era_tool, run, winepath

#: Explicit SDK libraries precede the objects' default LIBC/OLDNAMES search.
#: Retail places DirectInput's format tables and SDK thunks before CRT text,
#: and DXGUID/UUID before CRT readonly data. Naming LIBC here pulls those CRT
#: contributions ahead of the SDK bands. DLL lookup order is a separate sort.
LINK_LIBS = ["kernel32.lib", "user32.lib", "gdi32.lib", "advapi32.lib",
             "ddraw.lib", "dsound.lib", "dinput.lib", "winmm.lib", "dxguid.lib", "uuid.lib"]

#: LIBC defines _WinMainCRTStartup; the game defines the _WinMain@16 it calls.
ENTRY = "WinMainCRTStartup"

#: Units archived into static libraries before the link, by group. Retail shows
#: no archive evidence yet (a non-incremental link leaves no thunk band to tell
#: link-line objects from library members), so every group is empty and
#: `--engine-lib` links everything on the line.
ENGINE_MODULES: set[str] = set()
ZLIB_UNITS: set[str] = set()
TAIL_UNITS: set[str] = set()


def unresolved(output: str) -> set[str]:
    """The DECORATED unresolved-external names in a link log.

    LNK2001 prints a C symbol bare (`_malloc`) but a C++ one as demangled prose
    FOLLOWED by the real name in parentheses. A `(\\S+)` grab therefore collapses
    every C++ blocker into the few distinct first words of that prose, which
    silently hid the entire C++ backlog from the punch list. Take the trailing
    parenthesised name when there is one.
    """
    out = set()
    for ln in output.splitlines():
        m = re.search(r"unresolved external symbol (.*)$", ln)
        if not m:
            continue
        rest = m.group(1).strip()
        paren = re.search(r"\(([^()]+)\)\s*$", rest)
        out.add(paren.group(1) if paren else rest.split()[0])
    return out


def classify(sym: str) -> str:
    """Which link blocker `sym` is - the three buckets that need three fixes."""
    if sym.startswith("__imp_"):
        return "import (no import lib on the line)"
    if sym.startswith("?"):
        return "C++ (undefined method/variable - reconstruction backlog)"
    return "C (undefined free function/variable)"


def has_rsrc(exe: Path) -> bool:
    """True when the PE carries a .rsrc section - read straight out of the
    section table, so the check costs nothing and cannot be skipped."""
    try:
        data = exe.read_bytes()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        n = struct.unpack_from("<H", data, pe + 6)[0]
        first = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
        return any(data[first + i * 40:first + i * 40 + 8].rstrip(b"\0") == b".rsrc"
                   for i in range(n))
    except (OSError, struct.error, IndexError):
        return False


def collect_objs(objs_dir: Path, *, order: Path | None = None,
                 explicit: list[str] = ()) -> list[Path]:
    """The objects and their link ORDER.

    `order` (one stem or path per line) wins - that is how a hypothesised
    retail link order is tested; then explicit paths; then the complete retail
    contribution table, with alphabetical manifest order only when no table
    rows exist. The manifest filter matters: deleting a
    [[unit]] does not delete its stale object, and a bare glob then links the
    orphan, which surfaces as a phantom LNK2005 against the TU that legitimately
    owns the symbol now - and with no /FORCE that FAILS the link and looks like
    a real identity defect.
    """
    if order is not None:
        objs = []
        for ln in Path(order).read_text().splitlines():
            s = ln.strip()
            if not s or s.startswith("#"):
                continue
            p = Path(s) if Path(s).suffix else objs_dir / f"{s}.obj"
            if not p.exists():
                raise ToolError(f"order entry not found: {s} ({p})")
            objs.append(p)
        return objs
    if explicit:
        return [Path(o).resolve() for o in explicit]
    if not objs_dir.is_dir():
        raise ToolError(f"--objs-dir not found: {objs_dir}")
    from giten.manifest import units as manifest_units
    owned = {u["unit"] for u in manifest_units()}
    retail_order = RETAIL / "link_order.tsv"
    if retail_order.is_file() and any(
            ln.strip() and not ln.startswith("#")
            for ln in retail_order.read_text().splitlines()):
        from giten.core.tsv import read
        _banner, _header, rows = read(retail_order)
        names = [row["unit"] for row in rows]
        if len(names) != len(set(names)):
            raise ToolError(f"{retail_order}: repeated object unit")
        if set(names) != owned:
            missing = sorted(owned - set(names))
            extra = sorted(set(names) - owned)
            raise ToolError(f"{retail_order}: object coverage differs from manifest "
                            f"(missing={missing}, extra={extra})")
        objs = [objs_dir / f"{name}.obj" for name in names]
        missing_objs = [str(p) for p in objs if not p.is_file()]
        if missing_objs:
            raise ToolError(f"retail link order has missing objects: {missing_objs}")
        return objs
    objs, orphans = [], []
    for p in sorted(objs_dir.glob("*.obj")):
        (objs if p.stem in owned else orphans).append(p)
    if orphans:
        print(f"[link] skipping {len(orphans)} orphaned obj(s) with no [[unit]]: "
              + ", ".join(p.name for p in orphans[:6])
              + (" ..." if len(orphans) > 6 else ""))
    return objs


def engine_units() -> set[str]:
    """Unit names whose source lives in an engine module (or vendor/)."""
    from giten.manifest import units as manifest_units
    out = set()
    for u in manifest_units():
        src = u.get("source", "")
        parts = Path(src).parts
        mod = parts[1] if src.startswith("src/") and len(parts) > 1 else "vendor"
        if mod in ENGINE_MODULES or not src.startswith("src/"):
            out.add(u["unit"])
    return out


def archive_engine(objs: list[Path], out_dir: Path) -> tuple[list[Path], list[Path]]:
    """Split `objs` into (link-line objects, archives) using the real LIB.EXE."""
    eng_names = engine_units() - ZLIB_UNITS - TAIL_UNITS
    groups = [("engine.lib", [o for o in objs if o.stem in eng_names]),
              ("zlib.lib", [o for o in objs if o.stem in ZLIB_UNITS]),
              ("utils.lib", [o for o in objs if o.stem in TAIL_UNITS])]
    claimed = eng_names | ZLIB_UNITS | TAIL_UNITS
    rest = [o for o in objs if o.stem not in claimed]
    if not any(members for _n, members in groups):
        return objs, []
    lib_exe = era_tool("lib.exe")
    out_dir.mkdir(parents=True, exist_ok=True)
    made = []
    for name, members in groups:
        if not members:
            continue
        lib = out_dir / name
        lib.unlink(missing_ok=True)
        rsp = out_dir / name.replace(".lib", ".rsp")
        rsp.write_text(f"/OUT:{winepath(lib)}\n"
                       + "\n".join(f'"{winepath(o)}"' for o in members) + "\n")
        run(["wine", str(lib_exe), "/NOLOGO", f"@{winepath(rsp)}"],
            cwd=out_dir, success=lib)
        if not lib.exists():
            raise ToolError(f"LIB.EXE failed to build {name}")
        print(f"[link] archived {len(members)} obj(s) -> {name} "
              f"({lib.stat().st_size:,} B)")
        made.append(lib)
    print(f"[link] {len(rest)} obj(s) stay on the link line")
    return rest, made


def candidate(out: Path, objs_dir: Path, *, mapfile: Path | None = None,
              res: Path | None = None, order: Path | None = None,
              explicit: list[str] = (), extra_libs: list[str] = (),
              engine_lib: bool = False, incremental: bool = False,
              base: str = "0x400000", keep_all: bool = False,
              entry: str | None = None,
              fold_identical: bool = True,
              real_time: bool = False,
              extra_flags: list[str] = (), dry_run: bool = False) -> dict:
    """Link the candidate image; returns {objs, libs, unresolved, duplicates}.

    `dry_run` assembles the response file and stops before link.exe - the way
    to inspect the object order and the library line without a linker.
    """
    from giten.graph import implib, static_libraries
    from giten.tool import link as link_tool

    at = None if real_time else datetime.fromtimestamp(
        link_tool.pe_stamp(BUILD / "local/DDS.EXE"), timezone.utc
    ).strftime("%Y-%m-%d %H:%M:%S")
    if at is not None:
        print(f"[link] expected clock: {at} UTC")
    out = Path(out).resolve()
    mapf = Path(mapfile).resolve() if mapfile else out.with_suffix(".map")
    out.parent.mkdir(parents=True, exist_ok=True)

    objs = collect_objs(Path(objs_dir), order=order, explicit=explicit)
    archives: list[Path] = []
    if engine_lib:
        objs, archives = archive_engine(objs, out.parent)
    if not objs:
        raise ToolError("no objects to link")

    rsp_lines = [
        f"/OUT:{winepath(out)}", f"/MAP:{winepath(mapf)}",
        "/NOLOGO", "/SUBSYSTEM:WINDOWS", f"/BASE:{base}",
        "/INCREMENTAL:YES" if incremental else "/INCREMENTAL:NO",
        # Retail has NO .reloc and sets IMAGE_FILE_RELOCS_STRIPPED: /FIXED.
        # (The delinker's relocations are synthesized - docs/relocations.md.)
        "/FIXED",
    ]
    if entry:
        rsp_lines.append(f"/ENTRY:{entry}")
    # Retail retains ordinary dead code but removes unused CRT COMDATs.
    # Retail also shares identical CRT wrappers and empty SDK constructors.
    rsp_lines += ["/OPT:NOREF" if keep_all else "/OPT:REF",
                  "/OPT:ICF" if fold_identical else "/OPT:NOICF"]
    rsp_lines += list(extra_flags)

    libs = [*extra_libs, *(str(a) for a in archives)]
    made = implib.on_disk() if dry_run else implib.ensure_all()
    synth = {p.name.lower(): str(p) for p in made}
    try:
        synth.update({name: str(path) for name, path in static_libraries.on_disk().items()})
    except (OSError, ValueError) as error:
        raise ToolError(str(error)) from error
    libs += [synth.get(n, n) for n in LINK_LIBS]           # substitute IN PLACE
    rsp_lines += [winepath(x) if Path(x).exists() else x for x in libs]
    rsp_lines += [f'"{winepath(o)}"' for o in objs]
    if res is not None:
        rsp_lines.append(f'"{winepath(Path(res).resolve())}"')

    # VC5 link has a short argv limit under wine, hence the response file.
    rsp = out.parent / f"{out.stem}.objs.rsp"
    rsp.write_text("\n".join(rsp_lines) + "\n")
    if dry_run:
        print(f"[link] dry run: {len(objs)} obj(s) + {len(libs)} lib(s) -> {rsp}")
        return {"objs": len(objs), "libs": len(libs), "rsp": rsp,
                "unresolved": [], "duplicates": 0}
    for stale in (out, mapf, out.with_suffix(".ilk")):
        stale.unlink(missing_ok=True)

    logf = out.parent / f"{out.stem}.link.log"
    try:
        output = link_tool.link([f"@{winepath(rsp)}"], cwd=out.parent,
                                expect=[out, mapf], at=at)
    except ToolError as e:
        logf.write_text(str(e))
        raise
    logf.write_text(output)

    if res is not None and not has_rsrc(out):
        raise ToolError(
            f"{out.name} has no .rsrc although --res {res} was on the link "
            "line: the image would lack its bitmaps, sounds and icons.")
    if res is None:
        # The image is knowingly incomplete and nothing else says so: the
        # configure-time explanation lives in a generated manifest nobody
        # reads, and the .map - which is what phase 2 is for - is unaffected.
        print(f"[link] no .res on the link line, so {out.name} has NO .rsrc "
              "(DDS.EXE carries its bitmaps and sounds there): the image is a "
              "link-ORDER artifact (the .map), not a runnable game.")

    # No /FORCE: an unresolved extern or a duplicate FAILS the link above, so
    # reaching here means both are zero. They are still reported (and asserted)
    # because a silent regression to non-zero would mean the link stopped being
    # an oracle.
    dups = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    unres = sorted(unresolved(output))
    (out.parent / f"{out.stem}.unresolved.txt").write_text("\n".join(unres) + "\n")
    print(f"[link] {len(objs)} obj(s) + {len(libs)} explicit lib(s) -> {out} "
          f"({out.stat().st_size:,} B) + {mapf.name}")
    print(f"[link] {len(unres)} unresolved external(s), {dups} dup-symbol "
          "warning(s)  (no /FORCE - a real link)")
    for bucket, n in sorted(collections.Counter(
            classify(s) for s in unres).items(), key=lambda kv: -kv[1]):
        print(f"[link]   {n:5d}  {bucket}")
    if unres or dups:
        raise ToolError(f"link is no longer clean: {len(unres)} unresolved, "
                        f"{dups} duplicate(s). Fix the source - never re-add "
                        "/FORCE (see the module docstring).")
    return {"objs": len(objs), "libs": len(libs), "unresolved": unres,
            "duplicates": dups, "exe": out, "map": mapf}


def main() -> int:
    import argparse
    from giten import graph
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=REPO / graph.CANDIDATE_EXE)
    ap.add_argument("--map", dest="mapfile", type=Path,
                    help="map path (default: <out> with a .map suffix)")
    ap.add_argument("--objs-dir", type=Path, default=REPO / graph.BASE_DIR)
    ap.add_argument("--obj", action="append", default=[],
                    help="explicit object (repeatable)")
    ap.add_argument("--order", type=Path,
                    help="file listing object stems/paths in link order")
    ap.add_argument("--res", type=Path, help=".RES for the candidate's resources")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib (repeatable)")
    ap.add_argument("--engine-lib", action="store_true",
                    help="archive the ENGINE_MODULES/ZLIB_UNITS/TAIL_UNITS groups "
                         "and link those archives (all empty for DDS.EXE)")
    ap.add_argument("--incremental", action="store_true",
                    help="/INCREMENTAL:YES. Default is NO because retail is a "
                         "flat /FIXED link; use this only to isolate that variable")
    ap.add_argument("--base", default="0x400000", help="image base (/BASE)")
    ap.add_argument("--entry", help="explicit startup override; default lets the Windows subsystem select WinMainCRTStartup")
    selection = ap.add_mutually_exclusive_group()
    selection.add_argument("--keep-all", dest="keep_all", action="store_true",
                           help="retain unreferenced COMDATs for contribution analysis")
    selection.add_argument("--opt-ref", dest="keep_all", action="store_false",
                           help="remove unreferenced COMDATs (default)")
    ap.set_defaults(keep_all=False)
    ap.add_argument("--no-icf", dest="fold_identical", action="store_false",
                    help="keep identical COMDATs separate for contribution analysis")
    ap.add_argument("--dry-run", action="store_true",
                    help="assemble the response file and stop before link.exe")
    ap.add_argument("--real-time", action="store_true",
                    help="use the current clock for contribution analysis instead of retail's UTC timestamp")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="extra link flags after `--`")
    a = ap.parse_args()
    extra = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        candidate(a.out, a.objs_dir, mapfile=a.mapfile, res=a.res, order=a.order,
                  explicit=a.obj, extra_libs=a.lib, engine_lib=a.engine_lib,
                  incremental=a.incremental, base=a.base,
                  keep_all=a.keep_all, entry=a.entry,
                  fold_identical=a.fold_identical, real_time=a.real_time,
                  extra_flags=extra, dry_run=a.dry_run)
    except (ToolError, OSError) as e:
        print(f"[link] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
