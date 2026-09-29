"""The delink re-runs on a data-identity change, not on a code-only edit."""

import struct
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten import graph
from giten.delink import coffx
from giten.graph import dataid, emit

_TEXT = 0x60500020
_BSS = 0xC0300080


def _coff(text: bytes, symbols: list[tuple[str, int, int, int]]) -> bytes:
    """An i386 COFF with .text (payload `text`) and a 4-byte .bss section;
    `symbols` are (name <= 8 chars, value, section number, storage class)."""
    head = 20 + 2 * 40
    symptr = head + len(text)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 2, 0, symptr, len(symbols), 0, 0))
    for name, size, ptr, chars in ((b".text", len(text), head, _TEXT),
                                   (b".bss", 4, 0, _BSS)):
        out += struct.pack("<8sIIIIIIHHI", name, 0, 0, size, ptr, 0, 0, 0, 0, chars)
    out += text
    for name, value, secnum, scl in symbols:
        out += struct.pack("<8sIhHBB", name.encode(), value, secnum, 0, scl, 0)
    return bytes(out + struct.pack("<I", 4))


class DataIdentityTests(unittest.TestCase):
    def identity(self, blob: bytes) -> list[str]:
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "unit.obj"
            path.write_bytes(blob)
            return dataid.object_identity(coffx.Obj(path))

    def test_code_bytes_do_not_move_the_identity(self):
        syms = [("_f", 0, 1, 2), ("_g", 0, 2, 3)]
        self.assertEqual(self.identity(_coff(b"\xc3", syms)),
                         self.identity(_coff(b"\x90\x90\xc3", syms)))

    def test_bss_versus_common_moves_the_identity(self):
        local = [("_f", 0, 1, 2), ("_g", 0, 2, 3)]
        common = [("_f", 0, 1, 2), ("_g", 4, 0, 2)]
        self.assertNotEqual(self.identity(_coff(b"\xc3", local)),
                            self.identity(_coff(b"\xc3", common)))

    def test_ordinals_are_masked(self):
        a = self.identity(_coff(b"\xc3", [("$SG101", 0, 2, 3)]))
        b = self.identity(_coff(b"\xc3", [("$SG207", 0, 2, 3)]))
        self.assertEqual(a, b)

    def test_write_is_if_changed(self):
        with tempfile.TemporaryDirectory() as td:
            base, out = Path(td) / "base", Path(td) / "ids.tsv"
            base.mkdir()
            (base / "unit.obj").write_bytes(_coff(b"\xc3", [("_g", 0, 2, 3)]))
            self.assertTrue(dataid.write(base, out))
            (base / "unit.obj").write_bytes(_coff(b"\x90\xc3", [("_g", 0, 2, 3)]))
            self.assertFalse(dataid.write(base, out))
            (base / "unit.obj").write_bytes(_coff(b"\xc3", [("_g", 4, 0, 2)]))
            self.assertTrue(dataid.write(base, out))

    def test_delink_keys_on_the_identity_not_the_objects(self):
        class Scan:
            def headers(self, source):
                return []

            def scanned(self):
                return {"src/Test/Owner.c"}

        unit = {"unit": "owner", "source": "src/Test/Owner.c",
                "flags": "retail", "cflags": ["/c"]}
        with tempfile.TemporaryDirectory() as td, \
                mock.patch.object(emit, "load_units",
                                  return_value=({"flags": {"retail": ["/c"]}}, [unit])), \
                mock.patch.object(emit, "prune_orphan_artifacts", return_value=0), \
                mock.patch.object(emit, "write_toolchain_id"), \
                mock.patch.object(emit, "write_comparator_id"), \
                mock.patch.object(emit, "Scanner", return_value=Scan()):
            path = Path(td) / "build.ninja"
            emit.emit(path)
            lines = path.read_text().replace("$\n", "").splitlines()
        obj = f"{graph.BASE_DIR}/owner.obj"
        delink = next(ln for ln in lines if ln.startswith(f"build {graph.DELINK_STAMP}:"))
        ids = next(ln for ln in lines if ln.startswith(f"build {graph.DATA_IDS}:"))
        self.assertIn(graph.DATA_IDS, delink)
        self.assertNotIn(obj, delink)
        self.assertIn(obj, ids)


if __name__ == "__main__":
    unittest.main()
