"""giten.graph.emit - configure: config/units.toml -> build/build.ninja.

    python3 -m giten.graph            # (re)write build/build.ninja
    ninja -f build/build.ninja         # run the loop, from the repo root

The rules, in the order the loop runs them. Everything through `verify_fp`
is the DEFAULT target; `verify_check` runs only for the `verify` target
(merge preparation); `rc`/`link` are phase 2, opt-in:

    configure   the generator edge - re-emits this manifest when the unit
                census, the emitter, or ANY file the include scan read changes
    cl          source -> build/objdiff/base/<unit>.obj   (giten.graph.cc)
    compdb      units.toml -> build/clangd/compile_commands.json - the clang-cl
                flags extraction and the LSP consumers ride (giten.graph.compdb)
    labels      source + headers + base obj -> build/gen/claims/<unit>.tsv
    model       claims x censuses/providers -> build/gen/bindings.tsv
    dataid      base objs -> their data identity (the delink reads it)
    delink      bindings + data identity -> build/objdiff/target-new/<unit>.c.obj
    normalize   base + target objs -> the comparison copies
    project     the delinked directory -> compare-new/objdiff.json
    report      comparison copies + pairing -> compare-new/report.json
    verify_fp   sources x bindings -> the per-function fingerprint cache
    verify_readme report x ledger -> README's score block (write-if-changed)
    verify_check the MAX gate + the fast+normal tiers -> a stamp; FATAL;
                opt-in (`giten build verify`)
    rsrc_payloads / rc / link   PHASE 2, opt-in (`ninja candidate`): the
                original's payload files + Giten.rc -> .res; base objs + .res
                -> the candidate image + .map for the link-order study
    play        opt-in (`giten play`): the units whose source or headers test
                a play flag, again with the bug-fix defines -> build/play/obj;
                those + every other unit's base obj + the .res ->
                build/play/DDS.EXE

Two edges declare a STAMP rather than their real outputs, because neither set
can be enumerated at configure time: `delink` writes one object per unit that
has a claim (a unit with none writes nothing, so declaring every unit would
leave ninja re-running the whole delink on every build), and `normalize`
writes a variable pair of copies per unit. Both drivers are keyed on content
upstream, so the stamp only moves when something real did.

Restat is on `cl`, `compdb`, `labels`, `model`, `dataid` and `project` - the
producers that write if-changed. That is the whole incrementality story: a pure code
edit re-runs configure (every source is in the include scan's own dep set) +
cl + labels, stops at an unchanged claim fragment, and reaches the report
without re-delinking; a label edit carries on through model, delink and the
pairing. Labels declare the same per-TU header closure as `cl`: extraction
reads inline `RVA` annotations from headers even when MSVC emits no changed
bytes, so the object edge's `restat` cannot be allowed to hide a renamed claim.

`verify_fp` also carries restat, but MEASURED its producer rewrites the cache
unconditionally (identical bytes, fresh mtime), so the restat is inert there
and `verify_check` re-runs after any source edit.

The era toolchain ($MSVC_DIR/$DXSDK_DIR) and the vostok-delinker binary are
environment rather than files under the repo, so they are declared INDIRECTLY:
`toolchain_id()` renders all three into build/gen/toolchain.id (write-if-changed),
and the cl, compdb and delink edges list that file. A re-pin therefore
invalidates exactly the edges it should. This is not cosmetic - before it, a
toolchain swap recompiled only the units that happened to be dirty and left the
rest built by the previous cl, which for a byte-matching project is the worst
possible failure, and a delinker swap gave `ninja: no work to do`.
"""

from __future__ import annotations

import os
import re
import sys
from pathlib import Path

from giten import graph
from giten.core.paths import REPO
from giten.graph import ninja_syntax
from giten.graph.scan import Scanner

SCRIPTS = "scripts/giten"
MANIFEST = "config/units.toml"
RETAIL_EXE = "build/exe/DDS.EXE"
RELOC_SITES = "config/retail/reloc_sites.tsv"
COMPDB = "build/clangd/compile_commands.json"
RELOC_REFERENTS = "config/retail/reloc_referents.tsv"
#: The data-matching switch (giten.core.data_matching): it decides the delink
#: fence spelling, the comparison copies and the gate verdicts.
COMPARE_CONFIG = "config/compare.toml"

