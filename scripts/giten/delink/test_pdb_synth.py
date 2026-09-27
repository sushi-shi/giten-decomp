"""Fence spelling and the data-debt worklist under both data-matching modes."""

from __future__ import annotations

import tempfile
import io
import unittest
from pathlib import Path
from unittest import mock

from giten.core.tsv import read as read_tsv
from giten.delink import pdb_synth


class TextDataSymbolsTest(unittest.TestCase):
    def test_executable_section_constant_is_a_data_record(self):
        out = io.StringIO()
        bounds = {'.text': (0x1000, 0x2000), '.rdata': (0x2000, 0x3000),
                  '.data': (0x3000, 0x4000), '.idata': (0x4000, 0x5000)}
        with mock.patch.object(pdb_synth, 'sections_of', return_value=bounds), \
             mock.patch.object(pdb_synth, 'segments', return_value=(1, 2, 3, 4)):
            pdb_synth.emit_yaml([], [], [], [], {}, out,
                                text_syms=[(0x1080, '_sdkFormat')])
        yaml = out.getvalue()
        self.assertIn('S_LDATA32', yaml)
        self.assertIn('Offset:          128', yaml)
        self.assertIn('Segment:         1', yaml)
        self.assertIn("DisplayName:     '_sdkFormat'", yaml)
        self.assertNotIn('S_GPROC32', yaml)


def fences():
    rdata = [(0x1000, "UNPROVISIONED_00401000"), (0x1010, "DAT_00401010"),
             (0x1020, "??_C@_03ABCD@abc?$AA@")]
    data = [(0x2000, "_g_named"), (0x2004, "UNPROVISIONED_00402004")]
    return rdata, data


class FenceSpellingTest(unittest.TestCase):
    def test_strict_keeps_the_refused_spelling(self):
        rdata, data = fences()
        self.assertEqual(pdb_synth.relax_fences(rdata, data, True), 0)
        self.assertEqual((rdata, data), fences())

    def test_relaxed_respells_only_unprovisioned_fences(self):
        rdata, data = fences()
        self.assertEqual(pdb_synth.relax_fences(rdata, data, False), 2)
        self.assertEqual(rdata[0], (0x1000, "DAT_00401000"))
        self.assertEqual(data[1], (0x2004, "DAT_00402004"))
        self.assertEqual(rdata[1:], fences()[0][1:])      # others untouched
        self.assertEqual(data[0], fences()[1][0])
        self.assertFalse(any(name.startswith("UNPROVISIONED_")
                             for name in (n for _r, n in rdata + data)))


class DataDebtTest(unittest.TestCase):
    ROWS = [{"rva": 0x2004, "sites": [0x1234, 0x1300], "bands": ["cmdline"],
             "units": ["party"], "census": None}]

    def _write(self, rows, on):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "data_debt.tsv"
            with mock.patch("giten.core.data_matching.enabled",
                            return_value=on):
                pdb_synth.write_data_debt(rows, path)
            return read_tsv(path)

    def test_the_worklist_is_written_in_both_modes(self):
        for on in (True, False):
            banner, header, rows = self._write(self.ROWS, on)
            self.assertEqual(header, ["rva", "units", "bands", "sites", "census"])
            self.assertEqual(rows, [{"rva": "0x002004", "units": "party",
                                     "bands": "cmdline",
                                     "sites": "0x001234,0x001300",
                                     "census": "census=?"}])
            self.assertTrue(any(("true" if on else "false") in line
                                for line in banner))

    def test_an_empty_worklist_is_still_written(self):
        _banner, header, rows = self._write([], False)
        self.assertEqual((header[0], rows), ("rva", []))


if __name__ == "__main__":
    unittest.main()
