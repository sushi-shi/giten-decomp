"""Build a generated tree and check it against the matching tree.

The export only removes comments and scaffolding and decides conditionals, so
for the same decision (retail, or the play build's GITEN_BUGFIX) every unit
must preprocess to the matching source's token sequence. That is the check.

The objects are compared as well, and reported. MSVC 5.0's register
allocation and temporary numbering depend on the headers and macros a unit
reads, not only on its tokens (include/Ints.h), so removing include/rva.h and
include/Enums.h can move registers in an unchanged program. Objects are
compared without the `.file` record, which holds the source path, and with
compiler-private names (`$T644`, `$SG1234`, `name$S12`) renumbered in order of
appearance, since no linker resolves them by name.
"""

from __future__ import annotations

import os
import re
import struct
import subprocess
import sys
from pathlib import Path

from giten import graph


def _string(data: bytes, table: int, raw: bytes) -> str:
    if raw[:4] == b"\0\0\0\0":
        offset = table + struct.unpack_from("<I", raw, 4)[0]
        return data[offset:data.index(b"\0", offset)].decode("latin-1")
    return raw.rstrip(b"\0").decode("latin-1")


_PRIVATE = re.compile(r"(\$[A-Za-z]*)(\d+)")


def canonical(data: bytes) -> tuple:
    """An i386 COFF object without its TimeDateStamp and `.file` record."""
    machine, count, _stamp, pointer, symbols, optional, flags = struct.unpack_from(
        "<HHIIIHH", data, 0)
    table = pointer + 18 * symbols
    names: list[tuple] = []
    index_names: dict[int, int] = {}
    index = 0
    while index < symbols:
        offset = pointer + 18 * index
        value, section, kind, storage, aux = struct.unpack_from("<IhHBB", data, offset + 8)
        name = _string(data, table, data[offset:offset + 8])
        if storage != 103:                                  # IMAGE_SYM_CLASS_FILE
            index_names[index] = len(names)
            names.append((name, value, section, kind, storage,
                          data[offset + 18:offset + 18 * (1 + aux)]))
        index += 1 + aux
    ordinals: dict[str, str] = {}

    def renumber(match: re.Match) -> str:
        return ordinals.setdefault(match.group(0), f"{match.group(1)}#{len(ordinals)}")

    names = [(_PRIVATE.sub(renumber, name), *rest) for name, *rest in names]
    sections = []
    for number in range(count):
        base = 20 + optional + 40 * number
        raw = data[base:base + 8]
        name = (_string(data, table, b"\0\0\0\0" + struct.pack("<I", int(raw[1:].rstrip(b"\0"))))
                if raw[:1] == b"/" else raw.rstrip(b"\0").decode("latin-1"))
        size, where, relocations, _lines, nrel, _nlines, characteristics = struct.unpack_from(
            "<IIIIHHI", data, base + 16)
        body = data[where:where + size] if where else b""
        fixups = []
        for entry in range(nrel):
            address, symbol, kind = struct.unpack_from("<IIH", data, relocations + 10 * entry)
            fixups.append((address, index_names[symbol], kind))
        sections.append((name, size, characteristics, body, tuple(fixups)))
    return machine, flags, tuple(sections), tuple(names)


def preprocess(root: Path, units: list[dict], defines: list[str], out: Path) -> dict[str, list]:
    """{unit: its non-blank preprocessed tokens} under cl /EP with its flags."""
    from concurrent.futures import ThreadPoolExecutor
    from giten.branch.lexer import tokens
    from giten.core.paths import dxsdk_dir, msvc_dir
    from giten.tool.wine import era_tool, run, winepath

    cl = era_tool("cl.exe")
    includes = [root / "include", dxsdk_dir() / "Include", msvc_dir() / "include"]

    def one(unit: dict) -> tuple[str, list]:
        directory = out / unit["name"]
        directory.mkdir(parents=True, exist_ok=True)
        result = directory / (Path(unit["source"]).stem + ".i")
        run(["wine", str(cl), *(f"/I{winepath(d)}" for d in includes), *unit["flags"],
             *defines, "/EP", "/P", winepath(root / unit["source"])],
            cwd=directory, success=result)
        if not result.exists():
            raise ValueError(f"{unit['source']}: cl /EP produced nothing")
        text = result.read_text(encoding="latin-1")
        return unit["name"], [spelling for _kind, spelling in tokens(text)
                              if not spelling.isspace()]

    with ThreadPoolExecutor(os.cpu_count() or 1) as pool:
        return dict(pool.map(one, units))


