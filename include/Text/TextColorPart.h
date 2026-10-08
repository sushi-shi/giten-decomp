#ifndef GITEN_TEXT_TEXTCOLORPART_H
#define GITEN_TEXT_TEXTCOLORPART_H

#include <EnumDomain.h>
#include <Ints.h>

// The colours SetTextPlaneColor sets: the glyph, the dim colour and the
// background.
GZ_ENUM_BEGIN_SPLIT(TextColorPart, i16)
    TEXT_COLOR_GLYPH = 0,
    TEXT_COLOR_DIM = 1,
    TEXT_COLOR_BG = 2
GZ_ENUM_END_SPLIT(TextColorPart)

#endif // GITEN_TEXT_TEXTCOLORPART_H
