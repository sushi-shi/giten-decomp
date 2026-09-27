"""The label-only RVA_DECL channel: extraction from declaration cursors."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from giten.retail_labels.source import decl_claims


def _decl(name, ann, kind="func", defined=False):
    return {"kind": kind, "name": name, "annotations": [ann], "defined": defined,
            "file": "/repo/include/X.h", "internal": False}


class DeclClaims(unittest.TestCase):
    def test_prototype_binds_and_redeclarations_coalesce(self):
        claims, problems = decl_claims([
            _decl("_Foo", "decl:0x00012340"),
            _decl("_Foo", "decl:0x00012340"),              # a second prototype
            _decl("_Foo", "decl:0x00012340", defined=True),  # inherited by the body
        ])
        self.assertEqual((claims, problems), ([(0x12340, "_Foo")], []))

    def test_other_annotations_are_not_declaration_claims(self):
        claims, problems = decl_claims([
            _decl("_g", "data:0x00091540", kind="var"),
            _decl("_Bar", "rva:0x00012350 size:0x10", defined=True),
        ])
        self.assertEqual((claims, problems), ([], []))

    def test_decl_on_a_variable_is_a_problem(self):
        claims, problems = decl_claims([_decl("_g", "decl:0x00091540", kind="var")])
        self.assertEqual(claims, [])
        self.assertEqual(len(problems), 1)
        self.assertIn("not a function", problems[0])


class AnnotatedDeclsProbe(unittest.TestCase):
    """The real pylibclang probe over a TU that includes a repo header."""

    def test_header_prototype_reaches_the_including_tu(self):
        try:
            import clang.cindex  # noqa: F401
        except ImportError:
            self.skipTest("pylibclang unavailable")
        from giten.core import paths
        from giten.tool import clang as probe
        with tempfile.TemporaryDirectory(dir=paths.BUILD) as td:
            root = Path(td)
            (root / "Decl.h").write_text(
                '#include <rva.h>\n'
                'RVA_DECL(0x00012340)\nvoid Foo(short a);\n'
                'RVA_DECL(0x00012350)\nshort Bar(void);\n')
            tu = root / "decl.c"
            tu.write_text('#include "Decl.h"\n'
                          'RVA(0x00012350, 0x6)\nshort Bar(void) { return 1; }\n'
                          'int Use(void) { Foo(2); return Bar(); }\n')
            decls = probe.annotated_decls(str(tu), None)
        self.assertIsNotNone(decls)
        claims, problems = decl_claims(decls)
        self.assertEqual(problems, [])
        self.assertEqual(claims, [(0x12340, "_Foo"), (0x12350, "_Bar")])


if __name__ == "__main__":
    unittest.main()
