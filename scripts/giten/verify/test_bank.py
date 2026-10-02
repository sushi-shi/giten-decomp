"""Small controls for the CUR/MAX/HIST bank and its write precondition."""

import unittest
from unittest import mock

from giten.verify import verbs


class BankControls(unittest.TestCase):
    def bank(self, current, fingerprint):
        key = ("u", "f")
        row = {"best": 90.0, "cur": 90.0, "hist": 90.0,
               "tries": 1, "fp": "old", "addr": 0x1000, "state": ""}
        with mock.patch.object(verbs, "library_rvas", return_value=set()):
            rows, _stats, reset, _drop = verbs.bank_rows(
                {key: current}, {key: row},
                lambda _unit, _name: fingerprint, {key: 0x1000})
        return rows[key], reset

    def test_unchanged_source_keeps_best_after_a_dip(self):
        row, reset = self.bank(70.0, "old")
        self.assertEqual((row["cur"], row["best"], row["hist"]),
                         (70.0, 90.0, 90.0))
        self.assertEqual(reset, [])

    def test_source_edit_resets_best_but_keeps_history(self):
        row, reset = self.bank(70.0, "new")
        self.assertEqual((row["cur"], row["best"], row["hist"]),
                         (70.0, 70.0, 90.0))
        self.assertEqual(len(reset), 1)

    def test_unstaged_source_refuses_a_bank_write(self):
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["src/Game/fieldobj.c"]):
            with self.assertRaises(SystemExit) as result:
                verbs.require_bankable_tree("write the baseline")
        self.assertIn("src/Game/fieldobj.c", str(result.exception))
