from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from giten.permute import tu_state_noise as noise


class TuStateNoiseTests(unittest.TestCase):
    def test_source_hash_refreshes_stale_restored_unit(self):
        stale = set()

        def outdated(unit, symbol):
            stale.add(unit)
            return "cpp:temporary"

        fresh = mock.Mock(return_value="canonical")
        with mock.patch("giten.verify.fingerprints.fingerprinter", side_effect=[
            (outdated, None, stale), (fresh, None, set()),
        ]), mock.patch("giten.verify.fingerprints.regenerate") as regenerate:
            self.assertEqual(noise.current_source_hash("unit", "_Target"), "canonical")
            regenerate.assert_called_once_with()
            fresh.assert_called_once_with("unit", "_Target")

    def test_source_hash_preserves_fresh_unresolved_fallback(self):
        for value in ("canonical", "header:canonical", "cpp:unresolved"):
            with self.subTest(value=value), mock.patch(
                "giten.verify.fingerprints.fingerprinter",
                return_value=(mock.Mock(return_value=value), None, set()),
            ), mock.patch("giten.verify.fingerprints.regenerate") as regenerate:
                self.assertEqual(noise.current_source_hash("unit", "_Target"), value)
                regenerate.assert_not_called()

    def test_exact_closure_requires_score_size_and_ordered_relocations(self):
        metrics = {
            "reloc_stream_complete": True,
            "reloc_stream": ["00000001:0006:_target:04000000"],
        }
        self.assertEqual(
            noise.exact_closure_rejections(100.0, 6, 6, metrics, metrics), []
        )
        self.assertIn(
            "unrounded objdiff score is not exactly 100.0",
            noise.exact_closure_rejections(99.999, 6, 6, metrics, metrics),
        )
        other = dict(metrics, reloc_stream=["00000001:0006:_other:04000000"])
        self.assertIn(
            "ordered relocation offsets/types/identities/addends differ from retail",
            noise.exact_closure_rejections(100.0, 6, 6, metrics, other),
        )

    def test_disposable_objects_use_the_authoritative_canonical_view(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "raw.obj"
            output = Path(directory) / "normalized.obj"
            source.write_bytes(b"raw-coff")
            canonical = SimpleNamespace(data=b"canonical-coff")
            with mock.patch.object(
                noise, "canonicalize_coff", return_value=canonical
            ) as transform:
                result = noise.canonicalize_disposable_object(source, output)
            self.assertEqual(result, output)
            self.assertEqual(output.read_bytes(), b"canonical-coff")
            transform.assert_called_once_with(b"raw-coff")

    def test_disposable_function_alias_is_proved_and_normalized(self):
        from giten.compare.test_normalize import coff, targets
        raw = coff(b"\xe8" + bytes(4) + b"\xc3\x8b\xc1\xc3",
                   [(1, 1, 0x14)], b"", (),
                   [("_Caller", 0, 1, 0x20, 2), ("?Alias", 6, 1, 0x20, 2)])
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "raw.obj"
            output = Path(directory) / "normalized.obj"
            source.write_bytes(raw)
            proof = mock.Mock(return_value={"?Alias": ("?Primary", 0x1000)})
            noise.canonicalize_disposable_object(source, output,
                                                  alias_proof=proof, unit="caller")
            proof.assert_called_once_with({"caller": source})
            self.assertEqual(targets(output.read_bytes())[(1, 1)], ("?Primary", 0))

    def test_disposable_alias_proof_failure_cannot_write_a_comparison_object(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "raw.obj"
            output = Path(directory) / "normalized.obj"
            source.write_bytes(b"raw-coff")
            proof = mock.Mock(side_effect=ValueError("candidate body differs"))
            with self.assertRaisesRegex(ValueError, "candidate body differs"):
                noise.canonicalize_disposable_object(source, output,
                                                      alias_proof=proof, unit="caller")
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
