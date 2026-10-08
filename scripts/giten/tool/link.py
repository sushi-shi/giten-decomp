"""giten.tool.link - the era linker (genuine VC5 link.exe 5.10.7303).

    giten tool link --expect <exe> -- /NOLOGO /OUT:<exe.w> <objs.w...>

In-process (same function):
    from giten.tool import link
    link.link(["/NOLOGO", "/OUT:" + wine.winepath(exe), *obj_args], expect=[exe])

Callers build the full argument list themselves (link lines are the caller's
policy - candidate link order, /DLL /NOENTRY import-lib synthesis, ...); this
module only guarantees the tool LOADS (MSDIS100.DLL provisioned) and that the
expected artifacts exist afterwards. Libraries resolve via the wine registry
LIB (init_prefix).
"""

from __future__ import annotations

from pathlib import Path
from datetime import datetime, timezone
import re
import shutil
import struct

from giten.tool import ToolError
from giten.tool.wine import ensure_link_deps, era_tool, run


def clock_stamp(at: str) -> int:
    """Validate a frozen UTC wall clock and return its COFF timestamp."""
    if not re.fullmatch(r"\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}", at):
        raise ToolError("link time must be UTC YYYY-MM-DD HH:MM:SS")
    try:
        stamp = int(datetime.strptime(at, "%Y-%m-%d %H:%M:%S")
                    .replace(tzinfo=timezone.utc).timestamp())
    except ValueError as e:
        raise ToolError(f"invalid UTC link time: {at}") from e
    if not 0 <= stamp <= 0xffffffff:
        raise ToolError("link time is outside the COFF timestamp range")
    return stamp


def pe_stamp(path: Path) -> int:
    """Read the original COFF clock without changing the image."""
    data = Path(path).read_bytes()
    try:
        pe = struct.unpack_from("<I", data, 0x3c)[0]
        if data[:2] != b"MZ" or data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE image")
        return struct.unpack_from("<I", data, pe + 8)[0]
    except (struct.error, ValueError) as e:
        raise ToolError(f"cannot read COFF timestamp: {path}") from e


def clock_args(argv: list[str], at: str | None) -> list[str]:
    if at is None:
        return argv
    clock_stamp(at)
    faketime = shutil.which("faketime")
    if faketime is None:
        raise ToolError("faketime not found on PATH - run inside `nix develop`")
    return ["env", "TZ=UTC", "FAKETIME_DONT_FAKE_MONOTONIC=1",
            faketime, "-f", at, *argv]


def check_clock(paths: list[Path], at: str) -> None:
    expected = clock_stamp(at)
    for path in paths:
        if path.suffix.lower() in (".exe", ".dll"):
            actual = pe_stamp(path)
        elif path.suffix.lower() == ".map":
            matches = re.findall(r"^\s*Timestamp is ([0-9a-fA-F]+)\b",
                                 path.read_text(), re.MULTILINE)
            if len(matches) != 1:
                raise ToolError(f"missing or ambiguous map timestamp: {path}")
            actual = int(matches[0], 16)
        else:
            continue
        if actual != expected:
            raise ToolError(f"{path.name}: link timestamp {actual:08x}, "
                            f"expected {expected:08x} ({at} UTC)")


def link(args: list[str], *, cwd: Path | None = None,
         expect: list[Path] = (), timeout: float | None = None,
         at: str | None = None, native_runtime: bool = False) -> str:
    """Run link.exe with `args`; verify every `expect` path exists after."""
    ensure_link_deps()
    executable, environment = era_tool("link.exe"), None
    if native_runtime:
        from giten.tool import link_runtime
        executable, environment = link_runtime.prepare()
    argv = clock_args(["wine", str(executable), *args], at)
    expect = [Path(p) for p in expect]
    for p in expect:
        p.unlink(missing_ok=True)
    output, rc = run(argv, cwd=cwd, timeout=timeout,
                     success=expect[0] if expect else None, env=environment)
    missing = [p for p in expect if not p.exists()]
    if missing or (not expect and rc != 0):
        tail = "\n".join(output.strip().splitlines()[-12:])
        what = missing[0].name if missing else f"rc={rc}"
        raise ToolError(f"link failed ({what}):\n{tail}")
    if native_runtime:
        link_runtime.verifynativeload(output, executable, environment)
    if at is not None:
        check_clock(expect, at)
    return output


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--expect", action="append", default=[],
                    help="artifact that must exist afterwards (repeatable)")
    ap.add_argument("--at", help="freeze wall clock at UTC YYYY-MM-DD HH:MM:SS")
    ap.add_argument("--native-runtime", action="store_true",
                    help="use the pinned retail-era native MSVCRT in an isolated prefix")
    ap.add_argument("args", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    args = a.args[1:] if a.args and a.args[0] == "--" else a.args
    try:
        out = link(args, expect=[Path(p) for p in a.expect], at=a.at,
                   native_runtime=a.native_runtime)
        if out.strip():
            print(out)
    except (ToolError, OSError) as e:
        print(f"[link] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
