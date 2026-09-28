"""Behavioral controls for the boolean-return rewrite, using real libclang ASTs."""
from pathlib import Path
import tempfile
import unittest

from giten.tool import bool_returns as br


class BooleanReturns(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        (self.root / 'src').mkdir()
        (self.root / 'include').mkdir()
        self.source = self.root / 'src/unit.cpp'
        self.header = self.root / 'include/Ints.h'
        self.header.write_text('#ifndef TEST_INTS_H\n#define TEST_INTS_H\n'
                               'typedef unsigned char u8;\n'
                               'typedef int b32;\ntypedef signed char b8;\n'
                               'typedef short b16;\ntypedef unsigned int ub32;\n#endif\n')

    def scan(self, source, header=''):
        self.header.write_text(self.header.read_text() + header)
        self.source.write_text('#include <Ints.h>\n' + source)
        self.parsed, self.functions = br.collect({str(self.source): [
            '--target=i686-pc-windows-msvc', '-fms-extensions',
            '-fms-compatibility-version=11.00', '/I' + str(self.header.parent)
        ]}, self.root)
        self.candidates = br.infer(self.functions)
        return {self.functions[u].cursor.spelling for u in self.candidates}

    def rewrite(self):
        return br.apply(br.plan(self.functions, self.candidates, self.root), self.root)

    def test_recursive_groups_and_transitive_unknowns(self):
        names = self.scan('''
int Odd(int);
int Even(int x) { return x ? Odd(x - 1) : 1; }
int Odd(int x) { return x ? Even(x - 1) : 0; }
int Wrapper(int x) { return Odd(x); }
int Bad(int);
int BadWrapper(int x) { return Bad(x); }
int Bad(int x) { return x ? BadWrapper(x - 1) : 2; }
int Endless() { return Endless(); }
int External();
int Unknown() { return External(); }
''')
        self.assertEqual(names, {'Odd', 'Even', 'Wrapper'})

    def test_redeclarations_widths_and_literal_context(self):
        self.scan('''
int Predicate(int x) { return x == 0 ? 1 : 0; }
signed char Byte() { return 1; }
short Half() { return 0; }
unsigned int Unsigned() { return 1u; }
bool Native() { return false; }
''', 'int Predicate(int);\n')
        self.rewrite()
        self.assertIn('b32 Predicate(int);', self.header.read_text())
        source = self.source.read_text()
        self.assertIn('return x == 0 ? static_cast<b32>(true) : static_cast<b32>(false);', source)
        self.assertIn('b8 Byte()', source)
        self.assertIn('b16 Half()', source)
        self.assertIn('ub32 Unsigned()', source)
        self.assertIn('bool Native() { return false; }', source)
        self.scan(source.removeprefix('#include <Ints.h>\n'))
        self.assertEqual(self.rewrite(), 0)

    def test_unknown_values_nested_scopes_and_dispatch(self):
        names = self.scan('''
int Numeric() { return 2; }
int Local() { int x = 0; return x; }
int Missing(int x) { if (x) return 1; }
int Nested() { struct Inner { int Value() { return 2; } }; return 0; }
int Indirect(int (*f)()) { return f(); }
struct Base { virtual int Value() { return 1; } };
int Virtual(Base &b) { return b.Value(); }
int Compare(int x) { return x > 1; }
int Negate(int x) { return !x; }
int Mask(int x) { return x & 3; }
void *Pointer() { return 0; }
enum Choice { No, Yes };
Choice Enum() { return No; }
''')
        self.assertEqual(names, {'Nested', 'Value', 'Compare', 'Negate', 'Local'})

    def test_macros_do_not_edit_definitions_or_expand_unknowns(self):
        names = self.scan('''
#define NUMBER 1
#define UNKNOWN 2
#define TRUE 1
int Macro() { return NUMBER; }
int Wrong() { return UNKNOWN; }
int Windows() { return TRUE; }
''')
        self.assertEqual(names, {'Windows'})
        self.rewrite()
        self.assertIn('#define TRUE 1', self.source.read_text())
        self.assertIn('return true;', self.source.read_text())

    def test_misleading_macro_name_is_not_boolean(self):
        self.assertEqual(self.scan('#define TRUE 2\nint Wrong() { return TRUE; }'), set())

    def test_local_flag_writes_and_escapes(self):
        names = self.scan("""
void Mutate(int *);
void Ref(int &);
int Flag(int x) { int flag = 0; if (x) flag = 1; return flag; }
int Increment() { int flag = 0; ++flag; ++flag; return flag; }
int Escaped() { int flag = 0; Mutate(&flag); return flag; }
int Reference() { int flag = 0; Ref(flag); return flag; }
int Alias() { int flag = 0; int &r = flag; r = 4; return flag; }
int Uninitialized(int x) { int flag; if (x) flag = 1; return flag; }
int Nonboolean(int x) { int flag = 0; if (x) flag = 2; return flag; }
int Static() { static int flag = 0; return flag; }
int Parameter(int x, int test) { if (test) return x; x = 1; return x; }
""")
        self.assertEqual(names, {'Flag'})
        self.rewrite()
        self.assertIn('int flag = false; if (x) flag = true;', self.source.read_text())

    def test_missing_alias_is_defined_once_in_owner_header(self):
        self.scan('unsigned char Byte() { return 1; }')
        self.rewrite()
        self.assertEqual(self.header.read_text().count('typedef u8 ub8;'), 1)
        self.scan(self.source.read_text().removeprefix('#include <Ints.h>\n'))
        self.assertEqual(self.rewrite(), 0)

    def test_shared_header_must_be_boolean_in_every_translation_unit(self):
        self.header.write_text(self.header.read_text() +
                               '\n#if RESULT == 2\ninline int Configured() { return 2; }\n'
                               '#else\ninline int Configured() { return 1; }\n#endif\n')
        self.source.write_text('#include <Ints.h>\n')
        other = self.source.with_name('other.cpp')
        other.write_text('#include <Ints.h>\n')
        self.parsed, functions = br.collect({str(p): [
            '--target=i686-pc-windows-msvc', '/I' + str(self.header.parent),
            '/DRESULT=' + value] for p, value in ((self.source, '2'), (other, '1'))}, self.root)
        self.assertFalse(br.infer(functions))

    def test_parse_errors_abort(self):
        with self.assertRaises(ValueError):
            self.scan('int Broken( {')


if __name__ == '__main__':
    unittest.main()
