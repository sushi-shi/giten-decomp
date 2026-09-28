"""Protocol controls: incomplete runs and mismatches must never report parity."""
import io
import json
from pathlib import Path
import struct
import tempfile
import unittest
from contextlib import redirect_stdout
from giten.codecs.report import compare, result
from giten.codecs.corpus import decrypt


class ProtocolTests(unittest.TestCase):
    def test_truncated_result_is_an_error(self):
        with tempfile.NamedTemporaryFile() as file:
            file.write(struct.pack('<II', 1, 8) + b'abc')
            file.flush()
            with open(file.name, 'rb') as source:
                with self.assertRaisesRegex(ValueError, 'truncated'):
                    result(source)

    def test_zero_coverage_cannot_pass(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name)
            (out / 'cases.json').write_text('[]')
            (out / 'inventory.json').write_text('[]')
            for side in ('retail', 'rust'):
                (out / (side + '.bin')).write_bytes(struct.pack('<II', 0x53455247, 0))
            with redirect_stdout(io.StringIO()):
                self.assertTrue(compare(out))

    def test_disagreement_retains_resource_identity(self):
        with tempfile.TemporaryDirectory() as name:
            out = Path(name)
            (out / 'cases.json').write_text(json.dumps([{'kind': 1, 'name': 'original/record', 'arg': 0}]))
            (out / 'inventory.json').write_text('[]')
            header = struct.pack('<II', 0x53455247, 1)
            (out / 'retail.bin').write_bytes(header + struct.pack('<II', 0, 1) + b'a' + struct.pack('<II', 1, 1) + b'c')
            (out / 'rust.bin').write_bytes(header + struct.pack('<II', 1, 1) + b'b')
            with redirect_stdout(io.StringIO()):
                self.assertTrue(compare(out))
            diff = json.loads((out / 'report.json').read_text())['differences'][0]
            self.assertEqual(diff['name'], 'original/record')
            self.assertFalse(diff['candidate_equal'])
            self.assertFalse(diff['rust_equal'])

    def test_cipher_feedback_uses_ciphertext(self):
        self.assertEqual(decrypt(bytes([3, 1, 2])), bytes([0, 2, 3]))


if __name__ == '__main__':
    unittest.main()
