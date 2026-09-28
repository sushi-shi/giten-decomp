"""Find boolean-valued returns and locals with pylibclang; optionally rewrite them.

    python3 -m giten.graph.compdb
    python3 -m giten.tool.bool_returns          # report only
    python3 -m giten.tool.bool_returns --write  # definitions and redeclarations

Only src/ and include/ are edited. Integer boolean aliases preserve the exact
underlying type (including signedness); C++ bool remains bool. Unknown values,
indirect/virtual calls, macro-generated declarations and missing return paths
are not inferred. Recursive call groups need a concrete boolean return anchor.
"""
from __future__ import annotations

import argparse
from collections import defaultdict
from dataclasses import dataclass, field
from ctypes import c_void_p, c_int, c_longlong
from pathlib import Path
import re
import sys

from clang import cindex as ci

from giten.core.paths import REPO
from giten.tool import clang

K = ci.CursorKind
FUNCTIONS = {K.FUNCTION_DECL, K.CXX_METHOD, K.CONVERSION_FUNCTION}
SCOPES = {K.NAMESPACE, K.STRUCT_DECL, K.CLASS_DECL, K.UNION_DECL,
          K.LINKAGE_SPEC, K.UNEXPOSED_DECL}
WRAPPERS = {K.UNEXPOSED_EXPR, K.PAREN_EXPR, K.CXX_STATIC_CAST_EXPR}
# Do not substitute bool for an integer: that changes the ABI and codegen.
ALIASES = {'signed char': 'b8', 'short': 'b16', 'int': 'b32',
           'unsigned char': 'ub8', 'unsigned short': 'ub16',
           'unsigned int': 'ub32', 'long long': 'b64',
           'unsigned long long': 'ub64', 'bool': 'bool'}


@dataclass
class Function:
    cursor: object
    returns: list = field(default_factory=list)
    declarations: dict = field(default_factory=dict)
    dependencies: set[str] = field(default_factory=set)
    literals: set = field(default_factory=set)
    anchor: bool = False
    valid: bool = True
    variants: list = field(default_factory=list)


def project_file(cursor, root):
    if cursor.location.file:
        path = Path(cursor.location.file.name).resolve()
        if any(path.is_relative_to(root / d) for d in ('src', 'include')):
            return path
    return None


def body_returns(cursor):
    for child in cursor.get_children():
        if child.kind in FUNCTIONS | SCOPES | {K.LAMBDA_EXPR}:
            continue
        if child.kind == K.RETURN_STMT:
            yield child
        else:
            yield from body_returns(child)


def collect(units, root=REPO):
    """Keep TUs alive: cursors and tokens refer to libclang-owned memory."""
    parsed, functions, declarations = [], {}, defaultdict(dict)
    index = ci.Index.create()
    for path, flags in sorted(units.items()):
        if not Path(path).is_relative_to(root / 'src'):
            continue
        tu = index.parse(path, args=['--driver-mode=cl', *flags,
                                     *clang.MS_WARN, '-Wreturn-type'])
        errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
        if errors:
            raise ValueError('\n'.join(errors))
        parsed.append(tu)
        missing = [d.location for d in tu.diagnostics if d.option == '-Wreturn-type']

        def visit(parent):
            for c in parent.get_children():
                file = project_file(c, root)
                if not file:
                    continue
                if c.kind in SCOPES:
                    visit(c)
                if c.kind not in FUNCTIONS:
                    continue
                usr = c.get_usr()
                declarations[usr][(file, c.location.offset)] = c
                if c.is_definition():
                    f = Function(c, list(body_returns(c)))
                    f.valid = not any(loc.file == c.location.file and
                                      c.extent.start.offset <= loc.offset <= c.extent.end.offset
                                      for loc in missing)
                    f.variants = [(c, f.returns)]
                    if usr in functions:
                        functions[usr].variants.extend(f.variants)
                        functions[usr].valid &= f.valid
                    else:
                        functions[usr] = f
        visit(tu.cursor)
    for usr, f in functions.items():
        f.declarations = declarations[usr]
    return parsed, functions


def operator(cursor, children):
    """Read the operator between operands, not operators inside operands."""
    if len(children) != 2:
        return ''
    return ' '.join(t.spelling for t in cursor.get_tokens()
                    if children[0].extent.end.offset <= t.extent.start.offset
                    and t.extent.end.offset <= children[1].extent.start.offset)


