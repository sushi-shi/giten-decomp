"""Code and data boundaries can alternate inside the executable section."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten.retail_labels import censuses


class TextDataTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.code = Path(self.temp.name) / 'functions.tsv'
        self.data = Path(self.temp.name) / 'data.tsv'
        self.code.write_text('rva\tkind\n0x1000\t\n0x1020\t\n0x1050\t\n')
        self.data.write_text('rva\tkind\n0x1010\t\n0x1030\t\n0x2000\t\n0x2108\t\n')
        pe = mock.Mock()
        pe.text_span.return_value = (0x1000, 0x1060)
        pe.data_regions.return_value = {'data': (0x2000, 0x2100), 'bss': (0x2100, 0x2120)}
        patch = mock.patch.object(censuses, 'image', return_value=pe)
        patch.start()
        self.addCleanup(patch.stop)

    def test_code_and_data_cap_each_others_extents(self):
        code = censuses.functions(self.code, self.data)
        data = censuses.data(self.data, self.code)
        self.assertEqual([(r['rva'], r['size']) for r in code],
                         [(0x1000, 0x10), (0x1020, 0x10), (0x1050, 0x10)])
        self.assertEqual([(r['rva'], r['size'], r['region']) for r in data],
                         [(0x1010, 0x10, 'text'), (0x1030, 0x20, 'text'),
                          (0x2000, 0x108, 'data'), (0x2108, 0x18, 'bss')])

    def test_a_start_cannot_be_both_code_and_data(self):
        self.data.write_text('rva\tkind\n0x1020\t\n')
        with self.assertRaisesRegex(ValueError, 'code and data share census start'):
            censuses.functions(self.code, self.data)


if __name__ == '__main__':
    unittest.main()
