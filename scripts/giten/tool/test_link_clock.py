"""Controls for the frozen linker clock and emitted timestamp checks."""

from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from giten.tool import ToolError
from giten.tool import link


AT = "1999-11-05 07:37:31"
STAMP = 0x3822893b


class LinkClockControls(unittest.TestCase):
    def test_strict_utc_clock(self):
        self.assertEqual(link.clock_stamp(AT), STAMP)
        for value in ("1999-11-05", "1999-11-05 07:37:31Z",
                      "1999-02-30 07:37:31", "1969-01-01 00:00:00",
                      "1999-1-05 07:37:31", "@1999-11-05 07:37:31"):
            with self.subTest(value=value), self.assertRaises(ToolError):
                link.clock_stamp(value)

    def test_clock_arguments_and_missing_dependency(self):
        argv = ["wine", "link.exe", "@args.rsp"]
        self.assertEqual(link.clock_args(argv, None), argv)
        with patch.object(link.shutil, "which", return_value="/tool/faketime"):
            self.assertEqual(link.clock_args(argv, AT), [
                "env", "TZ=UTC", "FAKETIME_DONT_FAKE_MONOTONIC=1",
                "/tool/faketime", "-f", AT, *argv])
        with patch.object(link.shutil, "which", return_value=None):
            with self.assertRaises(ToolError):
                link.clock_args(argv, AT)

    def test_emitted_clocks_fail_closed(self):
        with tempfile.TemporaryDirectory() as temp:
            exe = Path(temp) / "test.exe"
            mapfile = Path(temp) / "test.map"
            data = bytearray(0x58)
            data[:2] = b"MZ"
            struct.pack_into("<I", data, 0x3c, 0x40)
            data[0x40:0x44] = b"PE\0\0"
            struct.pack_into("<I", data, 0x48, STAMP)
            exe.write_bytes(data)
            mapfile.write_text(" Timestamp is 3822893b (Fri Nov 05 07:37:31 1999)\n")
            link.check_clock([exe, mapfile], AT)
            for text in ("", "Timestamp is 3822893c\n",
                         "Timestamp is 3822893b\nTimestamp is 3822893b\n"):
                mapfile.write_text(text)
                with self.assertRaises(ToolError):
                    link.check_clock([mapfile], AT)
            struct.pack_into("<I", data, 0x48, STAMP + 1)
            exe.write_bytes(data)
            with self.assertRaises(ToolError):
                link.check_clock([exe], AT)
            exe.write_bytes(b"invalid")
            with self.assertRaises(ToolError):
                link.pe_stamp(exe)


if __name__ == "__main__":
    unittest.main()
