import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten.tool import clang

SOURCE = """\
namespace outer {
struct Box {
    int Get() const { static int cached; struct Local { __attribute__((annotate("decl:0x9"))) void Hit() {} }; return cached; }
    static int count;
    __attribute__((annotate("decl:0x2"))) void Later();
};
__attribute__((annotate("decl:0x3"))) extern int shared;
int Box::count;
}
extern "C" {
static int table[4];
int Free(int value) { int local = value; return local + table[0]; }
__attribute__((annotate("decl:0x1"))) int Proto(void);
}
"""


class PreorderTests(unittest.TestCase):
    """`preorder` replaces walk_preorder in every libclang probe, so it must
    visit the same cursors in the same order, bodies and local classes
    included."""

    def test_matches_walk_preorder(self):
        try:
            import clang.cindex as cidx
        except ImportError:
            self.skipTest("pylibclang unavailable")
        with tempfile.TemporaryDirectory() as directory:
            tu = Path(directory) / "unit.cpp"
            tu.write_text(SOURCE)
            parsed = clang.parse(str(tu), None)
            self.assertIsNotNone(parsed)
            K = cidx.CursorKind
            for kinds in ({K.VAR_DECL},
                          {K.FUNCTION_DECL, K.CXX_METHOD, K.CONSTRUCTOR, K.DESTRUCTOR},
                          {K.TRANSLATION_UNIT, K.STRUCT_DECL}):
                walked = [c for c in parsed.cursor.walk_preorder() if c.kind in kinds]
                self.assertTrue(walked)
                self.assertEqual(clang.preorder(parsed, kinds), walked)

    def test_annotated_decls_descend_scopes_but_not_bodies(self):
        with tempfile.TemporaryDirectory() as directory:
            tu = Path(directory).resolve() / "unit.cpp"
            tu.write_text(SOURCE)
            with mock.patch.object(clang, "REPO", tu.parent):
                decls = clang.annotated_decls(str(tu), None)
        self.assertEqual([(d["kind"], d["annotations"], d["defined"]) for d in decls],
                         [("func", ["decl:0x2"], False), ("var", ["decl:0x3"], False),
                          ("func", ["decl:0x1"], False)])


if __name__ == "__main__":
    unittest.main()
