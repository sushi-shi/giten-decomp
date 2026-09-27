from __future__ import annotations

import unittest
from pathlib import Path
from tempfile import TemporaryDirectory
from unittest.mock import patch

from giten.walls import abstractions


class SourceAbstractionTests(unittest.TestCase):
    def test_msvc_inline_helpers_are_found_through_includes(self):
        with TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "helpers.h").write_text('''
static __inline int Pack(int x) { return x + 1; }
inline int Plain(int x) { return x; }
// static __inline int Comment(int x) { return x; }
''')
            source = root / "source.c"
            source.write_text('''
#include "helpers.h"
static __inline int Local(int x) { return x; }
RVA(0x123450, 0x20)
int F(void) { return Pack(1) + Pack(2) + Plain(3) + Local(4); }
''')
            origin = abstractions.SourceIndex().origin(source, 0x123450)
        self.assertEqual(origin["inlines"], [
            {"name": "Local", "count": 1, "origin": "local"},
            {"name": "Pack", "count": 2, "origin": "helpers.h"},
            {"name": "Plain", "count": 1, "origin": "helpers.h"},
        ])
        self.assertTrue(origin["promote"])
        self.assertEqual(abstractions.choose_level(
            "regalloc", [], origin, False)[0], "textual")

    def test_lost_exact_match_remains_actionable_but_current_exact_is_state(self):
        rows = [
            {"unit": "sample", "symbol": "Lost", "rva": "0x10",
             "bank": 78.5, "hist_max": 100.0, "cur": 78.5,
             "lost": 21.5, "proven": True},
            {"unit": "sample", "symbol": "Held", "rva": "0x20",
             "bank": 100.0, "hist_max": 100.0, "cur": 88.0,
             "lost": 0.0, "proven": True},
        ]
        rows += [
            {**rows[0], "symbol": "Reviewed", "rva": "0x30"},
            {**rows[0], "symbol": "StaleReview", "rva": "0x40"},
        ]
        review = {"status": "bounded", "wall_class": "regalloc"}
        classified = [{**row, "classification": "regalloc"} for row in rows]
        with patch.object(abstractions.inventory, "build", return_value=rows), \
             patch.object(abstractions, "unit_modules", return_value={}), \
             patch("giten.permute.campaign.classified_candidates", return_value=classified), \
             patch("giten.manifest.units", return_value=[]), \
             patch("giten.walls.reviews.current", return_value={0x30: review}), \
             patch("giten.walls.reviews.load", return_value={0x30: review, 0x40: review}), \
             patch("giten.verify.fingerprints.fingerprinter",
                   return_value=(lambda *_: "123456789abc", {}, set())):
            complete = abstractions.build(with_aggregates=False)
            todo = abstractions.build(todo=True, with_aggregates=False)
        by_name = {row["symbol"]: row for row in complete}
        self.assertEqual(by_name["Lost"]["level"], "expression")
        self.assertTrue(by_name["Lost"]["actionable"])
        self.assertIn("source-hash transition in Git", by_name["Lost"]["next_action"])
        self.assertEqual(by_name["Held"]["level"], "state")
        self.assertFalse(by_name["Held"]["actionable"])
        self.assertFalse(by_name["Reviewed"]["actionable"])
        self.assertTrue(by_name["StaleReview"]["actionable"])
        self.assertEqual([row["symbol"] for row in todo], ["Lost", "StaleReview"])

    def test_function_body_is_brace_balanced_and_ignores_literal_braces(self):
        source = '''
RVA(0x00123450, 0x20)
int F() {
    const char* text = "}"; // }
    if (text) { return 1; }
    return 0;
}
RVA(0x00123470, 0x10)
int G() { return 2; }
'''
        body = abstractions.function_body(source, 0x123450)
        self.assertIn("if (text)", body)
        self.assertNotIn("int G", body)

    def test_hidden_inline_or_macro_origin_precedes_regalloc(self):
        origin = {"promote": True, "inlines": [
            {"name": "Pack", "count": 2, "origin": "local"}], "macros": []}
        level, evidence = abstractions.choose_level("regalloc", [], origin, False)
        self.assertEqual(level, "textual")
        self.assertTrue(any("inline Pack x2" in item for item in evidence))

    def test_aggregate_lead_precedes_call_set_classification(self):
        level, evidence = abstractions.choose_level(
            "inline", ["aggregate-read:under@+0x20"], {"promote": False}, False)
        self.assertEqual(level, "object")
        self.assertEqual(evidence, ["aggregate-read:under@+0x20"])

    def test_argument_copy_shape_does_not_route_as_an_object_lead(self):
        from giten.walls import aggregate_copies, aggdecl, aggscan, valuetemp

        arg_row = (88.65, "rezsync", "Run", 0x83450, 0x108,
                   ["SEP"], ["ARG"])
        empty_decl = ([], [], {}, 1, 0, 0)
        with patch.object(aggdecl, "scan", side_effect=[
                ([], [arg_row], {}, 1, 1, 0), empty_decl]), \
             patch.object(aggregate_copies, "scan", return_value=[]), \
             patch.object(valuetemp, "scan", return_value=([], [], [], [], [])), \
             patch.object(aggscan, "sweep", return_value={
                 "ours": [], "both": [], "retail": []}), \
             patch.object(aggscan, "perfunction", return_value={}):
            leads = abstractions.aggregate_leads()
        self.assertNotIn(("rezsync", "Run"), leads)

    def test_historical_max_remains_primary_queue_order(self):
        low_expression = {"hist_max": 70.0, "level": "expression",
                          "cur": 70.0, "rva": "0x20"}
        high_identity = {"hist_max": 80.0, "level": "identity",
                         "cur": 80.0, "rva": "0x10"}
        self.assertLess(abstractions.queue_priority(low_expression),
                        abstractions.queue_priority(high_identity))

    def test_generated_funclet_is_never_a_source_abstraction_claim(self):
        level, evidence = abstractions.choose_level(
            "regalloc", ["aggregate-copy-count"], {"promote": True}, True)
        self.assertEqual(level, "generated")
        self.assertEqual(evidence, ["EH-band funclet"])

    def test_state_is_a_terminal_routing_level(self):
        self.assertIn("state", abstractions.LEVEL_ORDER)
        self.assertIn("do not rewrite proven source", abstractions.NEXT_ACTION["state"])


if __name__ == "__main__":
    unittest.main()
