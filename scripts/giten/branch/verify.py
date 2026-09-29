"""Build a generated tree and require the matching build's objects and image.

The export only removes comments and scaffolding and decides conditionals, so
for the same decision (retail, or the play build's GITEN_BUGFIX) every object
it compiles must equal the matching build's: build/objdiff/base, or
build/play/obj for a unit the play build recompiles. Objects are compared
without their TimeDateStamp and `.file` record, which holds the source path.
With the original DDS.EXE supplied, the linked image must also equal the
matching tree's candidate (retail) or play (fixed) image apart from the PE
link timestamp.
"""

from __future__ import annotations

import os
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


def canonical(data: bytes) -> tuple:
    """An i386 COFF object without its TimeDateStamp and `.file` record.

    Symbol references (relocations, weak-external tags) are renumbered past
    the `.file` record, whose length follows the source path."""
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
    for position, (name, value, section, kind, storage, aux) in enumerate(names):
        if storage == 105 and aux:                          # IMAGE_SYM_CLASS_WEAK_EXTERNAL
            tag = index_names[struct.unpack_from("<I", aux)[0]]
            names[position] = (name, value, section, kind, storage,
                               struct.pack("<I", tag) + aux[4:])
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


def image(data: bytes) -> bytes:
    """A PE image without its link timestamp."""
    buffer = bytearray(data)
    pe = struct.unpack_from("<I", buffer, 0x3C)[0]
    struct.pack_into("<I", buffer, pe + 8, 0)
    return bytes(buffer)


def verify(output: Path, repo: Path, commit: str, fixes: bool) -> None:
    """Build `output` (retail, and with `fixes` also fixed) and compare it."""
    changed = subprocess.run(
        ["git", "-C", str(repo), "status", "--porcelain", "--untracked-files=all", "--",
         "src", "include", "config/units.toml"], capture_output=True, text=True, check=True)
    if changed.stdout or subprocess.run(
            ["git", "-C", str(repo), "diff", "--quiet", commit, "--",
             "src", "include", "config/units.toml"]).returncode:
        raise ValueError(f"the work tree's sources differ from {commit[:12]}; "
                         "verify compares with the matching build of that commit")
    exe = os.environ.get("GITEN_RETAIL_EXE")
    link = bool(exe and Path(exe).is_file())
    for fixed in [False, True] if fixes else [False]:
        label = "fixed" if fixed else "retail"
        command = [sys.executable, "build.py", *(["--fixes"] if fixed else [])]
        command += ["--exe", exe] if link else ["--compile-only"]
        print(f"[branch] {label}: building {output}", flush=True)
        subprocess.run(command, cwd=output, check=True)
        built = output / ("build/fixes" if fixed else "build")
        reference = reference_objects(repo, fixed)
        differing = compare(built / "obj", reference)
        if differing:
            raise ValueError(f"{label}: {len(differing)} object(s) differ from the matching "
                             f"build: {', '.join(differing)}")
        print(f"[branch] {label}: all {len(reference)} objects equal the matching build's",
              flush=True)
        if not link:
            print(f"[branch] {label}: no GITEN_RETAIL_EXE, so no image was linked or compared",
                  flush=True)
            continue
        from giten.graph.verbs import ninja
        target = graph.PLAY_EXE if fixed else graph.CANDIDATE_EXE
        if ninja([target]) != 0:
            raise ValueError(f"the matching build failed to link {target}")
        if image((built / "DDS.EXE").read_bytes()) != image((repo / target).read_bytes()):
            raise ValueError(f"{label}: DDS.EXE differs from {target}")
        print(f"[branch] {label}: DDS.EXE equals {target} apart from the link timestamp",
              flush=True)
