"""giten.verify.source_encoding - compiled source text is ASCII (fast tier).

    python3 -m giten.verify.source_encoding

cl 5.0 copies a string or character literal's bytes into the object as they
stand in the file. The game's text is Shift-JIS, and the sources are UTF-8, so
a literal typed as Japanese text compiles to UTF-8 bytes retail never held.
While data matching is off the pooled string's bytes are not compared, so the
mistake only surfaces later (an inline strcpy's length, a data_matching flip).
Retail text is therefore written as octal escapes with the text in a comment
(`"\\203\\175\\203\\142\\203\\112" /* マッカ */`); comments may hold any text.

The gate fails on any non-ASCII byte outside a comment in src/ or include/.
"""

from __future__ import annotations

import sys

from giten.verify.srcscan import blank_comments, rel, source_files


def gate_findings() -> list[str]:
    out = []
    for path in source_files():
        text = path.read_text(encoding="utf-8", errors="replace")
        code = blank_comments(text)
        for lineno, line in enumerate(code.splitlines(), 1):
            bad = [ch for ch in line if ord(ch) > 0x7f]
            if bad:
                out.append(f"{rel(path)}:{lineno}: non-ASCII {''.join(bad)[:12]!r} "
                           f"outside a comment - write retail text as Shift-JIS "
                           f"octal escapes with the text in a comment")
    return out


def main(argv=None) -> int:
    findings = gate_findings()
    for f in findings:
        print(f"[source-encoding] {f}")
    print(f"[source-encoding] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())
