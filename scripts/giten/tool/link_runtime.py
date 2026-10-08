"""Provision the original native CRT for LINK in a separate Wine prefix."""

from __future__ import annotations

import hashlib
import os
from pathlib import Path, PureWindowsPath
import re
import shutil
import subprocess
import tomllib

from giten.core.paths import BUILD, REPO, RETAIL, dxsdk_dir
from giten.tool import ToolError
from giten.tool import wine


CONTRACT = RETAIL / "link_runtime.toml"
_ENV_KEY = (r"HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control"
            r"\Session Manager\Environment")
_APP_KEY = r"HKEY_CURRENT_USER\Software\Wine\AppDefaults\LINKNCRT.EXE\DllOverrides"
_LOAD = re.compile(r'^([0-9a-fA-F]+):trace:loaddll:\S+ Loaded L"(.*?)" '
                   r'at [0-9a-fA-F]+: (native|builtin)\s*$', re.MULTILINE)


def contract() -> dict:
    try:
        spec = tomllib.loads(CONTRACT.read_text())
    except (OSError, tomllib.TOMLDecodeError) as e:
        raise ToolError(f"cannot read LINK runtime contract: {CONTRACT}") from e
    return _validated(spec)


def _validated(spec: dict) -> dict:
    strings = ("artifact", "version", "source_file", "sector_format")
    integers = ("iso_lba", "file_size", "raw_range_start", "raw_range_end")
    for name in strings:
        if not isinstance(spec.get(name), str) or not spec[name]:
            raise ToolError(f"LINK runtime contract needs a nonempty string: {name}")
    for name in integers:
        if type(spec.get(name)) is not int or spec[name] < 0:
            raise ToolError(f"LINK runtime contract needs a nonnegative integer: {name}")
    if not spec["file_size"] or spec["raw_range_end"] < spec["raw_range_start"]:
        raise ToolError("LINK runtime contract has invalid file/range bounds")
    artifact = spec["artifact"]
    path = Path(artifact)
    windows = PureWindowsPath(artifact)
    if (path.is_absolute() or windows.drive or windows.root
            or ".." in path.parts or ".." in windows.parts):
        raise ToolError("LINK runtime artifact must be a relative path without '..'")
    deps = spec.get("dependencies")
    if not isinstance(deps, dict) or set(deps) != {
            "msdis100.dll", "mspdb50.dll", "msvcp50.dll"}:
        raise ToolError("LINK runtime contract needs exactly MSDIS100/MSPDB50/MSVCP50")
    for name, value in [("sha256", spec.get("sha256")),
                        ("linker_sha256", spec.get("linker_sha256")), *deps.items()]:
        if not isinstance(value, str) or not re.fullmatch(r"[0-9a-f]{64}", value):
            raise ToolError(f"LINK runtime contract has malformed SHA-256: {name}")
    return spec


def _verified(path: Path, digest: str) -> None:
    try:
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
    except OSError as e:
        raise ToolError(f"LINK runtime input missing: {path}; supply the original "
                        f"artifact recorded in {CONTRACT}") from e
    if actual != digest:
        raise ToolError(f"LINK runtime identity mismatch: {path}")


def _directory(path: Path) -> None:
    """Generated state must never follow a symlink out of its build owner."""
    if ".." in path.parts:
        raise ToolError(f"LINK runtime directory contains '..': {path}")
    if BUILD.is_symlink():
        raise ToolError(f"LINK runtime build directory is a symlink: {BUILD}")
    try:
        parts = path.relative_to(BUILD).parts
    except ValueError as e:
        raise ToolError(f"LINK runtime write outside build: {path}") from e
    parent = BUILD
    for part in parts:
        parent /= part
        if parent.is_symlink():
            raise ToolError(f"LINK runtime directory is a symlink: {parent}")
    path.mkdir(parents=True, exist_ok=True)


def _copy(source: Path, target: Path, digest: str) -> None:
    _verified(source, digest)
    _directory(target.parent)
    if target.is_symlink():
        raise ToolError(f"LINK runtime copy is a symlink: {target}")
    if target.exists():
        _verified(target, digest)
        return
    try:
        with source.open("rb") as src, target.open("xb") as dst:
            shutil.copyfileobj(src, dst)
    except FileExistsError as e:
        raise ToolError(f"LINK runtime copy appeared during setup: {target}") from e
    _verified(target, digest)


def _private_runtime(prefix: Path) -> Path:
    if prefix != BUILD / "link-runtime/prefix":
        raise ToolError(f"LINK runtime prefix is not its private owner: {prefix}")
    return prefix / "drive_c/windows/syswow64/msvcrt.dll"


def _install_runtime(source: Path, prefix: Path, digest: str) -> None:
    """Replace only Wine's generated 32-bit DLL slot in this private prefix."""
    _verified(source, digest)
    target = _private_runtime(prefix)
    _directory(target.parent)
    if target.is_symlink():
        target.unlink()
    elif target.exists():
        if target.is_file() and hashlib.sha256(target.read_bytes()).hexdigest() == digest:
            return
        target.unlink()
    _copy(source, target, digest)


