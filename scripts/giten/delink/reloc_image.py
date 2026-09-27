"""giten.delink.reloc_image - the build copy of the retail image, with a .reloc.

    python3 -m giten.delink.reloc_image [--retail R] [--sites S] [--out O]

DDS.EXE was linked /FIXED: IMAGE_FILE_RELOCS_STRIPPED is set and there is no
.reloc section, so the absolute DIR32 address operands every relocation-driven
tool needs (the delinker, sema, the data/verify gates) are not in the image.
config/retail/reloc_sites.tsv states them - synthesized, reviewed, validated
against the LIBC.LIB bodies' own COFF fixups (docs/relocations.md). This edge
appends them to a copy of the retail image as a genuine base-relocation
section, so every consumer reads relocations the way it would from an image
that kept them:

  * one IMAGE_BASE_RELOCATION block per 4 KiB page, HIGHLOW (type 3) entries,
    each block padded to a DWORD with a type-0 (ABSOLUTE) entry;
  * a `.reloc` section header appended after the last section (header room is
    asserted), placed at the next SectionAlignment/FileAlignment boundary;
  * data directory 5 pointed at it, SizeOfImage grown, the RELOCS_STRIPPED
    characteristic cleared.

Nothing else moves: every existing section, RVA and byte is unchanged, so the
copy is interchangeable with retail for every other purpose. The pristine
image stays in the nix store ($GITEN_RETAIL_EXE); $GITEN_EXE names this copy.
Written if-changed (ninja restat).
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from pathlib import Path

from giten.core.paths import BUILD, RETAIL
from giten.core.tsv import read as read_tsv

SITES = RETAIL / "reloc_sites.tsv"
OUT = BUILD / "exe/DDS.EXE"

IMAGE_REL_BASED_ABSOLUTE = 0
IMAGE_REL_BASED_HIGHLOW = 3
IMAGE_FILE_RELOCS_STRIPPED = 0x0001
RELOC_CHARACTERISTICS = 0x42000040      # INITIALIZED_DATA | DISCARDABLE | READ


def align(value: int, to: int) -> int:
    return (value + to - 1) // to * to


def retail_path() -> Path:
    v = os.environ.get("GITEN_RETAIL_EXE")
    if not v:
        raise SystemExit("[reloc_image] $GITEN_RETAIL_EXE unset - run inside `nix develop`")
    return Path(v)


def load_sites(path: Path = SITES) -> list[int]:
    _banner, header, rows = read_tsv(path)
    if "site" not in header:
        raise ValueError(f"{path}: no `site` column")
    sites = sorted(int(r["site"], 16) for r in rows)
    for a, b in zip(sites, sites[1:]):
        if b - a < 4:
            raise ValueError(f"{path}: overlapping sites 0x{a:06x} / 0x{b:06x}")
    return sites


def reloc_blocks(sites: list[int]) -> bytes:
    """The .reloc payload: per-page IMAGE_BASE_RELOCATION blocks."""
    pages: dict[int, list[int]] = {}
    for s in sites:
        pages.setdefault(s & ~0xFFF, []).append(s & 0xFFF)
    out = bytearray()
    for page in sorted(pages):
        entries = [(IMAGE_REL_BASED_HIGHLOW << 12) | off for off in sorted(pages[page])]
        if len(entries) % 2:
            entries.append(IMAGE_REL_BASED_ABSOLUTE << 12)
        out += struct.pack("<II", page, 8 + 2 * len(entries))
        out += struct.pack(f"<{len(entries)}H", *entries)
    return bytes(out)


def synthesize(retail: bytes, sites: list[int]) -> bytes:
    d = bytearray(retail)
    pe = struct.unpack_from("<I", d, 0x3C)[0]
    opt = pe + 24
    nsec = struct.unpack_from("<H", d, pe + 6)[0]
    optsz = struct.unpack_from("<H", d, pe + 20)[0]
    sect_align, file_align = struct.unpack_from("<II", d, opt + 32)
    size_of_headers = struct.unpack_from("<I", d, opt + 60)[0]
    first = opt + optsz
    names = [d[first + 40 * i:first + 40 * i + 8].rstrip(b"\0") for i in range(nsec)]
    if b".reloc" in names:
        raise ValueError("the image already carries a .reloc section")
    hdr = first + 40 * nsec
    if hdr + 40 > size_of_headers:
        raise ValueError("no header room for a .reloc section header")
    if any(d[hdr:hdr + 40]):
        raise ValueError("the header slot after the last section is not zero")

    last = first + 40 * (nsec - 1)
    vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, last + 8)
    # Every site must address bytes inside an existing section's stored data.
    spans = [struct.unpack_from("<IIII", d, first + 40 * i + 8) for i in range(nsec)]
    for s in sites:
        if not any(sva <= s and s + 4 <= sva + min(svs, srs) for svs, sva, srs, _p in spans):
            raise ValueError(f"reloc site 0x{s:06x} is outside every section's data")

    payload = reloc_blocks(sites)
    new_va = align(va + max(vsize, rsize), sect_align)
    new_rptr = align(max(len(d), rptr + rsize), file_align)
    new_rsize = align(len(payload), file_align)
    d += bytes(new_rptr - len(d))
    d += payload + bytes(new_rsize - len(payload))
    struct.pack_into("<8sIIIIIIHHI", d, hdr, b".reloc", len(payload), new_va,
                     new_rsize, new_rptr, 0, 0, 0, 0, RELOC_CHARACTERISTICS)
    struct.pack_into("<H", d, pe + 6, nsec + 1)
    chars = struct.unpack_from("<H", d, pe + 22)[0]
    struct.pack_into("<H", d, pe + 22, chars & ~IMAGE_FILE_RELOCS_STRIPPED)
    struct.pack_into("<I", d, opt + 56, align(new_va + len(payload), sect_align))
    struct.pack_into("<II", d, opt + 96 + 8 * 5, new_va, len(payload))
    return bytes(d)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--retail", type=Path, help="pristine image (default $GITEN_RETAIL_EXE)")
    ap.add_argument("--sites", type=Path, default=SITES)
    ap.add_argument("--out", type=Path, default=OUT)
    a = ap.parse_args(argv)
    retail = (a.retail or retail_path()).read_bytes()
    try:
        image = synthesize(retail, load_sites(a.sites))
    except ValueError as e:
        print(f"[reloc_image] {e}", file=sys.stderr)
        return 1
    a.out.parent.mkdir(parents=True, exist_ok=True)
    if a.out.exists() and a.out.read_bytes() == image:
        return 0
    tmp = a.out.with_suffix(".tmp")
    tmp.write_bytes(image)
    tmp.replace(a.out)
    print(f"[reloc_image] {a.out}: {len(load_sites(a.sites))} site(s) -> .reloc "
          f"({len(image) - len(retail):,} B appended)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