def local_values(cursor):
    """Flow-insensitive proof for initialized, unaliased scalar local flags."""
    nodes = []

    def visit(c):
        for child in c.get_children():
            if child.kind in FUNCTIONS | SCOPES | {K.LAMBDA_EXPR}:
                continue
            nodes.append(child)
            visit(child)
    visit(cursor)
    values = {}
    blocked = set()
    for c in nodes:
        children = list(c.get_children())
        if c.kind == K.VAR_DECL:
            # Static state can be modified by earlier/reentrant calls.
            if c.storage_class == ci.StorageClass.STATIC:
                blocked.add(c.get_usr())
            values[c.get_usr()] = children[-1:] if children else []
        if c.kind == K.BINARY_OPERATOR and operator(c, children) == '=':
            lhs = children[0]
            while lhs.kind in {K.PAREN_EXPR, K.UNEXPOSED_EXPR}:
                operands = list(lhs.get_children())
                if len(operands) != 1:
                    break
                lhs = operands[0]
            if (lhs.kind == K.DECL_REF_EXPR and lhs.referenced
                    and lhs.referenced.get_usr() in values):
                values[lhs.referenced.get_usr()].append(children[1])
        tokens = list(c.get_tokens()) if c.kind == K.UNARY_OPERATOR else []
        unsafe = (c.kind in {K.CALL_EXPR, K.COMPOUND_ASSIGNMENT_OPERATOR,
                            K.CSTYLE_CAST_EXPR, K.CXX_REINTERPRET_CAST_EXPR,
                            K.CXX_CONST_CAST_EXPR, K.CXX_STATIC_CAST_EXPR}
                  or any(t.spelling in ('&', '++', '--') for t in tokens)
                  or c.type.kind in {ci.TypeKind.LVALUEREFERENCE, ci.TypeKind.RVALUEREFERENCE})
        if unsafe:
            blocked.update(n.referenced.get_usr() for n in c.walk_preorder()
                           if n.kind == K.DECL_REF_EXPR and n.referenced)
    # Never infer an uninitialized local from later assignments alone.
    for c in nodes:
        if c.kind == K.VAR_DECL and not list(c.get_children()):
            blocked.add(c.get_usr())
    if any(c.kind in {K.LAMBDA_EXPR, K.ASM_STMT, K.MS_ASM_STMT}
           for c in cursor.walk_preorder()):
        return {}
    return {u: v for u, v in values.items() if u not in blocked and v}


def integer_value(cursor):
    """libclang exposes evaluation in C, but cindex does not wrap it yet."""
    lib = ci.conf.lib
    for name, args, result in (
        ('clang_Cursor_Evaluate', [ci.Cursor], c_void_p),
        ('clang_EvalResult_getKind', [c_void_p], c_int),
        ('clang_EvalResult_getAsLongLong', [c_void_p], c_longlong),
        ('clang_EvalResult_dispose', [c_void_p], None),
    ):
        fn = getattr(lib, name)
        fn.argtypes, fn.restype = args, result
    value = lib.clang_Cursor_Evaluate(cursor)
    if not value:
        return None
    try:
        if lib.clang_EvalResult_getKind(value) == 1:  # CXEval_Int
            return lib.clang_EvalResult_getAsLongLong(value)
    finally:
        lib.clang_EvalResult_dispose(value)


