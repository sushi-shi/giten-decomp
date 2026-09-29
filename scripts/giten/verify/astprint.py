"""giten.verify.astprint - per-function AST fingerprints (the `ast:` domain).

A function's src_hash should change when its implementation changes, not when
its spelling does. The fingerprint hashes the libclang AST of each definition
in a unit's own source file, as the retail compile sees it: node kinds,
operators, canonical types, literal values and referenced declarations, with
every enum constant replaced by its value. Comments, layout, macro names,
typedef names and enumerator names therefore leave it unchanged; any change to
expressions, statements, types, calls or literal values changes it.
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path

from giten.core.paths import BUILD, REPO

CDB = BUILD / "clangd/compile_commands.json"
PREFIX = "ast:"

_FUNCTION_KINDS = ("FUNCTION_DECL", "CXX_METHOD", "CONSTRUCTOR", "DESTRUCTOR",
                   "CONVERSION_FUNCTION")


def _flags(entry: dict) -> list[str]:
    args = list(entry.get("arguments") or entry["command"].split())
    src = entry["file"]
    out = ["--driver-mode=cl"]
    for arg in args[1:]:
        if arg == "/c" or arg == src or arg.endswith(src):
            continue
        out.append(arg)
    return out


def _entry(path: Path) -> dict | None:
    if not CDB.is_file():
        return None
    for entry in json.loads(CDB.read_text()):
        file = Path(entry["file"])
        if not file.is_absolute():
            file = Path(entry.get("directory") or REPO) / file
        if file.resolve() == path.resolve():
            return entry
    return None


def _qualified(cursor) -> str:
    parts = [cursor.spelling]
    parent = cursor.semantic_parent
    while parent is not None and parent.kind.name in (
            "CLASS_DECL", "STRUCT_DECL", "NAMESPACE", "UNION_DECL"):
        parts.append(parent.spelling)
        parent = parent.semantic_parent
    return "::".join(reversed(parts))


def _operator(node) -> str:
    try:
        op = node.binary_operator
        if op is not None and op.name != "Invalid":
            return op.name
    except (AttributeError, ValueError):
        pass
    return node.spelling or ""


def _literal(cidx, node) -> str:
    tokens = [t.spelling for t in node.get_tokens()]
    text = tokens[0] if tokens else ""
    if node.kind == cidx.CursorKind.INTEGER_LITERAL:
        body = text.rstrip("uUlL")
        try:
            return str(int(body, 0))
        except ValueError:
            return text
    if node.kind == cidx.CursorKind.FLOATING_LITERAL:
        try:
            return repr(float(text.rstrip("fFlL")))
        except ValueError:
            return text
    return text


_EVAL = None


def _evaluator(cidx):
    """libclang's clang_Cursor_Evaluate through ctypes (not in the binding)."""
    global _EVAL
    if _EVAL is None:
        import ctypes
        lib = cidx.conf.lib
        lib.clang_Cursor_Evaluate.argtypes = [cidx.Cursor]
        lib.clang_Cursor_Evaluate.restype = ctypes.c_void_p
        lib.clang_EvalResult_getKind.argtypes = [ctypes.c_void_p]
        lib.clang_EvalResult_getKind.restype = ctypes.c_int
        lib.clang_EvalResult_getAsLongLong.argtypes = [ctypes.c_void_p]
        lib.clang_EvalResult_getAsLongLong.restype = ctypes.c_longlong
        lib.clang_EvalResult_getAsDouble.argtypes = [ctypes.c_void_p]
        lib.clang_EvalResult_getAsDouble.restype = ctypes.c_double
        lib.clang_EvalResult_dispose.argtypes = [ctypes.c_void_p]
        lib.clang_EvalResult_dispose.restype = None
        _EVAL = lib
    return _EVAL


