"""giten.graph.dataid - the base objects' data identity, as a delink input.

    python3 -m giten.graph.dataid --base-dir build/objdiff/base --out $out

giten.delink.{pdb_synth,data_manifest} read every base object's data
topology (COMMONs and their owners, `.bss`/`.data`/`.rdata` members, string
and vtable COMDATs), so a compile that changes only a symbol's storage class
- COMMON versus the unit's own `.bss` - changes the delink without changing a
claim. The delink edge cannot list the objects themselves: every code-only
edit rewrites one, and a whole-image delink per edit is exactly what the
bindings key exists to avoid. This edge renders the part of each object the
delink reads - non-code sections (shape, payload digest, relocation
referents) and the symbols defined in them or left COMMON - and writes it
if-changed, so its `restat` stops a code-only edit here.
"""

from __future__ import annotations

import hashlib
import re
import struct
import sys
from pathlib import Path

from giten.delink import coffx

#: IMAGE_SCN_CNT_CODE; IMAGE_SCN_LNK_INFO / IMAGE_SCN_LNK_REMOVE /
#: IMAGE_SCN_MEM_DISCARDABLE (.drectve, .debug$F frame records that follow
#: the code) never reach the image's data.
_CODE = 0x00000020
_NOT_IMAGE = 0x00000200 | 0x00000800 | 0x02000000
#: cl's TU-wide ordinals (`$SG<n>`, `$T<n>`, `name$S<n>`) move with any code
#: edit that declares something earlier; the delink pairs these by content.
_ORDINAL = re.compile(r"(\$(?:SG|T|L|S))[0-9]+")


def _name(name: str) -> str:
    return _ORDINAL.sub(r"\1", name)


def object_identity(obj: coffx.Obj) -> list[str]:
    """One object's data identity rows, independent of its code bytes.

    Sections are numbered among the data sections only, so a code COMDAT that
    appears or vanishes ahead of them does not renumber them."""
    data = {}
    for sec in obj.section_table:
        if not sec["characteristics"] & (_CODE | _NOT_IMAGE):
            data[sec["index"]] = len(data) + 1
    rows = []
    for idx, ordinal in data.items():
        sec = obj.section_table[idx - 1]
        assoc = data.get(sec["assoc"], "code") if sec["assoc"] else 0
        digest = hashlib.sha1(obj.section_payload(idx)).hexdigest()[:16]
        relocs = ",".join(f"{site:x}={_name(name)}" for site, name
                          in sorted(obj.relocations(idx).items()))
        rows.append(f"S\t{ordinal}\t{sec['name']}\t{sec['characteristics']:08x}\t"
                    f"{sec['size']:x}\t{sec['comdat']}\t{assoc}\t{digest}\t{relocs}")
    for sym, value, secnum in obj.iter_symbols():
        scl = obj.buf[obj.symptr + sym * 18 + 16]
        if secnum in data:
            where = data[secnum]
        elif secnum == 0 and scl == 2 and value:
            where = "common"
        else:
            continue
        rows.append(f"Y\t{_name(obj.sym_name(sym))}\t{scl}\t{where}\t{value:x}")
    return rows


def identity(base_dir: Path) -> str:
    """Every base object's data identity, keyed by unit stem."""
    out = []
    for path in sorted(Path(base_dir).glob("*.obj")):
        try:
            rows = object_identity(coffx.Obj(path))
        except (ValueError, OSError, struct.error):
            rows = ["unreadable"]
        out += [f"{path.stem}\t{row}" for row in rows]
    return "\n".join(out) + "\n"


def write(base_dir: Path, out: Path) -> bool:
    """Write the identity if-changed. True when the content moved."""
    want = identity(base_dir)
    if out.exists() and out.read_text() == want:
        return False
    out.parent.mkdir(parents=True, exist_ok=True)
    tmp = out.with_name(out.name + ".tmp")
    tmp.write_text(want)
    tmp.replace(out)
    return True


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="giten.graph.dataid", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--base-dir", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    a = ap.parse_args(argv)
    write(a.base_dir, a.out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
