#ifndef GITEN_TEXT_TEXTATTR_H
#define GITEN_TEXT_TEXTATTR_H

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

// The same fields as masks over TextAttr.value.
#define TEXT_ATTR_BG 0x000f
#define TEXT_ATTR_DIM 0x00f0
#define TEXT_ATTR_FG 0x0f00
#define TEXT_ATTR_OPAQUE 0x1000

// A plane's starting attribute, normal attribute and accent attribute.
#define TEXT_ATTR_DEFAULT 0x0400
#define TEXT_ATTR_NORMAL 0x0700
#define TEXT_ATTR_ACCENT 0x2540

// Set: the cell's glyph is drawn 8 pixels wide instead of 16.
#define TEXT_ATTR_HALF_WIDTH 0x8000

#define GetTextGlyphWidth(attr) (((attr) & TEXT_ATTR_HALF_WIDTH) ? 8 : 16)

void SpreadLowNibble(TextAttr* attr);
u16 SwapOuterNibbles(u16 value);
u16 SwapMiddleNibbles(u16 value);
u16 SwapLowNibbles(u16 value);

#endif // GITEN_TEXT_TEXTATTR_H
