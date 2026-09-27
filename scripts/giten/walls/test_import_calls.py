import unittest

from giten.walls.import_calls import cached_import_calls
from giten.walls.diagnose import _call_targets


def assembly(rows):
    return '\n'.join(f'{offset:x}:\t{encoded}\t{text}'
                     for offset, encoded, text in rows)


LOAD = (0, '8b 35 00 00 00 00', 'mov esi,DWORD PTR ds:0x0')
IMPORT = ('__imp__PeekMessageA@20', 6)
OTHER = ('__imp__TranslateMessage@4', 6)


class CachedImportsTests(unittest.TestCase):
    def resolve(self, rows, rel=None):
        return cached_import_calls({2: IMPORT} if rel is None else rel, assembly(rows))

    def test_multiple_calls_and_saved_register_copy(self):
        self.assertEqual(self.resolve([
            LOAD, (6, 'ff d6', 'call esi'), (8, '8b ee', 'mov ebp,esi'),
            (10, 'ff d5', 'call ebp'), (12, 'c3', 'ret'),
        ]), [IMPORT, IMPORT])

    def test_calls_preserve_nonvolatile_imports(self):
        self.assertEqual(self.resolve([
            LOAD, (6, 'e8 00 00 00 00', 'call 0xb'),
            (11, 'ff d6', 'call esi'), (13, 'c3', 'ret'),
        ]), [IMPORT])

    def test_loop_keeps_dominating_import(self):
        self.assertEqual(self.resolve([
            LOAD, (6, 'ff d6', 'call esi'), (8, '75 fc', 'jne 0x6'),
            (10, '5e', 'pop esi'), (11, 'c3', 'ret'),
        ]), [IMPORT])

    def test_loop_clobber_invalidates_backedge(self):
        self.assertEqual(self.resolve([
            LOAD, (6, 'ff d6', 'call esi'), (8, '33 f6', 'xor esi,esi'),
            (10, '75 fa', 'jne 0x6'), (12, 'c3', 'ret'),
        ]), [])

    def test_bypass_load_invalidates_join(self):
        self.assertEqual(self.resolve([
            (0, '75 06', 'jne 0x8'),
            (2, '8b 35 00 00 00 00', 'mov esi,DWORD PTR ds:0x0'),
            (8, 'ff d6', 'call esi'), (10, 'c3', 'ret'),
        ], {4: IMPORT}), [])

    def test_different_imports_at_join_are_unknown(self):
        self.assertEqual(self.resolve([
            LOAD, (6, '75 06', 'jne 0xe'),
            (8, '8b 35 00 00 00 00', 'mov esi,DWORD PTR ds:0x0'),
            (14, 'ff d6', 'call esi'), (16, 'c3', 'ret'),
        ], {2: IMPORT, 10: OTHER}), [])

    def test_same_import_at_join_is_proven(self):
        self.assertEqual(self.resolve([
            LOAD, (6, '75 06', 'jne 0xe'),
            (8, '8b 35 00 00 00 00', 'mov esi,DWORD PTR ds:0x0'),
            (14, 'ff d6', 'call esi'), (16, 'c3', 'ret'),
        ], {2: IMPORT, 10: IMPORT}), [IMPORT])

    def test_partial_register_clobber(self):
        self.assertEqual(self.resolve([
            LOAD, (6, '66 33 f6', 'xor si,si'), (9, 'ff d6', 'call esi'),
        ]), [])

    def test_unknown_instruction_discards_provenance(self):
        self.assertEqual(self.resolve([
            LOAD, (6, '61', 'popa'), (7, 'ff d6', 'call esi'),
        ]), [])

    def test_indirect_jump_rejects_function(self):
        self.assertEqual(self.resolve([
            LOAD, (6, 'ff d6', 'call esi'), (8, 'ff e0', 'jmp eax'),
        ]), [])

    def test_volatile_register_is_not_guessed(self):
        self.assertEqual(self.resolve([
            (0, '8b 0d 00 00 00 00', 'mov ecx,DWORD PTR ds:0x0'),
            (6, 'ff d1', 'call ecx'),
        ]), [])

    def test_nonimport_and_interior_claims_are_not_guessed(self):
        rows = [LOAD, (6, 'ff d6', 'call esi')]
        self.assertEqual(self.resolve(rows, {2: ('_callback', 0)}), [])
        self.assertEqual(self.resolve(rows, {2: (IMPORT[0], 0x14)}), [])
        interior = [(0, '8b 35 04 00 00 00', 'mov esi,DWORD PTR ds:0x4'),
                    (6, 'ff d6', 'call esi')]
        self.assertEqual(self.resolve(interior), [])

    def test_diagnostic_keeps_direct_and_cached_call_sites(self):
        rows = [LOAD, (6, 'ff d6', 'call esi'),
                (8, 'ff 15 00 00 00 00', 'call DWORD PTR ds:0x0'),
                (14, 'c3', 'ret')]
        self.assertCountEqual(_call_targets({2: IMPORT, 10: OTHER}, assembly(rows)),
                              [IMPORT, OTHER])


if __name__ == '__main__':
    unittest.main()
