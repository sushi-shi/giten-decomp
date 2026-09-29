"""Path acceleration keeps Wine's drive selection and fallback boundaries."""
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from giten.tool import wine


class WinePathTests(unittest.TestCase):
    def test_default_z_mapping_and_prefix_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            prefix = Path(directory)
            devices = prefix / 'dosdevices'
            devices.mkdir()
            c_drive = prefix / 'drive_c'
            c_drive.mkdir()
            (devices / 'c:').symlink_to(c_drive)
            (devices / 'z:').symlink_to('/')
            with patch.dict(os.environ, WINEPREFIX=str(prefix)):
                self.assertEqual(wine._default_drive_path('/tmp/a b/../new.obj'),
                                 'Z:\\tmp\\new.obj')
                self.assertIsNone(wine._default_drive_path(c_drive / 'inside.obj'))
                alias = prefix / 'alias'
                alias.symlink_to(c_drive)
                self.assertIsNone(wine._default_drive_path(alias / 'inside.obj'))
                for unusual in ('relative.obj', '//server/share', '/tmp/a\\b'):
                    self.assertIsNone(wine._default_drive_path(unusual))
                (devices / 'd:').symlink_to('/tmp')
                self.assertIsNone(wine._default_drive_path('/tmp/new.obj'))
                (devices / 'd:').unlink()
                (devices / 'z:').unlink()
                (devices / 'z:').symlink_to('/tmp')
                self.assertIsNone(wine._default_drive_path('/tmp/new.obj'))

    def test_uncertain_mapping_calls_winepath(self):
        with patch.object(wine, '_default_drive_path', return_value=None), \
                patch.object(wine, 'require', return_value='/bin/winepath'), \
                patch.object(wine.subprocess, 'check_output', return_value='C:\\x\n') as run:
            self.assertEqual(wine.winepath('relative'), 'C:\\x')
            self.assertEqual(run.call_args.args[0], ['/bin/winepath', '-w', 'relative'])


if __name__ == '__main__':
    unittest.main()
