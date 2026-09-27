"""Resource-directory transfer and .res serialization controls."""

import struct
import unittest
from types import SimpleNamespace
from unittest import mock

from giten.rsrc import retail_res


class RetailResTests(unittest.TestCase):
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
            directories=[(0, 0), (0, 0), (0x1000, len(image))],
            section=lambda name: {"va": 0x1000, "rsize": len(image),
                                  "rptr": 0},
            data=bytes(image),
            read=lambda rva, size: bytes(image[rva - 0x1000:rva - 0x1000 + size]),
        )
        with mock.patch.object(retail_res, "Pe", return_value=pe):
            self.assertEqual(retail_res.resources("retail.exe"),
                             [(2, 7, 0x409, b"abc")])

    def test_serializes_aligned_win32_res_record(self):
        data = retail_res.res_bytes([(2, "IMAGE", 0x409, b"abc")])
        self.assertEqual(struct.unpack_from("<II", data, 0), (0, 32))
        size, header = struct.unpack_from("<II", data, 32)
        self.assertEqual(size, 3)
        self.assertEqual(struct.unpack_from("<IHHII", data, 32 + header - 16),
                         (0, 0x1030, 0x409, 0, 0))
        self.assertEqual(data[32 + header:32 + header + size], b"abc")
        self.assertEqual(len(data) % 4, 0)


if __name__ == "__main__":
    unittest.main()