def expression(cursor, locals=None, seen=frozenset()):
    """Return (callee USRs, literal edits, concrete anchor), or unknown."""
    children = list(cursor.get_children())
    kind = cursor.kind
    if kind in WRAPPERS:
        operands = [c for c in children if c.kind != K.TYPE_REF]
        if len(operands) == 1:
            return expression(operands[0], locals, seen)
    if kind in {K.INTEGER_LITERAL, K.CXX_BOOL_LITERAL_EXPR}:
        # Macro expansions have misleading token ranges; only edit literal text
        # at the expression's actual spelling extent, never its macro definition.
        start, end = cursor.extent.start, cursor.extent.end
        if not start.file or start.file != end.file:
            return None
        path = Path(start.file.name)
        value = path.read_bytes()[start.offset:end.offset].decode()
        if value in ('true', 'false'):
            return set(), set(), True
        if value in ('TRUE', 'FALSE'):
            expected = 1 if value == 'TRUE' else 0
            if integer_value(cursor) != expected:
                return None
            return set(), {(path, start.offset, end.offset, value.lower())}, True
        if re.fullmatch(r'(?:0[xX]0*([01])|0*([01]))[uUlL]*', value):
            number = int(re.sub('[uUlL]+$', '', value), 16 if 'x' in value.lower() else 10)
            return set(), {(path, start.offset, end.offset, 'true' if number else 'false')}, True
        return None
    if kind == K.CALL_EXPR:
        target = cursor.referenced
        if cursor.type.get_canonical().kind == ci.TypeKind.BOOL:
            return set(), set(), True
        if target and target.kind in FUNCTIONS:
            if target.kind == K.CXX_METHOD and target.is_virtual_method():
                return None
            return {target.get_usr()}, set(), False
        return None
    if kind == K.UNARY_OPERATOR:
        tokens = list(cursor.get_tokens())
        if tokens and tokens[0].spelling == '!':
            return set(), set(), True
    if kind == K.BINARY_OPERATOR:
        op = operator(cursor, children)
        if op in ('==', '!=', '<', '<=', '>', '>=', '&&', '||'):
            return set(), set(), True
        if op == ',':
            return expression(children[-1], locals, seen)
        if op in ('&', '|', '^'):
            values = [expression(c, locals, seen) for c in children]
            if all(v is not None for v in values):
                return (values[0][0] | values[1][0], values[0][1] | values[1][1],
                        values[0][2] or values[1][2])
    if kind == K.CONDITIONAL_OPERATOR and len(children) == 3:
        arms = [expression(c, locals, seen) for c in children[1:]]
        if all(a is not None for a in arms):
            literals = arms[0][1] | arms[1][1]
            # In C++, two bool arms change an int ternary's type and VC5
            # instruction selection. Preserve its original arithmetic type.
            typename = ALIASES.get(cursor.type.get_canonical().spelling)
            if Path(cursor.translation_unit.spelling).suffix != '.c' and typename != 'bool':
                if not typename:
                    return None
                literals = {(p, a, b, f'static_cast<{typename}>({v})' if v in ('true', 'false') else v)
                            for p, a, b, v in literals}
            return (arms[0][0] | arms[1][0], literals,
                    arms[0][2] or arms[1][2])
    if cursor.type.get_canonical().kind == ci.TypeKind.BOOL:
        return set(), set(), True
    if kind == K.DECL_REF_EXPR and cursor.referenced and locals:
        usr = cursor.referenced.get_usr()
        if usr in locals and usr not in seen:
            values = [expression(v, locals, seen | {usr}) for v in locals[usr]]
            if all(v is not None for v in values):
                return (set().union(*(v[0] for v in values)),
                        set().union(*(v[1] for v in values)), any(v[2] for v in values))
    return None


def infer(functions):
    candidates = set()
    for usr, f in functions.items():
        if not f.valid or not f.returns:
            continue
        if f.cursor.result_type.get_canonical().spelling not in ALIASES:
            continue
        valid = True
        for cursor, returns in f.variants:
            locals = local_values(cursor)
            for ret in returns:
                children = list(ret.get_children())
                result = expression(children[0], locals) if len(children) == 1 else None
                if result is None:
                    valid = False
                    break
                deps, literals, anchor = result
                f.dependencies.update(deps)
                f.literals.update(literals)
                f.anchor |= anchor
            if not valid:
                break
        if valid:
            candidates.add(usr)
    # Eliminate dependencies on unknown/external/nonboolean functions to a fixed
    # point, including recursive groups with even one nonboolean return.
    while True:
        keep = {u for u in candidates if functions[u].dependencies <= candidates}
        if keep == candidates:
            break
        candidates = keep
    anchored = {u for u in candidates if functions[u].anchor}
    while True:
        keep = anchored | {u for u in candidates if functions[u].dependencies & anchored}
        if keep == anchored:
            return keep
        anchored = keep


def boolean_type(type):
    spelling = type.get_canonical().spelling
    return ALIASES.get(re.sub(r'\b(const|volatile)\s+', '', spelling))


def type_edit(cursor):
    """Find the declared type, excluding annotations and calling conventions."""
    type = cursor.type if cursor.kind == K.VAR_DECL else cursor.result_type
    alias = boolean_type(type)
    qualifiers = ' '.join(q for q in ('const', 'volatile')
                          if getattr(type, f'is_{q}_qualified')())
    target = (qualifiers + ' ' if qualifiers else '') + alias
    spelling = type.spelling
    if spelling == target:
        return None
    tokens = list(cursor.get_tokens())
    words = spelling.split()
    for i, token in enumerate(tokens):
        if token.extent.end.offset > cursor.location.offset:
            break
        group = tokens[i:i + len(words)]
        if [t.spelling for t in group] != words:
            continue
        start, end = group[0].extent.start, group[-1].extent.end
        if start.file != cursor.location.file or end.file != start.file:
            continue
        path = Path(start.file.name)
        actual = path.read_bytes()[start.offset:end.offset].decode()
        if actual.split() == words:
            return path, start.offset, end.offset, target
    raise ValueError(f'cannot safely locate return type: {cursor.spelling} at {cursor.location}')


