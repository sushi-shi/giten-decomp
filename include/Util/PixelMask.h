#ifndef GITEN_UTIL_PIXELMASK_H
#define GITEN_UTIL_PIXELMASK_H

#include <Ints.h>

// Pixel bit within a byte, leftmost pixel first.
extern const u8 g_pixelMasks[8];

#define GetPixelMask(index) (g_pixelMasks[(index) & 7])

#endif // GITEN_UTIL_PIXELMASK_H
