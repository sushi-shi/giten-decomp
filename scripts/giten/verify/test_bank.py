"""Score precision, source identity, and CUR/MAX/HIST banking."""

import os
import tempfile
import unittest
from pathlib import Path
from unittest import mock


class LedgerPrecisionControls(unittest.TestCase):

    def test_raw_and_banked_readings_agree(self):
        from giten.verify.baseline import at_ledger_precision, below_best
        best = 68.1947
        for raw in (68.18469, 68.1847, 68.18471, 68.0, 68.1947, 70.0):
            self.assertEqual(
                below_best(raw, best),
                below_best(at_ledger_precision(raw), best),
                f"{raw} reads differently once banked - unclearable gate")

    def test_a_dip_the_ledger_cannot_see_is_not_a_regression(self):
        from giten.verify.baseline import below_best
        self.assertFalse(below_best(68.18469, 68.1947))   # rounds to the edge
        self.assertTrue(below_best(68.1840, 68.1947))     # genuinely below

    def test_a_real_dip_is_still_fresh_after_banking(self):
        from giten.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 90.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (1, 0))

    def test_a_banked_dip_is_carried(self):
        from giten.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 80.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (0, 1))


class ReportInputControls(unittest.TestCase):
    """A bad --report is an operator error, not a traceback."""

    def test_missing_malformed_and_foreign_json_all_say_what_to_do(self):
        from giten.verify import scores
        with tempfile.TemporaryDirectory() as td:
            gone = Path(td) / "gone.json"
            with self.assertRaises(SystemExit) as e:
                scores.load(gone)
            self.assertIn("gone.json", str(e.exception))

            trunc = Path(td) / "trunc.json"
            trunc.write_text('{"units": [')
            with self.assertRaises(SystemExit) as e:
                scores.load(trunc)
            self.assertIn("not valid JSON", str(e.exception))
            self.assertIn("giten compare", str(e.exception))

            foreign = Path(td) / "other.json"
            foreign.write_text('{"hello": 1}')
            with self.assertRaises(SystemExit) as e:
                scores.load(foreign)
            self.assertIn("not an objdiff report", str(e.exception))


    def test_a_real_report_still_loads(self):
        from giten.verify import scores
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "r.json"
            p.write_text('{"units": [], "measures": {}}')
            self.assertEqual(scores.load(p)["units"], [])


class HeaderFingerprintControls(unittest.TestCase):
    def test_emitting_tu_edits_do_not_change_header_body_fingerprint(self):
        from giten.verify import fingerprints as fp
        from giten.tool import clang
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            repo = root / "repo"
            repo.mkdir()
            source = repo / "unit.cpp"
            source.write_text("int unrelated;\n")
            header = root / "sdk.h"
            header.write_bytes(b"inline int Value() { return 1; }")
            extent = {"file": str(header), "start": 0, "end": header.stat().st_size}
            with mock.patch.object(fp, "REPO", repo), \
                 mock.patch.object(clang, "compdb", return_value={}), \
                 mock.patch.object(clang, "function_definition_extents",
                                   return_value={"?Value@@YAHXZ": [extent]}):
                before = fp.header_fingerprints("unit.cpp")
                source.write_text("int changed_unrelated;\n")
                self.assertEqual(before, fp.header_fingerprints("unit.cpp"))
                header.write_bytes(b"inline int Value() { return 2; }")
                self.assertNotEqual(before, fp.header_fingerprints("unit.cpp"))

    def test_ambiguous_and_project_owned_definitions_keep_the_fallback(self):
        from giten.verify import fingerprints as fp
        from giten.tool import clang
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            repo = root / "repo"
            repo.mkdir()
            source = repo / "unit.cpp"
            source.write_text("int unrelated;\n")
            project_header = repo / "project.h"
            project_header.write_text("inline int Value() { return 1; }\n")
            sdk_header = root / "sdk.h"
            sdk_header.write_text("inline int Other() { return 2; }\n")
            extent = {"file": str(project_header), "start": 0,
                      "end": project_header.stat().st_size}
            external = {"file": str(sdk_header), "start": 0,
                        "end": sdk_header.stat().st_size}
            definitions = {"?Value@@YAHXZ": [extent],
                           "?Other@@YAHXZ": [external, dict(external, end=5)]}
            with mock.patch.object(fp, "REPO", repo), \
                 mock.patch.object(clang, "compdb", return_value={}), \
                 mock.patch.object(clang, "function_definition_extents", return_value=definitions):
                self.assertEqual(fp.header_fingerprints("unit.cpp"), {})