def _constant(cidx, node) -> str | None:
    """The value of a literal-only subtree (literals, signs, parentheses,
    implicit conversions and enum constants), else None."""
    allowed = {cidx.CursorKind.INTEGER_LITERAL, cidx.CursorKind.FLOATING_LITERAL,
               cidx.CursorKind.PAREN_EXPR, cidx.CursorKind.UNARY_OPERATOR,
               cidx.CursorKind.UNEXPOSED_EXPR, cidx.CursorKind.DECL_REF_EXPR}
    for sub in node.walk_preorder():
        if sub.kind not in allowed:
            return None
        if sub.kind == cidx.CursorKind.DECL_REF_EXPR and (
                sub.referenced is None
                or sub.referenced.kind != cidx.CursorKind.ENUM_CONSTANT_DECL):
            return None
    if not any(sub.kind in (cidx.CursorKind.INTEGER_LITERAL,
                            cidx.CursorKind.FLOATING_LITERAL,
                            cidx.CursorKind.DECL_REF_EXPR)
               for sub in node.walk_preorder()):
        return None
    lib = _evaluator(cidx)
    result = lib.clang_Cursor_Evaluate(node)
    if not result:
        return None
    try:
        kind = lib.clang_EvalResult_getKind(result)
        if kind == 1:
            return str(lib.clang_EvalResult_getAsLongLong(result))
        if kind == 2:
            return repr(lib.clang_EvalResult_getAsDouble(result))
        return None
    finally:
        lib.clang_EvalResult_dispose(result)


def _canonical(ty) -> str:
    return ty.get_canonical().spelling if ty is not None else ""


def _emit(cidx, node, out: list[str]) -> None:
    kind = node.kind
    if kind.is_expression():
        value = _constant(cidx, node)
        if value is not None:
            out.append(f"CONST|{value}|{_canonical(node.type)}")
            return
    if kind == cidx.CursorKind.PAREN_EXPR:
        for child in node.get_children():
            _emit(cidx, child, out)
        return
    item = [kind.name]
    if kind in (cidx.CursorKind.INTEGER_LITERAL, cidx.CursorKind.FLOATING_LITERAL,
                cidx.CursorKind.CHARACTER_LITERAL, cidx.CursorKind.STRING_LITERAL):
        item.append(_literal(cidx, node))
    elif kind in (cidx.CursorKind.DECL_REF_EXPR, cidx.CursorKind.MEMBER_REF_EXPR):
        ref = node.referenced
        if ref is not None and ref.kind == cidx.CursorKind.ENUM_CONSTANT_DECL:
            item = ["INTEGER_LITERAL", str(ref.enum_value)]
        else:
            item.append(node.spelling)
    elif kind in (cidx.CursorKind.BINARY_OPERATOR,
                  cidx.CursorKind.COMPOUND_ASSIGNMENT_OPERATOR,
                  cidx.CursorKind.UNARY_OPERATOR):
        item.append(_operator(node))
        if kind == cidx.CursorKind.UNARY_OPERATOR:
            item.append(" ".join(t.spelling for t in node.get_tokens())[:1])
    elif kind in (cidx.CursorKind.VAR_DECL, cidx.CursorKind.PARM_DECL,
                  cidx.CursorKind.FIELD_DECL):
        item += [node.spelling, str(node.storage_class)]
    elif kind == cidx.CursorKind.LABEL_STMT or kind == cidx.CursorKind.LABEL_REF:
        item.append(node.spelling)
    if kind.is_expression() or kind in (cidx.CursorKind.VAR_DECL,
                                        cidx.CursorKind.PARM_DECL):
        item.append(_canonical(node.type))
    out.append("|".join(item))
    for child in node.get_children():
        _emit(cidx, child, out)


def unit_fingerprints(source: str) -> dict[str, str]:
    """{qualified source name: 'ast:<12 hex>'} for the unit's own definitions."""
    import clang.cindex as cidx

    path = (REPO / source).resolve()
    entry = _entry(path)
    if entry is None or not path.is_file():
        return {}
    tu = cidx.Index.create().parse(str(path), args=_flags(entry))
    errors = [d for d in tu.diagnostics if d.severity >= cidx.Diagnostic.Error]
    if errors:
        raise RuntimeError(f"{source}: cannot fingerprint, the unit does not "
                           f"parse: {errors[0]}")
    chunks: dict[str, list[str]] = {}
    for node in tu.cursor.walk_preorder():
        if node.kind.name not in _FUNCTION_KINDS or not node.is_definition():
            continue
        if node.location.file is None or Path(node.location.file.name).resolve() != path:
            continue
        out = [f"FUNCTION|{_canonical(node.type)}|{node.storage_class}"]
        for child in node.get_children():
            _emit(cidx, child, out)
        chunks.setdefault(_qualified(node), []).append("\n".join(out))
    return {name: PREFIX + hashlib.sha1("\n--\n".join(parts).encode()).hexdigest()[:12]
            for name, parts in chunks.items()}