#: The census + provider tables giten.model joins the claims against. Named
#: rather than globbed: reloc_referents.tsv is a DELINKER input and belongs on
#: that edge, and a new table should be a deliberate edit here.
MODEL_TABLES = [
    "config/retail/functions.tsv", "config/retail/data.tsv",
    "config/retail/link_order.tsv", "config/retail/link_bands.tsv",
    "config/retail/functions_static_libs.tsv", "config/retail/functions_zlib.tsv",
    "config/retail/data_zlib.tsv", "config/retail/data_vtables.tsv",
    "config/retail/data_static_libs.tsv", "config/retail/data_compgen.tsv",
]


def _mods(*rel: str) -> list[str]:
    """Repo-relative module paths under scripts/giten that exist."""
    out = []
    for r in rel:
        p = f"{SCRIPTS}/{r}"
        if r.endswith("/"):
            out += sorted(str(q.relative_to(REPO))
                          for q in (REPO / p).glob("*.py")) if (REPO / p).is_dir() else []
        elif (REPO / p).exists():
            out.append(p)
    return out


#: Per-edge module deps. Hand-listed rather than "every .py under scripts/":
#: the labels edge is 311 clang passes, and making it depend on the whole
#: toolchain would re-run all of them whenever an unrelated module is touched.
TOOL_MODS = _mods("tool/__init__.py", "tool/wine.py", "core/paths.py")
CL_MODS = _mods("graph/cc.py", "tool/cl.py") + TOOL_MODS
COMPDB_MODS = _mods("graph/compdb.py", "tool/clang.py", "manifest.py",
                    "core/paths.py")
LABELS_MODS = _mods("retail_labels/", "tool/clang.py", "core/coff.py",
                    "core/tsv.py", "manifest.py", "core/paths.py", "core/msvc_names.py")
MODEL_MODS = _mods("model.py", "retail_labels/", "core/tsv.py", "core/paths.py",
                   "core/msvc_names.py")
DELINK_MODS = _mods("delink/", "tool/delinker.py", "core/pe.py",
                    "core/coff.py", "core/msvc_names.py", "core/data_matching.py",
                    "model.py") + TOOL_MODS
DATAID_MODS = _mods("graph/dataid.py", "delink/coffx.py")
NORMALIZE_MODS = _mods("compare/normalize.py", "compare/canonicalize.py",
                       "delink/eh_band.py", "core/coff.py", "core/msvc_names.py",
                       "core/data_matching.py")
PROJECT_MODS = _mods("compare/project.py", "compare/normalize.py", "manifest.py")
REPORT_MODS = _mods("tool/objdiff.py")
LINK_MODS = _mods("graph/link.py", "graph/implib.py", "graph/import_contract.py",
                  "graph/static_libraries.py", "tool/link.py",
                  "tool/link_runtime.py",
                  "core/pe.py") + TOOL_MODS
VERIFY_MODS = _mods("verify/", "model.py", "core/tsv.py", "core/paths.py",
                    "core/msvc_names.py", "core/data_matching.py",
                    "walls/pairscan.py", "graph/scan.py")
#: committed inputs of the default-tier verify gates (fast+normal): the MAX
#: ledger and every gate's own baseline/allowlist. Named so a bless re-runs
#: the check edge.
VERIFY_BASELINES = [
    "config/match_baseline.tsv",
    "config/cleanliness/cleanliness-text-baseline.tsv",
    "config/cleanliness/cleanliness-semantic-baseline.tsv",
    "config/cleanliness/tu-order-baseline.tsv",
    "config/cleanliness/data-tu-order-baseline.tsv",
    "config/cleanliness/kept-comdat-exiles.tsv",
    COMPARE_CONFIG,
]
FINGERPRINTS = "build/gen/func_fingerprints.tsv"
VERIFY_STAMP = "build/objdiff/.verify.stamp"
README_STAMP = "build/gen/readme.stamp"
CONFIGURE_MODS = _mods("graph/", "manifest.py", "core/paths.py")


# --------------------------------------------------------------------------- #
# manifest
# --------------------------------------------------------------------------- #
def load_units() -> tuple[dict, list[dict]]:
    """(manifest, units) with each unit's `cflags` resolved from its profile.

    Every [[unit]] names ONE [flags] profile and the profile is the FULL flag
    set - there is no global default to inherit and no per-TU append, so a
    unit's flag choice stays one explicit, greppable name. A stray `extra` key
    is a hard error rather than a silent bolt-on.
    """
    from giten.manifest import load
    data = load()
    profiles = data.get("flags", {})
    if not profiles:
        raise SystemExit(f"{MANIFEST}: [flags] must define at least one profile")
    units = data.get("unit", [])
    if not units:
        raise SystemExit(f"{MANIFEST}: no [[unit]] entries")
    seen: set[str] = set()
    for u in units:
        for key in ("unit", "source", "flags"):
            if key not in u:
                raise SystemExit(f"{MANIFEST}: a [[unit]] is missing '{key}'")
        if u["unit"] in seen:
            raise SystemExit(f"{MANIFEST}: duplicate unit '{u['unit']}'")
        seen.add(u["unit"])
        if u["flags"] not in profiles:
            raise SystemExit(f"{MANIFEST}: unit '{u['unit']}' references unknown "
                             f"flags profile '{u['flags']}' "
                             f"(defined: {sorted(profiles)})")
        if "extra" in u:
            raise SystemExit(
                f"{MANIFEST}: unit '{u['unit']}' sets 'extra' - per-TU flag "
                "bolt-ons are not supported. Add (or reuse) a [flags] profile "
                "carrying the FULL set instead.")
        u["cflags"] = list(profiles[u["flags"]])
    return data, units


