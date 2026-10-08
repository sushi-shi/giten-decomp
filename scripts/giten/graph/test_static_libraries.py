"""Static-library selection must fail closed before a native link."""
from dataclasses import asdict, replace
import csv
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from giten.core.pe import Pe
from giten.graph import static_libraries as libraries


def fixture():
    payload = bytes(range(32))
    names = b'_GUID_First\0_GUID_Second\0'
    head = struct.pack('<HHIIIHH', 0x14c, 1, 0, 92, 2, 0, 0)
    section = struct.pack('<8sIIIIIIHHI', b'.rdata\0\0', 0, 0, 32, 60, 0, 0, 0, 0, 0x40400040)
    syms = struct.pack('<IIIhHBB', 0, 4, 0, 1, 0, 2, 0)
    syms += struct.pack('<IIIhHBB', 0, 16, 16, 1, 0, 2, 0)
    body = head + section + payload + syms + struct.pack('<I', 4 + len(names)) + names
    def member(name, data):
        h = name.encode().ljust(16) + b'0'.ljust(12) + b'0'.ljust(6) * 2 + b'0'.ljust(8)
        return h + str(len(data)).encode().ljust(10) + b'`\n' + data + (b'\n' if len(data) & 1 else b'')
    first_size = 12 + len(names)
    second_size = 16 + len(names)
    offset = 8 + 60 + first_size + (first_size & 1) + 60 + second_size + (second_size & 1)
    first = struct.pack('>III', 2, offset, offset) + names
    second = struct.pack('<IIIHH', 1, offset, 2, 1, 1) + names
    archive = b'!<arch>\n' + member('/', first) + member('/', second) + member('guid.obj/', body)
    # Minimal stored PE section; comparison must read real bytes rather than zero-fill.
    pe = bytearray(0x400)
    pe[:2] = b'MZ'; struct.pack_into('<I', pe, 0x3c, 0x80)
    pe[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HH', pe, 0x84, 0x14c, 1)
    struct.pack_into('<H', pe, 0x94, 0xe0)
    struct.pack_into('<H', pe, 0x98, 0x10b)
    struct.pack_into('<I', pe, 0xb4, 0x400000)
    at = 0x178
    pe[at:at + 8] = b'.rdata\0\0'
    struct.pack_into('<IIII', pe, at + 8, 32, 0x1000, 0x200, 0x200)
    pe[0x200:0x220] = payload
    return archive, body, bytes(pe), payload


def contract(path, archive, body, payload):
    digest = lambda b: hashlib.sha256(b).hexdigest()
    return libraries.Library('GUID.LIB', str(path), len(archive), digest(archive), 'guid.obj/', digest(body),
                             '.rdata', 8, 2, 0x1000, len(payload), digest(payload), 'original SDK fixture',
                             'https://example.test/sdk.exe', 1, '0' * 64, '0' * 40, '0' * 32)


class StaticLibraryControls(unittest.TestCase):
    def test_original_member_payload_and_in_place_filename_selection(self):
        archive, body, pe, payload = fixture()
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory); lib = p / 'original.lib'; lib.write_bytes(archive)
            retail = p / 'retail.exe'; retail.write_bytes(pe)
            spec = contract(lib, archive, body, payload)
            manifest = p / 'libraries.tsv'
            with manifest.open('w') as f:
                writer = csv.DictWriter(f, fieldnames=asdict(spec), delimiter='\t')
                writer.writeheader(); writer.writerow(asdict(spec))
            self.assertEqual(libraries.on_disk(manifest, retail=retail), {'guid.lib': lib.resolve()})
            self.assertEqual(lib.read_bytes(), archive)

    def test_missing_or_changed_archive_never_falls_back(self):
        archive, body, pe, payload = fixture()
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory); lib = p / 'guid.lib'; retail = p / 'retail.exe'; retail.write_bytes(pe)
            spec = contract(lib, archive, body, payload)
            with self.assertRaisesRegex(ValueError, 'missing authentic'):
                libraries.validate(spec, lib, Pe(retail))
            lib.write_bytes(archive + b'x')
            with self.assertRaisesRegex(ValueError, 'size/hash mismatch'):
                libraries.validate(spec, lib, Pe(retail))

    def test_corrupt_archive_index_is_rejected_even_with_updated_archive_digest(self):
        archive, body, pe, payload = fixture()
        bad = bytearray(archive); struct.pack_into('>I', bad, 8 + 60 + 4, 8)
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory); lib = p / 'guid.lib'; lib.write_bytes(bad)
            retail = p / 'retail.exe'; retail.write_bytes(pe)
            spec = contract(lib, bytes(bad), body, payload)
            with self.assertRaisesRegex(ValueError, 'archive index'):
                libraries.validate(spec, lib, Pe(retail))

    def test_member_payload_hash_and_original_bytes_are_independent_checks(self):
        archive, body, pe, payload = fixture()
        with tempfile.TemporaryDirectory() as directory:
            p = Path(directory); lib = p / 'guid.lib'; lib.write_bytes(archive)
            retail = p / 'retail.exe'; retail.write_bytes(pe)
            spec = contract(lib, archive, body, payload)
            for field in ['member_sha256', 'payload_sha256']:
                with self.subTest(field=field), self.assertRaisesRegex(ValueError, 'hash mismatch'):
                    libraries.validate(replace(spec, **{field: '0' * 64}), lib, Pe(retail))
            wrong = bytearray(pe); wrong[0x200] ^= 1; retail.write_bytes(wrong)
            with self.assertRaisesRegex(ValueError, 'differs from the original'):
                libraries.validate(spec, lib, Pe(retail))


if __name__ == '__main__':
    unittest.main()
