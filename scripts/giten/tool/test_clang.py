"""Source-definition evidence from the native front end."""
from pathlib import Path
import tempfile
import unittest

from giten.tool import clang


class FunctionDefinitionControls(unittest.TestCase):
    def test_header_definitions_have_exact_extents_and_declarations_do_not(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            header = root / "math.h"
            body = b"inline int Value(int x) { return x + 1; }"
            header.write_bytes(b"int Declaration(int);\n" + body + b"\n")
            source = root / "unit.cpp"
            source.write_text('#include "math.h"\nint Use() { return Value(2); }\n')
            definitions = clang.function_definition_extents(
                str(source), ["--target=i686-pc-windows-msvc", "/Zp1"])
            self.assertIsNotNone(definitions)
            self.assertFalse(any("Declaration" in name for name in definitions))
            extents = next(value for name, value in definitions.items() if "Value" in name)
            self.assertEqual(len(extents), 1)
            extent = extents[0]
            self.assertEqual(Path(extent["file"]), header.resolve())
            self.assertEqual(header.read_bytes()[extent["start"]:extent["end"]], body)

    def test_invalid_translation_unit_has_no_definition_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "broken.cpp"
            source.write_text("int Broken( {\n")
            self.assertIsNone(clang.function_definition_extents(
                str(source), ["--target=i686-pc-windows-msvc"]))


if __name__ == "__main__":
    unittest.main()