class BankRatchetControls(unittest.TestCase):
    """bank_rows is what edits config/match_baseline.tsv. Every rule that
    protects a banked MAX gets a control here; nothing writes the ledger."""

    def _bank(self, cur, base, fps=None, rvas=None, library=()):
        from giten.verify import verbs
        fps = fps or {}
        with mock.patch.object(verbs, "library_rvas", return_value=set(library)):
            return verbs.bank_rows(cur, base,
                                   lambda u, f: fps.get((u, f), "h1"),
                                   rvas or {})

    def _row(self, best=90.0, cur=90.0, fp="h1", addr=0x1000, hist=None,
             state="", tries=1):
        return {"best": best, "cur": cur, "tries": tries, "fp": fp,
                "addr": addr, "hist": best if hist is None else hist,
                "state": state}

    def test_a_dip_never_lowers_a_best_while_the_source_is_unchanged(self):
        key = ("u", "f")
        new, stats, reset, _drop = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, cur=90.0)},
            rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)     # MAX held
        self.assertEqual(new[key]["cur"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)
        self.assertEqual(reset, [])

    def test_a_real_source_edit_resets_best_but_never_hist(self):
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="old")},
            fps={key: "new"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)     # the all-time peak holds
        self.assertEqual(len(reset), 1)

    def test_a_fallback_fingerprint_is_not_an_edit(self):
        from giten.verify.fingerprints import FALLBACK
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="real")},
            fps={key: FALLBACK + "abc"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)
        self.assertEqual(new[key]["fp"], "real")     # the real hash is kept
        self.assertEqual(reset, [])

    def test_the_rva_moving_is_the_only_rva_keyed_reset(self):
        key = ("u", "f")
        new, stats, _r, _d = self._bank(
            {key: 55.0}, {key: self._row(best=90.0, addr=0x1000)},
            rvas={key: 0x2000})
        self.assertEqual(stats["rebounds"], 1)
        self.assertEqual(new[key]["best"], 55.0)     # a different BODY
        self.assertEqual(new[key]["hist"], 90.0)

    def test_a_unit_move_migrates_the_high_water_by_rva(self):
        old, new_key = ("olda", "f"), ("newb", "f")
        new, stats, _r, _d = self._bank(
            {new_key: 80.0}, {old: self._row(best=95.0, addr=0x1000)},
            rvas={new_key: 0x1000})
        self.assertEqual(stats["moved"], 1)
        self.assertEqual(new[new_key]["best"], 95.0)
        self.assertNotIn(old, new)

    def test_an_unscored_body_is_preserved_absent_and_round_trips(self):
        from giten.verify import baseline as bl
        key = ("u", "f")
        new, stats, _r, dropped = self._bank(
            {}, {key: self._row(best=100.0, addr=0x1000)}, rvas={})
        self.assertEqual(stats["preserved_absent"], 1)
        self.assertEqual(new[key]["state"], "absent")
        self.assertEqual(new[key]["best"], 100.0)
        self.assertEqual(dropped, [])
        self.assertEqual(bl.load(bl.render(new)), new)     # survives the file

    def test_an_absent_row_is_dropped_once_its_rva_is_claimed_elsewhere(self):
        key, other = ("u", "f"), ("u", "g")
        new, stats, _r, dropped = self._bank(
            {other: 100.0}, {key: self._row(best=100.0, addr=0x1000)},
            rvas={other: 0x1000})
        # 0x1000 is now claimed under another name: keeping the row would pin
        # a phantom, and the high-water travelled with the body (moved).
        self.assertEqual(stats["moved"] + len(dropped), 1)
        self.assertNotIn("absent", {r.get("state") for r in new.values()})

    def test_banking_twice_changes_nothing(self):
        key = ("u", "f")
        base = {key: self._row(best=90.0, cur=90.0)}
        first, _s, _r, _d = self._bank({key: 95.0}, base, rvas={key: 0x1000})
        second, stats, _r, _d = self._bank({key: 95.0}, first,
                                           rvas={key: 0x1000})
        self.assertEqual(first, second)
        self.assertEqual(stats["raised"], 0)

    def test_the_ledger_round_trips_through_render_and_load(self):
        from giten.verify import baseline as bl
        rows = {("u", "f"): self._row(best=99.1234, cur=98.7654, hist=100.0),
                ("u", "g"): self._row(best=100.0, cur=100.0, addr=None,
                                      state="absent")}
        self.assertEqual(bl.load(bl.render(rows)), rows)

    def test_render_keeps_the_explicit_state_field(self):
        from giten.verify import baseline as bl
        rows = {("u", "scored"): self._row(),
                ("u", "gone"): self._row(state="absent")}
        function_lines = [line for line in bl.render(rows).splitlines()
                          if line.startswith("u\t") and line.count("\t") > 2]
        scored = next(line for line in function_lines if "\tscored\t" in line)
        absent = next(line for line in function_lines if "\tgone\t" in line)
        self.assertTrue(scored.endswith("\t"))
        self.assertTrue(absent.endswith("\tabsent"))


