"""Reject incomplete objects and publish compiler output atomically."""

import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock


def _coff(nsec: int = 0, symptr: int = 0, nsym: int = 0, machine: int = 0x14C,
          sections: list[tuple[int, int]] = (), tail: bytes = b"") -> bytes:
    """A hand-built COFF header (+ `sections` as (size, ptr) pairs)."""
    import struct
    out = bytearray(struct.pack("<HHIIIHH", machine, nsec, 0, symptr, nsym, 0, 0))
    for size, ptr in sections:
        raw = bytearray(40)
        struct.pack_into("<II", raw, 16, size, ptr)
        out += raw
    return bytes(out) + tail


class ClEdgeObjectIntegrityControls(unittest.TestCase):

    def test_a_truncated_object_is_refused_not_installed(self):
        from giten.graph import cc
        from giten.tool import ToolError
        # one section header promising 0x1000 bytes at 0x40 in a 60-byte file
        torn = _coff(nsec=1, sections=[(0x1000, 0x40)])
        self.assertIsNotNone(cc.coff_defect(torn))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            with self.assertRaises(ToolError):
                cc.install(torn, out)
            self.assertFalse(out.exists(), "a torn object was installed anyway")

    def test_a_truncated_string_table_is_refused(self):
        from giten.graph import cc
        import struct
        # symbol table present, string-table length dword promises past EOF
        body = _coff(nsec=0, symptr=20, nsym=1,
                     tail=b"\0" * 18 + struct.pack("<I", 0x1000))
        self.assertIn("string table", cc.coff_defect(body) or "")

    def test_a_complete_object_passes(self):
        from giten.graph import cc
        self.assertIsNone(cc.coff_defect(_coff()))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            self.assertTrue(cc.install(_coff(), out))
            self.assertFalse(cc.install(_coff(), out), "rewrote unchanged bytes")
            self.assertEqual(list(Path(td).iterdir()), [out])   # no temp left

    def test_install_does_not_use_the_shared_temp_name(self):
        """A sibling build holding `<unit>.obj.install` must not break us."""
        from giten.graph import cc
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            (Path(td) / "unit.obj.install").mkdir()       # the old, shared name
            self.assertTrue(cc.install(_coff(), out))
            self.assertTrue(out.is_file())

    def test_the_staging_path_is_per_process(self):
        from giten.graph import cc
        seen = {}

        def fake(src, staged, flags):
            seen["staged"] = Path(staged)
            Path(staged).write_bytes(_coff())
            return ""

        with tempfile.TemporaryDirectory() as td:
            src = Path(td) / "u.c"
            src.write_text("int main(void){return 0;}\n")
            with mock.patch("giten.tool.cl.compile", fake):
                cc.compile_unit(src, Path(td) / "base" / "u.obj", [])
        parts = seen["staged"].parts
        self.assertIn(str(os.getpid()), parts, f"shared staging dir: {parts}")
        self.assertIn(".tmp", parts)


if __name__ == "__main__":
    unittest.main()
