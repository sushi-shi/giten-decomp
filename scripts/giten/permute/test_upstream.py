"""Contracts at the upstream-parser/full-TU compiler boundary."""
import json
import os
from pathlib import Path
import tempfile
import unittest

from giten.permute import upstream


class UpstreamTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        path = os.environ.get('GITEN_DECOMP_PERMUTER')
        if not path:
            raise unittest.SkipTest('run inside nix develop with GITEN_DECOMP_PERMUTER')
        cls.ast, cls.randomizer, cls.weights = upstream.load_upstream(Path(path))

    def test_seed_replay_and_generated_helpers_survive_feedback(self):
        weights = {key: 1 if key == 'perm_inline' else 0 for key in self.weights}
        body = 'int f(int x, int y) { return (x + y) * (x - y); }'
        signature = 'int f(int x, int y)'
        args = (self.ast, self.randomizer, weights, '', 'f', signature, [body], 4, 79, 2)
        first = upstream.generate_options(*args)
        self.assertEqual(first, upstream.generate_options(*args))
        self.assertEqual(first[2], [])
        parent = first[0][1]['replace']
        self.assertIn('__inline static', parent)
        options, _, errors = upstream.generate_options(
            self.ast, self.randomizer, weights, '', 'f', signature,
            [body, parent], 8, 31, 2,
        )
        self.assertEqual(errors, [])
        self.assertEqual(options[1]['replace'], parent)
        children = [row for row in options if row['name'].startswith('parent-1-')]
        self.assertTrue(children)
        for row in children:
            parsed = self.ast.parse_c(row['replace'])
            target = next(node for node in parsed.ext
                          if hasattr(node, 'decl') and node.decl.name == 'f')
            self.assertEqual(self.ast.to_c_raw(target.decl), signature)
            self.assertGreaterEqual(len(parsed.ext), 3)

    def test_context_preserves_macro_calls_and_ignores_comment_tokens(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'unit.c'
            original = (b'#define RVA(a,b)\n#define VALUE(x) ((x)+1)\n'
                        b'RVA(0x00001234, 0x10)\nint f(int x) {\n'
                        b'/* keep this outside the parser */\n'
                        b'const char *s = "// literal"; // comment\n'
                        b'return VALUE(x);\n}\nint after;\n')
            source.write_bytes(original)
            context = upstream.source_context(root, source, 0x1234, self.ast)
            blob, start, end, _, authored, _, _, parser_body = context
            self.assertEqual(blob, original)
            self.assertEqual(authored.encode(), original[start:end])
            self.assertIn('VALUE(x)', parser_body)
            self.assertIn('"// literal"', parser_body)
            self.assertNotIn('keep this outside', parser_body)
            self.assertEqual(source.read_bytes(), original)

    def test_nested_unbraced_statements_are_normalized_for_upstream(self):
        body = ('int f(int x) { if (x) return x + 1; '
                'else if (x < 0) return -x; return 0; }')
        weights = {key: 1 if key == 'perm_temp_for_expr' else 0 for key in self.weights}
        options, _, errors = upstream.generate_options(
            self.ast, self.randomizer, weights, '', 'f', 'int f(int x)',
            [body], 16, 42, 4,
        )
        self.assertEqual(len(options), 17)
        self.assertEqual(errors, [])

    def test_sibling_pointer_conversion_does_not_block_declaration_context(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'unit.c'
            original = (b'#define RVA(a,b)\nstruct Cell { int value; };\n'
                        b'void sibling(void) { struct Cell *cell; char *raw; cell = raw; }\n'
                        b'RVA(0x00001234, 0x10)\nint f(int x) { return x; }\n')
            source.write_bytes(original)
            context = upstream.source_context(root, source, 0x1234, self.ast)
            self.assertEqual(context[-1], 'int f(int x) { return x; }')
            self.assertNotIn('cell = raw', context[5])
            self.assertEqual(source.read_bytes(), original)

    def test_target_and_global_pointer_conversion_errors_still_block(self):
        for invalid in ('int f(int x) { struct Cell *cell; char *raw; cell = raw; return x; }',
                        'int f(int x) { return x; }\nstruct Cell *cell = (char *)0;'):
            with self.subTest(invalid=invalid), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                source = root / 'unit.c'
                source.write_text('#define RVA(a,b)\nstruct Cell { int value; };\n'
                                  'RVA(0x00001234, 0x10)\n' + invalid + '\n')
                with self.assertRaisesRegex(ValueError, 'cannot derive type context'):
                    upstream.source_context(root, source, 0x1234, self.ast)

    def test_enum_annotations_follow_the_definition_at_each_use(self):
        for expansion in ['storage', 'name']:
            with self.subTest(expansion=expansion), tempfile.TemporaryDirectory() as directory:
                root = Path(directory)
                source = root / 'unit.c'
                original = (
                    '#define RVA(a,b)\n#define VALUE(x) ((x)+1)\n'
                    'typedef enum Domain { ZERO } Domain;\n'
                    f'#define GZ_ENUM_STORAGE(name, storage) {expansion}\n'
                    'RVA(0x00001234, 0x10)\n'
                    'GZ_ENUM_STORAGE(Domain, short) f(int x) {\n'
                    'GZ_ENUM_STORAGE(Domain, short) value = VALUE(x);\n'
                    'return value;\n}\n'
                    '#undef GZ_ENUM_STORAGE\n'
                    '#define GZ_ENUM_STORAGE(name, storage) unsupported\n'
                ).encode()
                source.write_bytes(original)
                context = upstream.source_context(root, source, 0x1234, self.ast)
                blob, start, end, _, authored, prelude, signature, body = context
                parsed = self.ast.parse_c(prelude + '\n' + body)
                fn, _ = self.ast.extract_fn(parsed, 'f')
                expected = 'short' if expansion == 'storage' else 'Domain'
                self.assertEqual(self.ast.to_c_raw(fn.decl), f'{expected} f(int x)')
                self.assertEqual(signature, self.ast.to_c_raw(fn.decl))
                self.assertIn('VALUE(x)', body)
                self.assertEqual(authored.encode(), blob[start:end])
                self.assertEqual(source.read_bytes(), original)

    def test_frontier_rejects_changes_outside_emitted_region(self):
        original = b'prefix\nint f() { return 0; }\nsuffix\n'
        start = original.index(b'int f')
        end = original.index(b'\nsuffix')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            frontier = path / 'frontier'
            frontier.mkdir()
            (frontier / 'frontier.json').write_text(json.dumps([{'source': 'one.c'}]))
            candidate = frontier / 'one.c'
            candidate.write_bytes(b'changed prefix\nint f() { return 1; }\nsuffix\n')
            with self.assertRaisesRegex(ValueError, 'outside target'):
                upstream.frontier_parents(path, original, start, end)
            candidate.write_bytes(original[:start] + b'int f() { return 1; }' + original[end:])
            self.assertEqual(upstream.frontier_parents(path, original, start, end),
                             ['int f() { return 0; }', 'int f() { return 1; }'])


if __name__ == '__main__':
    unittest.main()