class BankPreconditionControls(unittest.TestCase):
    def test_an_unstaged_build_input_refuses_and_names_the_paths(self):
        from giten.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["src/Giten/Grunt.cpp"]):
            with self.assertRaises(SystemExit) as e:
                verbs.require_bankable_tree("write the baseline")
        msg = str(e.exception)
        self.assertIn("src/Giten/Grunt.cpp", msg)
        self.assertIn("--dirty", msg)

    def test_dirty_warns_loudly_and_proceeds(self):
        import contextlib
        import io

        from giten.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["include/Giten/Grunt.h"]):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                verbs.require_bankable_tree("write the baseline",
                                            allow_dirty=True)
        self.assertIn("WARNING", err.getvalue())
        self.assertIn("include/Giten/Grunt.h", err.getvalue())

    def test_a_clean_tree_is_silent(self):
        from giten.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs", return_value=[]):
            verbs.require_bankable_tree("write the baseline")

    def test_a_stale_report_is_called_out(self):
        import contextlib
        import io

        from giten.verify import verbs
        with tempfile.TemporaryDirectory() as td:
            report = Path(td) / "report.json"
            report.write_text("{}")
            objs = Path(td) / "build/objdiff/base"
            objs.mkdir(parents=True)
            obj = objs / "a.obj"
            obj.write_bytes(b"x")
            os.utime(obj, (report.stat().st_mtime + 600,) * 2)
            with mock.patch.object(verbs, "REPO", Path(td)):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    verbs._warn_stale_report(report)
        self.assertIn("STALE", err.getvalue())
        self.assertIn("giten build", err.getvalue())


class MaxGateClassificationControls(unittest.TestCase):

    def _kinds(self, pct, prev_cur, prev_fp, cur_fp):
        from giten.verify import classify as cl
        base = {("u", "f"): {"best": 100.0, "cur": prev_cur, "fp": prev_fp,
                             "addr": None, "hist": 100.0, "tries": 1,
                             "state": ""}}
        return [k for k, *_ in cl.classify({("u", "f"): pct}, base,
                                           lambda *_: cur_fp, {})]

    def test_an_unedited_dip_is_not_a_regression(self):
        self.assertEqual(self._kinds(89.5, 100.0, "aaaa", "aaaa"), ["DIP"])

    def test_an_edit_that_lowers_cur_is_a_regression(self):
        self.assertEqual(self._kinds(89.5, 100.0, "aaaa", "bbbb"), ["REGRESS"])

    def test_an_edit_that_keeps_cur_is_a_reset(self):
        self.assertEqual(self._kinds(97.4, 97.4, "aaaa", "bbbb"), ["RESET"])

    def test_a_changed_fallback_fingerprint_counts_as_an_edit(self):
        from giten.verify.fingerprints import FALLBACK
        self.assertEqual(
            self._kinds(89.5, 100.0, FALLBACK + "1", FALLBACK + "2"), ["REGRESS"])

    def test_only_regress_fails_the_gate(self):
        from giten.verify import classify as cl
        base = {("u", k): {"best": 100.0, "cur": c, "fp": "a", "addr": None,
                           "hist": 100.0, "tries": 1, "state": ""}
                for k, c in (("dip", 100.0), ("reset", 97.0), ("drop", 100.0))}
        cur = {("u", "dip"): 90.0, ("u", "reset"): 97.0, ("u", "drop"): 90.0}
        fps = {("u", "dip"): "a", ("u", "reset"): "b", ("u", "drop"): "b"}
        buckets = cl.buckets_of(cur, base, lambda *k: fps[k], {})
        regress = buckets.get("REGRESS", [])
        self.assertEqual([r[1] for r in regress], ["drop"])
        self.assertEqual([r[1] for r in cl.fresh_regressions(cur, base, regress)],
                         ["drop"])


