#ifndef GITEN_TEXT_TEXTATTR_H
#define GITEN_TEXT_TEXTATTR_H

#include <Enums.h>
#include <Ints.h>

// A text cell's 16-bit attribute. The glyph blitter reads `bg` as the cell
// fill colour and `fg` as the glyph colour (both index the 16-colour text
// palette); a nonzero `dim` with zero colours draws the glyph grey, and bit 0
// of `flags` keeps a zero `bg` opaque.
typedef union TextAttr {
    u16 value;
    struct {
        u16 bg : 4;
        u16 dim : 4;
        u16 fg : 4;
        u16 flags : 4;
    };
} TextAttr;

#define SetTextAttrFlag(attr, mask, on)                                                            \
    do {                                                                                           \
        if (on) {                                                                                  \
            (attr)->flags |= (mask);                                                               \
        } else {                                                                                   \
            (attr)->flags &= ~(mask);                                                              \
        }                                                                                          \
    } while (0)

// The 16 text palette colours (s_textPalette), named after their RGB values.
GZ_ENUM_BEGIN(TextColorIndex)
    TEXT_COLOR_BLACK = 0,
    TEXT_COLOR_DARK_RED = 1,
    TEXT_COLOR_DARK_GREEN = 2,
    TEXT_COLOR_OLIVE = 3,
    TEXT_COLOR_WHITE = 4,
    TEXT_COLOR_RED = 5,
    TEXT_COLOR_GREEN = 6,
    TEXT_COLOR_YELLOW = 7,
    TEXT_COLOR_NAVY = 8,
    TEXT_COLOR_PURPLE = 9,
    TEXT_COLOR_TEAL = 10,
    TEXT_COLOR_GREY = 11,
    TEXT_COLOR_DARK_GREY = 12,
    TEXT_COLOR_BLUE = 13,
    TEXT_COLOR_MAGENTA = 14,
    TEXT_COLOR_CYAN = 15
GZ_ENUM_END(TextColorIndex)

// An attribute value from its glyph (fg), dim and fill (bg) colours; flags
// are ORed in with the TEXT_ATTR_OPAQUE .. TEXT_ATTR_HALF_WIDTH bits.
#define TEXT_ATTR(fg, dim, bg) (((fg) << 8) | ((dim) << 4) | (bg))

// The same fields as masks over TextAttr.value.
#define TEXT_ATTR_BG 0x000f
#define TEXT_ATTR_DIM 0x00f0
#define TEXT_ATTR_FG 0x0f00
#define TEXT_ATTR_OPAQUE 0x1000
// The flag bits SetWindowAttrFlag1 and SetWindowAttrFlag2 set; the glyph
// blitter does not read them.
#define TEXT_ATTR_FLAG1 0x2000
#define TEXT_ATTR_FLAG2 0x4000

// A plane's starting attribute, normal attribute and accent attribute.
#define TEXT_ATTR_DEFAULT TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
#define TEXT_ATTR_NORMAL TEXT_ATTR(TEXT_COLOR_YELLOW, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
#define TEXT_ATTR_ACCENT                                                                           \
    (TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_WHITE, TEXT_COLOR_BLACK))

// Set: the cell's glyph is drawn 8 pixels wide instead of 16.
#define TEXT_ATTR_HALF_WIDTH 0x8000

#define GetTextGlyphWidth(attr) (((attr) & TEXT_ATTR_HALF_WIDTH) ? 8 : 16)

void SpreadLowNibble(TextAttr* attr);
u16 SwapOuterNibbles(u16 value);
u16 SwapMiddleNibbles(u16 value);
u16 SwapLowNibbles(u16 value);

#endif // GITEN_TEXT_TEXTATTR_H
