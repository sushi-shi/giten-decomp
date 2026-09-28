from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten.permute import batch_source_variants as batch


class BatchSourceVariantTests(unittest.TestCase):

    def test_manifest_requires_unique_exact_spans_and_nonoverlap(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("int value = left + right;\n")
            manifest = root / "axes.json"
            manifest.write_text(json.dumps({
                "schema": 1,
                "source": "src/unit.cpp",
                "rva": "0x1234",
                "axes": [{
                    "name": "order",
                    "find": "left + right",
                    "options": [
                        {"name": "keep"},
                        {"name": "swap", "replace": "right + left"},
                    ],
                }],
            }))
            _payload, _source, _original, axes, candidates, rva = \
                batch.load_manifest(manifest, root)
            self.assertEqual(rva, 0x1234)
            self.assertEqual(len(axes), 1)
            self.assertEqual(candidates, ())

            payload = json.loads(manifest.read_text())
            payload["axes"].append(dict(payload["axes"][0], name="overlap"))
            manifest.write_text(json.dumps(payload))
            with self.assertRaisesRegex(ValueError, "axes overlap"):
                batch.load_manifest(manifest, root)

    def test_axis_option_extra_edit_is_atomic(self):
        original = b"int helper;\nint result = old_call;\n"
        axis = batch.Axis("call", 25, 33, b"old_call", (
            batch.AxisOption("keep", b"old_call"),
            batch.AxisOption(
                "helper", b"new_call",
                (batch.Edit(0, 0, b"", b"static int new_call;\n"),),
            ),
        ))
        variants = list(batch.iter_variants(original, (axis,), ()))
        self.assertEqual(variants[1][1], {"call": "helper"})
        self.assertEqual(
            variants[1][0],
            b"static int new_call;\nint helper;\nint result = new_call;\n",
        )

    def test_disposable_sibling_is_removed_after_compile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src" / "unit.cpp"
            source.parent.mkdir()
            source.write_bytes(b"original\n")
            scratch = root / "scratch"
            scratch.mkdir()

            def inspect_source(_root, probe, output, _flags, _timeout):
                self.assertEqual(probe.read_bytes(), b"candidate\n")
                self.assertEqual(output, scratch / "trial-0007.obj")
                return True, "", False

            with mock.patch.object(batch, "compile_object", side_effect=inspect_source):
                result = batch.compile_disposable_sibling(
                    root, source, scratch, 7, b"candidate\n", [], 12.0
                )
            self.assertEqual(result, (7, (True, "", False)))
            self.assertFalse((source.parent / ".unit.sourcevariant0007.cpp").exists())


if __name__ == "__main__":
    unittest.main()
