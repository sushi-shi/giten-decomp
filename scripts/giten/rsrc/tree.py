"""giten.rsrc.tree - a PE image's resource directory, read exactly.

Walks the three-level type/name/language tree of `.rsrc` and keeps what the
image records: every directory table's header fields, every leaf's
IMAGE_RESOURCE_DATA_ENTRY and payload, in directory order. Payload extraction
(giten.rsrc.payloads) and the candidate-vs-retail check (giten.rsrc.check)
both read through here. Malformed, cyclic or out-of-section trees are errors.
"""

from __future__ import annotations

import struct
from dataclasses import dataclass
from pathlib import Path

from giten.core.pe import Pe

RT_CURSOR = 1
RT_BITMAP = 2
RT_ICON = 3
RT_GROUP_CURSOR = 12
RT_GROUP_ICON = 14

Identity = int | str


@dataclass(frozen=True)
class Table:
    """One IMAGE_RESOURCE_DIRECTORY: its path from the root and header."""
    path: tuple[Identity, ...]
    offset: int
    characteristics: int
    timestamp: int
    version: tuple[int, int]
    entries: tuple[Identity, ...]


@dataclass(frozen=True)
class Leaf:
    """One IMAGE_RESOURCE_DATA_ENTRY and the payload it points at."""
    kind: Identity
    name: Identity
    language: int
    entry: int          # the data entry's offset from the directory root
    rva: int
    codepage: int
    reserved: int
    payload: bytes

    @property
    def key(self) -> tuple[Identity, Identity, int]:
        return self.kind, self.name, self.language


@dataclass(frozen=True)
class Directory:
    section_va: int
    root_rva: int
    root_size: int      # the resource data directory's declared size
    image: bytes        # .rsrc's stored bytes, up to its virtual size
    tables: tuple[Table, ...]
    leaves: tuple[Leaf, ...]


def _name(value: int, read_dir) -> Identity:
    if not value & 0x80000000:
        if value > 0xFFFF:
            raise ValueError(f"invalid resource ordinal 0x{value:x}")
        return value
    offset = value & 0x7FFFFFFF
    length = struct.unpack("<H", read_dir(offset, 2))[0]
    return read_dir(offset + 2, length * 2).decode("utf-16le")


def read(exe: Path | str | Pe) -> Directory:
    """Walk the resource tree; reject malformed or missing data."""
    pe = Pe(exe) if isinstance(exe, (str, Path)) else exe
    if len(pe.directories) <= 2 or not pe.directories[2][0]:
        raise ValueError(f"{pe.path}: no PE resource directory")
    root_rva, root_size = pe.directories[2]
    section = pe.section(".rsrc")
    if not (section["va"] <= root_rva < section["va"] + section["rsize"]):
        raise ValueError(f"{pe.path}: resource directory outside .rsrc")
    root_file = section["rptr"] + root_rva - section["va"]
    bound = min(root_size, section["rsize"] - (root_rva - section["va"]))

    def read_dir(offset: int, size: int) -> bytes:
        if offset < 0 or size < 0 or offset + size > bound:
            raise ValueError(f"{pe.path}: resource directory range outside .rsrc")
        data = pe.data[root_file + offset:root_file + offset + size]
        if len(data) != size:
            raise ValueError(f"{pe.path}: truncated resource directory")
        return data

    tables: list[Table] = []
    leaves: list[Leaf] = []
    visited: set[tuple[int, int]] = set()

    def walk(offset: int, path: tuple[Identity, ...]) -> None:
        depth = len(path)
        if depth >= 3 or (offset, depth) in visited:
            raise ValueError(f"{pe.path}: cyclic or over-deep resource directory")
        visited.add((offset, depth))
        characteristics, timestamp, major, minor, named, numbered = \
            struct.unpack("<IIHHHH", read_dir(offset, 16))
        count = named + numbered
        entries = read_dir(offset + 16, count * 8)
        children = []
        for index in range(count):
            raw_name, raw_target = struct.unpack_from("<II", entries, index * 8)
            identity = _name(raw_name, read_dir)
            if (index < named) != isinstance(identity, str):
                raise ValueError(f"{pe.path}: named/ordinal entry count mismatch")
            children.append((identity, raw_target))
        tables.append(Table(path, offset, characteristics, timestamp,
                            (major, minor), tuple(c[0] for c in children)))
        for identity, raw_target in children:
            target = raw_target & 0x7FFFFFFF
            if depth < 2:
                if not raw_target & 0x80000000:
                    raise ValueError(f"{pe.path}: short resource directory path")
                walk(target, (*path, identity))
                continue
            if raw_target & 0x80000000 or not isinstance(identity, int):
                raise ValueError(f"{pe.path}: invalid resource language leaf")
            payload_rva, size, codepage, reserved = struct.unpack(
                "<IIII", read_dir(target, 16))
            if not (section["va"] <= payload_rva
                    and payload_rva + size <= section["va"] + section["rsize"]):
                raise ValueError(f"{pe.path}: resource payload outside stored .rsrc")
            payload = pe.read(payload_rva, size)
            if payload is None or len(payload) != size:
                raise ValueError(f"{pe.path}: truncated resource payload at 0x{payload_rva:x}")
            leaves.append(Leaf(path[0], path[1], identity, target, payload_rva,
                               codepage, reserved, payload))

    walk(0, ())
    if not leaves:
        raise ValueError(f"{pe.path}: empty PE resource directory")
    if len({leaf.key for leaf in leaves}) != len(leaves):
        raise ValueError(f"{pe.path}: duplicate resource identity")
    stored = min(section["vsize"] or section["rsize"], section["rsize"])
    image = pe.data[section["rptr"]:section["rptr"] + stored]
    return Directory(section["va"], root_rva, root_size, image, tuple(tables),
                     tuple(leaves))


def label(kind: Identity, name: Identity, language: int | None = None) -> str:
    """`RT_BITMAP 101` / `WAVE 383` (+ `/1041`), for messages."""
    names = {RT_CURSOR: "RT_CURSOR", RT_BITMAP: "RT_BITMAP", RT_ICON: "RT_ICON",
             RT_GROUP_CURSOR: "RT_GROUP_CURSOR", RT_GROUP_ICON: "RT_GROUP_ICON"}
    text = f"{names.get(kind, kind)} {name}"
    return text if language is None else f"{text}/{language}"
