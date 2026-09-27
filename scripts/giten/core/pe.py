"""giten.core.pe - the retail image, parsed once.

The PE section table is the authority for every address-space edge; nothing
in the tree hardcodes an image constant. Parsed lazily and cached per
process (the image never changes).
"""

from __future__ import annotations

import struct
from functools import lru_cache
from pathlib import Path

from giten.core.paths import retail_exe


class Pe:
    def __init__(self, path: Path | str | None = None):
        self.path = Path(path or retail_exe())
        self.data = d = self.path.read_bytes()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        magic = struct.unpack_from("<H", d, pe + 24)[0]
        if magic != 0x10B:
            raise ValueError(f"{self.path}: not a PE32 image (magic 0x{magic:x})")
        self.image_base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        ndirs = struct.unpack_from("<I", d, pe + 24 + 92)[0]
        self.directories = [struct.unpack_from("<II", d, pe + 24 + 96 + 8 * i)
                            for i in range(ndirs)]
        self.sections: list[dict] = []
        for i in range(nsec):
            base = pe + 24 + optsz + i * 40
            name = d[base:base + 8].rstrip(b"\0").decode("latin-1")
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, base + 8)
            self.sections.append({"name": name, "va": va, "vsize": vsize,
                                  "rsize": rsize, "rptr": rptr})

    def section(self, name: str) -> dict:
        s = next((s for s in self.sections if s["name"] == name), None)
        if s is None:
            raise KeyError(f"{self.path}: no section {name}")
        return s

    def section_index(self, name: str) -> int:
        """1-based index of section `name` in PE order (the numbering the
        delinker's object crate and the synthetic PDB's segments use)."""
        return next(i for i, s in enumerate(self.sections, 1) if s["name"] == name)

    def iat_span(self) -> tuple[int, int]:
        """[lo, hi) of the import address table. DDS.EXE has no .idata
        section - the non-incremental link merged the import tables into
        .rdata - so the PE data directory (IMAGE_DIRECTORY_ENTRY_IAT) is the
        authority; an .idata section, where present, bounds it instead."""
        if any(s["name"] == ".idata" for s in self.sections):
            it = self.section(".idata")
            return it["va"], it["va"] + max(it["vsize"], it["rsize"])
        rva, size = self.directories[12]
        return rva, rva + size

    def iat_segment(self) -> int:
        """The synthetic-PDB segment of IAT symbols: .idata's index, else one
        past the real sections (nix/patches/vostok-iat-in-rdata.patch)."""
        if any(s["name"] == ".idata" for s in self.sections):
            return self.section_index(".idata")
        return len(self.sections) + 1

    def text_span(self) -> tuple[int, int]:
        """[lo, hi) of .text's VIRTUAL extent (what function extents cap at)."""
        t = self.section(".text")
        return t["va"], t["va"] + t["vsize"]

    def data_regions(self) -> dict[str, tuple[int, int]]:
        """The four data regions: the IAT (`idata`, listed first: in DDS.EXE
        it lies INSIDE .rdata, so it must win the lookup), .rdata's stored
        bytes, .data's raw (initialized) bytes, and .data's loader-zero
        virtual tail (this image has no separate .bss header). .rdata's raw
        size is FileAlignment-padded past its virtual size; the padding is
        not image content, so the region stops at the smaller of the two."""
        rd, da = self.section(".rdata"), self.section(".data")
        rd_hi = rd["va"] + (min(rd["rsize"], rd["vsize"]) if rd["vsize"] else rd["rsize"])
        return {"idata": self.iat_span(),
                "rdata": (rd["va"], rd_hi),
                "data": (da["va"], da["va"] + da["rsize"]),
                "bss": (da["va"] + da["rsize"], da["va"] + da["vsize"])}

    def read(self, rva: int, size: int) -> bytes | None:
        """Bytes at rva; loader zero-fill (past a section's raw size) reads as
        ZEROS, never as the next section's file bytes; short reads are None."""
        for s in self.sections:
            if s["va"] <= rva and rva + size <= s["va"] + max(s["vsize"], s["rsize"]):
                raw_end = s["va"] + s["rsize"]
                if rva >= raw_end:
                    return bytes(size)
                stored = min(size, raw_end - rva)
                off = s["rptr"] + rva - s["va"]
                chunk = self.data[off:off + stored]
                if len(chunk) != stored:
                    return None
                return chunk + bytes(size - stored)
        return None


@lru_cache(maxsize=1)
def image() -> Pe:
    return Pe()
