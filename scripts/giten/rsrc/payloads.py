"""giten.rsrc.payloads - the original's resource payloads as RC.EXE input files.

    python3 -m giten.rsrc.payloads --out build/gen/rsrc [--exe DDS.EXE | --disc IMAGE]

The resource script (src/Giten/Giten.rc) is tracked source; its payloads are
not. This writes them from the user's original executable ($GITEN_RETAIL_EXE,
the disc's DDSWIN/DDS.EXE) or straight from the disc image, as the files the
script names:

    bitmap_NNNN.bmp  RT_BITMAP with its BITMAPFILEHEADER restored
    wave_NNNN.wav    the WAVE payload as stored
    cursor_NNNN.cur  RT_GROUP_CURSOR NNNN and its RT_CURSOR images
    icon_NNNN.ico    RT_GROUP_ICON NNNN and its RT_ICON images

RC.EXE reverses each transform (drops the file header; splits .cur/.ico into
images numbered in script order plus a group), so the compiled .res carries
the original bytes again - `giten rsrc check` proves it on the linked image.
Files are rewritten only when their bytes change; the `--stamp` listing is the
graph's output and changes only when the payload set does.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import struct
import sys
import tempfile
from pathlib import Path

from giten.rsrc.tree import (RT_BITMAP, RT_CURSOR, RT_GROUP_CURSOR,
                             RT_GROUP_ICON, RT_ICON, Identity, Leaf, label,
                             read)

#: resource type -> (file stem, extension). Types outside this table are
#: rejected: DDS.EXE carries no others, and guessing a file form would be
#: unverified.
FILE_FORMS: dict[Identity, tuple[str, str]] = {
    RT_BITMAP: ("bitmap", "bmp"),
    "WAVE": ("wave", "wav"),
    RT_GROUP_CURSOR: ("cursor", "cur"),
    RT_GROUP_ICON: ("icon", "ico"),
}

#: group type -> the image type its entries name.
GROUP_IMAGES = {RT_GROUP_CURSOR: RT_CURSOR, RT_GROUP_ICON: RT_ICON}


def filename(kind: Identity, name: Identity) -> str:
    """The build-directory file the resource script names for a resource."""
    if kind not in FILE_FORMS:
        raise ValueError(f"no file form for resource type {kind!r}")
    if not isinstance(name, int):
        raise ValueError(f"{label(kind, name)}: named resources have no file form")
    stem, ext = FILE_FORMS[kind]
    return f"{stem}_{name:04d}.{ext}"


def bitmap_file(dib: bytes) -> bytes:
    """A packed DIB (RT_BITMAP) with its 14-byte BITMAPFILEHEADER restored."""
    if len(dib) < 4:
        raise ValueError("truncated bitmap resource")
    header = struct.unpack_from("<I", dib)[0]
    if header == 12:                                    # BITMAPCOREHEADER
        bits = struct.unpack_from("<H", dib, 10)[0]
        colors, entry, masks = (1 << bits if bits <= 8 else 0), 3, 0
    elif header >= 40:                                  # BITMAPINFOHEADER+
        bits, compression = struct.unpack_from("<HI", dib, 14)
        used = struct.unpack_from("<I", dib, 32)[0]
        colors = used or (1 << bits if bits <= 8 else 0)
        entry = 4
        masks = 12 if header == 40 and compression == 3 else 0   # BI_BITFIELDS
    else:
        raise ValueError(f"unknown bitmap header size {header}")
    bits_offset = 14 + header + masks + colors * entry
    if bits_offset > 14 + len(dib):
        raise ValueError("bitmap colour table past the resource")
    return struct.pack("<2sIHHI", b"BM", 14 + len(dib), 0, 0, bits_offset) + dib


def group_file(kind: Identity, group: bytes, images: dict[int, bytes]) -> bytes:
    """Reassemble a .cur/.ico file from a group resource and its images.

    Group entries (GRPICONDIRENTRY / the cursor form) carry the image's
    resource ID where the file's ICONDIRENTRY carries its file offset. A
    cursor image resource starts with its hotspot, which the file keeps in the
    entry's planes/bit-count slots; the file's cursor height is the single
    image height, half the group's AND+XOR height.
    """
    reserved, restype, count = struct.unpack_from("<HHH", group)
    cursor = kind == RT_GROUP_CURSOR
    if reserved or restype != (2 if cursor else 1) or len(group) != 6 + 14 * count:
        raise ValueError(f"malformed {label(kind, '').strip()} header")
    entries, blobs = [], []
    offset = 6 + 16 * count
    for index in range(count):
        base = 6 + 14 * index
        if cursor:
            width, height, planes, bitcount, size, image_id = \
                struct.unpack_from("<HHHHIH", group, base)
            data = images[image_id]
            if size != len(data) or size < 4:
                raise ValueError(f"cursor image {image_id}: size {len(data)} != group {size}")
            hot_x, hot_y = struct.unpack_from("<HH", data)
            data = data[4:]
            colors = 1 << (planes * bitcount) if planes * bitcount < 8 else 0
            entry = struct.pack("<BBBBHHII", width & 0xFF, (height // 2) & 0xFF,
                                colors, 0, hot_x, hot_y, len(data), offset)
        else:
            width, height, colors, zero, planes, bitcount, size, image_id = \
                struct.unpack_from("<BBBBHHIH", group, base)
            data = images[image_id]
            if size != len(data):
                raise ValueError(f"icon image {image_id}: size {len(data)} != group {size}")
            entry = struct.pack("<BBBBHHII", width, height, colors, zero,
                                planes, bitcount, len(data), offset)
        entries.append(entry)
        blobs.append(data)
        offset += len(data)
    return struct.pack("<HHH", 0, 2 if cursor else 1, count) \
        + b"".join(entries) + b"".join(blobs)


def payload_files(leaves: list[Leaf] | tuple[Leaf, ...]) -> dict[str, bytes]:
    """{file name: bytes} for every resource; every image belongs to a group."""
    by_key = {leaf.key: leaf for leaf in leaves}
    claimed: set[tuple[Identity, Identity, int]] = set()
    files: dict[str, bytes] = {}
    for leaf in leaves:
        if leaf.kind in GROUP_IMAGES.values():
            continue
        name = filename(leaf.kind, leaf.name)
        if leaf.kind in GROUP_IMAGES:
            image_kind = GROUP_IMAGES[leaf.kind]
            count = struct.unpack_from("<H", leaf.payload, 4)[0]
            images = {}
            for index in range(count):
                image_id = struct.unpack_from("<H", leaf.payload, 6 + 14 * index + 12)[0]
                key = (image_kind, image_id, leaf.language)
                if key not in by_key:
                    raise ValueError(f"{label(*leaf.key)} names missing {label(*key)}")
                claimed.add(key)
                images[image_id] = by_key[key].payload
            data = group_file(leaf.kind, leaf.payload, images)
        elif leaf.kind == RT_BITMAP:
            data = bitmap_file(leaf.payload)
        else:
            data = leaf.payload
        if name in files:
            raise ValueError(f"{label(*leaf.key)}: a second language of {name} "
                             "has no file form")
        files[name] = data
    orphans = [label(*leaf.key) for leaf in leaves
               if leaf.kind in GROUP_IMAGES.values() and leaf.key not in claimed]
    if orphans:
        raise ValueError(f"image resources outside any group: {', '.join(orphans)}")
    return files


def write(files: dict[str, bytes], out: Path, stamp: Path | None = None) -> int:
    """Write-if-changed; drop stale payload files; return the files changed."""
    out.mkdir(parents=True, exist_ok=True)
    forms = {f".{ext}" for _stem, ext in FILE_FORMS.values()}
    for path in out.iterdir():
        if path.suffix in forms and path.name not in files:
            path.unlink()
    changed = 0
    for name, data in files.items():
        path = out / name
        if path.exists() and path.read_bytes() == data:
            continue
        path.write_bytes(data)
        changed += 1
    if stamp is not None:
        listing = "".join(f"{name}\t{len(data)}\t{hashlib.sha256(data).hexdigest()}\n"
                          for name, data in sorted(files.items()))
        if not stamp.exists() or stamp.read_text() != listing:
            stamp.write_text(listing)
    return changed


def exe_from_disc(disc: Path, member: str = "DDSWIN/DDS.EXE") -> bytes:
    """The original executable's bytes, read from a raw or cooked CD image."""
    from giten.tool.cdfs import Image, walk
    for sector in (2352, 2048):
        image = Image(str(disc), sector)
        pvd = image.read(16, 1)
        if pvd[1:6] != b"CD001":
            continue
        root = pvd[156:190]
        rlba, rsize = struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0]
        for path, lba, size, directory in walk(image, rlba, rsize):
            if not directory and path.upper() == member:
                return image.read(lba, (size + 2047) // 2048)[:size]
        raise ValueError(f"{disc}: no {member} on the disc")
    raise ValueError(f"{disc}: no ISO9660 volume (tried 2352- and 2048-byte sectors)")


def leaves_from(exe: Path | None, disc: Path | None) -> tuple[Leaf, ...]:
    if disc is not None:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "DDS.EXE"
            path.write_bytes(exe_from_disc(disc))
            return read(path).leaves
    if exe is None:
        value = os.environ.get("GITEN_RETAIL_EXE")
        if not value:
            raise ValueError("no --exe/--disc and $GITEN_RETAIL_EXE unset")
        exe = Path(value)
    return read(exe).leaves


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="giten rsrc extract", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--exe", type=Path,
                        help="the original DDS.EXE (default $GITEN_RETAIL_EXE)")
    source.add_argument("--disc", type=Path,
                        help="the original disc image (.bin 2352 or .iso 2048)")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--stamp", type=Path)
    args = parser.parse_args(argv)
    try:
        files = payload_files(leaves_from(args.exe, args.disc))
    except (OSError, ValueError, KeyError, struct.error) as error:
        print(f"[rsrc] {error}", file=sys.stderr)
        return 1
    changed = write(files, args.out, args.stamp)
    print(f"[rsrc] {len(files)} payload file(s) -> {args.out} ({changed} rewritten)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
