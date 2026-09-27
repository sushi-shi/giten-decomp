"""`python3 -m giten.ghidra <verb> ...` - the same dispatch `giten ghidra` uses."""

from __future__ import annotations

import sys

from giten.ghidra import main

if __name__ == "__main__":
    sys.exit(main())
