"""Atomic publication of tables shared by concurrent build steps."""

import tempfile
import unittest
from pathlib import Path
from unittest import mock


class TsvAtomicWriteControls(unittest.TestCase):

    def test_write_replaces_atomically(self):
        import os
        from giten.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a", "b"], [["1", "2"]])
            before = path.read_text()
            seen = []
            real_replace = os.replace

            def spy(src, dst):
                # the destination must still hold the OLD table right up to
                # the instant of replacement - never a truncated one
                seen.append(Path(dst).read_text())
                return real_replace(src, dst)

            with mock.patch("os.replace", spy):
                tsv.write(path, ["# b"], ["a", "b"], [["3", "4"]])
            self.assertEqual(seen, [before])
            self.assertIn("3\t4", path.read_text())

    def test_no_temp_file_survives(self):
        from giten.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a"], [["1"]])
            self.assertEqual([p.name for p in Path(d).iterdir()], ["t.tsv"])


if __name__ == "__main__":
    unittest.main()
