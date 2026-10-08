"""Reviewed import hints may change metadata, never ABI/archive structure."""
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest

from giten.graph import import_contract as contract
from giten.graph import implib


def fixture():
    names = b'_Test@0\0__imp__Test@0\0'
    strings = struct.pack('<I', 4 + len(names)) + names
    pointer = 20 + 3 * 40 + 6 + 4 + 8 + 20
    head = struct.pack('<HHIIIHH', 0x14c, 3, 0, pointer, 3, 0, 0x100)
    def section(name, size, raw, reloc, count, flags):
        return struct.pack('<8sIIIIIIHHI', name.ljust(8, b'\0'), 0, 0,
                           size, raw, reloc, 0, count, 0, flags)
    sections = section(b'.text', 6, 140, 158, 1, 0x60201020)
    sections += section(b'.idata$5', 4, 146, 168, 1, 0xc0301040)
    sections += section(b'.idata$6', 8, 150, 0, 0, 0xc0201040)
    symbols = struct.pack('<IIIhHBB', 0, 4, 0, 1, 0x20, 2, 0)
    symbols += struct.pack('<IIIhHBB', 0, 12, 0, 2, 0, 2, 0)
    symbols += struct.pack('<8sIhHBB', b'.idata$6', 0, 3, 0, 3, 0)
    body = head + sections + b'\xff\x25\0\0\0\0' + bytes(4) + b'\x03\0Test\0\0'
    body += struct.pack('<IIH', 2, 1, 6) + struct.pack('<IIH', 0, 2, 7)
    body += symbols + strings
    def member(name, payload):
        header = name.encode().ljust(16) + b'0'.ljust(12) + b'0'.ljust(6) * 2 + b'0'.ljust(8)
        header += str(len(payload)).encode().ljust(10) + b'`\n'
        return header + payload + (b'\n' if len(payload) & 1 else b'')
    first_size = 4 + 8 + len(names)
    second_size = 4 + 4 + 4 + 4 + len(names)
    offset = 8 + 60 + first_size + (first_size & 1) + 60 + second_size + (second_size & 1)
    first = struct.pack('>III', 2, offset, offset) + names
    # Second index symbol names are alphabetically sorted.
    second = struct.pack('<IIIHH', 1, offset, 2, 1, 1) + names
    return b'!<arch>\n' + member('/', first) + member('/', second) + member('TEST.dll/', body)


class ImportContractControls(unittest.TestCase):
    def test_reconstruction_changes_only_named_word_not_publics_or_relocations(self):
        source = fixture()
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'TEST.LIB'
            path.write_bytes(source)
            spec = contract.Import('TEST.dll', 0, 0, 'name', 'Test', 500, '_Test@0',
                                   'TEST.dll/', path.name, hashlib.sha256(source).hexdigest())
            rebuilt, proof = contract.reconstruct(path, [spec])
        self.assertEqual(len(rebuilt), len(source))
        self.assertEqual(proof['members'], 3)
        at = proof['imports'][0]['archive_field_offset']
        self.assertEqual(rebuilt[:at], source[:at])
        self.assertEqual(rebuilt[at + 2:], source[at + 2:])
        self.assertEqual(struct.unpack_from('<H', rebuilt, at)[0], 500)
        self.assertEqual(contract.archive_members(rebuilt)[0]['body'],
                         contract.archive_members(source)[0]['body'])

    def test_wrong_caller_and_changed_sdk_archive_fail_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'TEST.LIB'
            path.write_bytes(fixture())
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            for caller, sha in [('_Test@4', digest), ('_Test@0', '0' * 64)]:
                spec = contract.Import('TEST.dll', 0, 0, 'name', 'Test', 500, caller,
                                       'TEST.dll/', path.name, sha)
                with self.assertRaises(ValueError):
                    contract.reconstruct(path, [spec])

    def test_archive_index_cannot_point_at_undefined_or_other_member(self):
        source = bytearray(fixture())
        struct.pack_into('>I', source, 8 + 60 + 4, 8)
        with self.assertRaises(ValueError):
            contract.archive_members(bytes(source))

    def test_cached_archive_bytes_and_input_hashes_must_both_match(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / 'test.lib'
            output.write_bytes(fixture())
            key = {'contract': 'one', 'sdk': 'two', 'generator': 'three'}
            output.with_suffix('.json').write_text(json.dumps({
                'cache_key': key,
                'archive_sha256': hashlib.sha256(output.read_bytes()).hexdigest()}))
            self.assertTrue(implib._valid(output, key))
            self.assertFalse(implib._valid(output, {**key, 'sdk': 'different'}))
            output.write_bytes(output.read_bytes() + b'x')
            self.assertFalse(implib._valid(output, key))


if __name__ == '__main__':
    unittest.main()
