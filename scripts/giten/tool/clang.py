"""giten.tool.clang - the native front-end (extraction's tool).

Four probes over one TU, all under the MSVC-compat flag set:
    emit_ir()   textual LLVM IR - @llvm.global.annotations pairs each RVA()
                annotation DIRECTLY with the function's mangled symbol
    ast_dump()  JSON AST - VarDecls for the DATA() join
    var_facts() pylibclang - exact byte extents and storage of main-file globals
    annotated_decls()  pylibclang - annotated declarations in repo files (the
                label-only RVA_DECL channel)

The pylibclang probes accept one shared `parse()` of the TU.

Per-TU flags come from the clangd compdb (`/imsvc` lowercase-mirror include
dirs that make header lookup work on case-sensitive Linux), falling back to
the bare MS flag set. clang's mangled name is a PROPOSAL in one narrow sense
only: cl 5.0's own spelling is a deterministic rewrite of it, applied by
core/msvc_names.
"""

from __future__ import annotations

import functools
import json
import os
import subprocess
import tempfile
from pathlib import Path

from giten.core.paths import BUILD, INCLUDE, REPO, VENDOR, dxsdk_dir

COMPDB = BUILD / "clangd/compile_commands.json"

TARGET = "i686-pc-windows-msvc"
MSC_COMPAT = "1100"
# Two VC5-accepted constructs are hard errors in clang: `&Temporary()`
# (MSVC C4238) and a signed switch whose SDK case macro is an unsigned
# 0x80000000-range `long`. Demote both so the probes read the same dialect.
MS_WARN = ["-Wno-address-of-temporary", "-Wno-c++11-narrowing"]
MS_FLAGS = [f"--target={TARGET}", f"-fms-compatibility-version={MSC_COMPAT}",
            "-fms-extensions", *MS_WARN]


def _include_dirs() -> list[str]:
    dirs = [str(INCLUDE)]
    if VENDOR.is_dir():
        dirs += sorted(str(d) for d in VENDOR.iterdir() if d.is_dir())
    # DX6 SDK headers must win over the toolchain's DirectX 3-era ones.
    try:
        dx = dxsdk_dir() / "Include"
        if dx.is_dir():
            dirs.append(str(dx))
    except RuntimeError:
        pass
    return dirs


def inc_cl() -> list[str]:
    return [f"/I{d}" for d in _include_dirs()]


def inc_gcc() -> list[str]:
    return [f"-I{d}" for d in _include_dirs()]


def compdb(path: Path = COMPDB) -> dict[str, list[str]]:
    """{realpath(source): [clang-cl flags]} - driver, `/c` and the TU dropped
    (each probe re-adds its own driver mode, action, and source)."""
    try:
        db = json.loads(Path(path).read_text())
    except (OSError, json.JSONDecodeError):
        return {}
    out = {}
    for entry in db:
        args = entry.get("arguments") or []
        flags = [a for a in args[1:]
                 if a not in ("/c", "-c") and a != entry.get("file")
                 and not a.startswith("/Fo")]
        src = os.path.realpath(os.path.join(entry.get("directory", "."),
                                            entry["file"]))
        out[src] = flags
    return out


def _clang() -> str:
    return os.environ.get("GITEN_CLANG") or "clang"


#: clang's stderr from the last emit_ir that produced no IR, for the caller's
#: FATAL message (extraction runs one TU per process).
LAST_IR_ERROR = ""


def emit_ir(tu: str, cl_flags: list[str] | None) -> str | None:
    """Textual LLVM IR, or None (an error the caller must surface - a TU that
    compiles under cl but yields no IR would silently drop every label; its
    clang diagnostics are left in LAST_IR_ERROR)."""
    global LAST_IR_ERROR
    if cl_flags is not None:
        # clang-cl rejects `-S -o -`; -emit-llvm writes only to a real file.
        # Retried once: under parallel extraction the temp .ll can vanish.
        for _attempt in range(2):
            with tempfile.NamedTemporaryFile(suffix=".ll", delete=False) as tf:
                ll = tf.name
            try:
                cmd = [_clang(), "--driver-mode=cl", "/c", "/DGITEN_EMIT_META",
                       *cl_flags, *MS_WARN, *inc_cl(),
                       "-Xclang", "-emit-llvm", "-o", ll, tu]
                res = subprocess.run(cmd, capture_output=True, text=True)
                ir = Path(ll).read_text() \
                    if os.path.exists(ll) and os.path.getsize(ll) else ""
            finally:
                try:
                    os.unlink(ll)
                except OSError:
                    pass
            if ir:
                return ir
        LAST_IR_ERROR = res.stderr
        return None  # the caller surfaces this, with LAST_IR_ERROR
    cmd = [_clang(), "-DGITEN_EMIT_META", *MS_FLAGS, *inc_gcc(),
           "-S", "-emit-llvm", "-o", "-", tu]
    res = subprocess.run(cmd, capture_output=True, text=True)
    return res.stdout or None


