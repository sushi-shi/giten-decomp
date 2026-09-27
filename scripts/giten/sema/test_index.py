"""Code/data navigation follows census identity, not section placement."""
import unittest
from unittest import mock

from giten.model import Binding, Model
from giten.sema.index import Index
from giten.sema import map as address_map
from giten.sema import rva as address_dossier


class TextDataControls(unittest.TestCase):
    def setUp(self):
        self.function = Binding(0x1000, 0x10, "", "text", "", "", "", ())
        self.datum = Binding(0x1010, 0x20, "", "text", "", "", "", ())
        self.index = Index(Model([self.function], [self.datum], []))

    def test_text_data_is_not_displayed_as_a_function(self):
        self.assertTrue(self.index.is_function(self.function))
        self.assertFalse(self.index.is_function(self.datum))
        self.assertIn("unclaimed datum", self.index.display(self.datum))
        self.assertIn("unclaimed func", self.index.display(self.function))
        self.assertIsNone(self.index.owner(0x1018))
        self.assertEqual(self.index.covering(0x1018), self.datum)

    def test_dossier_does_not_query_code_for_text_data(self):
        image = mock.Mock(base=0x400000)
        image.section_name.return_value = ".text"
        image.refs_to_range.return_value = []
        image.string_at.return_value = None
        with mock.patch.object(address_dossier, "index", return_value=self.index), \
             mock.patch.object(address_dossier, "retail", return_value=image), \
             mock.patch.object(address_dossier, "report") as report:
            lines, result = address_dossier.dossier(self.datum.rva)
        self.assertEqual(result, 0)
        report.assert_not_called()
        image.jmp_target.assert_not_called()
        self.assertFalse(any("callers" in line for line in lines))

    def test_map_uses_data_neighbors_for_text_data(self):
        with mock.patch.object(address_map, "index", return_value=self.index):
            lines, result = address_map.at(0x1018)
        self.assertEqual(result, 0)
        self.assertTrue(any("unclaimed datum" in line for line in lines))
        self.assertFalse(any("unclaimed func" in line for line in lines))


if __name__ == "__main__":
    unittest.main()
