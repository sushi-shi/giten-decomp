"""`python3 -m giten.sema <view> ...` - the same dispatch `giten sema` uses."""

from __future__ import annotations

import sys

from giten.sema import main

if __name__ == "__main__":
    sys.exit(main())
