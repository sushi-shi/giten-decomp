"""Small controls for the CUR/MAX/HIST bank and its write precondition."""

import json
import tempfile
import unittest
from contextlib import ExitStack, redirect_stdout
from io import StringIO
from pathlib import Path
from unittest import mock

from giten.graph import scan
from giten.verify import astprint, fingerprints, verbs


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


class FingerprintCacheControls(unittest.TestCase):
    def test_header_and_compile_context_edits_refresh_function_fingerprints(self):
        with tempfile.TemporaryDirectory() as directory, ExitStack() as stack:
            root = Path(directory)
            (root / "include").mkdir()
            header = root / "include/entry.h"
            header.write_text("struct Entry { unsigned short value; };\n")
            (root / "unit.c").write_text(
                '#include "entry.h"\n'
                'int read(struct Entry *entry) { return entry->value; }\n'
                'int constant(void) { return VALUE; }\n')
            cdb = root / "compile_commands.json"
            entry = {"directory": str(root), "file": str(root / "unit.c"),
                     "arguments": ["clang-cl", "/c", str(root / "unit.c"),
                                   "/I" + str(root / "include"), "/DVALUE=7",
                                   "--target=i386-pc-windows-msvc"]}
            cdb.write_text(json.dumps([entry]))
            for module in (fingerprints, astprint, scan):
                stack.enter_context(mock.patch.object(module, "REPO", root))
            stack.enter_context(mock.patch.object(astprint, "CDB", cdb))
            for name in ("CACHE", "SEED"):
                stack.enter_context(mock.patch.object(
                    fingerprints, name, root / (name + ".tsv")))
            stack.enter_context(mock.patch.object(
                fingerprints, "unit_sources", return_value={"u": "unit.c"}))
            stack.enter_context(mock.patch.object(
                fingerprints, "unit_mangled", return_value={"u": {"_read", "_constant"}}))
            parse = stack.enter_context(mock.patch.object(
                astprint, "unit_fingerprints", wraps=astprint.unit_fingerprints))
            stack.enter_context(redirect_stdout(StringIO()))

            def current():
                fingerprints.regenerate()
                fp, _, stale = fingerprints.fingerprinter()
                result = (fp("u", "_read"), fp("u", "_constant"))
                self.assertFalse(stale)
                return result

            original = current()
            self.assertTrue(all(fp.startswith(astprint.PREFIX) for fp in original))
            self.assertEqual(current(), original)
            self.assertEqual(parse.call_count, 1)

            # A type change in a transitive header must invalidate the cache
            # before regeneration; only the function using that type changes.
            (root / "include/type.h").write_text("typedef short Value;\n")
            header.write_text('#include "type.h"\nstruct Entry { Value value; };\n')
            fp, _, stale = fingerprints.fingerprinter()
            self.assertTrue(fingerprints.is_fallback(fp("u", "_read")))
            self.assertEqual(stale, {"u"})
            changed = current()
            self.assertNotEqual(changed[0], original[0])
            self.assertEqual(changed[1], original[1])

            (root / "include/type.h").write_text("typedef unsigned short Value;\n")
            self.assertEqual(current(), original)
            self.assertEqual(parse.call_count, 3)

            # Header spelling alone triggers a reparse without resetting MAX.
            (root / "include/type.h").write_text("typedef unsigned short Value; // same type\n")
            self.assertEqual(current(), original)
            self.assertEqual(parse.call_count, 4)

            entry["arguments"][4] = "/DVALUE=8"
            cdb.write_text(json.dumps([entry]))
            changed = current()
            self.assertEqual(changed[0], original[0])
            self.assertNotEqual(changed[1], original[1])

            # Old source-only caches must also be reparsed on upgrade.
            cache = fingerprints.CACHE
            lines = cache.read_text().splitlines()
            cache.write_text("\n".join(
                "\t".join(line.split("\t")[:3]) if line.startswith("u\t") else line
                for line in lines) + "\n")
            self.assertEqual(current(), changed)
            self.assertEqual(parse.call_count, 6)
