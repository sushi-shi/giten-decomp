"""Pinned local-static callback recognition in shared executable sections."""

from __future__ import annotations

import unittest
from unittest import mock

from giten.delink import static_dtors


class SharedTextObject:
    """An owner, its callback and a following function in one COFF section."""

    section_table = [{"characteristics": 0x60000020}]

    def __init__(self, callback, following=True):
        self.names = ["?Draw@@YAXXZ", "_$E1"]
        self.offsets = [0, 16]
        self.payload = bytes.fromhex("6800000000e800000000c3") + b"\x90" * 5
        self.payload += callback
        if following:
            self.names.append("?Next@@YAXXZ")
            self.offsets.append(len(self.payload))
            self.payload += bytes.fromhex("33c0c3")

    def iter_symbols(self):
        return ((i, offset, 1) for i, offset in enumerate(self.offsets))

    def sym_name(self, index):
        return self.names[index]

    def defined_symbols(self, section):
        return list(zip(self.offsets, self.names))

    def section_payload(self, section):
        return self.payload

    def typed_relocations(self, section):
        return {1: ("_$E1", 6), 6: ("_atexit", 0x14)}


class SharedSectionCallbackTest(unittest.TestCase):
    def callbacks(self, callback, following=True):
        obj = SharedTextObject(callback, following)
        # Argument decoding is independent of the callback section boundary.
        with mock.patch.object(static_dtors, "argument_site", return_value=1):
            return static_dtors._base_callbacks(obj, {"?Draw@@YAXXZ"})

    def test_empty_callback_stops_before_following_function(self):
        self.assertEqual(self.callbacks(b"\xc3" + b"\x90" * 15),
                         {"?Draw@@YAXXZ": [("_$E1", 1)]})

    def test_teardown_callback_stops_before_following_function(self):
        body = bytes.fromhex("b900000000e900000000")
        self.assertEqual(self.callbacks(body + b"\xcc" * 6),
                         {"?Draw@@YAXXZ": [("_$E1", 10)]})

    def test_last_callback_uses_section_end(self):
        self.assertEqual(self.callbacks(b"\xc3", following=False),
                         {"?Draw@@YAXXZ": [("_$E1", 1)]})

    def test_extra_code_inside_callback_is_not_tail_padding(self):
        self.assertEqual(self.callbacks(bytes.fromhex("c333c0c3")), {})


if __name__ == "__main__":
    unittest.main()
