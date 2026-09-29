"""Resource-tree walking, payload file reconstruction and the rebased check."""

import dataclasses
import struct
import unittest
from types import SimpleNamespace
from unittest import mock

from giten.rsrc import check, payloads, tree
from giten.rsrc.tree import Leaf


def _leaf(kind, name, payload, rva=0x1000):
    return Leaf(kind, name, 1041, 0, rva, 0, 0, payload)


class TreeTests(unittest.TestCase):
    def test_walks_type_name_language_to_payload(self):
        image = bytearray(0x200)
        for offset, identity, child in ((0, 2, 0x80000020),
                                        (0x20, 7, 0x80000040),
                                        (0x40, 0x409, 0x60)):
            struct.pack_into("<H", image, offset + 14, 1)
            struct.pack_into("<II", image, offset + 16, identity, child)
        struct.pack_into("<IIII", image, 0x60, 0x1080, 3, 0, 0)
        image[0x80:0x83] = b"abc"
        pe = SimpleNamespace(
            path="retail.exe",
            directories=[(0, 0), (0, 0), (0x1000, len(image))],
            section=lambda name: {"va": 0x1000, "rsize": len(image),
                                  "vsize": len(image), "rptr": 0},
            data=bytes(image),
            read=lambda rva, size: bytes(image[rva - 0x1000:rva - 0x1000 + size]),
        )
        with mock.patch.object(tree, "Pe", return_value=pe):
            directory = tree.read("retail.exe")
        self.assertEqual([table.path for table in directory.tables],
                         [(), (2,), (2, 7)])
        self.assertEqual([(leaf.key, leaf.entry, leaf.rva, leaf.payload)
                          for leaf in directory.leaves],
                         [((2, 7, 0x409), 0x60, 0x1080, b"abc")])


class PayloadTests(unittest.TestCase):
    def test_bitmap_file_restores_header_before_palette_and_bits(self):
        info = struct.pack("<IiiHHIIiiII", 40, 2, 1, 1, 8, 0, 4, 0, 0, 2, 0)
        dib = info + b"\1\2\3\0\4\5\6\0" + b"\0\1\0\0"
        data = payloads.bitmap_file(dib)
        self.assertEqual(struct.unpack_from("<2sIHHI", data),
                         (b"BM", 14 + len(dib), 0, 0, 14 + 40 + 2 * 4))
        self.assertEqual(data[14:], dib)

    def test_cursor_group_reassembles_hotspot_and_image(self):
        image = struct.pack("<HH", 3, 5) + b"DIB-BYTES"
        group = struct.pack("<HHH", 0, 2, 1) + struct.pack(
            "<HHHHIH", 32, 64, 1, 1, len(image), 1)
        data = payloads.payload_files([
            _leaf(tree.RT_CURSOR, 1, image),
            _leaf(tree.RT_GROUP_CURSOR, 178, group),
        ])
        self.assertEqual(list(data), ["cursor_0178.cur"])
        cur = data["cursor_0178.cur"]
        self.assertEqual(struct.unpack_from("<HHH", cur), (0, 2, 1))
        self.assertEqual(struct.unpack_from("<BBBBHHII", cur, 6),
                         (32, 32, 2, 0, 3, 5, len(image) - 4, 22))
        self.assertEqual(cur[22:], b"DIB-BYTES")

    def test_icon_group_keeps_entry_fields_and_points_at_image(self):
        image = b"ICON-DIB"
        group = struct.pack("<HHH", 0, 1, 1) + struct.pack(
            "<BBBBHHIH", 32, 32, 16, 0, 1, 4, len(image), 3)
        ico = payloads.payload_files([
            _leaf(tree.RT_ICON, 3, image),
            _leaf(tree.RT_GROUP_ICON, 715, group),
        ])["icon_0715.ico"]
        self.assertEqual(struct.unpack_from("<BBBBHHII", ico, 6),
                         (32, 32, 16, 0, 1, 4, len(image), 22))
        self.assertEqual(ico[22:], image)

    def test_image_outside_every_group_is_rejected(self):
        with self.assertRaises(ValueError):
            payloads.payload_files([_leaf(tree.RT_ICON, 3, b"x")])


class CheckTests(unittest.TestCase):
    def _directory(self, section_va, payload):
        image = bytearray(0x40)
        struct.pack_into("<I", image, 0x10, section_va + 0x20)
        image[0x20:0x20 + len(payload)] = payload
        leaf = Leaf(2, 101, 1041, 0x10, section_va + 0x20, 0, 0, payload)
        table = tree.Table((), 0, 0, 0, (0, 0), (2,))
        return tree.Directory(section_va, section_va, 0x40, bytes(image),
                              (table,), (leaf,))

    def test_section_rva_alone_is_not_a_difference(self):
        self.assertEqual(check.findings(self._directory(0x93000, b"abc"),
                                        self._directory(0x94000, b"abc")), [])

    def test_payload_and_codepage_differences_are_findings(self):
        retail = self._directory(0x93000, b"abc")
        candidate = self._directory(0x93000, b"abd")
        leaf = dataclasses.replace(candidate.leaves[0], codepage=932)
        candidate = dataclasses.replace(candidate, leaves=(leaf,))
        found = check.findings(retail, candidate)
        self.assertTrue(any("payload differs" in line for line in found))
        self.assertTrue(any("code page" in line for line in found))


if __name__ == "__main__":
    unittest.main()
