"""giten.rsrc - the resource script's payloads and the candidate's .rsrc check.

    giten rsrc extract --out DIR [--exe EXE | --disc IMAGE] [--stamp FILE]
                                  write the original's payload files
    giten rsrc check [--no-link]  link the candidate, compare .rsrc to retail
"""

from __future__ import annotations

import sys


def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        return 0 if argv else 2
    verb, rest = argv[0], argv[1:]
    if verb == "extract":
        from giten.rsrc.payloads import main as extract
        return extract(rest)
    if verb == "check":
        from giten.rsrc.check import main as check
        return check(rest)
    print(f"giten rsrc: unknown verb {verb!r} (have: extract, check)", file=sys.stderr)
    return 2