def reference_objects(repo: Path, fixes: bool) -> dict[str, Path]:
    """Build and return the matching tree's objects for each unit."""
    from giten.graph.verbs import configure_if_needed, ninja
    configure_if_needed()
    manifest = (repo / graph.NINJA).read_text()
    units = sorted(line.split()[1].removeprefix(f"{graph.BASE_DIR}/").removesuffix(".obj:")
                   for line in manifest.splitlines()
                   if line.startswith(f"build {graph.BASE_DIR}/") and ".obj:" in line)
    objects = {unit: repo / graph.BASE_DIR / f"{unit}.obj" for unit in units}
    if fixes:
        for unit in units:
            if f"build {graph.PLAY_OBJ_DIR}/{unit}.obj:" in manifest:
                objects[unit] = repo / graph.PLAY_OBJ_DIR / f"{unit}.obj"
    targets = [str(path.relative_to(repo)) for path in objects.values()]
    if ninja(targets) != 0:
        raise ValueError("the matching build failed")
    return objects


def compare(built: Path, reference: dict[str, Path]) -> list[str]:
    names = {path.stem for path in built.glob("*.obj")}
    if names != set(reference):
        return [f"unit sets differ: {sorted(names ^ set(reference))}"]
    return [unit for unit in sorted(reference)
            if canonical((built / f"{unit}.obj").read_bytes())
            != canonical(reference[unit].read_bytes())]


def verify(output: Path, repo: Path, commit: str, fixes: bool) -> None:
    """Build `output` (retail, and with `fixes` also fixed) and compare objects."""
    changed = subprocess.run(
        ["git", "-C", str(repo), "status", "--porcelain", "--untracked-files=all", "--",
         "src", "include", "config/units.toml"], capture_output=True, text=True, check=True)
    if changed.stdout or subprocess.run(
            ["git", "-C", str(repo), "diff", "--quiet", commit, "--",
             "src", "include", "config/units.toml"]).returncode:
        raise ValueError(f"the work tree's sources differ from {commit[:12]}; "
                         "verify compares with the matching build of that commit")
    import json
    from giten.graph import PLAY_DEFINES

    units = json.loads((output / "build.json").read_text())["units"]
    exe = os.environ.get("GITEN_RETAIL_EXE")
    for fixed in [False, True] if fixes else [False]:
        label = "fixed" if fixed else "retail"
        command = [sys.executable, "build.py", *(["--fixes"] if fixed else [])]
        command += ["--exe", exe] if exe and Path(exe).is_file() else ["--compile-only"]
        print(f"[branch] {label}: building {output}", flush=True)
        subprocess.run(command, cwd=output, check=True)
        built = output / ("build/fixes" if fixed else "build")
        defines = PLAY_DEFINES if fixed else []
        scratch = repo / "build/branch/preprocessed" / label
        theirs = preprocess(output, units, defines, scratch / "export")
        ours = preprocess(repo, units, defines, scratch / "main")
        differing = [unit for unit in sorted(ours) if ours[unit] != theirs[unit]]
        if differing:
            raise ValueError(f"{label}: {len(differing)} unit(s) preprocess differently from "
                             f"the matching source: {', '.join(differing)}")
        print(f"[branch] {label}: all {len(ours)} units preprocess to the matching "
              "source's tokens", flush=True)
        reference = reference_objects(repo, fixed)
        moved = compare(built / "obj", reference)
        print(f"[branch] {label}: {len(reference) - len(moved)}/{len(reference)} objects equal "
              f"the matching build's" + (f"; differing: {', '.join(moved)}"
                                         if moved else ""), flush=True)