# --------------------------------------------------------------------------- #
# orphan artifacts
# --------------------------------------------------------------------------- #
#: (directory, "<prefix>{}<suffix>") pairs whose stems must be live units.
#: build/objdiff/target-new and build/delink/named are NOT listed: their
#: producers rmtree them, so they cannot hold an orphan.
_ORPHAN_PATTERNS = [
    (graph.BASE_DIR, "{}.obj"),
    (f"{graph.COMPARE_DIR}/base", "{}.obj"),
    (f"{graph.COMPARE_DIR}/base", "{}.symbols.tsv"),
    (f"{graph.COMPARE_DIR}/target", "{}.c.obj"),
    (f"{graph.COMPARE_DIR}/target", "{}.symbols.tsv"),
    (graph.CLAIMS_DIR, "{}.tsv"),
]


def prune_orphan_artifacts(units: list[dict]) -> int:
    """Delete build artifacts of units no longer in config/units.toml.

    Ninja has no concept of an output whose EDGE disappeared, so dropping a
    unit - a source deleted, a TU folded, a branch switched in a shared
    worktree - leaves its object, claim fragment and comparison copies behind,
    and every downstream reader that GLOBS rather than follows the graph keeps
    consuming them. That is not cosmetic: giten.delink.pdb_synth and
    giten.delink.data_manifest both read `build/objdiff/base/*.obj`, so a
    stale object re-enrols its vtables and RTTI into the data manifest for a
    unit that has no source in the tree, and `giten.delink.run` collects a
    target object for every stem in build/gen/claims. Prune at configure time,
    where the live unit set is known.

    The delink stamp goes with them: the manifests are regenerated in-process
    from whatever objects survive, and without dropping the stamp a prune that
    leaves bindings.tsv unchanged would never re-run the delinker.
    """
    live = {u["unit"] for u in units}
    stems: set[str] = set()
    for rel, pat in _ORPHAN_PATTERNS:
        d = REPO / rel
        if not d.is_dir():
            continue
        head, tail = pat.split("{}")
        for p in d.iterdir():
            if p.is_file() and p.name.startswith(head) and p.name.endswith(tail):
                stem = p.name[len(head):len(p.name) - len(tail)]
                if stem and stem not in live:
                    stems.add(stem)
    if not stems:
        return 0
    n = 0
    for rel, pat in _ORPHAN_PATTERNS:
        for stem in stems:
            p = REPO / rel / pat.format(stem)
            if p.exists():
                p.unlink()
                n += 1
    stamp = REPO / graph.DELINK_STAMP
    if stamp.exists():
        stamp.unlink()
        n += 1
    return n


# --------------------------------------------------------------------------- #
# the graph
# --------------------------------------------------------------------------- #
def toolchain_id() -> str:
    """The pinned toolchain's identity, as the text that goes in TOOLCHAIN_ID.

    Three values, because three different edges depend on them: $MSVC_DIR and
    $DXSDK_DIR decide what `cl` and the compilation database mean, and the
    vostok-delinker binary decides what the target objects are. All three were
    pure environment, so ninja could not see a re-pin: swapping the delinker
    gave `ninja: no work to do`, and swapping the toolchain recompiled only
    the units that happened to be dirty, mixing two compilers' output in one
    object set.

    Unset/absent values are recorded as `-` rather than skipped: going from
    unset to set is itself a change the edges must see.
    """
    import shutil
    parts = []
    for name in ("MSVC_DIR", "DXSDK_DIR"):
        parts.append(f"{name}={os.environ.get(name) or '-'}")
    delinker = shutil.which("vostok-delinker")
    parts.append("delinker=" + (os.path.realpath(delinker) if delinker else "-"))
    return "\n".join(parts) + "\n"


