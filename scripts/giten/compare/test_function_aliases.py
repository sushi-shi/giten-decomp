"""Only explicitly pinned and fully byte-proven function aliases normalize."""
from pathlib import Path
from types import SimpleNamespace as NS
import tempfile
import unittest
from unittest.mock import patch

from giten.compare import canonicalize as canon
from giten.compare import function_aliases as fa
from giten.compare import normalize
from giten.compare.test_normalize import coff, targets

BODY = bytes.fromhex('8bc1c3')
PRIMARY, ALIAS = '?VertexCtor', '?VectorCtor'


def object_body(name, body=BODY, relocs=()):
    return coff(body, relocs, b'', (), [(name, 0, 1, 0x20, 2)])


def binding(name=PRIMARY, rva=0x1000, aliases=(), channel='src_compgen'):
    return NS(name=name, rva=rva, size=3, unit='primary',
              channel=channel, aliases=aliases)


class AliasProofTest(unittest.TestCase):
    def prove(self, other=BODY, *, aliases=True, relocs=(), size=3,
              primary_channel='src_compgen', alias_channel='src_compgen'):
        claim = NS(name=ALIAS, unit='alias', channel=alias_channel, size=size)
        model = NS(functions=[binding(aliases=[claim] if aliases else [],
                                      channel=primary_channel)])
        img = NS(read=lambda rva, size: BODY, relocs_in=lambda lo, hi: [])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'primary.obj').write_bytes(object_body(PRIMARY))
            (root / 'alias.obj').write_bytes(object_body(ALIAS, other, relocs))
            return fa.prove(model, root, img)

    def test_same_rva_and_bodies_prove_alias(self):
        self.assertEqual(self.prove(BODY + b'\x90\xcc'),
                         {ALIAS: (PRIMARY, 0x1000)})

    def test_identical_unpinned_body_is_not_an_alias(self):
        self.assertEqual(self.prove(aliases=False), {})

    def test_authored_and_generated_claims_share_the_same_proof(self):
        for primary, alias in [('src', 'src_compgen'), ('src_compgen', 'src'),
                               ('src', 'src')]:
            with self.subTest(primary=primary, alias=alias):
                self.assertEqual(self.prove(primary_channel=primary, alias_channel=alias),
                                 {ALIAS: (PRIMARY, 0x1000)})
                with self.assertRaisesRegex(ValueError, 'does not equal retail'):
                    self.prove(bytes.fromhex('33c0c3'), primary_channel=primary,
                               alias_channel=alias)

    def test_declaration_only_claim_does_not_prove_a_body(self):
        self.assertEqual(self.prove(alias_channel='src_decl'), {})
        self.assertEqual(self.prove(primary_channel='src_decl'), {})

    def test_different_body_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'does not equal retail'):
            self.prove(bytes.fromhex('33c0c3'))

    def test_nonpadding_tail_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'does not equal retail'):
            self.prove(BODY + b'\xc3')

    def test_conflicting_claim_extent_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'conflicting extent'):
            self.prove(size=2)

    def test_relocated_body_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'does not equal retail'):
            self.prove(BODY, relocs=[(0, 0, 6)])

    def test_disposable_body_is_reproved_even_outside_the_claim_owner(self):
        claim = NS(name=ALIAS, unit='alias', channel='src_compgen', size=3)
        model = NS(functions=[binding(aliases=[claim])])
        img = NS(read=lambda rva, size: BODY, relocs_in=lambda lo, hi: [])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'primary.obj').write_bytes(object_body(PRIMARY))
            (root / 'alias.obj').write_bytes(object_body(ALIAS))
            candidate = root / 'candidate.obj'
            candidate.write_bytes(object_body(ALIAS))
            self.assertEqual(fa.prove(model, root, img, overrides={'caller': candidate}),
                             {ALIAS: (PRIMARY, 0x1000)})
            candidate.write_bytes(object_body(ALIAS, bytes.fromhex('33c0c3')))
            with self.assertRaisesRegex(ValueError, 'candidate .*does not equal retail'):
                fa.prove(model, root, img, overrides={'caller': candidate})
            with self.assertRaisesRegex(ValueError, 'does not equal retail'):
                fa.prove(model, root, img, overrides={'alias': candidate})

    def test_cached_base_objects_invalidate_on_equal_length_body_change(self):
        claim = NS(name=ALIAS, unit='alias', channel='src_compgen', size=3)
        model = NS(functions=[binding(aliases=[claim])])
        img = NS(read=lambda rva, size: BODY, relocs_in=lambda lo, hi: [])
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            primary, alias, candidate = [root / name for name in
                                         ('primary.obj', 'alias.obj', 'candidate.obj')]
            primary.write_bytes(object_body(PRIMARY))
            alias.write_bytes(object_body(ALIAS))
            candidate.write_bytes(object_body(ALIAS))
            cache = {}
            with patch.object(canon, 'CoffObject', wraps=canon.CoffObject) as parse:
                for _ in range(2):
                    self.assertEqual(fa.prove(model, root, img, _base_objects=cache,
                                             overrides={'caller': candidate}),
                                     {ALIAS: (PRIMARY, 0x1000)})
                self.assertEqual(parse.call_count, 4)
            self.assertEqual(set(cache), {primary, alias})
            candidate.write_bytes(object_body(ALIAS, bytes.fromhex('33c0c3')))
            with self.assertRaisesRegex(ValueError, 'candidate .*does not equal retail'):
                fa.prove(model, root, img, _base_objects=cache, overrides={'caller': candidate})
            alias.write_bytes(object_body(ALIAS, bytes.fromhex('33c0c3')))
            with self.assertRaisesRegex(ValueError, 'does not equal retail'):
                fa.prove(model, root, img, _base_objects=cache)


