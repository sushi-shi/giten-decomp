"""giten.verify.placement - declarations and data claims where they belong.

Two board rows (ratcheted: a committed floor that may only fall):

  declarations away from their definition
      A prototype of a function that `RVA()` defines in unit F, found in a
      header F does not include (transitively) or in another unit's source
      file. The owner header of a function is one its defining TU sees; a
      declaration anywhere else is placed for some other reason than
      ownership.

  data claims short of their row
      A `DATA()` definition in .rdata/.data whose claimed size stops more than
      alignment short of its census row, leaving nonzero retail bytes of the
      row unclaimed: a table or record declared shorter than the image holds.

    python3 -m giten.verify.placement [--list]
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

from giten.core.paths import REPO
from giten.verify.srcscan import blank_comments, rel

INCLUDE = REPO / "include"
SRC = REPO / "src"
_INC = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.M)
_RVA = re.compile(r"^RVA\(0x[0-9a-fA-F]+,\s*(?:0x[0-9a-fA-F]+|\d+)\)[^\n]*\n", re.M)
#: a top-level prototype: column-0 type tokens, a name, a parameter list, `;`
_PROTO = re.compile(r"^(?!return\b|if\b|else\b|while\b|for\b|switch\b|case\b)"
                    r"[A-Za-z_][\w \t\*]*?\b([A-Za-z_]\w*)\s*\([^;{}()]*"
                    r"(?:\([^;{}()]*\)[^;{}()]*)*\)\s*;", re.M)
_NAME = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
ALIGN = 3


def _includes(path: Path, seen: set[Path]) -> set[Path]:
    """Headers `path` reaches through project includes (transitive)."""
    try:
        text = path.read_text(errors="ignore")
    except OSError:
        return seen
    for name in _INC.findall(text):
        header = INCLUDE / name
        if header.is_file() and header not in seen:
            seen.add(header)
            _includes(header, seen)
    return seen


def _definitions() -> dict[str, Path]:
    """{C function name: defining source file} for every RVA() definition."""
    out: dict[str, Path] = {}
    for path in sorted(SRC.rglob("*")):
        if path.suffix not in (".c", ".cpp") or not path.is_file():
            continue
        code = blank_comments(path.read_text(errors="ignore"))
        for m in _RVA.finditer(code):
            line = code[m.end():code.find("\n", m.end())]
            if "::" in line:
                continue
            names = _NAME.findall(line)
            if names:
                out.setdefault(names[0], path)
    return out


def _prototypes() -> list[tuple[str, Path]]:
    out = []
    for root in (INCLUDE, SRC):
        for path in sorted(root.rglob("*")):
            if path.suffix not in (".h", ".c", ".cpp") or not path.is_file():
                continue
            code = blank_comments(path.read_text(errors="ignore"))
            out += [(m.group(1), path) for m in _PROTO.finditer(code)]
    return out


def misplaced_declarations() -> list[str]:
    defs = _definitions()
    seen_by: dict[Path, set[Path]] = {}
    out = []
    for name, where in _prototypes():
        owner = defs.get(name)
        if owner is None or where == owner:
            continue
        if owner not in seen_by:
            seen_by[owner] = _includes(owner, set())
        if where.suffix in (".c", ".cpp"):
            out.append(f"{rel(where)}: local prototype of {name} "
                       f"(defined in {rel(owner)})")
        elif where not in seen_by[owner]:
            out.append(f"{rel(where)}: declares {name}, but its definition "
                       f"({rel(owner)}) does not include this header")
    return out


def short_data_claims() -> list[str]:
    from giten.core.pe import image
    from giten.model import resolve
    from giten.retail_labels import censuses
    img = image()
    rows = {r["rva"]: r for r in censuses.data()}

    def raw(rva: int, n: int) -> bytes:
        for s in img.sections:
            if s["va"] <= rva and rva + n <= s["va"] + s["rsize"]:
                off = s["rptr"] + rva - s["va"]
                return img.data[off:off + n]
        return b""
    out = []
    for b in resolve().data:
        if b.channel != "src" or b.space not in ("rdata", "data"):
            continue
        row = rows.get(b.rva)
        if row is None or b.size >= row["size"] - ALIGN:
            continue
        tail = raw(b.rva + b.size, row["size"] - b.size)
        if any(tail):
            out.append(f"{b.unit}: {b.name} at 0x{b.rva:06x} claims 0x{b.size:x} "
                       f"of its 0x{row['size']:x}-byte row; the rest holds "
                       f"nonzero retail bytes")
    return out


def main(argv=None) -> int:
    rows = [("declarations away from their definition", misplaced_declarations()),
            ("data claims short of their row", short_data_claims())]
    for label, findings in rows:
        print(f"{label}: {len(findings)}")
        if argv and "--list" in argv:
            for f in findings:
                print(f"  {f}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