def write_toolchain_id(out: Path | None = None) -> bool:
    """Write TOOLCHAIN_ID if-changed. True when the content moved.

    If-changed matters: this file is an implicit input of all 300 cl edges, so
    rewriting it unconditionally at every configure would recompile the tree
    whenever anything else re-ran configure.
    """
    path = Path(out) if out is not None else REPO / graph.TOOLCHAIN_ID
    want = toolchain_id()
    if path.exists() and path.read_text() == want:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(want)
    return True


def comparator_id() -> str:
    """Identify the PATH-resolved comparator, including a changed Nix pin."""
    import shutil
    from giten.tool.objdiff import OBJDIFF_CLI
    exe = shutil.which(OBJDIFF_CLI)
    return "objdiff=" + (os.path.realpath(exe) if exe else "-") + "\n"


def write_comparator_id(out: Path | None = None) -> bool:
    """Refresh the report dependency only when the comparator changes."""
    path = Path(out) if out is not None else REPO / graph.COMPARATOR_ID
    want = comparator_id()
    if path.exists() and path.read_text() == want:
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(want)
    return True


def emit_link_phase(w: ninja_syntax.Writer, base_objs: list[str], retail: str) -> None:
    """PHASE 2: base objs -> candidate .EXE + .map. Opt-in, never in `all`.

    The deliverable is the `.map`: every function's link-assigned RVA and its
    source object, which cross-referenced with the retail RVAs is what recovers
    the original build order (intra-TU order = source-definition order,
    cross-TU = object link order). A normal build never links, so this stays
    out of the default target and behind `ninja candidate` / `giten link`.

    The .rsrc compiles from the tracked resource script with the era RC.EXE.
    The payload files it names are written from the locally supplied original
    into an ignored build directory; nothing from .rsrc enters the tracked
    source tree. `giten rsrc check` compares the linked .rsrc with retail's.
    """
    w.comment("=== PHASE 2: link -> candidate .EXE + .map (opt-in: `ninja candidate`) ===")
    w.rule("rsrc_payloads",
           command=(f"$py -m giten.rsrc.payloads --exe $in "
                    f"--out {graph.RESOURCE_PAYLOADS} --stamp $out"),
           description="write the original's resource payloads -> $out", restat=True)
    w.build(graph.RESOURCE_PAYLOAD_LIST, "rsrc_payloads", inputs=retail,
            implicit=_mods("rsrc/payloads.py", "rsrc/tree.py", "core/pe.py",
                           "core/paths.py"))
    w.rule("rc",
           command=(f"$py -m giten.tool.rc --out $out --src $in "
                    f"--include {graph.RESOURCE_PAYLOADS}"),
           description="rc $out")
    w.build(graph.RESOURCE_RES, "rc", inputs=graph.RESOURCE_SCRIPT,
            implicit=[graph.RESOURCE_HEADER, graph.RESOURCE_PAYLOAD_LIST,
                      graph.TOOLCHAIN_ID, *_mods("tool/rc.py"), *TOOL_MODS])
    w.rule("link",
           command=(f"$py -m giten.graph.link --out {graph.CANDIDATE_EXE} "
                    f"--objs-dir {graph.BASE_DIR} --res {graph.RESOURCE_RES}"),
           description="link candidate EXE + map")
    w.build([graph.CANDIDATE_EXE, graph.CANDIDATE_MAP], "link",
            inputs=base_objs,
            implicit=[graph.RESOURCE_RES, MANIFEST, graph.TOOLCHAIN_ID, "build/local/DDS.EXE",
                      "config/retail/link_order.tsv", "config/retail/imports.tsv",
                      "config/retail/link_libraries.tsv", "build/local/lib/dxguid.lib",
                      "config/retail/link_runtime.toml", "build/local/runtime/msvcrt.dll"] + LINK_MODS)
    w.build("candidate", "phony", inputs=[graph.CANDIDATE_EXE, graph.CANDIDATE_MAP])
    w.newline()


#: A conditional directive (`#if`, `#ifdef`, `#ifndef`, `#elif`) and its
#: condition, continuation lines included.
_CONDITIONAL_RE = re.compile(
    r"^[ \t]*#[ \t]*(?:if|ifdef|ifndef|elif)\b((?:[^\n]*\\\n)*[^\n]*)", re.M)
_PLAY_FLAG_RE = re.compile(r"\bGITEN_(?:COMPAT|BUGFIX)\b")
#: include/Ints.h's `GITEN_BUGFIX implies GITEN_COMPAT` conditional. Every unit
#: reaches it and it changes nothing without a use elsewhere, so it alone does
#: not make a unit play-specific. Any other spelling there counts as a use.
_PLAY_FLAG_IMPLICATION = ("include/Ints.h",
                          "defined(GITEN_BUGFIX) && !defined(GITEN_COMPAT)")