def prepare() -> tuple[Path, dict[str, str]]:
    """Return the verified LINK clone and its dedicated-prefix environment.

    Prefix reuse requires serialized LINK calls. It is separate from the
    compiler/game prefix and does not replace its DLLs or registry values.
    Setup runs before the caller freezes the linker clock.
    """
    spec = contract()
    runtime = REPO / spec["artifact"]
    root = BUILD / "link-runtime"
    appdir, prefix = root / "bin", root / "prefix"
    source_bin = wine.toolchain_root() / "bin"
    sources = {"LINKNCRT.EXE": (wine.era_tool("link.exe"), spec["linker_sha256"])}
    for name, digest in spec["dependencies"].items():
        source = wine.find_ci(source_bin, name)
        if source is None:
            raise ToolError(f"LINK dependency missing: {source_bin / name}")
        sources[name] = (source, digest)
    _verified(runtime, spec["sha256"])
    for source, digest in sources.values():
        _verified(source, digest)
    for name, (source, digest) in sources.items():
        _copy(source, appdir / name, digest)
    stale_runtime = appdir / "msvcrt.dll"
    if stale_runtime.is_symlink():
        raise ToolError(f"LINK app-directory runtime is a symlink: {stale_runtime}")
    if stale_runtime.exists():
        _verified(stale_runtime, spec["sha256"])
        stale_runtime.unlink()
    _directory(prefix)

    env = dict(os.environ)
    env.update(WINEPREFIX=str(prefix.resolve()), TZ="UTC",
               WINEDLLOVERRIDES="mscoree,mshtml=",
               WINEDEBUG="+loaddll,fixme-all,err-kerberos")
    wine.boot_prefix(env=env)
    _install_runtime(runtime, prefix, spec["sha256"])
    msvc = wine.toolchain_root()
    try:
        dx = dxsdk_dir()
    except RuntimeError as e:
        raise ToolError(str(e)) from e
    values = {
        "PATH": ("REG_EXPAND_SZ", wine.winepath(msvc / "bin", env)
                 + r";%SystemRoot%\system32;%SystemRoot%"),
        "INCLUDE": ("REG_SZ", ";".join(wine.winepath(p, env)
                                      for p in (dx / "Include", msvc / "include"))),
        "LIB": ("REG_SZ", ";".join(wine.winepath(p, env)
                                  for p in (dx / "Lib", msvc / "lib"))),
    }
    for name, (kind, value) in values.items():
        _registry(env, _ENV_KEY, name, kind, value)
    _registry(env, _APP_KEY, "msvcrt", "REG_SZ", "native")
    _stop_private_server(env)
    return appdir / "LINKNCRT.EXE", env


def _stop_private_server(env: dict[str, str]) -> None:
    """Discard Wine's KnownDLL sections after replacing their backing file."""
    prefix = Path(env.get("WINEPREFIX", ""))
    _private_runtime(prefix)
    _directory(prefix)
    executable = wine.require("wineserver")
    for option in ("-k", "--wait"):
        try:
            result = subprocess.run([executable, option], env=dict(env),
                                    stdin=subprocess.DEVNULL,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL, timeout=30)
        except (OSError, subprocess.TimeoutExpired) as e:
            raise ToolError(f"cannot stop private LINK wineserver: {prefix}") from e
        if result.returncode:
            raise ToolError(f"private LINK wineserver {option} failed: {prefix}")


def _registry(env: dict[str, str], key: str, name: str,
              kind: str, value: str) -> None:
    output, rc = wine.run(["wine", "reg", "add", key, "/v", name,
                           "/t", kind, "/d", value, "/f"], env=env)
    if rc:
        raise ToolError(f"LINK prefix registry setup failed ({name}): {output.strip()}")


def verifynativeload(output: str, exe: Path, env: dict[str, str]) -> None:
    """Require the i386 LINK thread to load its verified private 32-bit DLL."""
    spec = contract()
    _verified(exe, spec["linker_sha256"])
    prefix = Path(env.get("WINEPREFIX", ""))
    runtime = _private_runtime(prefix)
    c_drive = prefix / "dosdevices/c:"
    if (not c_drive.is_symlink()
            or c_drive.resolve() != (prefix / "drive_c").resolve()
            or runtime.is_symlink()
            or any(parent.is_symlink() for parent in runtime.parents
                   if parent == BUILD or parent.is_relative_to(BUILD))):
        raise ToolError("LINK runtime is not mapped to its private Wine drive")
    _verified(runtime, spec["sha256"])
    def normalized(path: str) -> str:
        return path.replace("\\\\", "\\").casefold()
    loads = [(thread, normalized(path), mode)
             for thread, path, mode in _LOAD.findall(output)]
    exe_path = normalized(wine.winepath(exe, env))
    link_loads = [(thread, mode) for thread, path, mode in loads if path == exe_path]
    if len(link_loads) != 1 or link_loads[0][1] != "native":
        raise ToolError("missing or ambiguous LINK process load identity")
    thread = link_loads[0][0]
    crt_loads = [(path, mode) for owner, path, mode in loads
                 if owner == thread and path.endswith(r"\msvcrt.dll")]
    expected_paths = {r"c:\windows\syswow64\msvcrt.dll",
                      r"c:\windows\system32\msvcrt.dll"}
    if (len(crt_loads) != 1 or crt_loads[0][1] != "native"
            or crt_loads[0][0] not in expected_paths):
        raise ToolError("LINK did not load the verified private native MSVCRT")
