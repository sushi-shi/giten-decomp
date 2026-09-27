"""Compiler helpers are compared without inflating reconstruction totals."""

import json
from pathlib import Path
import tempfile
import unittest

from giten.verify import scores


class CompilerCallbackScoresTest(unittest.TestCase):
    def test_callback_removed_from_rows_and_weighted_totals(self):
        measures = {"total_code": "30", "matched_code": "10",
                    "total_functions": 2, "matched_functions": 1,
                    "fuzzy_match_percent": 50.0}
        data = {"name": "$anon_data_storage_0", "size": "4"}
        doc = {"measures": dict(measures), "units": [{
            "name": "winmain", "measures": dict(measures),
            "functions": [
                {"name": "$anon_data_callback_0", "size": "10",
                 "fuzzy_match_percent": 100.0},
                {"name": "?Draw@@YAXXZ", "size": "20",
                 "fuzzy_match_percent": 25.0}], "data": [data]}]}
        with tempfile.TemporaryDirectory() as directory:
            report = Path(directory) / "report.json"
            report.write_text(json.dumps(doc))
            loaded = scores.load(report)
        self.assertEqual(scores.functions(loaded),
                         {("winmain", "?Draw@@YAXXZ"): 25.0})
        self.assertEqual(loaded["units"][0]["data"], [data])
        for result in (loaded["measures"], loaded["units"][0]["measures"]):
            self.assertEqual(result["total_code"], "20")
            self.assertEqual(result["matched_code"], "0")
            self.assertEqual(result["total_functions"], 1)
            self.assertEqual(result["matched_functions"], 0)
            self.assertEqual(result["fuzzy_match_percent"], 25.0)
        self.assertEqual(scores.split_generated_callbacks(loaded), [])

    def test_eh_band_api_keeps_callbacks_for_separate_accounting(self):
        callback = {"name": "$anon_data_callback_0", "size": "1"}
        doc = {"units": [{"functions": [callback]}]}
        self.assertEqual(scores.split_eh_band(doc), [])
        self.assertEqual(doc["units"][0]["functions"], [callback])


if __name__ == "__main__":
    unittest.main()
