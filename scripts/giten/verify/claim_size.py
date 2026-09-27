"""giten.verify.claim_size - an RVA() claim covers its whole contribution (fast tier).

    python3 -m giten.verify.claim_size

A function's `RVA(addr, size)` must reach the end of everything cl emitted for
it: code, alignment filler and the jump/index tables that follow the body
(docs/build-system.md). A code-only size leaves the tables outside the claim;
their relocations then bleed into the next function's comparison and the
claimed function scores its own residue against a truncated target.

For each claim the bytes between `addr + size` and the next admitted function
start must be at most 15 bytes of 0x90/0xcc alignment padding. Anything else
(a table's addresses, more code) means the size stops short.
"""

from __future__ import annotations

import bisect
import re
import sys

from giten.core.paths import RETAIL
from giten.core.pe import image
from giten.verify.srcscan import blank_comments, rel, source_files

_RVA = re.compile(r"^RVA\((0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+|\d+)\)", re.M)
PADDING = {0x90, 0xCC}
MAX_PAD = 15


def _starts() -> list[int]:
    return sorted(int(line.split("\t", 1)[0], 16)
                  for line in (RETAIL / "functions.tsv").read_text().splitlines()
                  if line.startswith("0x"))


def gate_findings() -> list[str]:
    img = image()
    text = img.section(".text")
    lo, raw = text["va"], img.data[text["rptr"]:text["rptr"] + text["rsize"]]
    hi = lo + len(raw)
    starts = _starts()
    out = []
    for path in source_files((".c", ".cpp")):
        code = blank_comments(path.read_text(encoding="utf-8", errors="replace"))
        for m in _RVA.finditer(code):
            rva, size = int(m.group(1), 16), int(m.group(2), 0)
            if not lo <= rva < hi:
                continue
            i = bisect.bisect_right(starts, rva)
            nxt = starts[i] if i < len(starts) else hi
            gap = raw[rva + size - lo:nxt - lo]
            if len(gap) > MAX_PAD or any(b not in PADDING for b in gap):
                end = nxt
                while end > rva + size and raw[end - 1 - lo] in PADDING:
                    end -= 1
                out.append(f"{rel(path)}: RVA(0x{rva:08x}, 0x{size:x}) stops "
                           f"{end - rva - size:#x} bytes short of its contribution "
                           f"(0x{end - rva:x}; next start 0x{nxt:06x})")
    return out


def main(argv=None) -> int:
    findings = gate_findings()
    for f in findings:
        print(f"[claim-size] {f}")
    print(f"[claim-size] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
