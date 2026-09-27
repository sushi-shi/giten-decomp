import struct
import unittest

from giten.retail_labels.source import compgen_value


class CompgenValueTests(unittest.TestCase):
    def test_constant_division_preserves_pool_type_and_rounding(self):
        self.assertEqual(compgen_value("-1.0f / 3.0f"), ("f32", bytes.fromhex("ab aa aa be")))
        self.assertEqual(
            compgen_value("1.0 / 3.0"),
            ("f64", struct.pack("<d", 1.0 / 3.0)),
        )

    def test_runtime_division_is_not_a_pin(self):
        self.assertIsNone(compgen_value("value / 3.0")[0])


if __name__ == "__main__":
    unittest.main()