def ast_dump(tu: str, cl_flags: list[str] | None) -> dict | None:
    if cl_flags is not None:
        cmd = [_clang(), "--driver-mode=cl", "/DGITEN_EMIT_META", *cl_flags,
               *inc_cl(), tu, "-fsyntax-only", "-Xclang", "-ast-dump=json"]
    else:
        cmd = [_clang(), "-DGITEN_EMIT_META", *MS_FLAGS, *inc_gcc(), tu,
               "-fsyntax-only", "-Xclang", "-ast-dump=json"]
    res = subprocess.run(cmd, capture_output=True, text=True)
    try:
        return json.loads(res.stdout)
    except json.JSONDecodeError:
        return None


#: realpath per cursor dominates a walk; a file's answer cannot change mid-run
_realpath = functools.lru_cache(maxsize=None)(os.path.realpath)


def parse(tu: str, cl_flags: list[str] | None):
    """One pylibclang parse under the probes' flag set, or None when
    pylibclang is missing or the TU does not parse cleanly. The libclang
    probes below share it: a second parse of the same TU and flags can only
    repeat the first."""
    try:
        import clang.cindex as cidx
    except ImportError:
        return None
    args = (["--driver-mode=cl", "/DGITEN_EMIT_META", *cl_flags, *inc_cl()]
            if cl_flags is not None
            else ["-DGITEN_EMIT_META", *MS_FLAGS, *inc_gcc()])
    try:
        parsed = cidx.Index.create().parse(tu, args=args)
    except cidx.LibclangError:
        return None
    if any(d.severity >= cidx.Diagnostic.Error for d in parsed.diagnostics):
        return None
    return parsed


def preorder(parsed, kinds) -> list:
    """The cursors of `kinds`, in `walk_preorder()` order, from ONE native
    recursive visit. walk_preorder builds a Python child list per node, which
    costs seconds over a TU that includes the SDK; recursing inside libclang
    and filtering on the raw kind id visits the same cursors in the same
    order."""
    from clang.cindex import conf, cursor_visit_callback
    ids = {k.value for k in kinds}
    root = parsed.cursor
    out = [root] if root._kind_id in ids else []

    def visit(child, _parent, acc):
        if child._kind_id in ids:
            child._tu = parsed          # keep the TU alive, as get_children does
            acc.append(child)
        return 2                        # CXChildVisit_Recurse

    conf.lib.clang_visitChildren(root, cursor_visit_callback(visit), out)
    return out


def function_definition_extents(tu: str, cl_flags: list[str] | None) -> dict[str, list[dict]] | None:
    """Exact mangled function definitions and their spelling-file byte extents.

    Unlike documentSymbol, this includes definitions in headers. Declarations,
    cross-file extents and invalid parses provide no source-body evidence.
    File ownership and hashing are the consumer's policy.
    """
    parsed = parse(tu, cl_flags)
    if parsed is None:
        return None
    import clang.cindex as cidx
    kinds = {cidx.CursorKind.FUNCTION_DECL, cidx.CursorKind.CXX_METHOD,
             cidx.CursorKind.CONSTRUCTOR, cidx.CursorKind.DESTRUCTOR}
    out: dict[str, list[dict]] = {}
    for cursor in preorder(parsed, kinds):
        if not cursor.is_definition():
            continue
        name = cursor.mangled_name
        start, end = cursor.extent.start, cursor.extent.end
        if not name or start.file is None or end.file is None:
            continue
        path = _realpath(start.file.name)
        if path != _realpath(end.file.name) or end.offset <= start.offset:
            continue
        out.setdefault(name, []).append({"file": path, "start": start.offset,
                                        "end": end.offset})
    return out


