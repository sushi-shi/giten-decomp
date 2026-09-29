"""Build DDS.EXE with Microsoft Visual C++ 5.0 under Wine.

    python3 build.py [--toolchain DIR] [--exe DDS.EXE] [--fixes] [-j N]

Every unit in build.json compiles with its own flags; link.exe then links the
objects, in build.json's order, with the C runtime, the Win32 and DirectX 6
import libraries and the resources of your original DDS.EXE.
"""

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import shutil
import signal
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent


class BuildError(RuntimeError):
    pass


def find(directory: Path, name: str) -> Path:
    """A file by case-insensitive name: the toolchain mixes CL.EXE and cl.exe."""
    if directory.is_dir():
        for path in directory.iterdir():
            if path.name.lower() == name.lower():
                return path
    raise BuildError(f"{name} not found in {directory}")


class Wine:
    def __init__(self, prefix: Path):
        self.prefix = prefix
        self.env = {**os.environ, "WINEPREFIX": str(prefix),
                    "WINEDEBUG": os.environ.get("WINEDEBUG", "-all"),
                    "WINEDLLOVERRIDES": "mscoree,mshtml="}
        for program in ("wine", "wineboot", "wineserver"):
            if shutil.which(program) is None:
                raise BuildError(f"{program} not found on PATH; run inside `nix develop`")
        if not (prefix / "drive_c").is_dir():
            prefix.mkdir(parents=True, exist_ok=True)
            subprocess.run(["wineboot", "--init"], env=self.env, check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            subprocess.run(["wineserver", "--wait"], env=self.env, check=False)
        drive = prefix / "dosdevices" / "z:"
        if not drive.is_symlink() or drive.resolve() != Path("/"):
            raise BuildError(f"{prefix}: drive Z: must map to /")

    @staticmethod
    def path(path: Path) -> str:
        return "Z:" + str(path.resolve()).replace("/", "\\")

    def run(self, argv: list[str], *, cwd: Path, expect: Path, timeout: float = 600) -> str:
        """Run one tool; `expect` is the success signal, not Wine's exit code.

        Output goes to a file and the tool gets its own process group: a Wine
        helper that outlives the tool would otherwise hold a pipe open."""
        expect.unlink(missing_ok=True)
        with tempfile.TemporaryFile() as log:
            process = subprocess.Popen(["wine", *argv], cwd=cwd, env=self.env,
                                       stdin=subprocess.DEVNULL, stdout=log,
                                       stderr=subprocess.STDOUT, start_new_session=True)
            try:
                process.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
            log.seek(0)
            output = log.read().decode("utf-8", "replace")
        if not expect.exists():
            tail = "\n".join(output.strip().splitlines()[-20:])
            raise BuildError(f"{Path(argv[0]).name} did not produce {expect.name}:\n{tail}")
        return output


def resources(exe: Path) -> bytes:
    """The PE resource tree of `exe` as a Win32 .res file."""
    data = exe.read_bytes()
    try:
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        count = struct.unpack_from("<H", data, pe + 6)[0]
        optional = struct.unpack_from("<H", data, pe + 20)[0]
        root_rva, root_size = struct.unpack_from("<II", data, pe + 24 + 96 + 8 * 2)
        sections = []
        for index in range(count):
            base = pe + 24 + optional + 40 * index
            sections.append((data[base:base + 8].rstrip(b"\0"),
                             *struct.unpack_from("<IIII", data, base + 8)))
    except struct.error as error:
        raise BuildError(f"{exe}: not a PE image") from error
    section = next((s for s in sections if s[0] == b".rsrc"), None)
    if section is None or not root_rva:
        raise BuildError(f"{exe}: no resources")
    _name, _vsize, va, raw_size, raw = section
    if not va <= root_rva < va + raw_size:
        raise BuildError(f"{exe}: resource directory outside .rsrc")
    root = raw + root_rva - va
    bound = min(root_size, raw_size - (root_rva - va))

    def read(offset: int, size: int) -> bytes:
        if offset < 0 or offset + size > bound:
            raise BuildError(f"{exe}: malformed resource directory")
        return data[root + offset:root + offset + size]

    def name(value: int):
        if not value & 0x80000000:
            return value
        offset = value & 0x7FFFFFFF
        length = struct.unpack("<H", read(offset, 2))[0]
        return read(offset + 2, 2 * length).decode("utf-16le")

    found = []

    def walk(offset: int, path: tuple) -> None:
        named, numbered = struct.unpack_from("<HH", read(offset, 16), 12)
        for index in range(named + numbered):
            raw_name, target = struct.unpack("<II", read(offset + 16 + 8 * index, 8))
            identity = name(raw_name)
            if len(path) < 2:
                if not target & 0x80000000:
                    raise BuildError(f"{exe}: malformed resource directory")
                walk(target & 0x7FFFFFFF, (*path, identity))
                continue
            if target & 0x80000000 or not isinstance(identity, int):
                raise BuildError(f"{exe}: malformed resource directory")
            rva, size, codepage, _ = struct.unpack("<IIII", read(target, 16))
            if codepage or not (va <= rva and rva + size <= va + raw_size):
                raise BuildError(f"{exe}: unsupported resource entry")
            found.append((*path, identity, data[raw + rva - va:raw + rva - va + size]))

    walk(0, ())

    def field(value) -> bytes:
        if isinstance(value, int):
            return struct.pack("<HH", 0xFFFF, value)
        return value.encode("utf-16le") + b"\0\0"

    output = bytearray(struct.pack("<IIHHHHIHHII", 0, 32, 0xFFFF, 0, 0xFFFF, 0, 0, 0, 0, 0, 0))
    for kind, identity, language, payload in found:
        names = field(kind) + field(identity)
        names += bytes(-len(names) & 3)
        output += struct.pack("<II", len(payload), 8 + len(names) + 16) + names
        output += struct.pack("<IHHII", 0, 0x1030, language, 0, 0)
        output += payload + bytes(-len(payload) & 3)
    return bytes(output)


def main(argv=None) -> int:
    manifest = json.loads((ROOT / "build.json").read_text())
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--toolchain", type=Path, default=os.environ.get("GITEN_TOOLCHAIN"),
                        help="directory holding msvc/{bin,include,lib} and dx/{Include,Lib} "
                             "(default: $GITEN_TOOLCHAIN)")
    parser.add_argument("--exe", type=Path, default=os.environ.get("GITEN_RETAIL_EXE"),
                        help="your original DDS.EXE, whose resources are linked in "
                             "(default: $GITEN_RETAIL_EXE)")
    parser.add_argument("--out", type=Path,
                        help="output directory (default: build, or build/fixes with --fixes)")
    parser.add_argument("--prefix", type=Path, default=os.environ.get("WINEPREFIX"),
                        help="Wine prefix (default: $WINEPREFIX, else build/wineprefix)")
    if manifest.get("fixes"):
        parser.add_argument("--fixes", action="store_true",
                            help="build with the bug fixes (" + " ".join(manifest["fixes"]) + ")")
    parser.add_argument("--compile-only", action="store_true", help="stop before linking")
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 1)
    args = parser.parse_args(argv)
    try:
        if args.toolchain is None:
            raise BuildError("no toolchain: pass --toolchain or set GITEN_TOOLCHAIN")
        if args.exe is None and not args.compile_only:
            raise BuildError("no original DDS.EXE for the resources: pass --exe or set "
                             "GITEN_RETAIL_EXE (or use --compile-only)")
        msvc, dx = args.toolchain / "msvc", args.toolchain / "dx"
        fixes = getattr(args, "fixes", False)
        out = (args.out or ROOT / ("build/fixes" if fixes else "build")).resolve()
        objects = out / "obj"
        objects.mkdir(parents=True, exist_ok=True)
        wine = Wine((args.prefix or ROOT / "build/wineprefix").resolve())
        cl = find(msvc / "bin", "cl.exe")
        includes = [ROOT / name for name in manifest["include"]] + [dx / "Include", msvc / "include"]
        defines = manifest["fixes"] if fixes else []

        def compile_unit(unit: dict) -> Path:
            obj = objects / f"{unit['name']}.obj"
            wine.run([str(cl), *(f"/I{wine.path(d)}" for d in includes), *unit["flags"],
                      *defines, f"/Fo{wine.path(obj)}", wine.path(ROOT / unit["source"])],
                     cwd=objects, expect=obj)
            return obj

        units = manifest["units"]
        with concurrent.futures.ThreadPoolExecutor(max(1, args.jobs)) as pool:
            futures = {pool.submit(compile_unit, unit): unit for unit in units}
            failed = []
            for done, future in enumerate(concurrent.futures.as_completed(futures), 1):
                unit = futures[future]
                try:
                    future.result()
                    print(f"[{done}/{len(units)}] {unit['source']}", flush=True)
                except BuildError as error:
                    failed.append(f"{unit['source']}: {error}")
        if failed:
            raise BuildError("\n".join(failed))
        if args.compile_only:
            return 0

        res = out / "DDS.res"
        res.write_bytes(resources(args.exe))
        exe = out / "DDS.EXE"
        link = manifest["link"]
        response = out / "DDS.rsp"
        response.write_text("\n".join([
            f"/OUT:{wine.path(exe)}", f"/MAP:{wine.path(out / 'DDS.map')}", *link["flags"],
            f"/LIBPATH:{wine.path(dx / 'Lib')}", f"/LIBPATH:{wine.path(msvc / 'lib')}",
            *link["libraries"],
            *(f'"{wine.path(objects / (unit["name"] + ".obj"))}"' for unit in units),
            f'"{wine.path(res)}"']) + "\n")
        output = wine.run([str(find(msvc / "bin", "link.exe")), f"@{wine.path(response)}"],
                          cwd=out, expect=exe)
        duplicates = [line for line in output.splitlines() if "LNK4006" in line]
        if duplicates:
            exe.unlink()
            raise BuildError("duplicate symbols:\n" + "\n".join(duplicates))
        print(f"{exe} ({exe.stat().st_size:,} bytes)")
        return 0
    except (BuildError, OSError, subprocess.CalledProcessError) as error:
        print(f"build.py: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
