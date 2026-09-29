import unittest

from giten.branch.lexer import finish, resolve_conditionals

RETAIL = {"GITEN_BUGFIX": False, "GITEN_COMPAT": False}
FIXED = {"GITEN_BUGFIX": True, "GITEN_COMPAT": True}

SOURCE = """\
#if defined(GITEN_BUGFIX) && !defined(GITEN_COMPAT)
#define GITEN_COMPAT
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifdef GITEN_BUGFIX
b32 Discard(void);
#else
void Discard(void);
#endif
void Frame(void) {
#ifndef GITEN_COMPAT
    Retail();
#elif defined(GITEN_BUGFIX)
    Fixed();
#else
    Compatible();
#endif
#ifdef GITEN_COMPAT
#ifdef _DEBUG
    Trace();
#endif
    Pump();
#endif
}
#ifdef __cplusplus
}
#endif
"""


class ResolveConditionals(unittest.TestCase):
    def test_retail_keeps_the_retail_arm_and_unrelated_conditionals(self):
        self.assertEqual(resolve_conditionals(SOURCE, RETAIL), """\
#ifdef __cplusplus
extern "C" {
#endif
void Discard(void);
void Frame(void) {
    Retail();
}
#ifdef __cplusplus
}
#endif
""")

    def test_fixed_keeps_the_fix_arm(self):
        self.assertEqual(resolve_conditionals(SOURCE, FIXED), """\
#ifdef __cplusplus
extern "C" {
#endif
b32 Discard(void);
void Frame(void) {
    Fixed();
#ifdef _DEBUG
    Trace();
#endif
    Pump();
}
#ifdef __cplusplus
}
#endif
""")

    def test_a_flag_mixed_with_other_macros_is_refused(self):
        for text in ("#if defined(GITEN_BUGFIX) && _MSC_VER\n#endif\n",
                     "#if GITEN_BUGFIX\n#endif\n",
                     "#ifdef _DEBUG\n#elif defined(GITEN_COMPAT)\n#endif\n"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                resolve_conditionals(text, RETAIL)

    def test_unbalanced_conditionals_are_refused(self):
        for text in ("#ifdef GITEN_BUGFIX\n", "#endif\n", "#else\n"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                resolve_conditionals(text, RETAIL)

    def test_continued_directives_are_decided_whole(self):
        text = "#if defined(GITEN_BUGFIX) \\\n    || defined(GITEN_COMPAT)\nFix();\n#endif\n"
        self.assertEqual(resolve_conditionals(text, RETAIL), "")
        self.assertEqual(resolve_conditionals(text, FIXED), "Fix();\n")

    def test_finish_drops_lines_that_held_only_removed_text(self):
        self.assertEqual(finish("a;\n\x01\n  \x01 \nb; \x01\n\n\n\nc;\n"), "a;\nb;\n\nc;\n")


if __name__ == "__main__":
    unittest.main()
