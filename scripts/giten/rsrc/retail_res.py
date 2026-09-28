"""Build an ignored Windows .res file from the supplied retail EXE's .rsrc.

Only the resource payloads and their type/name/language identities are copied.
The VC5 linker builds the candidate's resource directory at its own RVA.
No resource content is checked into the source tree.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path

from giten.core.pe import Pe


def _name(value: int, read) -> int | str:
    if not value & 0x80000000:
        if value > 0xFFFF:
            raise ValueError(f"invalid resource ordinal 0x{value:x}")
        return value
    offset = value & 0x7FFFFFFF
    length = struct.unpack("<H", read(offset, 2))[0]
    return read(offset + 2, length * 2).decode("utf-16le")


def resources(exe: Path | str) -> list[tuple[int | str, int | str, int, bytes]]:
    """Walk the three-level PE resource tree; reject malformed or missing data."""
    pe = Pe(exe)
    if len(pe.directories) <= 2 or not pe.directories[2][0]:
        raise ValueError(f"{exe}: no PE resource directory")
    root_rva, root_size = pe.directories[2]
    section = pe.section(".rsrc")
    if not (section["va"] <= root_rva < section["va"] + section["rsize"]):
        raise ValueError(f"{exe}: resource directory outside .rsrc")
    root_file = section["rptr"] + root_rva - section["va"]
    bound = min(root_size, section["rsize"] - (root_rva - section["va"]))

    def read(offset: int, size: int) -> bytes:
        if offset < 0 or size < 0 or offset + size > bound:
            raise ValueError(f"{exe}: resource directory range outside .rsrc")
        data = pe.data[root_file + offset:root_file + offset + size]
        if len(data) != size:
            raise ValueError(f"{exe}: truncated resource directory")
        return data

    found: list[tuple[int | str, int | str, int, bytes]] = []
    visited: set[tuple[int, int]] = set()

    def walk(offset: int, path: tuple[int | str, ...]) -> None:
        depth = len(path)
        if depth >= 3 or (offset, depth) in visited:
            raise ValueError(f"{exe}: cyclic or over-deep resource directory")
        visited.add((offset, depth))
        header = read(offset, 16)
        named, numbered = struct.unpack_from("<HH", header, 12)
        count = named + numbered
        entries = read(offset + 16, count * 8)
        for index in range(count):
            raw_name, raw_target = struct.unpack_from("<II", entries, index * 8)
            identity = _name(raw_name, read)
            target = raw_target & 0x7FFFFFFF
            if depth < 2:
                if not raw_target & 0x80000000:
                    raise ValueError(f"{exe}: short resource directory path")
                walk(target, (*path, identity))
                continue
            if raw_target & 0x80000000 or not isinstance(identity, int):
                raise ValueError(f"{exe}: invalid resource language leaf")
            payload_rva, size, codepage, _reserved = struct.unpack(
                "<IIII", read(target, 16))
            if codepage:
                raise ValueError(f"{exe}: resource code page {codepage} cannot be represented in .res")
            if not (section["va"] <= payload_rva
                    and payload_rva + size <= section["va"] + section["rsize"]):
                raise ValueError(f"{exe}: resource payload outside stored .rsrc")
            payload = pe.read(payload_rva, size)
            if payload is None:
                raise ValueError(f"{exe}: truncated resource payload at 0x{payload_rva:x}")
            found.append((path[0], path[1], identity, payload))

    walk(0, ())
    if not found:
        raise ValueError(f"{exe}: empty PE resource directory")
    if len({(kind, name, lang) for kind, name, lang, _ in found}) != len(found):
        raise ValueError(f"{exe}: duplicate resource identity")
    return found


def _field(value: int | str) -> bytes:
    if isinstance(value, int):
        if not 0 <= value <= 0xFFFF:
            raise ValueError(f"resource ordinal out of range: {value}")
        return struct.pack("<HH", 0xFFFF, value)
    return value.encode("utf-16le") + b"\0\0"


def res_bytes(items: list[tuple[int | str, int | str, int, bytes]]) -> bytes:
    """Serialize standard Win32 .res records for link.exe/cvtres."""
    output = bytearray(struct.pack("<IIHHHHIHHII", 0, 32, 0xFFFF, 0,
                                   0xFFFF, 0, 0, 0, 0, 0, 0))
    for kind, name, language, payload in items:
        if not 0 <= language <= 0xFFFF:
            raise ValueError(f"resource language out of range: {language}")
        names = _field(kind) + _field(name)
        names += bytes((-len(names)) & 3)
        header_size = 8 + len(names) + 16
        output += struct.pack("<II", len(payload), header_size)
        output += names
        output += struct.pack("<IHHII", 0, 0x1030, language, 0, 0)
        output += payload
        output += bytes((-len(payload)) & 3)
    return bytes(output)


def write(exe: Path | str, out: Path | str) -> int:
    items = resources(exe)
    data = res_bytes(items)
    out = Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    if not out.exists() or out.read_bytes() != data:
        out.write_bytes(data)
    return len(items)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    count = write(args.exe, args.out)
    print(f"[retail-res] {count} resource(s) from {args.exe} -> {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
