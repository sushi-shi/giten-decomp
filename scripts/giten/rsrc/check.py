"""giten.rsrc.check - the candidate's .rsrc against the original's.

    giten rsrc check [--no-link] [--candidate EXE] [--retail EXE]

Builds the candidate through the graph (`giten link`) unless told not to,
then compares the two resource sections:

  tree      every directory table in walk order: path, characteristics,
            timestamp, version, and its entries (types, names, languages)
            in order;
  leaves    every data entry: identity, payload bytes, code page, reserved;
  layout    the root's, every table's, every data entry's and every payload's
            offset within .rsrc, the resource data directory's size and the
            section's stored length;
  bytes     the whole stored section, with each data entry's OffsetToData
            rebased from an RVA to a section offset.

The section's own RVA is the one field outside the script's control: it
follows from the size of the sections the linker places before .rsrc, so a
candidate whose code is not yet byte-identical puts .rsrc elsewhere, and every
OffsetToData moves by that delta. The check reports the delta and compares
everything else exactly; any other difference is a finding (exit 1).
"""

from __future__ import annotations

import argparse
import os
import struct
import sys
from pathlib import Path

from giten.rsrc.tree import Directory, label, read


def _rebased(directory: Directory) -> bytes:
    """The stored section with each OffsetToData made section-relative."""
    image = bytearray(directory.image)
    root = directory.root_rva - directory.section_va
    for leaf in directory.leaves:
        struct.pack_into("<I", image, root + leaf.entry,
                         leaf.rva - directory.section_va)
    return bytes(image)


def findings(retail: Directory, candidate: Directory, limit: int = 20) -> list[str]:
    """Every difference except the section RVA itself."""
    out: list[str] = []

    def add(message: str) -> None:
        if len(out) < limit:
            out.append(message)
        elif len(out) == limit:
            out.append("... (limit reached)")

    if (retail.root_rva - retail.section_va) != (candidate.root_rva - candidate.section_va):
        add("layout: the directory root sits at a different .rsrc offset")
    if retail.root_size != candidate.root_size:
        add(f"layout: resource data directory size {candidate.root_size:#x}, "
            f"retail {retail.root_size:#x}")
    if len(retail.tables) != len(candidate.tables):
        add(f"tree: {len(retail.tables)} directory table(s) in retail, "
            f"{len(candidate.tables)} in the candidate")
    for want, got in zip(retail.tables, candidate.tables):
        where = "/".join(map(str, want.path)) or "root"
        if want.path != got.path:
            add(f"tree: walk order differs at {where} (candidate has "
                f"{'/'.join(map(str, got.path)) or 'root'})")
            break
        if want.entries != got.entries:
            missing = [e for e in want.entries if e not in got.entries]
            extra = [e for e in got.entries if e not in want.entries]
            detail = (f"missing {missing}" if missing else "") \
                + (f" extra {extra}" if extra else "")
            add(f"tree: {where} entries differ: {detail.strip() or 'order'}")
        for field in ("characteristics", "timestamp", "version"):
            if getattr(want, field) != getattr(got, field):
                add(f"tree: {where} {field} {getattr(want, field)!r} != "
                    f"{getattr(got, field)!r}")
        if want.offset != got.offset:
            add(f"layout: table {where} at +{got.offset:#x}, retail +{want.offset:#x}")
    theirs = {leaf.key: leaf for leaf in candidate.leaves}
    for want in retail.leaves:
        name = label(*want.key)
        got = theirs.pop(want.key, None)
        if got is None:
            add(f"leaves: {name} missing from the candidate")
            continue
        if want.payload != got.payload:
            first = next((i for i, (a, b) in enumerate(zip(want.payload, got.payload))
                          if a != b), min(len(want.payload), len(got.payload)))
            add(f"leaves: {name} payload differs ({len(want.payload)} vs "
                f"{len(got.payload)} B, first at +{first:#x})")
        if (want.codepage, want.reserved) != (got.codepage, got.reserved):
            add(f"leaves: {name} code page/reserved {want.codepage}/{want.reserved} "
                f"!= {got.codepage}/{got.reserved}")
        if want.entry != got.entry:
            add(f"layout: {name} data entry at +{got.entry:#x}, retail +{want.entry:#x}")
        if want.rva - retail.section_va != got.rva - candidate.section_va:
            add(f"layout: {name} payload at .rsrc+{got.rva - candidate.section_va:#x}, "
                f"retail .rsrc+{want.rva - retail.section_va:#x}")
    for got in theirs.values():
        add(f"leaves: {label(*got.key)} is not in retail")
    if len(retail.image) != len(candidate.image):
        add(f"layout: .rsrc stores {len(candidate.image):#x} B, retail {len(retail.image):#x} B")
    if not out:
        want, got = _rebased(retail), _rebased(candidate)
        if want != got:
            first = next(i for i, (a, b) in enumerate(zip(want, got)) if a != b)
            add(f"bytes: rebased .rsrc differs first at +{first:#x} "
                "(outside every compared field: padding or name strings)")
    return out


def summary(retail: Directory, candidate: Directory) -> str:
    kinds: dict[object, int] = {}
    for leaf in retail.leaves:
        kinds[leaf.kind] = kinds.get(leaf.kind, 0) + 1
    counts = ", ".join(f"{n} {label(k, '').strip()}" for k, n in kinds.items())
    delta = candidate.section_va - retail.section_va
    rva = ("at retail's RVA" if not delta else
           f"at RVA {candidate.section_va:#x} (retail {retail.section_va:#x}, "
           f"{delta:+#x}; every OffsetToData moves with it)")
    return (f"{len(retail.leaves)} resources ({counts}), {len(retail.tables)} "
            f"directory tables; candidate .rsrc {rva}")


def main(argv: list[str] | None = None) -> int:
    from giten.core.paths import BUILD
    parser = argparse.ArgumentParser(
        prog="giten rsrc check", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--candidate", type=Path,
                        help="the image to check (default: the graph's candidate)")
    parser.add_argument("--retail", type=Path,
                        help="the original (default $GITEN_RETAIL_EXE)")
    parser.add_argument("--no-link", action="store_true",
                        help="compare the existing candidate without linking first")
    args = parser.parse_args(argv)
    if args.candidate is None and not args.no_link:
        from giten.graph.verbs import link_main
        rc = link_main([])
        if rc:
            return rc
    candidate = args.candidate or BUILD / "exe/DDS.candidate.EXE"
    retail = args.retail or os.environ.get("GITEN_RETAIL_EXE")
    if not retail:
        print("[rsrc] $GITEN_RETAIL_EXE unset and no --retail", file=sys.stderr)
        return 2
    try:
        want, got = read(retail), read(candidate)
    except (OSError, ValueError, KeyError) as error:
        print(f"[rsrc] {error}", file=sys.stderr)
        return 2
    bad = findings(want, got)
    print(f"[rsrc] {summary(want, got)}", flush=True)
    for line in bad:
        print(f"  {line}", file=sys.stderr)
    if bad:
        print("[rsrc] FAIL: the candidate's .rsrc differs from retail's", file=sys.stderr)
        return 1
    print("[rsrc] OK: tree, payloads, code pages, layout and bytes identical "
          "(section-relative)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
