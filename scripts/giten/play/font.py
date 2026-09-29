"""giten.play.font - the MS Gothic stand-in the game's text is rendered with.

DDS.EXE asks GDI for a 17-pixel `FF_MODERN`, `FW_THIN` font with no face name,
which Japanese Windows answers with MS Gothic. RenderGlyph copies a glyph's
bitmap into a 16x16 cell from its top (`gmptGlyphOrigin.y`, a row-table index)
down to the row just below the baseline: only rows whose top edge lies 0..14
pixels above the baseline land in place. A higher top shifts the glyph down,
and lower rows are dropped. MS Gothic's bitmaps at this size fit that window.

This cuts "Giten Gothic" from Noto Sans Mono CJK JP:

- The face is subset to the cp932 repertoire and pinned to a static wght=400
  instance, so Wine sees one face and FW_THIN has no other weight to pick.
- Noto sets 12% of the ideographic em box below the baseline, so at any
  legible em nearly every kanji and full-width bracket would lose its bottom
  row. Like MS Gothic's design, every outline is raised so the glyphs sit
  inside the copied window.
- Wine derives an integer ppem from the cell height and winAscent +
  winDescent, which are rescaled to give the fitted em.

The fit runs over the JIS X 0208 glyphs: fewest rows outside the window, then
the largest em, then the raise centred in the range achieving that (slack for
the rasteriser's rounding). It models Wine's black box, whose top is one row
above the outline's rounded-up top (the row is usually blank), so the window
holds 14 rows of outline. Every glyph fits at an 11-pixel em. A 12-pixel em
would cut or shift four rarely used glyphs (｀＾ÅЙ), and 13 pixels 28 (the
descenders of full-width Latin, Greek and Cyrillic, and ㎎㎏); the fit keeps
every glyph whole at the cost of that pixel of em. `giten play` prints the
count clipped at the chosen em.
"""
from __future__ import annotations

import math
from collections import Counter
from pathlib import Path

FAMILY = "Giten Gothic"
SOURCE_FAMILY = "Noto Sans Mono CJK JP"
#: The height RenderGlyph's font is created with: the cell, in pixels.
CELL_PX = 17
#: The highest and lowest row tops, in pixels above the baseline, it copies.
TOP_ROW, BOTTOM_ROW = 14, 0
#: Rows Wine's GGO_BITMAP black box adds above the outline's rounded-up top
#: (measured under Wine 11 on 96% of the JIS glyphs; the rest add none).
BOX_TOP_ROWS = 1
#: The smallest em the fit considers legible.
MIN_EM = 10
WEIGHT = 400
#: Bump when the build below changes; `giten play` rebuilds a stale font.
VERSION = "3"


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


def cp932_chars() -> list[str]:
    """Every character an ANSI (cp932) program can ask GDI for."""
    single = [bytes([b]).decode("cp932") for b in [*range(0x20, 0x7F), *range(0xA1, 0xE0)]]
    return single + jis_chars()


def mul_div(a: int, b: int, c: int) -> int:
    """FreeType's FT_MulDiv: a * b / c, rounded half up."""
    return (a * b + c // 2) // c


def load(source: Path):
    """The source face, subset to cp932 and pinned to a static WEIGHT."""
    from fontTools import subset
    from fontTools.ttLib import TTCollection
    from fontTools.varLib.instancer import instantiateVariableFont

    font = next(f for f in TTCollection(str(source)).fonts
                if f["name"].getDebugName(1) == SOURCE_FAMILY)
    options = subset.Options()
    options.layout_features = []
    options.notdef_outline = True
    options.drop_tables += ["BASE", "VORG", "VVAR", "vhea", "vmtx"]
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=[ord(c) for c in cp932_chars()])
    subsetter.subset(font)
    if "fvar" in font:
        font = instantiateVariableFont(font, {"wght": WEIGHT})
    if "STAT" in font:
        del font["STAT"]
    return font


def box_top(y1: float, scale: float) -> int:
    """The black-box top, in pixels above the baseline, of an outline top."""
    return math.ceil(y1 * scale) + BOX_TOP_ROWS


def box_bottom(y0: float, scale: float) -> int:
    return math.floor(y0 * scale)


