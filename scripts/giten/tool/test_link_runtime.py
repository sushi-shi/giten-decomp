"""Native LINK runtime identity, safe-copy and loader controls."""

import hashlib
import copy
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from giten.tool import ToolError
from giten.tool import link_runtime as runtime


def digest(data):
    return hashlib.sha256(data).hexdigest()


class LinkRuntimeControls(unittest.TestCase):
    def test_contract_rejects_missing_types_paths_and_hashes(self):
        valid = {
            "artifact": "build/local/runtime/msvcrt.dll", "version": "5.00.7303",
            "source_file": "MSVCRT.DLL",
            "sector_format": "MODE1/2352", "iso_lba": 412, "file_size": 277776,
            "raw_range_start": 969024, "raw_range_end": 1288895,
            "sha256": "a" * 64, "linker_sha256": "b" * 64,
            "dependencies": {name: "c" * 64 for name in
                             ("msdis100.dll", "mspdb50.dll", "msvcp50.dll")},
        }
        self.assertEqual(runtime._validated(valid), valid)
        invalid = []
        missing = copy.deepcopy(valid)
        del missing["sha256"]
        invalid.append(missing)
        for key, value in (("artifact", "/outside"), ("artifact", "../outside"),
                           ("artifact", "C:\\outside"), ("artifact", "a\\..\\outside"),
                           ("sha256", "bad"), ("version", 7303), ("iso_lba", True),
                           ("raw_range_end", 1), ("dependencies", [])):
            spec = copy.deepcopy(valid)
            spec[key] = value
            invalid.append(spec)
        wrong_name = copy.deepcopy(valid)
        wrong_name["dependencies"]["other.dll"] = wrong_name["dependencies"].pop("msvcp50.dll")
        invalid.append(wrong_name)
        for spec in invalid:
            with self.subTest(spec=spec), self.assertRaises(ToolError):
                runtime._validated(spec)

    def test_identity_and_safe_copy(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp) / "build"
            build.mkdir()
            source, dest = build / "source", build / "bin/runtime.dll"
            source.write_bytes(b"original")
            with patch.object(runtime, "BUILD", build):
                with self.assertRaises(ToolError):
                    runtime._copy(build / "missing", dest, digest(b"original"))
                with self.assertRaises(ToolError):
                    runtime._copy(source, dest, digest(b"different"))
                self.assertFalse(dest.exists())
                runtime._copy(source, dest, digest(b"original"))
                runtime._copy(source, dest, digest(b"original"))
                self.assertEqual(dest.read_bytes(), b"original")
                dest.write_bytes(b"changed")
                with self.assertRaises(ToolError):
                    runtime._copy(source, dest, digest(b"original"))
                self.assertEqual(dest.read_bytes(), b"changed")
                outside = Path(temp) / "outside"
                outside.write_bytes(b"preserve")
                dest.unlink()
                dest.symlink_to(outside)
                with self.assertRaises(ToolError):
                    runtime._copy(source, dest, digest(b"original"))
                self.assertEqual(outside.read_bytes(), b"preserve")
                with self.assertRaises(ToolError):
                    runtime._copy(source, outside, digest(b"original"))
                with self.assertRaises(ToolError):
                    runtime._directory(build / ".." / "outside-directory")

    def test_only_link_thread_and_verified_path_prove_runtime(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp)
            exe = build / "link-runtime/bin/LINKNCRT.EXE"
            prefix = build / "link-runtime/prefix"
            dll = prefix / "drive_c/windows/syswow64/msvcrt.dll"
            exe.parent.mkdir(parents=True)
            dll.parent.mkdir(parents=True)
            (prefix / "dosdevices").mkdir()
            (prefix / "dosdevices/c:").symlink_to("../drive_c")
            exe.write_bytes(b"link")
            dll.write_bytes(b"crt")
            spec = {"linker_sha256": digest(b"link"), "sha256": digest(b"crt")}
            def winepath(path, env):
                return "Z:\\private\\" + Path(path).name
            def load(thread, name, kind="native"):
                path = ("C:\\\\windows\\\\system32\\\\" + name
                        if name.lower() == "msvcrt.dll" else "Z:\\\\private\\\\" + name)
                return (f'{thread}:trace:loaddll:build_module Loaded L"'
                        f'{path}" at 78000000: {kind}\n')
            good = load("00ec", "LINKNCRT.EXE") + load("00ec", "msvcrt.dll")
            child = load("00f4", "MSVCRT.dll", "builtin")
            with patch.object(runtime, "BUILD", build), \
                    patch.object(runtime, "contract", return_value=spec), \
                    patch.object(runtime.wine, "winepath", side_effect=winepath):
                env = {"WINEPREFIX": str(prefix)}
                runtime.verifynativeload(good + child, exe, env)
                runtime.verifynativeload(good.replace("system32", "syswow64"), exe, env)
                for bad in (load("00ec", "LINKNCRT.EXE") + load("00f4", "msvcrt.dll"),
                            load("00ec", "LINKNCRT.EXE") + load("00ec", "msvcrt.dll", "builtin"),
                            good + load("00ec", "msvcrt.dll"),
                            good.replace("system32\\\\msvcrt.dll", "other\\\\msvcrt.dll"),
                            good + load("00f4", "LINKNCRT.EXE")):
                    with self.subTest(bad=bad), self.assertRaises(ToolError):
                        runtime.verifynativeload(bad, exe, env)
                dll.write_bytes(b"changed")
                with self.assertRaises(ToolError):
                    runtime.verifynativeload(good, exe, env)

    def test_private_builtin_slot_replacement_never_follows_symlinks(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp) / "build"
            build.mkdir()
            prefix = build / "link-runtime/prefix"
            source = build / "original.dll"
            source.write_bytes(b"native")
            outside = Path(temp) / "outside.dll"
            outside.write_bytes(b"preserve")
            target = prefix / "drive_c/windows/syswow64/msvcrt.dll"
            target.parent.mkdir(parents=True)
            target.symlink_to(outside)
            with patch.object(runtime, "BUILD", build):
                runtime._install_runtime(source, prefix, digest(b"native"))
                self.assertFalse(target.is_symlink())
                self.assertEqual(target.read_bytes(), b"native")
                self.assertEqual(outside.read_bytes(), b"preserve")
                target.write_bytes(b"builtin")
                runtime._install_runtime(source, prefix, digest(b"native"))
                self.assertEqual(target.read_bytes(), b"native")
                runtime._install_runtime(source, prefix, digest(b"native"))
                with self.assertRaises(ToolError):
                    runtime._install_runtime(source, Path(temp), digest(b"native"))

    def test_registry_setup_propagates_private_environment_and_failure(self):
        env = {"WINEPREFIX": "/private/prefix", "TZ": "UTC"}
        with patch.object(runtime.wine, "run", return_value=("", 0)) as run:
            runtime._registry(env, runtime._APP_KEY, "msvcrt", "REG_SZ", "native")
            self.assertEqual(run.call_args.kwargs["env"], env)
            self.assertIn(runtime._APP_KEY, run.call_args.args[0])
        with patch.object(runtime.wine, "run", return_value=("denied", 1)):
            with self.assertRaises(ToolError):
                runtime._registry(env, runtime._APP_KEY, "msvcrt", "REG_SZ", "native")

    def test_server_shutdown_is_private_and_waits_before_link(self):
        with tempfile.TemporaryDirectory() as temp:
            build = Path(temp) / "build"
            env = {"WINEPREFIX": str(build / "link-runtime/prefix"), "TZ": "UTC"}
            with patch.object(runtime, "BUILD", build), \
                    patch.object(runtime.wine, "require", return_value="/tool/wineserver"), \
                    patch.object(runtime.subprocess, "run") as run:
                run.return_value.returncode = 0
                runtime._stop_private_server(env)
                self.assertEqual([c.args[0] for c in run.call_args_list],
                                 [["/tool/wineserver", "-k"],
                                  ["/tool/wineserver", "--wait"]])
                self.assertTrue(all(c.kwargs["env"] == env for c in run.call_args_list))
                run.reset_mock()
                with self.assertRaises(ToolError):
                    runtime._stop_private_server({"WINEPREFIX": str(build / "shared")})
                run.assert_not_called()
                run.return_value.returncode = 1
                with self.assertRaises(ToolError):
                    runtime._stop_private_server(env)
                self.assertEqual(run.call_count, 1)

    def test_runner_and_server_use_the_same_private_environment(self):
        env = {"WINEPREFIX": "/private/prefix", "TZ": "UTC"}
        with patch.object(runtime.wine, "ensure_wineserver") as server, \
                patch.object(runtime.wine.subprocess, "Popen") as popen:
            popen.return_value.returncode = 0
            self.assertEqual(runtime.wine.run(["wine", "tool.exe"], env=env), ("", 0))
            passed = popen.call_args.kwargs["env"]
            self.assertEqual(passed["WINEPREFIX"], env["WINEPREFIX"])
            self.assertEqual(server.call_args.args[0], passed)
            self.assertNotIn("WINEDEBUG", env)
        with patch.object(runtime.wine.shutil, "which", return_value="/tool/wineserver"), \
                patch.object(runtime.wine.subprocess, "run") as run:
            runtime.wine.ensure_wineserver(env)
            self.assertEqual(run.call_args.kwargs["env"], env)


if __name__ == "__main__":
    unittest.main()
