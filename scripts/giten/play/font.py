"""giten.play.font - the MS Gothic stand-in the game's text is rendered with.

DDS.EXE asks GDI for a 17-pixel `FF_MODERN` font with no face name, which
Japanese Windows answers with MS Gothic, and copies each glyph into a 16x16
cell. This cuts "Giten Gothic" from Noto Sans Mono CJK JP with its Windows
line metrics rescaled so that cell height yields the largest em whose JIS X
0208 glyphs still fit in 15 rows.
"""
from __future__ import annotations

import math
from pathlib import Path

FAMILY = "Giten Gothic"
CELL_PX = 17
MAX_ROWS = 15


def jis_chars() -> list[str]:
    """Every character of the Shift-JIS double-byte range."""
    chars = []
    for lead in [*range(0x81, 0xA0), *range(0xE0, 0xEB)]:
        for trail in [*range(0x40, 0x7F), *range(0x80, 0xFD)]:
            try:
                chars.append(bytes([lead, trail]).decode("cp932"))
            except UnicodeDecodeError:
                pass
    return chars


def build(source: Path, out: Path) -> float:
    """Write the stand-in to `out`; returns its em size in pixels."""
    from fontTools.pens.boundsPen import BoundsPen
    from fontTools.ttLib import TTCollection

    font = next(f for f in TTCollection(str(source)).fonts
                if f["name"].getDebugName(1) == "Noto Sans Mono CJK JP")
    cmap, glyphs, upem = font.getBestCmap(), font.getGlyphSet(), font["head"].unitsPerEm
    boxes = []
    for char in jis_chars():
        if ord(char) in cmap:
            pen = BoundsPen(glyphs)
            glyphs[cmap[ord(char)]].draw(pen)
            if pen.bounds:
                boxes.append(pen.bounds)

    def rows(em: float) -> int:
        scale = em / upem
        return max(math.ceil(y1 * scale) - math.floor(y0 * scale) for _, y0, _, y1 in boxes)

    em = float(CELL_PX)
    while rows(em) > MAX_ROWS:
        em -= 0.25
    os2 = font["OS/2"]
    ascent, descent = os2.usWinAscent, os2.usWinDescent
    cell = round(upem * CELL_PX / em)
    os2.usWinAscent = round(cell * ascent / (ascent + descent))
    os2.usWinDescent = cell - os2.usWinAscent
    font["hhea"].ascent, font["hhea"].descent = os2.usWinAscent, -os2.usWinDescent
    for record in font["name"].names:
        if record.nameID in (1, 4, 16):
            record.string = FAMILY
        elif record.nameID == 6:
            record.string = FAMILY.replace(" ", "")
    out.parent.mkdir(parents=True, exist_ok=True)
    font.save(str(out))
    return em