def local_edits(functions, candidates, root):
    """Prove local declarations independently of their enclosing return type."""
    observations = {}
    groups = {}
    for f in functions.values():
        for cursor, _ in f.variants:
            values = local_values(cursor)
            for node in cursor.walk_preorder():
                if node.kind != K.DECL_STMT:
                    continue
                variables = [v for v in node.get_children() if v.kind == K.VAR_DECL]
                keys = []
                for variable in variables:
                    path = project_file(variable, root)
                    if path is None:
                        continue
                    key = (path, variable.location.offset)
                    keys.append(key)
                    usr = variable.get_usr()
                    results = [expression(v, values, {usr}) for v in values.get(usr, [])]
                    proven = (boolean_type(variable.type) is not None and bool(results)
                              and all(r is not None and r[0] <= candidates for r in results))
                    if key not in observations:
                        observations[key] = [variable, proven, set()]
                    else:
                        observations[key][1] &= proven
                    if proven:
                        observations[key][2].update(set().union(*(r[1] for r in results)))
                if keys:
                    groups[tuple(keys)] = node
    edits = set()
    count = 0
    for keys, statement in groups.items():
        # A shared type specifier must never retype an unproven sibling.
        # Leave mixed declarations intact (including for-loop initializers).
        if not all(observations[key][1] for key in keys):
            continue
        own = set()
        try:
            edit = type_edit(observations[keys[0]][0])
            if edit:
                own.add(edit)
            for key in keys:
                own.update(observations[key][2])
        except ValueError as error:
            print(f'skip local: {error}', file=sys.stderr)
            continue
        edits.update(own)
        count += len(keys)
        for key in keys:
            variable = observations[key][0]
            print(f'{key[0].relative_to(root)}:{variable.location.line}: local '
                  f'{variable.spelling}: {variable.type.spelling} -> {boolean_type(variable.type)}')
    print(f'{count} boolean-valued locals', file=sys.stderr)
    return edits


def plan(functions, candidates, root=REPO):
    edits = set()
    for usr in sorted(candidates):
        f = functions[usr]
        own = set(f.literals)
        try:
            for c in f.declarations.values():
                edit = type_edit(c)
                if edit:
                    own.add(edit)
        except ValueError as error:
            print(f'skip: {error}', file=sys.stderr)
            continue
        edits.update(own)
        c = f.cursor
        print(f'{Path(c.location.file.name).relative_to(root)}:{c.location.line}: '
              f'{c.displayname}: {c.result_type.spelling} -> '
              f'{ALIASES[c.result_type.get_canonical().spelling]}')
    edits.update(local_edits(functions, candidates, root))
    # Add only aliases the inferred functions actually need; preserve their
    # primitive types and keep all definitions in the existing owner header.
    header = root / 'include/Ints.h'
    data = header.read_bytes()
    aliases = {ALIASES[functions[u].cursor.result_type.get_canonical().spelling]
               for u in candidates} - {'bool'}
    aliases.update(a for *_, text in edits
                   for a in re.findall(r'\b(?:ub|b)(?:8|16|32|64)\b', text))
    missing = [a for a in sorted(aliases) if not re.search(rb'\btypedef\s+[^;]+\s+'
                                                  + a.encode() + rb'\s*;', data)]
    if missing:
        offset = data.rfind(b'#endif')
        if offset < 0:
            raise ValueError('Ints.h has no closing include guard')
        declarations = ''.join(f'typedef {"u" + a[2:] if a.startswith("ub") else "i" + a[1:]} {a};\n'
                               for a in missing)
        edits.add((header, offset, offset, declarations + '\n'))
    return edits


def apply(edits, root=REPO):
    files = defaultdict(set)
    for path, start, end, replacement in edits:
        if not any(path.resolve().is_relative_to(root / d) for d in ('src', 'include')):
            raise ValueError(f'edit outside project sources: {path}')
        files[path].add((start, end, replacement))
    # Validate all spans before writing any files.
    output = {}
    for path, changes in files.items():
        data = path.read_bytes()
        boundary = len(data)
        for start, end, replacement in sorted(changes, reverse=True):
            if not 0 <= start <= end <= boundary:
                raise ValueError(f'overlapping edits in {path}')
            data = data[:start] + replacement.encode() + data[end:]
            boundary = start
        output[path] = data
    for path, data in output.items():
        path.write_bytes(data)
    return len(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--write', action='store_true', help='apply reported edits')
    args = parser.parse_args()
    units = clang.compdb()
    if not units:
        parser.error('missing compilation database; run python3 -m giten.graph.compdb')
    try:
        parsed, functions = collect(units)
        candidates = infer(functions)
        edits = plan(functions, candidates)
        print(f'{len(functions)} definitions; {len(candidates)} boolean-valued functions; '
              f'{len(edits)} edits', file=sys.stderr)
        if args.write:
            print(f'updated {apply(edits)} files', file=sys.stderr)
    except (ValueError, ci.TranslationUnitLoadError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