class BankDataMatchingModeControls(unittest.TestCase):
    """Scores from the two modes never compare: check and bank refuse a
    mismatch, and the explicit re-base is the only way across."""

    def _row(self, best, cur, hist, state=""):
        return {"best": best, "cur": cur, "tries": 1, "fp": "a",
                "addr": 0x1000, "hist": hist, "state": state}

    def test_the_ledger_records_its_mode_and_a_legacy_ledger_is_strict(self):
        from giten.verify import baseline as bl
        rows = {("u", "f"): self._row(90.0, 90.0, 95.0)}
        self.assertTrue(bl.load_mode("# old banner\n# [functions]\tunit\n"))
        for on in (True, False):
            text = bl.render(rows, on)
            self.assertEqual(bl.load_mode(text), on)
            self.assertEqual(bl.load(text), rows)      # rows still round-trip

    def test_a_mode_change_is_a_mismatch(self):
        from giten.verify import baseline as bl
        strict = bl.render({}, True)
        with mock.patch("giten.core.data_matching.enabled", return_value=False):
            self.assertIn("--rebase-data-matching", bl.mode_mismatch(strict))
            self.assertIsNone(bl.mode_mismatch(bl.render({}, False)))
        with mock.patch("giten.core.data_matching.enabled", return_value=True):
            self.assertIsNone(bl.mode_mismatch(strict))

    def test_the_max_gate_refuses_to_compare_across_modes(self):
        import argparse
        import contextlib
        import io

        from giten.verify import verbs
        args = argparse.Namespace(report=None, strict=False, all=False)
        with mock.patch("giten.verify.baseline.mode_mismatch",
                        return_value="banked under the other mode"), \
             mock.patch.object(verbs, "load_state") as load:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(verbs._report(args, gate=True), 1)
                self.assertEqual(verbs._report(args, gate=False), 0)
        load.assert_not_called()          # no row was compared at all
        self.assertIn("banked under the other mode", out.getvalue())

    def test_bank_refuses_a_mode_change_without_the_rebase_flag(self):
        from giten.verify import verbs
        with mock.patch("giten.verify.baseline.mode_mismatch",
                        return_value="banked under the other mode"), \
             mock.patch.object(verbs, "require_bankable_tree") as tree:
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_bank([])
        self.assertIn("refusing to bank", str(e.exception))
        tree.assert_not_called()

    def test_rebase_is_refused_when_the_mode_did_not_change(self):
        from giten.verify import verbs
        with mock.patch("giten.verify.baseline.mode_mismatch",
                        return_value=None):
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_bank(["--rebase-data-matching"])
        self.assertIn("refusing to re-base", str(e.exception))

    def test_rebase_sets_max_and_hist_to_cur_and_drops_absent_rows(self):
        from giten.verify import verbs
        rows = {("u", "rose"): self._row(90.0, 100.0, 95.0),
                ("u", "fell"): self._row(100.0, 97.5, 100.0),
                ("u", "gone"): self._row(100.0, 100.0, 100.0, state="absent")}
        rebased, dropped = verbs.rebase_rows(rows)
        self.assertEqual(dropped, [("u", "gone")])
        for key in (("u", "rose"), ("u", "fell")):
            row = rebased[key]
            self.assertEqual((row["best"], row["hist"]), (row["cur"], row["cur"]))
        # nothing reads as a lost match (HIST > MAX) after the re-base
        self.assertFalse(any(r["hist"] > r["best"] for r in rebased.values()))


if __name__ == "__main__":
    unittest.main()
