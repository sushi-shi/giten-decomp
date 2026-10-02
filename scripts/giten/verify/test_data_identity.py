"""A relocation addend must not hide conflicting data ownership."""

import unittest

from giten.verify import data_identity as di


def site(name, retail, addend=0, unit="u"):
    return di.Site(unit=unit, function="_F", offset=1,
                   key=name, symbol=name, local=False, addend=addend,
                   retail=retail, target="retail")


class DataIdentityControls(unittest.TestCase):
    def test_two_source_names_for_one_retail_base_fail(self):
        first = site("_g_left", 0x91000, unit="left")
        second = site("_g_right", 0x91004, addend=4, unit="right")
        findings = di.conflicts([first, second], {})
        self.assertEqual(len(findings), 1)
        self.assertIn("_g_left", findings[0])
        self.assertIn("_g_right", findings[0])

    def test_one_source_name_for_two_retail_bases_fails(self):
        findings = di.conflicts([site("_g_value", 0x91000),
                                 site("_g_value", 0x92000)], {})
        self.assertEqual(len(findings), 1)
        self.assertIn("reaches 2 retail bases", findings[0])