class AliasRewriteTest(unittest.TestCase):
    def fixture(self, existing):
        symbols = [('_Caller', 0, 1, 0x20, 2), (ALIAS, 6, 1, 0x20, 2)]
        raw = b'\xe8' + bytes(4) + b'\xc3' + BODY
        if existing:
            symbols.append((PRIMARY, 9, 1, 0x20, 2))
            raw += BODY
        return coff(raw, [(1, 1, 0x14)], bytes(4), [(0, 1, 6)], symbols)

    def test_missing_primary_renames_existing_symbol(self):
        original = self.fixture(False)
        result, rows = fa.rewrite(original, {ALIAS: (PRIMARY, 0x1000)})
        self.assertEqual(targets(result)[(1, 1)], (PRIMARY, 0))
        self.assertEqual(targets(result)[(2, 0)], (PRIMARY, 0))
        self.assertEqual(len(rows), 1)
        before, after = canon.CoffObject(original), canon.CoffObject(result)
        self.assertEqual(len(before.symbols), len(after.symbols))
        for a, b in zip(before.sections, after.sections):
            self.assertEqual(before.section_bytes(a), after.section_bytes(b))

    def test_existing_primary_redirects_without_merging_definitions(self):
        original = self.fixture(True)
        result, _ = fa.rewrite(original, {ALIAS: (PRIMARY, 0x1000)})
        self.assertEqual(targets(result)[(1, 1)], (PRIMARY, 0))
        names = [s.name for s in canon.CoffObject(result).symbols.values()]
        self.assertIn(ALIAS, names)
        self.assertEqual(names.count(PRIMARY), 1)

    def test_per_object_cache_rewrites_when_aliases_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, output, sidecar = [root / n for n in ('in.obj', 'out.obj', 'out.tsv')]
            source.write_bytes(self.fixture(False))
            self.assertEqual(normalize._normalize_one(source, output, sidecar), 'wrote')
            self.assertEqual(normalize._normalize_one(source, output, sidecar), 'skip')
            self.assertEqual(normalize._normalize_one(source, output, sidecar,
                             aliases={ALIAS: (PRIMARY, 0x1000)}), 'wrote')
            self.assertEqual(targets(output.read_bytes())[(1, 1)], (PRIMARY, 0))


if __name__ == '__main__':
    unittest.main()