def lost_rows(top: int, bottom: int) -> int:
    """Rows of a black box spanning `bottom`..`top` (pixels above the baseline)
    that RenderGlyph does not copy to their place."""
    return max(0, BOTTOM_ROW - 1 - bottom) + max(0, top - TOP_ROW)


def rows(boxes: list[tuple[int, int]], em: int, upem: int, raise_: int) -> tuple[int, int]:
    """(glyphs, rows) outside the copied window at `em`, outlines raised by
    `raise_` units; `boxes` are the glyphs' (yMin, yMax) control boxes."""
    scale = em / upem
    glyphs = lost = 0
    for y0, y1 in boxes:
        n = lost_rows(box_top(y1 + raise_, scale), box_bottom(y0 + raise_, scale))
        if n:
            glyphs += 1
            lost += n
    return glyphs, lost


def fit(boxes: list[tuple[int, int]], upem: int) -> tuple[int, int]:
    """(em, raise) as the module docstring describes."""
    bottoms, tops = Counter(y0 for y0, _ in boxes), Counter(y1 for _, y1 in boxes)
    best = None
    for em in range(CELL_PX, MIN_EM - 1, -1):
        scale = em / upem
        scores = []
        for raise_ in range(-upem // 4, upem // 2, max(1, upem // (em * 16))):
            lost = sum(n * lost_rows(TOP_ROW, box_bottom(y0 + raise_, scale))
                       for y0, n in bottoms.items())
            lost += sum(n * lost_rows(box_top(y1 + raise_, scale), BOTTOM_ROW)
                        for y1, n in tops.items())
            scores.append((lost, raise_))
        lost = min(score for score, _ in scores)
        if best is None or lost < best[0]:
            raises = [raise_ for score, raise_ in scores if score == lost]
            best = (lost, em, raises[len(raises) // 2])
    return best[1], best[2]


def raise_outlines(font, units: int) -> None:
    """Move every CFF2 outline up by `units` (dropping its hints)."""
    from fontTools.pens.t2CharStringPen import T2CharStringPen
    from fontTools.pens.transformPen import TransformPen

    glyphs = font.getGlyphSet()
    charstrings = font["CFF2"].cff.topDictIndex[0].CharStrings
    raised = {}
    for name in font.getGlyphOrder():
        pen = T2CharStringPen(None, glyphs, CFF2=True)
        glyphs[name].draw(TransformPen(pen, (1, 0, 0, 1, 0, units)))
        old = charstrings[name]
        raised[name] = pen.getCharString(private=old.private, globalSubrs=old.globalSubrs)
    for name, charstring in raised.items():
        charstrings[name] = charstring


def build(source: Path, out: Path) -> tuple[int, int, int]:
    """Write the stand-in to `out`; returns (em px, clipped glyphs, clipped rows)."""
    from fontTools.pens.boundsPen import ControlBoundsPen

    font = load(source)
    cmap, glyphs, upem = font.getBestCmap(), font.getGlyphSet(), font["head"].unitsPerEm
    boxes = []
    for char in jis_chars():
        if ord(char) in cmap:
            pen = ControlBoundsPen(glyphs)
            glyphs[cmap[ord(char)]].draw(pen)
            if pen.bounds:
                boxes.append((pen.bounds[1], pen.bounds[3]))
    em, raise_ = fit(boxes, upem)
    raise_outlines(font, raise_)

    # Wine's ppem for a positive height, less one if the cell would outgrow it.
    cell = round(upem * CELL_PX / em)
    assert mul_div(upem, CELL_PX, cell) == em and mul_div(cell, em, upem) <= CELL_PX
    os2 = font["OS/2"]
    os2.usWinDescent = round(cell / CELL_PX)   # the one row below the baseline
    os2.usWinAscent = cell - os2.usWinDescent
    font["hhea"].ascent, font["hhea"].descent = os2.usWinAscent, -os2.usWinDescent
    for record in font["name"].names:
        if record.nameID in (1, 4, 16):
            record.string = FAMILY
        elif record.nameID == 6:
            record.string = FAMILY.replace(" ", "")
    out.parent.mkdir(parents=True, exist_ok=True)
    font.save(str(out))
    return em, *rows(boxes, em, upem, raise_)