def var_facts(tu: str, cl_flags: list[str] | None, parsed=None) -> dict[str, dict] | None:
    """{mangled VarDecl name: {'size': bytes, 'internal': bool, 'defined': bool}} for main-file
    globals - THE DATA-extent authority (laid out under the TU's real
    i386/MSVC flags) and the storage a claim's cl 5.0 spelling depends on.

    The key is libclang's mangled name, which already carries the i386 COFF
    global prefix. `internal` is true for anything cl gives TU-local storage:
    a file static, a namespace-scope `const`, and a function-local static (no
    linkage at all) alike. None when pylibclang could not parse cleanly;
    incomplete types (negative get_size) and cross-decl size conflicts are
    omitted. `parsed` reuses a `parse()` of the same TU and flags."""
    parsed = parsed if parsed is not None else parse(tu, cl_flags)
    if parsed is None:
        return None
    import clang.cindex as cidx
    main_real = os.path.realpath(tu)
    facts: dict[str, dict] = {}
    conflicts = set()
    for cursor in preorder(parsed, {cidx.CursorKind.VAR_DECL}):
        if cursor.location.file is None:
            continue
        if _realpath(cursor.location.file.name) != main_real:
            continue
        name, size = cursor.mangled_name, cursor.type.get_size()
        if not name or size < 0:
            continue
        internal = cursor.linkage != cidx.LinkageKind.EXTERNAL
        fact = {"size": size, "internal": internal, "defined": cursor.is_definition()}
        if name in facts and any(facts[name][k] != fact[k] for k in ("size", "internal")):
            conflicts.add(name)
        else:
            fact["defined"] |= facts.get(name, {}).get("defined", False)
            facts[name] = fact
    for name in conflicts:
        facts.pop(name, None)
    return facts


def annotated_decls(tu: str, cl_flags: list[str] | None, parsed=None) -> list[dict] | None:
    """Every annotated function/variable declaration the TU sees in a repo
    file (the TU itself or a project header), one record per redeclaration:
    {'kind': 'func'|'var', 'name': libclang's mangled name, 'annotations':
    [str], 'defined': bool, 'file': realpath, 'internal': bool}.

    A clang annotation on a declaration never reaches IR, so the declaration
    channel (`RVA_DECL`) reads it here. Only namespace/record/linkage scopes
    are descended (never a function body), and a cursor outside the repo
    (SDK, CRT) is skipped before its children are read. None when pylibclang
    could not parse cleanly; `parsed` reuses a `parse()` of the same TU."""
    parsed = parsed if parsed is not None else parse(tu, cl_flags)
    if parsed is None:
        return None
    import clang.cindex as cidx
    from clang.cindex import conf, cursor_visit_callback
    K = cidx.CursorKind
    scopes = {k.value for k in (K.NAMESPACE, K.CLASS_DECL, K.STRUCT_DECL,
                                K.UNION_DECL, K.LINKAGE_SPEC, K.UNEXPOSED_DECL)}
    funcs = {k.value for k in (K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR,
                               K.DESTRUCTOR)}
    decls = funcs | {K.VAR_DECL.value}
    annotate = K.ANNOTATE_ATTR.value
    repo = os.path.realpath(REPO) + os.sep
    real: dict[str, str] = {}
    out: list[dict] = []

    def in_repo(cursor) -> str | None:
        f = cursor.location.file
        if f is None:
            return None
        path = real.get(f.name)
        if path is None:
            path = real[f.name] = os.path.realpath(f.name)
        return path if path.startswith(repo) else None

    # One native visit (see preorder): scopes recurse, a declaration is taken
    # with its direct ANNOTATE_ATTR children, anything else is not descended.
    found: list[tuple] = []

    def visit(child, parent, acc):
        if parent._kind_id in decls:            # a declaration's own children
            if child._kind_id == annotate:
                child._tu = parsed
                acc[-1][1].append(child)
            return 1                            # CXChildVisit_Continue
        if child._kind_id in scopes:
            return 2                            # CXChildVisit_Recurse
        if child._kind_id in decls:
            child._tu = parsed
            acc.append((child, []))
            return 2
        return 1

    conf.lib.clang_visitChildren(parsed.cursor, cursor_visit_callback(visit), found)
    for cursor, attrs in found:
        path = in_repo(cursor)
        if path is None or not attrs:
            continue
        out.append({"kind": "func" if cursor._kind_id in funcs else "var",
                    "name": cursor.mangled_name,
                    "annotations": [c.spelling for c in attrs],
                    "defined": cursor.is_definition(),
                    "file": path,
                    "internal": cursor.linkage != cidx.LinkageKind.EXTERNAL})
    return out
