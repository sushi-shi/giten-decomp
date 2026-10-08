"""Raw link identity cannot silently waive bytes or missing inputs."""

from pathlib import Path
import tempfile
import struct
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from giten.verify import link_tier as link


class StrictLinkControls(unittest.TestCase):
    def test_max_exact_current_dip_is_not_excluded(self):
        ledger = {("u", "_Exact"): {"best": 100., "cur": 99., "addr": 0x1000,
                                    "fp": "exact"},
                  ("u", "_Partial"): {"best": 90., "cur": 89., "addr": 0x1020,
                                      "fp": "partial"}}
        self.assertEqual([r["name"] for r in link.partial_exclusions(ledger)],
                         ["_Partial"])

    def test_referent_bytes_are_not_masked(self):
        a = b"\xe8\x00\x10\x00\x00\xc3"
        b = b"\xe8\x00\x20\x00\x00\xc3"
        result = link.raw_difference(a, b)
        self.assertFalse(result["equal"])
        self.assertEqual(result["different_bytes"], 1)

    def test_only_named_interval_is_waived_and_extra_tail_fails(self):
        result = link.raw_difference(b"abcX", b"aYcZ!", [(1, 2)])
        self.assertEqual(result["different_bytes"], 2)
        self.assertEqual([s["offset"] for s in result["samples"]], [3, 4])
        with self.assertRaises(ValueError):
            link.raw_difference(b"abc", b"ab", [(1, 3)])

    def test_map_base_and_conflicting_symbol_addresses_preserved(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "candidate.map"
            path.write_text("Preferred load address is 00500000\n"
                            " 0001:00000000 _F 00501000 f u.obj\n"
                            " 0001:00000020 _F 00501020 f v.obj\n")
            result = link.strict_map(path, 0x500000)
            self.assertEqual(result["_F"]["rvas"], [0x1000, 0x1020])
            with self.assertRaises(ValueError):
                link.strict_map(path, 0x400000)

    def test_missing_image_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(link, "CAND", Path(directory) / "missing.exe"):
                result = link.strict_report(Path(directory) / "retail.exe")
        self.assertFalse(result["summary"]["equal"])
        self.assertIn("input failure", result["findings"][0])

    def test_missing_symbol_and_unreadable_body_fail_closed(self):
        # Minimal real PE32 input exercises the join/read paths without retail
        # snapshots, a linker invocation or mocking away byte comparison.
        blob = bytearray(0x400)
        struct.pack_into("<I", blob, 0x3c, 0x80)
        blob[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HHI", blob, 0x84, 0x14c, 1, 0x1234)
        struct.pack_into("<H", blob, 0x94, 0xe0)
        struct.pack_into("<H", blob, 0x98, 0x10b)
        struct.pack_into("<I", blob, 0x98 + 28, 0x400000)
        struct.pack_into("<I", blob, 0x98 + 60, 0x200)
        blob[0x178:0x180] = b".text\0\0\0"
        struct.pack_into("<IIII", blob, 0x180, 4, 0x1000, 0x200, 0x200)
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / "image.exe"
            cmap = Path(directory) / "image.map"
            exe.write_bytes(blob)
            cmap.write_text("Preferred load address is 00400000\n"
                            "Timestamp is 1234\n"
                            " 0001:00000000 _F 00401000 f library.obj\n")
            for name, size, expected in [("_Missing", 4, "missing or ambiguous"),
                                          ("_F", 0x300, "unreadable or truncated")]:
                binding = SimpleNamespace(name=name, unit="", rva=0x1000, size=size)
                with patch.object(link, "CAND", exe), patch.object(link, "CMAP", cmap), \
                        patch("giten.model.resolve", return_value=SimpleNamespace(functions=[binding])), \
                        patch("giten.verify.baseline.load", return_value={}):
                    result = link.strict_report(exe)
                self.assertFalse(result["summary"]["equal"])
                self.assertTrue(any(expected in f for f in result["findings"]))

    def test_owner_trim_is_diagnostic_but_raw_alignment_bytes_still_compare(self):
        blob = bytearray(0x400)
        struct.pack_into("<I", blob, 0x3c, 0x80)
        blob[0x80:0x84] = b"PE\0\0"
        struct.pack_into("<HHI", blob, 0x84, 0x14c, 1, 0x1234)
        struct.pack_into("<H", blob, 0x94, 0xe0)
        struct.pack_into("<H", blob, 0x98, 0x10b)
        struct.pack_into("<I", blob, 0x98 + 28, 0x400000)
        struct.pack_into("<I", blob, 0x98 + 60, 0x200)
        blob[0x178:0x180] = b".text\0\0\0"
        struct.pack_into("<IIII", blob, 0x180, 12, 0x1000, 0x200, 0x200)
        code = b"\x33\xc0\x90\xc3" + b"\xcc" * 8
        blob[0x200:0x20c] = code
        coff = struct.pack("<HHIIIHH", 0x14c, 1, 0, 60 + len(code), 1, 0, 0)
        coff += struct.pack("<8sIIIIIIHHI", b".text\0\0\0", 0, 0,
                            len(code), 60, 0, 0, 0, 0, 0x60000020)
        coff += code + struct.pack("<8sIhHBB", b"_F\0\0\0\0\0\0", 0, 1, 0x20, 2, 0)
        coff += struct.pack("<I", 4)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            owner = root / "objdiff/base/u.obj"
            owner.parent.mkdir(parents=True)
            owner.write_bytes(coff)
            retail, candidate, cmap = root / "retail.exe", root / "candidate.exe", root / "candidate.map"
            retail.write_bytes(blob)
            candidate.write_bytes(blob)
            cmap.write_text("Preferred load address is 00400000\nTimestamp is 1234\n"
                            " 0001:00000000 _F 00401000 f u.obj\n")
            binding = SimpleNamespace(name="_F", unit="u", rva=0x1000, size=12)
            with patch.object(link, "BUILD", root), patch.object(link, "CAND", candidate), \
                    patch.object(link, "CMAP", cmap), \
                    patch("giten.model.resolve", return_value=SimpleNamespace(functions=[binding])), \
                    patch("giten.verify.baseline.load", return_value={}):
                result = link.strict_report(retail)
                self.assertTrue(result["summary"]["equal"], result["findings"])
                self.assertEqual(result["functions"][0]["candidate_object_size"], 4)
                self.assertEqual(result["functions"][0]["retail_size"], 12)
                self.assertTrue(result["functions"][0]["diagnostics"])
                blob[0x20b] = 0x90
                candidate.write_bytes(blob)
                different = link.strict_report(retail)
            self.assertFalse(different["summary"]["equal"])
            self.assertFalse(different["file"]["equal"])
            self.assertFalse(different["functions"][0]["raw_bytes"]["equal"])


if __name__ == "__main__":
    unittest.main()
