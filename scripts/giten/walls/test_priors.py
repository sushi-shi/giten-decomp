import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten.walls import priors


class SourcePriorTests(unittest.TestCase):
    def test_c_cpp_and_header_pins_share_numeric_address_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'src/vector.c').write_text(
                '// @early-stop: bounded C function.\n'
                'RVA(0x0000d6b0, 0x20)\nvoid UnprojectPoint(void) {}\n')
            (root / 'src/vector.cpp').write_text(
                '// C++ spelling.\nRVA(0xd6b0, 0x20)\nvoid Other(void) {}\n')
            (root / 'include/vector.h').write_text(
                '// Header spelling.\nRVA_COMPGEN(0x000D6B0, 0x20, _Alias)\n')
            (root / 'src/notes.txt').write_text('RVA(0xd6b0, 0x20)\n')
            with mock.patch.object(priors, 'REPO', root):
                sites = priors._pin_sites()
                self.assertEqual(sites, {0xd6b0: [
                    ('src/vector.c', 2), ('src/vector.cpp', 2), ('include/vector.h', 2),
                ]})
                self.assertEqual(priors._comment_above(*sites[0xd6b0][0]),
                                 ['// @early-stop: bounded C function.'])

    def test_dynamic_init_and_whitespace_in_c_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'src/unit.c').write_text(
                '// prior\n  RVA_DYNINIT ( 0x00001680, 0x8, Init)\n')
            with mock.patch.object(priors, 'REPO', root):
                self.assertEqual(priors._pin_sites(), {0x1680: [('src/unit.c', 2)]})


if __name__ == '__main__':
    unittest.main()