def tests_play_flag(rel: str) -> bool:
    """Whether repo file `rel` has a conditional naming a play-build flag."""
    try:
        text = (REPO / rel).read_text(encoding="utf-8", errors="replace")
    except OSError:
        return False
    for m in _CONDITIONAL_RE.finditer(text):
        condition = " ".join(m.group(1).replace("\\\n", " ").split())
        if (rel, condition) == _PLAY_FLAG_IMPLICATION:
            continue
        if _PLAY_FLAG_RE.search(condition):
            return True
    return False


def play_units(cl_edges: list[tuple]) -> set[str]:
    """The units whose source or scanned headers test a play-build flag.

    Every other unit compiles to the same bytes with graph.PLAY_DEFINES as
    without, so the play link reuses its matching object.
    """
    tested: dict[str, bool] = {}
    out = set()
    for _obj, src, headers, _cflags, unit in cl_edges:
        for rel in [src, *headers]:
            if rel not in tested:
                tested[rel] = tests_play_flag(rel)
            if tested[rel]:
                out.add(unit)
                break
    return out


def prune_play_objs(recompiled: set[str]) -> int:
    """Delete play objects of units the play phase no longer recompiles.

    No edge produces them any more (the unit left the manifest or stopped
    testing a play flag), and build/play/obj holds the recompiled units only.
    """
    d = REPO / graph.PLAY_OBJ_DIR
    if not d.is_dir():
        return 0
    n = 0
    for p in d.glob("*.obj"):
        if p.is_file() and p.stem not in recompiled:
            p.unlink()
            n += 1
    return n


def emit_play_phase(w: ninja_syntax.Writer, cl_edges: list[tuple],
                    recompiled: set[str]) -> None:
    """Opt-in (`giten play`): bug-fixed objects + retail .res -> build/play/DDS.EXE.

    The `recompiled` units compile again with their own profile plus
    graph.PLAY_DEFINES into a separate object tree, so the matching objects
    never see the defines. The link takes every unit in the candidate link's
    order (sorted object file names), each from that tree when recompiled and
    from the base objects otherwise. The .res edge is the candidate link's
    (emit_link_phase).
    """
    w.comment("=== play: bug-fixed objects + retail .res -> playable EXE (opt-in: `giten play`) ===")
    link_objs = []
    for obj, src, headers, cflags, unit in sorted(cl_edges, key=lambda e: f"{e[4]}.obj"):
        if unit in recompiled:
            obj = f"{graph.PLAY_OBJ_DIR}/{unit}.obj"
            w.build(obj, "cl", inputs=src,
                    implicit=headers + CL_MODS + [graph.TOOLCHAIN_ID],
                    variables={"unit": unit,
                               "cflags": " ".join([*cflags, *graph.PLAY_DEFINES])})
        link_objs.append(obj)
    w.rule("play_link",
           command=(f"$py -m giten.graph.link --out {graph.PLAY_EXE} "
                    f"--res {graph.RESOURCE_RES} $objs"),
           description="link playable EXE")
    w.build([graph.PLAY_EXE, graph.PLAY_MAP], "play_link", inputs=link_objs,
            implicit=[graph.RESOURCE_RES, MANIFEST, graph.TOOLCHAIN_ID, "build/local/DDS.EXE",
                      "config/retail/link_order.tsv", "config/retail/imports.tsv",
                      "config/retail/link_libraries.tsv", "build/local/lib/dxguid.lib",
                      "config/retail/link_runtime.toml", "build/local/runtime/msvcrt.dll"] + LINK_MODS,
            variables={"objs": " ".join(f"--obj {o}" for o in link_objs)})
    w.build("play", "phony", inputs=[graph.PLAY_EXE])
    w.newline()


