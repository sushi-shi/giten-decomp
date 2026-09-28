"""Comparator re-pins must invalidate reports without recompiling sources."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock

from giten import graph
from giten.graph import emit, verbs


class ComparatorDependencyTests(unittest.TestCase):
    def test_repin_reconfigures_but_unchanged_identity_preserves_mtime(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            marker = root / graph.COMPARATOR_ID
            ninja = root / graph.NINJA
            ninja.parent.mkdir(parents=True)
            ninja.write_text("# existing manifest\n")
            with mock.patch.object(verbs, "REPO", root), \
                    mock.patch.object(verbs, "toolchain_repinned", return_value=False), \
                    mock.patch.object(emit, "emit", return_value=(1, 0)) as configure, \
                    mock.patch("shutil.which", return_value="/nix/store/old/bin/objdiff-cli"):
                emit.write_comparator_id(marker)
                before = marker.stat().st_mtime_ns
                self.assertFalse(emit.write_comparator_id(marker))
                self.assertEqual(marker.stat().st_mtime_ns, before)
                verbs.configure_if_needed()
                configure.assert_not_called()
                with mock.patch("shutil.which", return_value="/nix/store/new/bin/objdiff-cli"):
                    verbs.configure_if_needed()
                    configure.assert_called_once()
                    self.assertTrue(emit.write_comparator_id(marker))

    def test_report_declares_comparator_but_compile_does_not(self):
        class Scan:
            def headers(self, source):
                return []

            def scanned(self):
                return {"src/Test/Owner.c"}

        unit = {"unit": "owner", "source": "src/Test/Owner.c",
                "flags": "retail", "cflags": ["/c"]}
        manifest = {"flags": {"retail": ["/c"]}}
        with tempfile.TemporaryDirectory() as td, \
                mock.patch.object(emit, "load_units", return_value=(manifest, [unit])), \
                mock.patch.object(emit, "prune_orphan_artifacts", return_value=0), \
                mock.patch.object(emit, "write_toolchain_id"), \
                mock.patch.object(emit, "write_comparator_id"), \
                mock.patch.object(emit, "Scanner", return_value=Scan()):
            path = Path(td) / "build.ninja"
            emit.emit(path)
            lines = path.read_text().replace("$\n", "").splitlines()
        report = next(line for line in lines if line.startswith(f"build {graph.REPORT_JSON}:"))
        compile_edge = next(line for line in lines if line.startswith("build build/objdiff/base/owner.obj:"))
        self.assertIn(graph.COMPARATOR_ID, report)
        self.assertNotIn(graph.COMPARATOR_ID, compile_edge)


if __name__ == "__main__":
    unittest.main()
