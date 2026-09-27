#ifndef GITEN_TEXT_TEXTBAND_H
#define GITEN_TEXT_TEXTBAND_H

#include <Enums.h>

// Independent field-message surfaces, positioned by DrawFieldMessage.
// clang-format off
GZ_ENUM_BEGIN(TextBand)
    TEXT_BAND_LEFT = -1,
    TEXT_BAND_CENTER = 0,
    TEXT_BAND_RIGHT = 1
GZ_ENUM_END(TextBand);
// clang-format on

#endif // GITEN_TEXT_TEXTBAND_H
