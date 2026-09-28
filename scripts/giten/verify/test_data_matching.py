"""Data gate verdicts in strict and relaxed comparison modes."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock


def _binding(rva, name, unit="probe", channel="src", kind="", size=0x10,
             space="text", aliases=()):
    from giten.model import Binding
    return Binding(rva, size, kind, space, name, unit, channel,
                   tuple(aliases), ())


def _model(functions=(), data=(), violations=()):
    from giten.model import Model
    return Model(list(functions), list(data), list(violations))


def _relaxed(on: bool):
    """Pin the data-matching switch for one control."""
    return mock.patch("giten.core.data_matching.enabled", return_value=on)


class UndefinedClosureDataMatchingControls(unittest.TestCase):
    """A placeholder `extern` is listed debt while data matching is off and a
    FAILURE while it is on - the switch itself is the re-enable worklist."""

    DEBT = {"_g_owned": {"units": ["party"], "reason": "undefined"}}

    def _verdict(self, on, debt=None, hard=()):
        from giten.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            Path(td, "probe.obj").write_bytes(b"")
            with _relaxed(on), mock.patch.object(uc, "BASE", Path(td)), \
                 mock.patch.object(uc, "placeholder_externs",
                                   return_value=dict(self.DEBT if debt is None
                                                     else debt)), \
                 mock.patch.object(uc, "_closure_findings",
                                   return_value=list(hard)):
                return uc.gate_verdict(), uc.gate_findings()

    def test_undefined_closure_placeholder_extern_fails_when_strict(self):
        (hard, advisory), findings = self._verdict(True)
        self.assertEqual(len(hard), 1)
        self.assertIn("_g_owned", hard[0])
        self.assertEqual(advisory, [])
        self.assertEqual(findings, hard)

    def test_undefined_closure_placeholder_extern_is_listed_when_relaxed(self):
        (hard, advisory), findings = self._verdict(False)
        self.assertEqual(hard, [])
        self.assertEqual(findings, [])
        self.assertEqual(len(advisory), 1)
        self.assertIn("_g_owned", advisory[0])

    def test_undefined_closure_other_findings_fail_in_both_modes(self):
        for on in (True, False):
            (hard, _adv), _f = self._verdict(on, debt={},
                                             hard=["pure-phantom class CX"])
            self.assertEqual(hard, ["pure-phantom class CX"])

    def test_undefined_closure_classifies_placeholder_externs(self):
        from giten.verify import undefined_closure as uc
        refs = {"_g_placeholder": {"a"}, "_g_owned": {"a"},
                "_g_unclaimed": {"b"}, "_g_provided": {"a"},
                "__imp__GetTickCount@4": {"a"}, "__iob": {"b"}}
        defined = {"_g_owned", "_g_unclaimed"}
        with mock.patch.object(uc, "_data_externs",
                               return_value=(refs, defined)), \
             mock.patch.object(uc, "_data_claims",
                               return_value=({"_g_owned"}, {"_g_provided"})):
            debt = uc.placeholder_externs(paths=[], libs={"__iob"})
        self.assertEqual(debt, {
            "_g_placeholder": {"units": ["a"], "reason": "undefined"},
            "_g_unclaimed": {"units": ["b"], "reason": "unclaimed"}})

    def test_undefined_closure_reads_only_data_externs_from_objects(self):
        from giten.compare.test_normalize import coff
        from giten.verify import undefined_closure as uc
        symbols = [("_Func", 0, 1, 0x20, 2), ("_g_data", 0, 0, 0x00, 2),
                   ("_Callee", 0, 0, 0x20, 2), ("_g_common", 4, 0, 0x00, 2),
                   ("_g_defined", 0, 2, 0x00, 2)]
        obj = coff(bytes(8), [], bytes(4), [], symbols)
        with tempfile.TemporaryDirectory() as td:
            path = Path(td, "unit.obj")
            path.write_bytes(obj)
            refs, defined = uc._data_externs([path])
        self.assertEqual(dict(refs), {"_g_data": {"unit"}})
        self.assertTrue({"_Func", "_g_common", "_g_defined"} <= defined)

    def test_undefined_closure_leaves_data_externs_out_of_declared_only(self):
        from giten.verify import undefined_closure as uc
        with mock.patch.object(uc, "live_base_objs", return_value=["x"]), \
             mock.patch.object(uc, "_sym_sets",
                               side_effect=[(set(), {"?g_x@@3HA"}),
                                            (set(), set())]), \
             mock.patch.object(uc, "_data_externs",
                               return_value=({"?g_x@@3HA": {"x"}}, set())), \
             mock.patch.object(uc, "lib_symbols", return_value=set()), \
             mock.patch.object(uc, "_rtti_classes", return_value=set()), \
             mock.patch.object(uc, "source_library_shadows", return_value=[]):
            _p, _s, declared = uc.analyse()
        self.assertEqual(declared, set())


class DataIdentityControls(unittest.TestCase):

    def _gate(self, sites, *, paired=1, base_objs=True):
        from giten.verify import data_identity as di
        from giten.verify import tiers

        def scan(stats):
            stats["functions paired"] += paired
            return sites, {}, {}
        gate = dict(tiers.TIERS["normal"])["data-identity"]
        verdicts = {}
        with tempfile.TemporaryDirectory() as td:
            if base_objs:
                Path(td, "probe.obj").write_bytes(b"")
            with mock.patch("giten.verify.undefined_closure.BASE", Path(td)), \
                 mock.patch.object(di, "scan", side_effect=scan), \
                 mock.patch.object(di, "TSV", Path(td, "data_identity.tsv")):
                for on in (True, False):
                    with _relaxed(on):
                        verdicts[on] = gate()
        return verdicts

    @staticmethod
    def _sites(menu_symbol):
        from giten.verify.test_data_identity import sites, unit
        return (sites("mouse", unit([("_GetA", [("_g_hoveredObjectId", 0)])]),
                      unit([("_GetA", [("DAT_004919e0", 0)])]),
                      {("_GetA", 1): 0x919e0})
                + sites("menuresult", unit([("_GetB", [(menu_symbol, 0)])]),
                        unit([("_GetB", [("DAT_004919e0", 0)])]),
                        {("_GetB", 1): 0x919e0}))

    def test_data_identity_duplicate_extern_fails_in_both_modes(self):
        for on, findings in self._gate(self._sites("_g_lastMenuItem")).items():
            self.assertEqual(len(findings), 1, f"data_matching={on}")
            self.assertIn("_g_lastMenuItem", findings[0])

    def test_data_identity_one_extern_passes(self):
        for findings in self._gate(self._sites("_g_hoveredObjectId")).values():
            self.assertEqual(findings, [])

    def test_data_identity_never_passes_vacuously(self):
        for findings in self._gate([], base_objs=False).values():
            self.assertIn("no base objs", findings[0])
        for findings in self._gate([], paired=0).values():
            self.assertIn("no function paired", findings[0])

    def test_data_identity_is_not_a_data_matching_gate(self):
        from giten.verify import tiers
        self.assertIn("data-identity", dict(tiers.TIERS["normal"]))
        self.assertNotIn("data-identity", tiers.DATA_MATCHING_GATES)


class DataPlacementDataMatchingControls(unittest.TestCase):
    """The data identity/placement gates report without failing only while
    data matching is off; the tier runner prints their count either way."""

    GATES = (("data-tu-order", "giten.verify.data_tu_order.gate_findings"),
             ("data-coverage", "giten.verify.data_coverage.gate_findings"))

    def _tier_fn(self, label):
        from giten.verify import tiers
        return dict(tiers.TIERS["normal"])[label]

    def test_data_tu_order_and_data_coverage_follow_the_switch(self):
        from giten.verify import tiers
        for label, target in self.GATES:
            with mock.patch(target, return_value=["finding"]):
                with _relaxed(True):
                    self.assertEqual(self._tier_fn(label)(), ["finding"])
                with _relaxed(False):
                    self.assertEqual(self._tier_fn(label)(),
                                     tiers.Verdict([], ["finding"]))

    def test_data_relocs_keeps_its_integrity_rows_failing(self):
        from giten.verify import tiers
        parts = (["WRONG referent"], ["live unit 'x' has no scored target"])
        with mock.patch("giten.verify.data_relocs.gate_parts",
                        return_value=parts):
            with _relaxed(True):
                self.assertEqual(self._tier_fn("data-relocs")(),
                                 parts[0] + parts[1])
            with _relaxed(False):
                self.assertEqual(self._tier_fn("data-relocs")(),
                                 tiers.Verdict(parts[1], parts[0]))

    def test_data_access_stays_failing_when_relaxed(self):
        with mock.patch("giten.verify.data_access.gate_findings",
                        return_value=["data-access: [width] probe"]), \
             _relaxed(False):
            self.assertEqual(self._tier_fn("data-access")(),
                             ["data-access: [width] probe"])

    def test_every_data_matching_gate_is_a_tier_member(self):
        from giten.verify import tiers
        labels = {n for rows in tiers.TIERS.values() for n, _f in rows}
        self.assertTrue(set(tiers.DATA_MATCHING_GATES) <= labels)

    def test_the_runner_lists_advisory_findings_without_failing(self):
        import contextlib
        import io

        from giten.verify import tiers
        gates = [("probe", lambda: tiers.Verdict([], ["debt one", "debt two"]))]
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(tiers.run(["fast"]), 0)
        text = out.getvalue()
        self.assertIn("ADVISORY (2 finding(s)", text)
        self.assertIn("debt two", text)
        gates = [("probe", lambda: tiers.Verdict(["hard"], ["debt"]))]
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(tiers.run(["fast"]), 1)
        self.assertIn("advisory (1, not failing)", out.getvalue())


if __name__ == "__main__":
    unittest.main()