def emit(out: Path | None = None) -> tuple[int, int]:
    """Write build/build.ninja. Returns (units, pruned artifacts)."""
    manifest, units = load_units()
    pruned = prune_orphan_artifacts(units)
    out = Path(out) if out is not None else REPO / graph.NINJA
    out.parent.mkdir(parents=True, exist_ok=True)
    # Before the edges that declare it, so the first build after a re-pin sees
    # the new identity rather than racing it.
    write_toolchain_id()
    write_comparator_id()
    scan = Scanner()
    global_cflags = next(iter(manifest["flags"].values()))

    # Resolved BEFORE the writer opens, so `scan.scanned()` is complete by the
    # time the generator edge is emitted (ninja does not care about edge order).
    cl_edges = [(f"{graph.BASE_DIR}/{u['unit']}.obj", u["source"],
                 scan.headers(u["source"]), u["cflags"], u["unit"]) for u in units]
    base_objs = [e[0] for e in cl_edges]
    headers_by_unit = {e[4]: e[2] for e in cl_edges}
    recompiled = play_units(cl_edges)
    pruned += prune_play_objs(recompiled)

    with out.open("w", encoding="utf-8") as f:
        w = ninja_syntax.Writer(f)
        w.comment("GENERATED by giten.graph from config/units.toml - do not edit.")
        w.comment("Regenerate: python3 -m giten.graph   "
                  "Run: ninja -f build/build.ninja (from the repo root)")
        w.newline()

        w.variable("ninja_required_version", "1.11")
        # .ninja_log / .ninja_deps live beside the manifest, not at the repo root.
        w.variable("builddir", "build")
        # The interpreter line pins PYTHONPATH to THIS checkout's scripts. A
        # shell entered in one worktree exports another's, and a build that
        # silently ran a sibling tree's modules is the worst kind of wrong.
        w.variable("py", f"PYTHONPATH={REPO / 'scripts'} GITEN_DIR={REPO} python3")
        w.variable("cflags", " ".join(global_cflags))
        w.newline()

        # Wine serialises more than it appears under one shared wineserver;
        # past ~8 concurrent cl.exe the server thrashes and the build gets
        # SLOWER. Cap the compiler edges without capping ninja's own -j.
        w.pool("wine", graph.WINE_POOL_DEPTH)
        w.newline()

        w.comment("=== generator: re-emit this manifest when configure inputs move ===")
        w.rule("configure", command="$py -m giten.graph",
               description="configure (regenerate build/build.ninja)",
               generator=True)
        # The `cl` dep lists below are baked HERE from the include graph as it
        # stands, so an edit that CHANGES that graph invalidates them. Without
        # the scanned set on this edge nothing notices: ninja keeps using the
        # stale list, and a later edit to a newly-included header does not
        # rebuild its TU. That is silent, and it is exactly the failure a
        # byte-neutrality claim from a header edit depends on not happening.
        w.build(graph.NINJA, "configure",
                implicit=[MANIFEST, *CONFIGURE_MODS, *sorted(scan.scanned())])
        # A scanned source or header deleted since this manifest was written
        # must re-run configure, not abort ninja with "missing and no known
        # rule": an input-less phony edge makes the missing path merely dirty.
        for path in sorted(scan.scanned()):
            w.build(path, "phony")
        w.newline()

        w.comment("=== cl: source -> base .obj (cl 5.0 /O2 /ML under wine) ===")
        w.rule("cl",
               command="$py -m giten.graph.cc --out $out --src $in "
                       "--unit $unit -- $cflags",
               description="cl $unit", pool="wine", restat=True)
        w.newline()
        for obj, src, headers, cflags, unit in cl_edges:
            variables = {"unit": unit}
            if cflags != global_cflags:
                variables["cflags"] = " ".join(cflags)
            w.build(obj, "cl", inputs=src,
                    implicit=headers + CL_MODS + [graph.TOOLCHAIN_ID],
                    variables=variables)
        w.newline()

        w.comment("=== compdb: units.toml -> the clang-cl compilation db ===")
        # Written if-changed + restat, so a manifest edit that leaves every
        # surviving entry intact re-runs nothing downstream. Extraction reads
        # per-TU flags from this file and a unit with NO entry silently falls
        # back to bare MS flags - the edge is what keeps that from rotting.
        # A toolchain re-pin is visible here: $MSVC_DIR/$DXSDK_DIR are part
        # of graph.TOOLCHAIN_ID, which this edge declares.
        w.rule("compdb", command="$py -m giten.graph.compdb --quiet",
               description="compdb", restat=True)
        w.build(COMPDB, "compdb", inputs=MANIFEST,
                implicit=COMPDB_MODS + [graph.TOOLCHAIN_ID])
        w.newline()

        w.comment("=== labels: source + headers + base obj -> per-unit claim fragment ===")
        # One TU's clang IR pass per edge (the expensive step), so a single
        # edit re-extracts only THAT unit. Fragments are written if-changed and
        # the rule restats, so an unchanged symbol set stops here and never
        # reaches model/delink. The header closure is independently required:
        # a header-only RVA claim can be renamed while cl emits no COMDAT in
        # this TU, leaving the object byte-identical and therefore restatted.
        w.rule("labels", command="$py -m giten.retail_labels.source --unit $unit",
               description="labels $unit", restat=True)
        fragments = []
        for u in units:
            frag = f"{graph.CLAIMS_DIR}/{u['unit']}.tsv"
            fragments.append(frag)
            w.build(frag, "labels", inputs=u["source"],
                    implicit=[*headers_by_unit[u["unit"]],
                              f"{graph.BASE_DIR}/{u['unit']}.obj", MANIFEST,
                              COMPDB, *LABELS_MODS],
                    variables={"unit": u["unit"]})
        w.newline()

        w.comment("=== reloc_image: retail + reloc_sites.tsv -> build/exe/DDS.EXE ===")
        # DDS.EXE is a /FIXED link with no .reloc; every relocation-driven
        # consumer ($GITEN_EXE) reads this copy, which appends the reviewed,
        # synthesized site set as a genuine .reloc (docs/relocations.md).
        retail = os.environ.get("GITEN_RETAIL_EXE")
        if not retail:
            raise SystemExit("[configure] $GITEN_RETAIL_EXE unset - run inside `nix develop`")
        w.rule("reloc_image",
               command="$py -m giten.delink.reloc_image --retail $in --out $out",
               description="synthesize .reloc -> $out", restat=True)
        w.build(RETAIL_EXE, "reloc_image", inputs=retail,
                implicit=[RELOC_SITES, *_mods("delink/reloc_image.py", "core/tsv.py",
                                              "core/paths.py")])
        w.newline()

        w.comment("=== model: claims x censuses/providers -> bindings.tsv ===")
        w.rule("model", command="$py -m giten.model", description="model",
               restat=True)
        w.build([graph.BINDINGS, graph.VIOLATIONS], "model", inputs=fragments,
                implicit=MODEL_TABLES + MODEL_MODS + [RETAIL_EXE])
        w.newline()

        w.comment("=== delink: bindings -> synth pdb -> per-unit target objs ===")
        # Keyed on the bindings CONTENT: the delinker re-resolves the model
        # itself, so bindings.tsv is the fingerprint of everything that decides
        # the delink, and model writes it if-changed. A pure code edit never
        # reaches here. The declared output is a STAMP - units with no claim
        # produce no object, so declaring all of them would leave the edge
        # perpetually unbuilt and re-run the whole delink on every build.
        # giten.delink.{pdb_synth,data_manifest} also read the base objects'
        # data topology (COMMONs, .bss/.data members, string/vtable/RTTI
        # COMDATs), which a compile can move without moving a claim - a
        # storage-class change alone is one. The objects themselves would
        # re-delink on every code edit, so the edge keys on their DATA_IDS
        # rendering instead, written if-changed and restatted.
        w.rule("dataid",
               command=(f"$py -m giten.graph.dataid --base-dir {graph.BASE_DIR} "
                        f"--out $out"),
               description="base-object data identity -> $out", restat=True)
        w.build(graph.DATA_IDS, "dataid", inputs=base_objs,
                implicit=DATAID_MODS)
        w.rule("delink",
               command=(f"$py -m giten.delink.run --target-dir {graph.TARGET_DIR} "
                        f"--delink-dir {graph.DELINK_RAW} && touch $out"),
               description="delink DDS.EXE -> target objs")
        w.build(graph.DELINK_STAMP, "delink",
                inputs=[graph.BINDINGS, RETAIL_EXE],
                implicit=[RELOC_REFERENTS, COMPARE_CONFIG, *DELINK_MODS,
                          graph.DATA_IDS, graph.TOOLCHAIN_ID])
        w.newline()

        w.comment("=== normalize: base + target -> content-addressed copies ===")
        # objdiff pairs BY NAME, so compiler-private data names ($SG/$T/$S),
        # weak externals and jump-table DIR32 labels are rewritten into
        # disposable side-by-side copies. The real objects are untouched, so
        # the transform is matching-NEUTRAL. One stamped edge drives the set;
        # the driver mtime-skips unchanged objects, so a single recompile
        # re-normalizes exactly one pair.
        w.rule("normalize",
               command=(f"$py -m giten.compare.normalize --base-dir {graph.BASE_DIR} "
                        f"--target-dir {graph.TARGET_DIR} --out-dir {graph.COMPARE_DIR} "
                        f"--stamp $out"),
               description="normalize base/target objs")
        w.build(graph.NORMALIZE_STAMP, "normalize",
                inputs=base_objs + [graph.DELINK_STAMP],
                implicit=[MANIFEST, COMPARE_CONFIG, *NORMALIZE_MODS])
        w.newline()

        w.comment("=== project: the delinked directory -> objdiff.json ===")
        # AFTER the delink, because the pairing census is a DIRECTORY READ:
        # whether a unit pairs with its real target object or with the empty
        # dummy is read off what the delinker wrote, never predicted. Predicting
        # it is what once left two data-only units on the dummy - a pairing
        # objdiff scores 100.00% on every measure with zero totals.
        w.rule("project",
               command=(f"$py -m giten.compare.project --target-dir {graph.TARGET_DIR} "
                        f"--out-dir {graph.COMPARE_DIR}"),
               description="project (pairing -> objdiff.json)", restat=True)
        w.build(graph.OBJDIFF_JSON, "project", inputs=[graph.DELINK_STAMP],
                implicit=[MANIFEST, *PROJECT_MODS])
        w.newline()

        w.comment("=== report: comparison copies + pairing -> report.json ===")
        # In-graph so it regenerates when an object, pairing or comparator moved,
        # which is what lets `giten match` say "nothing rebuilt, nothing to
        # report" instead of re-scoring 311 units for a no-op build.
        w.rule("report",
               command=(f"$py -m giten.tool.objdiff --project {graph.COMPARE_DIR} "
                        f"--out $out"),
               description="objdiff report")
        w.build(graph.REPORT_JSON, "report",
                inputs=[graph.NORMALIZE_STAMP, graph.OBJDIFF_JSON],
                implicit=REPORT_MODS + [graph.COMPARATOR_ID])
        w.newline()

        w.comment("=== verify: fingerprints (beside compare) + the tiered "
                  "check (after) ===")
        # The fingerprint cache is BINDINGS x source/header closure x clangd.
        # It needs no report, so ninja may schedule it alongside compare. The
        # ordering that matters is fingerprints-before-CHECK, and the check
        # edge's inputs state it. The cache keeps the MAX gate's edit
        # detection honest (a stale cache degrades TOUCHED/REGRESS).
        w.rule("verify_fp", command="$py -m giten.verify fingerprints",
               description="verify fingerprints", restat=True)
        w.build(FINGERPRINTS, "verify_fp",
                inputs=sorted(scan.scanned()),
                implicit=[graph.BINDINGS, MANIFEST, COMPDB, *VERIFY_MODS])
        # The fast+normal tiers, opt-in via the `verify` target: gates run
        # when preparing a merge, never in the matching loop. The full/link
        # tiers stay behind `giten verify check --tier full`.
        w.rule("verify_check",
               command="$py -m giten.verify check && touch $out",
               description="verify check (MAX gate + fast+normal tiers)")
        # The README score block is a pure function of the report and the
        # ledger, so the default build keeps it current (write-if-changed).
        w.rule("verify_readme",
               command="$py -m giten.verify readme && touch $out",
               description="verify readme (score block)")
        w.build(README_STAMP, "verify_readme",
                inputs=[graph.REPORT_JSON, FINGERPRINTS],
                implicit=[MANIFEST, "config/match_baseline.tsv", *VERIFY_MODS])
        w.build(VERIFY_STAMP, "verify_check",
                inputs=[graph.REPORT_JSON, FINGERPRINTS],
                implicit=[MANIFEST, *VERIFY_BASELINES, *VERIFY_MODS])
        w.newline()

        w.comment("=== aliases ===")
        w.build("base", "phony", inputs=base_objs)
        w.build("claims", "phony", inputs=fragments)
        w.build("target", "phony", inputs=[graph.DELINK_STAMP])
        w.build("compare", "phony", inputs=[graph.REPORT_JSON])
        w.build("verify", "phony", inputs=[VERIFY_STAMP])
        w.build("all", "phony",
                inputs=base_objs + [graph.BINDINGS, graph.DELINK_STAMP,
                                    graph.OBJDIFF_JSON, graph.REPORT_JSON,
                                    FINGERPRINTS, README_STAMP])
        w.default(["all"])
        w.newline()

        emit_link_phase(w, base_objs, retail)
        emit_play_phase(w, cl_edges, recompiled)

    return len(units), pruned


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="giten configure", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", type=Path, help=f"manifest path (default {graph.NINJA})")
    a = ap.parse_args(argv)
    try:
        n, pruned = emit(a.out)
    except OSError as e:
        print(f"[configure] cannot write {a.out or graph.NINJA}: {e}",
              file=sys.stderr)
        return 1
    if pruned:
        print(f"[configure] pruned {pruned} stale artifact(s): units no longer "
              "in config/units.toml, play objects no longer recompiled",
              file=sys.stderr)
    print(f"[configure] wrote {a.out or graph.NINJA} ({n} units)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
