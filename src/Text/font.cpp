// The text and screen layer code: the glyph rendering and text planes, the
// text windows, the hotspot highlight, and the screen layers over the 3D view
// with their hit tests, navigation pad, character panel and panel dragging.
// One TU: its .data run (0x46c230..0x46dbe7) holds the text tables with the
// layer tables among them, and its .bss statics interleave the text and layer
// ones (0x490af0..0x490beb); CreateScreenLayer calls operator new, so it is
// C++, with the text API keeping C linkage through the headers.

#include <rva.h>

#include <Gfx/Texture.h>
#include <Giten/Resource.h>
#include <Platform/Com.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>
#include <Platform/WindowsX.h>
#include <Text/FontApi.h>

#include <stdio.h>
#include <string.h>

// The memory DC holding the text font.
DATA(0x00090bd0)
HDC g_fontDC;

// The built-in 8x16 half-width glyphs for codes 0x20-0xdf, one byte per row.
DATA(0x0006c230)
static u8 s_halfWidthGlyphs[192][16] = {
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x6c,
     0x6c,
     0x24,
     0x48,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x28,
     0x28,
     0x28,
     0x7c,
     0x28,
     0x28,
     0x28,
     0xfc,
     0x50,
     0x50,
     0x50,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x38,
     0x54,
     0x54,
     0x30,
     0x18,
     0x14,
     0x54,
     0x54,
     0x38,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x44,
     0xa4,
     0xa8,
     0x48,
     0x10,
     0x10,
     0x24,
     0x2a,
     0x4a,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x30,
     0x48,
     0x48,
     0x30,
     0x30,
     0x52,
     0x8a,
     0x8c,
     0x8c,
     0x72,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x30,
     0x30,
     0x10,
     0x20,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x08,
     0x10,
     0x10,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x10,
     0x10,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x10,
     0x10,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x10,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x92,
     0x54,
     0x38,
     0x10,
     0x38,
     0x54,
     0x92,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0xfe,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x30,
     0x30,
     0x10,
     0x20,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0xfe,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x10,
     0x20,
     0x20,
     0x40,
     0x40,
     0x80,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x30,
     0x70,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x08,
     0x08,
     0x10,
     0x20,
     0x20,
     0x40,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x04,
     0x18,
     0x08,
     0x04,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x08,
     0x18,
     0x18,
     0x28,
     0x28,
     0x28,
     0x48,
     0x48,
     0x7c,
     0x08,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x40,
     0x40,
     0x40,
     0x78,
     0x44,
     0x04,
     0x04,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x40,
     0x78,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x08,
     0x08,
     0x08,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x38,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x44,
     0x3c,
     0x04,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x30,
     0x30,
     0x00,
     0x00,
     0x00,
     0x30,
     0x30,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x30,
     0x30,
     0x00,
     0x00,
     0x00,
     0x30,
     0x30,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x02,
     0x04,
     0x08,
     0x10,
     0x20,
     0x40,
     0x20,
     0x10,
     0x08,
     0x04,
     0x02,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0xfe,
     0x00,
     0x00,
     0xfe,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x20,
     0x10,
     0x08,
     0x04,
     0x02,
     0x04,
     0x08,
     0x10,
     0x20,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x08,
     0x08,
     0x10,
     0x10,
     0x10,
     0x00,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x82,
     0xba,
     0xca,
     0xca,
     0xca,
     0xb4,
     0x80,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x28,
     0x28,
     0x28,
     0x44,
     0x44,
     0x7c,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x78,
     0x44,
     0x44,
     0x44,
     0x44,
     0x78,
     0x44,
     0x44,
     0x44,
     0x44,
     0x78,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x40,
     0x40,
     0x40,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x70,
     0x48,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x48,
     0x70,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x40,
     0x40,
     0x40,
     0x40,
     0x78,
     0x40,
     0x40,
     0x40,
     0x40,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x40,
     0x40,
     0x40,
     0x78,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x40,
     0x5c,
     0x44,
     0x44,
     0x44,
     0x44,
     0x3c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x7c,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x44,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x48,
     0x50,
     0x60,
     0x50,
     0x48,
     0x48,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x6c,
     0x6c,
     0x6c,
     0x54,
     0x54,
     0x54,
     0x54,
     0x54,
     0x54,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x64,
     0x64,
     0x64,
     0x54,
     0x54,
     0x4c,
     0x4c,
     0x4c,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x28,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x28,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x42,
     0x42,
     0x42,
     0x7c,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x28,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x74,
     0x4c,
     0x2c,
     0x3a,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x78,
     0x44,
     0x44,
     0x44,
     0x44,
     0x78,
     0x50,
     0x48,
     0x48,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x20,
     0x10,
     0x08,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x28,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x28,
     0x28,
     0x28,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x54,
     0x54,
     0x54,
     0x54,
     0x54,
     0x54,
     0x6c,
     0x6c,
     0x6c,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x28,
     0x28,
     0x10,
     0x10,
     0x10,
     0x28,
     0x28,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x28,
     0x28,
     0x28,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x08,
     0x08,
     0x08,
     0x10,
     0x20,
     0x20,
     0x20,
     0x40,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x20,
     0x20,
     0x20,
     0xfc,
     0x20,
     0x20,
     0x20,
     0x2c,
     0x32,
     0x22,
     0x22,
     0x22,
     0x22,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0xfe,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x18,
     0x18,
     0x10,
     0x08,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x04,
     0x3c,
     0x44,
     0x44,
     0x3c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x78,
     0x44,
     0x44,
     0x44,
     0x78,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x40,
     0x40,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x3c,
     0x44,
     0x44,
     0x44,
     0x3c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x7c,
     0x40,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x0c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x3c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x04,
     0x38,
     0x48,
     0x70,
     0x40,
     0x78,
     0x44,
     0x44,
     0x38},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x58,
     0x64,
     0x44,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x08,
     0x00,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x48,
     0x30},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x48,
     0x50,
     0x60,
     0x50,
     0x50,
     0x48,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x68,
     0x54,
     0x54,
     0x54,
     0x54,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x40,
     0x78,
     0x64,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x44,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x78,
     0x44,
     0x44,
     0x44,
     0x78,
     0x40,
     0x40,
     0x40},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x3c,
     0x44,
     0x44,
     0x44,
     0x3c,
     0x04,
     0x04,
     0x04},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x58,
     0x64,
     0x40,
     0x40,
     0x40,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x44,
     0x30,
     0x08,
     0x44,
     0x38,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x38,
     0x10,
     0x10,
     0x10,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x44,
     0x44,
     0x3c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x28,
     0x28,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x54,
     0x54,
     0x54,
     0x7c,
     0x28,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x44,
     0x28,
     0x10,
     0x28,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x28,
     0x18,
     0x10,
     0x10,
     0x60},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x7c,
     0x08,
     0x10,
     0x20,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x1c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x30,
     0x10,
     0x10,
     0x10,
     0x10,
     0x1c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x70,
     0x10,
     0x10,
     0x10,
     0x10,
     0x18,
     0x10,
     0x10,
     0x10,
     0x10,
     0x70,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x34,
     0x4c,
     0x48,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x28,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x1c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x70,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x20,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x18,
     0x18,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x7c,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x18,
     0x10,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x0c,
     0x18,
     0x70,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x10,
     0x7c,
     0x44,
     0x04,
     0x08,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x7c,
     0x10,
     0x10,
     0x7c,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x08,
     0x7c,
     0x08,
     0x18,
     0x68,
     0x18,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x20,
     0x7c,
     0x24,
     0x28,
     0x20,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x38,
     0x08,
     0x08,
     0x7c,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x78,
     0x08,
     0x78,
     0x08,
     0x78,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x54,
     0x54,
     0x04,
     0x08,
     0x30,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x80,
     0x7e,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x14,
     0x14,
     0x18,
     0x10,
     0x10,
     0x10,
     0x10,
     0x20,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x08,
     0x10,
     0x70,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x7c,
     0x44,
     0x44,
     0x44,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x08,
     0x08,
     0x7c,
     0x08,
     0x08,
     0x18,
     0x18,
     0x28,
     0x28,
     0x48,
     0x18,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x7c,
     0x14,
     0x14,
     0x14,
     0x14,
     0x24,
     0x24,
     0x2c,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x1c,
     0x70,
     0x10,
     0x1c,
     0x70,
     0x08,
     0x08,
     0x08,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x3c,
     0x24,
     0x24,
     0x44,
     0x44,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x20,
     0x3c,
     0x28,
     0x48,
     0x48,
     0x08,
     0x08,
     0x10,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x28,
     0x28,
     0x7c,
     0x28,
     0x28,
     0x28,
     0x28,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x64,
     0x14,
     0x04,
     0x64,
     0x14,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x18,
     0x18,
     0x24,
     0x24,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x20,
     0x20,
     0x7c,
     0x24,
     0x24,
     0x28,
     0x20,
     0x20,
     0x20,
     0x1c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x24,
     0x24,
     0x24,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x3c,
     0x24,
     0x24,
     0x74,
     0x4c,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x18,
     0x70,
     0x10,
     0x10,
     0x7c,
     0x10,
     0x10,
     0x20,
     0x20,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x54,
     0x54,
     0x54,
     0x54,
     0x54,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x00,
     0x00,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x20,
     0x20,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x60,
     0x70,
     0x48,
     0x44,
     0x40,
     0x40,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x20,
     0x20,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x78,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x48,
     0x28,
     0x10,
     0x18,
     0x28,
     0x24,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x7c,
     0x04,
     0x08,
     0x10,
     0x38,
     0x54,
     0x10,
     0x10,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x10,
     0x20,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x08,
     0x28,
     0x28,
     0x28,
     0x28,
     0x28,
     0x28,
     0x24,
     0x44,
     0x44,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x4c,
     0x70,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x3c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x30,
     0x28,
     0x28,
     0x48,
     0x48,
     0x04,
     0x04,
     0x04,
     0x04,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x7c,
     0x10,
     0x10,
     0x58,
     0x54,
     0x54,
     0x54,
     0x54,
     0x10,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x48,
     0x38,
     0x30,
     0x10,
     0x10,
     0x08,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x60,
     0x18,
     0x04,
     0x00,
     0x60,
     0x18,
     0x00,
     0x00,
     0x60,
     0x18,
     0x04,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x10,
     0x10,
     0x10,
     0x10,
     0x20,
     0x28,
     0x28,
     0x28,
     0x4c,
     0x74,
     0x04,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x04,
     0x04,
     0x04,
     0x04,
     0x28,
     0x18,
     0x18,
     0x08,
     0x14,
     0x24,
     0x40,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x7c,
     0x10,
     0x10,
     0x10,
     0x10,
     0x10,
     0x0c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x20,
     0x20,
     0x3c,
     0x64,
     0x24,
     0x28,
     0x20,
     0x10,
     0x10,
     0x08,
     0x08,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x3c,
     0x04,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x08,
     0x08,
     0x08,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x04,
     0x3c,
     0x04,
     0x04,
     0x04,
     0x04,
     0x7c,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x00,
     0x00,
     0x7c,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x28,
     0x28,
     0x28,
     0x28,
     0x28,
     0x28,
     0x2a,
     0x2a,
     0x2a,
     0x4c,
     0x48,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x40,
     0x44,
     0x44,
     0x48,
     0x50,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x44,
     0x7c,
     0x44,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x7c,
     0x44,
     0x44,
     0x44,
     0x44,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x20,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x40,
     0x24,
     0x14,
     0x04,
     0x04,
     0x04,
     0x04,
     0x08,
     0x08,
     0x10,
     0x60,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x50,
     0x28,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
    {0x00,
     0x00,
     0x00,
     0x30,
     0x48,
     0x48,
     0x30,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00,
     0x00},
};

// Per layer slot: origin, size and whether it gets a second work surface.
DATA(0x0006ce30)
static POINT s_layerOrigin[SCREEN_LAYER_COUNT] = {
    {0, 0},
    {560, 80},
    {4, 4},
    {40, 6},
    {108, 6},
    {532, 4},
    {4, 80},
    {536, 216},
    {16, 336},
    {224, 336},
    {432, 336},
    {16, 408},
    {224, 408},
    {432, 408},
    {0, 328},
};

DATA(0x0006cea8)
static SIZE
s_layerSize[SCREEN_LAYER_COUNT] = {
    {640, 32},
    {80, 192},
    {36, 36},
    {64, 32},
    {216, 32},
    {104, 56},
    {128, 128},
    {96, 96},
    {192, 64},
    {192, 64},
    {192, 64},
    {192, 64},
    {192, 64},
    {192, 64},
    {640, 152},
};

DATA(0x0006cf20)
static b32 s_layerHasWorkSurface[SCREEN_LAYER_COUNT] =
    {false, false, true, true, true, true, true, false, true, true, true, true, true, true, false};

DATA(0x0006cf60)
static BITMAPINFO s_glyphInfo = {{sizeof(BITMAPINFOHEADER), 16, 16, 1, 24}};

DATA(0x0006cf90)
static TextColor s_textPalette[16] = {
    {0x00, 0x00, 0x00},
    {0x80, 0x00, 0x00},
    {0x00, 0x80, 0x00},
    {0x80, 0x80, 0x00},
    {0xff, 0xff, 0xff},
    {0xff, 0x00, 0x00},
    {0x00, 0xff, 0x00},
    {0xff, 0xff, 0x00},
    {0x00, 0x00, 0x80},
    {0x80, 0x00, 0x80},
    {0x00, 0x80, 0x80},
    {0x80, 0x80, 0x80},
    {0x40, 0x40, 0x40},
    {0x00, 0x00, 0xff},
    {0xff, 0x00, 0xff},
    {0x00, 0xff, 0xff},
};

DATA(0x0006cfc0)
static MAT2 s_identityMatrix = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};

// The glyph-buffer offset of a full-width glyph's first row, by the glyph
// origin's y.
DATA(0x0006cfd0)
static i32 s_glyphRowOffset[24] = {28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2};

// Renders `code` as a 16x16 one-bit glyph (two bytes per row) into `glyph`.
RVA(0x00051230, 0x113)
u8* RenderGlyph(u16 code, u8* glyph) {
    u8* src;
    GLYPHMETRICS metrics;
    char bits[128];
    i32 i;
    i32 j;

    memset(bits, 0, sizeof(bits));
    if (code >= 0x20 && code <= 0xdf) {
        src = s_halfWidthGlyphs[code - 0x20];
        for (i = 0; i < 16; i++) {
            glyph[i * 2] = src[i];
            glyph[i * 2 + 1] = 0;
        }
    } else {
#ifdef GITEN_COMPAT
        // @bug Retail passes 64 of the buffer's 128 bytes and ignores the result. GDI
        // fails for an empty glyph (U+3000) or a bitmap larger than the buffer
        // (more than 16 rows; 32 with the whole buffer) and leaves `metrics` unset,
        // so the row lookup below indexed with stack garbage. Cleared metrics
        // render the cleared `bits` as a blank glyph and keep the underline.
        if (GetGlyphOutline(
                g_fontDC,
                code,
                GGO_BITMAP,
                &metrics,
                sizeof(bits),
                bits,
                &s_identityMatrix
            )
            == GDI_ERROR) {
            memset(&metrics, 0, sizeof(metrics));
        }
#else
        GetGlyphOutline(g_fontDC, code, GGO_BITMAP, &metrics, 64, bits, &s_identityMatrix);
#endif
        if (metrics.gmBlackBoxX <= 7) {
            for (i = 0; i < 64; i += 4) {
                char carry = bits[i] & 0x0f;
                bits[i] = (bits[i] >> 4) & 0x0f;
                bits[i + 1] = (carry << 4) | (bits[i + 1] >> 4);
            }
        }
        if (code != SJIS_LOW_LINE) {
#ifdef GITEN_COMPAT
            // @bug Retail indexes the table with the glyph top unchecked. MS Gothic
            // keeps the top within it; another font need not, and a top outside
            // the table reads a row offset from neighbouring memory that the rows
            // are then copied at.
            const LONG tops = sizeof(s_glyphRowOffset) / sizeof(s_glyphRowOffset[0]);
            if (metrics.gmptGlyphOrigin.y < 0) {
                metrics.gmptGlyphOrigin.y = 0;
            } else if (metrics.gmptGlyphOrigin.y >= tops) {
                metrics.gmptGlyphOrigin.y = tops - 1;
            }
#endif
            for (i = s_glyphRowOffset[metrics.gmptGlyphOrigin.y], j = 0; i < 30; i += 2, j += 4) {
                glyph[i + 2] = bits[j];
                glyph[i + 3] = bits[j + 1];
            }
        } else {
            glyph[30] = glyph[31] = 0xff;
        }
    }
    return glyph;
}

// Blits a 16x16 one-bit glyph at (x, y): the cell fills with the attribute's
// background colour, and each set pixel takes the glyph colour with a
// half-bright shadow to its right.
RVA(0x00051350, 0x1ec)
void BlitGlyph(i32 x, i32 y, u16 attr, HDC dc, u8* glyph, i32 width) {
    RGBTRIPLE pixels[256];
    u8 inkR;
    u8 inkG;
    u8 inkB;
    u8 shadeR;
    u8 shadeG;
    u8 shadeB;
    i32 background;
    u16 bg;
    u16 fg;
    i32 row;
    i32 bit;
    i32 i;

    background = attr & TEXT_ATTR_BG;
    bg = background;
    if (!(attr & TEXT_ATTR_OPAQUE) && bg == 0) {
        memset(pixels, 0, sizeof(pixels));
    } else {
        RGBTRIPLE fill;
        fill.rgbtRed = s_textPalette[bg].r;
        fill.rgbtGreen = s_textPalette[bg].g;
        fill.rgbtBlue = s_textPalette[bg].b;
        for (i = 0; i < 256; i++) {
            pixels[i] = fill;
        }
    }
    fg = (attr & TEXT_ATTR_FG) >> 8;
    inkR = s_textPalette[fg].r;
    inkG = s_textPalette[fg].g;
    inkB = s_textPalette[fg].b;
    if (fg == 0) {
        if (attr & TEXT_ATTR_DIM) {
            if (bg == 0) {
                inkR = shadeR = s_textPalette[TEXT_COLOR_GREY].r;
                inkG = shadeG = s_textPalette[TEXT_COLOR_GREY].g;
                inkB = shadeB = s_textPalette[TEXT_COLOR_GREY].b;
            } else {
                shadeR = inkR >> 1;
                shadeG = inkG >> 1;
                shadeB = inkB >> 1;
            }
        } else if (bg == 0) {
            inkR = s_textPalette[TEXT_COLOR_WHITE].r;
            inkG = s_textPalette[TEXT_COLOR_WHITE].g;
            inkB = s_textPalette[TEXT_COLOR_WHITE].b;
            shadeR = s_textPalette[TEXT_COLOR_GREY].r;
            shadeG = s_textPalette[TEXT_COLOR_GREY].g;
            shadeB = s_textPalette[TEXT_COLOR_GREY].b;
        } else {
            shadeR = inkR >> 1;
            shadeG = inkG >> 1;
            shadeB = inkB >> 1;
        }
    } else {
        shadeR = inkR >> 1;
        shadeG = inkG >> 1;
        shadeB = inkB >> 1;
    }
    for (row = 0; row < 32; row++) {
        for (bit = 0; bit < 8; bit++) {
            if ((glyph[row] >> (7 - bit)) & 1) {
                i = ((15 - row / 2) * 2 + (row & 1)) * 8 + bit;
                pixels[i].rgbtBlue = inkB;
                pixels[i].rgbtGreen = inkG;
                pixels[i].rgbtRed = inkR;
                i++;
                pixels[i].rgbtBlue = shadeB;
                pixels[i].rgbtGreen = shadeG;
                pixels[i].rgbtRed = shadeR;
            }
        }
    }
    StretchDIBits(
        dc,
        x,
        y,
        width,
        16,
        0,
        0,
        width,
        16,
        pixels,
        &s_glyphInfo,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

// The font's DC holds a 1x1 scratch surface's; the font is 17-pixel thin
// modern type.
DATA(0x00090af0)
static HFONT s_glyphFont;

DATA(0x00090af4)
static LPDIRECTDRAWSURFACE s_glyphSurface;

// Creates the glyph scratch surface and selects the text font into its DC
// (g_fontDC); FALSE on failure.
RVA(0x00051540, 0xbd)
b32 CreateGlyphSurface(void) {
    DDSURFACEDESC desc;

    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.dwWidth = 1;
    desc.dwHeight = 1;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    if (g_ddraw->CreateSurface(&desc, &s_glyphSurface, NULL) != DD_OK) {
        return false;
    }
    s_glyphFont = CreateFontA(
        17,
        0,
        0,
        0,
        FW_THIN,
        false,
        false,
        false,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_EMBEDDED,
        DEFAULT_QUALITY,
        FF_MODERN,
        NULL
    );
    if (s_glyphSurface->GetDC(&g_fontDC) != DD_OK) {
        return false;
    }
    SelectFont(g_fontDC, s_glyphFont);
    return true;
}

// Frees the text font and the glyph scratch surface.
RVA(0x00051600, 0x41)
void FreeGlyphSurface(void) {
    if (s_glyphSurface != NULL) {
        DeleteFont(s_glyphFont);
        s_glyphSurface->ReleaseDC(g_fontDC);
        ReleaseComObject(s_glyphSurface);
    }
}

// Draws `text` (Shift-JIS; tabs and newlines skipped) with attribute `attr`
// at pixel (x, y) of layer `layer`'s work surface.
// API-forced: the public text is char*; the decoder reads its encoded bytes.
#define ReadStringTextChar(code, text, pos)                                                        \
    ReadTextChar((code), reinterpret_cast<const u8*>(text), (pos))
RVA(0x00051650, 0xf8)
void DrawLayerText(i16 layer, i16 x, i16 y, const char* text, i32 attr) {
    HDC dc;
    u8 glyph[32];
    i16 pos;
    i16 next;

    g_screenLayers[layer]->canvas->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    pos = 0;
    while (text[pos] != 0) {
        u16 code;

        next = ReadStringTextChar(&code, text, pos);
        memset(glyph, 0, sizeof(glyph));
        if (code != '\t' && code != '\n') {
            BlitGlyph(x, y, attr, dc, RenderGlyph(code, glyph), GetTextGlyphWidth(attr));
        }
        x += 8;
        if (next - pos == SJIS_WIDE_BYTES) {
            x += 8;
        }
        pos = next;
    }
    g_screenLayers[layer]->canvas->ReleaseDC(dc);
}

// The same onto text plane `plane`'s glyph surface.
RVA(0x00051750, 0xfb)
void DrawPlaneText(i16 plane, i16 x, i16 y, const char* text, i32 attr) {
    HDC dc;
    u8 glyph[32];
    i16 pos;
    i16 next;

    GetTextPlane(plane)->glyphSurface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    pos = 0;
    while (text[pos] != 0) {
        u16 code;

        next = ReadStringTextChar(&code, text, pos);
        memset(glyph, 0, sizeof(glyph));
        if (code != '\t' && code != '\n') {
            BlitGlyph(x, y, attr, dc, RenderGlyph(code, glyph), GetTextGlyphWidth(attr));
        }
        x += 8;
        if (next - pos == SJIS_WIDE_BYTES) {
            x += 8;
        }
        pos = next;
    }
    GetTextPlane(plane)->glyphSurface->ReleaseDC(dc);
}

// The same onto the status picture, at cell (x, y) (8-pixel cells).
RVA(0x00051850, 0xfa)
void DrawStatusText(i16 x, i16 y, const char* text, i32 attr) {
    HDC dc;
    u8 glyph[32];
    i16 pos;
    i16 next;

    x *= 8;
    y *= 8;
    g_statusPicture.surface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    pos = 0;
    while (text[pos] != 0) {
        u16 code;

        next = ReadStringTextChar(&code, text, pos);
        memset(glyph, 0, sizeof(glyph));
        if (code != '\t' && code != '\n') {
            BlitGlyph(x, y, attr, dc, RenderGlyph(code, glyph), GetTextGlyphWidth(attr));
        }
        x += 8;
        if (next - pos == SJIS_WIDE_BYTES) {
            x += 8;
        }
        pos = next;
    }
    g_statusPicture.surface->ReleaseDC(dc);
}

// Draws `text` onto band picture `band` (-1, 0 or 1; cleared first) and
// blits the band at pixel (x, y) of the render target, clipped at the right
// screen edge for band 1. Returns 0.
RVA(0x00051950, 0x1b2)
b16 DrawBandText(i16 x, i16 y, const char* text, i32 attr, i16 band) {
    Picture* picture;
    HDC dc;
    u8 glyph[32];
    i16 pos;
    i16 next;
    i16 px;
    RECT source;

    switch (band) {
        case TEXT_BAND_LEFT:
            picture = &g_leftFieldMessagePicture;
            break;
        case TEXT_BAND_CENTER:
            picture = &g_centerFieldMessagePicture;
            break;
        case TEXT_BAND_RIGHT:
            picture = &g_rightFieldMessagePicture;
            break;
    }
    picture->surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
    px = 0;
    picture->surface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    pos = 0;
    while (text[pos] != 0) {
        u16 code;

        next = ReadStringTextChar(&code, text, pos);
        memset(glyph, 0, sizeof(glyph));
        if (code != '\t' && code != '\n') {
            BlitGlyph(px, 0, attr, dc, RenderGlyph(code, glyph), GetTextGlyphWidth(attr));
        }
        px += 8;
        if (next - pos == SJIS_WIDE_BYTES) {
            px += 8;
        }
        pos = next;
    }
    picture->surface->ReleaseDC(dc);
    if (band == TEXT_BAND_RIGHT) {
        source = picture->rect;
        source.right = min(SCREEN_WIDTH - x, source.right);
        g_renderTarget->BltFast(x, y, picture->surface, &source, DDBLTFAST_SRCCOLORKEY);
        return false;
    }
    g_renderTarget->BltFast(x, y, picture->surface, &picture->rect, DDBLTFAST_SRCCOLORKEY);
    return false;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x00051b10, 0x4)
b16 ReturnZero(void) {
    return false;
}

// Repaints text plane `plane`'s newline cells, a row per line; every other
// non-tab character moves the pen to the next pixel row block.
// @identity-TODO: why only newline cells get a glyph is unexplained; `mode`
// is not read.
RVA(0x00051b20, 0x188)
void RepaintTextPlane(i16 plane, i16 mode) {
    TextPlane* p;
    HDC dc;
    u8 glyph[32];
    u16 code;
    i32 px;
    i32 py;
    i32 row;
    i32 col;
    i16 next;
    u8* text;
    TextAttr* attrs;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    px = 0;
    if (plane == 0) {
        GetTextPlane(0)->visible = true;
    }
    p = GetTextPlane(plane);
    p->glyphSurface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    py = 0;
    for (row = 0; row < p->rows; row++, py += 16) {
        text = TextPlaneTextRow(p, row);
        attrs = TextPlaneAttrRow(p, row);
        for (col = 0; col < p->cols; col++) {
            next = ReadTextChar(&code, text, col);
            memset(glyph, 0, sizeof(glyph));
            if (code != '\t') {
                if (code != '\n') {
                    py += 16;
                    px = 0;
                } else {
                    BlitGlyph(
                        px + 8,
                        py + 8,
                        attrs[col].value,
                        dc,
                        RenderGlyph('\n', glyph),
                        GetTextGlyphWidth(attrs[col].value)
                    );
                }
            }
            px += 8;
            if (next - col == SJIS_WIDE_BYTES) {
                px += 8;
            }
            col = next;
        }
    }
    p->glyphSurface->ReleaseDC(dc);
}

RVA(0x00051cb0, 0xc1)
i16 DrawTextCell(i16 plane, u16 code, u16 attr, i16 px, i16 y) {
    HDC dc;
    u8 glyph[32];
    i32 width;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    if (plane == 0) {
        GetTextPlane(plane)->visible = true;
    }
    GetTextPlane(plane)->glyphSurface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    memset(glyph, 0, sizeof(glyph));
    width = IsTwoByteTextChar(code) ? 16 : 8;
    y *= 16;
    BlitGlyph(px + 8, y + 8, attr, dc, RenderGlyph(code, glyph), width);
    GetTextPlane(plane)->glyphSurface->ReleaseDC(dc);
    return px + width;
}

RVA(0x00051d80, 0xb4)
void RedrawTextPlane(i16 plane) {
    TextPlane* p;
    i16 y;
    i16 x;
    i16 px;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    for (y = 0; y < p->rows; y++) {
        px = 0;
        for (x = 0; x < p->cols;) {
            DrawNextTextCell(plane, p, x, y, px);
        }
    }
}

// @early-stop register/scheduling residue: pixel x occupies the dead x
// parameter home and the plane pointer occupies ebp, as in retail. The
// pixel initialization precedes the guard here; retail schedules it after
// the plane lookup, but moving it there hands ebp to the pixel and spills the
// plane pointer. The attribute and next-position registers are swapped.
RVA(0x00051e40, 0xa6)
void RedrawTextRun(i16 plane, i16 x, i16 y, i16 count) {
    TextPlane* p;
    i16 px = x * 8;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    count += x;
    while (x < count) {
        DrawNextTextCell(plane, p, x, y, px);
    }
}

RVA(0x00051ef0, 0xa0)
i16 ToggleTextRunHighlight(i16 plane, i16 x, i16 y) {
    TextPlane* p;
    i16 i;
    TextAttr* row;
    u16 attr;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    p = GetTextPlane(plane);
    for (i = x; i < p->cols; i++) {
        if (TextPlaneTextRow(p, y)[i] == 0) {
            return i - x;
        }
        row = TextPlaneAttrRow(p, y);
        attr = row[i].value;
        if (p->flags.highlight == TEXT_HIGHLIGHT_MIDDLE) {
            attr = SwapMiddleNibbles(attr);
        } else if (p->flags.highlight == TEXT_HIGHLIGHT_OUTER) {
            attr = SwapOuterNibbles(attr);
        } else {
            attr = SwapLowNibbles(attr);
        }
        row[i].value = attr;
    }
    return i - x;
}

RVA(0x00051f90, 0x6b)
i16 SetTextRunAttr(i16 plane, i16 x, i16 y, u16 attr) {
    TextPlane* p;
    i16 i;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    p = GetTextPlane(plane);
    for (i = x; i < p->cols; i++) {
        if (TextPlaneTextRow(p, y)[i] == 0) {
            break;
        }
        TextPlaneAttrRow(p, y)[i].value = attr;
    }
    return i - x;
}

// Swaps the glyph and background colours of `count` cells from (x, y).
RVA(0x00052000, 0x52)
void ReverseTextRun(i16 plane, i16 x, i16 y, i16 count) {
    TextAttr* row;
    i16 i;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    row = TextPlaneAttrRow(GetTextPlane(plane), y);
    for (i = 0; i < count; i++) {
        row[x + i].value = SwapOuterNibbles(row[x + i].value);
    }
}

// Paints `count` cells from (x, y) in their background colour.
RVA(0x00052060, 0x4c)
void BlankTextRun(i16 plane, i16 x, i16 y, i16 count) {
    TextAttr* row;
    i16 i;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    row = TextPlaneAttrRow(GetTextPlane(plane), y);
    for (i = 0; i < count; i++) {
        SpreadLowNibble(&row[x + i]);
    }
}

// The plane kinds' windows: screen position, size and text grid.
DATA(0x0006d030)
static TextPlaneLayout s_planeLayouts[TEXT_PLANE_COUNT] = {
    {0, 328, 640, 152, 76, 6},   {0, 358, 640, 80, 76, 4},    {104, 248, 432, 80, 52, 4},
    {0, 30, 640, 288, 76, 16},   {10, 30, 304, 288, 34, 16},  {40, 40, 196, 168, 24, 9},
    {416, 88, 208, 208, 24, 12}, {334, 30, 288, 80, 32, 4},   {334, 140, 288, 80, 32, 4},
    {208, 56, 416, 200, 52, 11}, {10, 30, 280, 288, 34, 16},  {334, 250, 288, 80, 32, 4},
    {0, 358, 640, 80, 76, 4},    {0, 0, 0, 0, 0, 0},          {104, 232, 432, 96, 52, 5},
    {120, 264, 400, 64, 48, 3},  {416, 116, 208, 112, 24, 6}, {64, 272, 504, 176, 10, 10},
    {24, 80, 288, 136, 34, 11},  {160, 312, 456, 116, 54, 7}, {0, 0, 0, 0, 0, 0},
    {464, 56, 152, 272, 18, 16}, {0, 0, 0, 0, 0, 0},          {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},          {16, 48, 288, 184, 34, 10},  {56, 40, 528, 288, 64, 16},
    {0, 0, 0, 0, 0, 0},          {416, 48, 176, 208, 20, 12}, {0, 0, 0, 0, 0, 0},
    {130, 32, 352, 184, 42, 9},  {0, 0, 640, 328, 0, 0},      {72, 264, 496, 112, 60, 6},
    {304, 48, 256, 184, 30, 10}, {480, 32, 144, 240, 16, 14}, {490, 32, 136, 160, 15, 1},
    {112, 384, 416, 80, 50, 4},
};

// Creates a text plane of kind `kind` in the first free slot: its window
// surface painted with the kind's frame image (the analyze name plate's own
// image while exploring), a keyed glyph surface, the text grid and its
// defaults. Returns the slot; -1 when no slot is free or a surface fails, 0
// for kind 0 while plane 0 is open.
RVA(0x000520b0, 0x343)
i16 CreateTextPlane(u16 kind, i16 arg) {
    DDSURFACEDESC primary;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    i32 plane;
    TextPlane* p;
    i32 i;

    for (plane = 0; plane < TEXT_PLANE_COUNT; plane++) {
        if (GetTextPlane(plane)->kind == TEXT_PLANE_FREE) {
            break;
        }
    }
    if (plane == TEXT_PLANE_COUNT) {
        return TEXT_PLANE_NONE;
    }
    if (kind == 0 && GetTextPlane(0)->surface != NULL) {
        return 0;
    }
    p = GetTextPlane(plane);
    primary.dwSize = sizeof(primary);
    primary.dwFlags = DDSD_ALL;
    if (g_primarySurface->GetSurfaceDesc(&primary) != DD_OK) {
        return 0;
    }
    InitOffscreenSurfaceDesc(
        desc,
        s_planeLayouts[kind].width,
        s_planeLayouts[kind].height,
        primary.ddpfPixelFormat
    );
    if (g_ddraw->CreateSurface(&desc, &p->surface, NULL) != DD_OK) {
        return TEXT_PLANE_NONE;
    }
    if (kind == TEXT_PLANE_KIND_ANALYZE_NAME && GetFieldBattleActive()) {
        if (!BlitImage(p->surface, ANALYZE_NAME_EXPLORING_IMAGE, 0, 0)) {
            ReleaseComObject(p->surface);
            return TEXT_PLANE_NONE;
        }
    } else if (!BlitImage(p->surface, g_textPlaneImages[kind], 0, 0)) {
        ReleaseComObject(p->surface);
        return TEXT_PLANE_NONE;
    }
    if (g_ddraw->CreateSurface(&desc, &p->glyphSurface, NULL) != DD_OK) {
        ReleaseComObject(p->surface);
        return TEXT_PLANE_NONE;
    }
    ZeroMemory(&key, sizeof(key));
    p->glyphSurface->SetColorKey(DDCKEY_SRCBLT, &key);
    p->text = new u8*[TEXT_PLANE_MAX_ROWS];
    p->attrs = new TextAttr*[TEXT_PLANE_MAX_ROWS];
    for (i = 0; i < TEXT_PLANE_MAX_ROWS; i++) {
        p->text[i] = new u8[TEXT_PLANE_MAX_COLS];
        memset(p->text[i], 0, TEXT_PLANE_MAX_COLS);
        p->attrs[i] = new TextAttr[TEXT_PLANE_MAX_COLS];
        memset(p->attrs[i], 0, TEXT_PLANE_MAX_COLS * sizeof(TextAttr));
    }
    p->arg = arg;
    p->kind = kind;
    p->cols = s_planeLayouts[kind].cols;
    p->rows = s_planeLayouts[kind].rows;
    p->cursorX = 0;
    p->cursorY = 0;
    p->indent = 0;
    if (kind == TEXT_PLANE_KIND_KEYPAD) {
        p->indent = 1;
    }
    p->lineStep = 1;
    p->savedAttr.value = TEXT_ATTR_DEFAULT;
    p->attr.value = TEXT_ATTR_DEFAULT;
    p->headerRows = 0;
    p->normalAttr.value = TEXT_ATTR_NORMAL;
    p->accentAttr.value = TEXT_ATTR_ACCENT;
    p->flags.cancelEnabled = 0;
    p->flags.indentEnabled = true;
    p->flags.savedIndentEnabled = 1;
    p->firstSelectableRow = 1;
    p->highlightY = TEXT_MENU_HIGHLIGHT_NONE;
    p->highlightX = TEXT_MENU_HIGHLIGHT_NONE;
    if (kind == TEXT_PLANE_KIND_TWO_COLUMN_MENU) {
        p->flags.twoColumns = true;
    } else {
        p->flags.twoColumns = false;
    }
    p->flags.highlight = TEXT_HIGHLIGHT_MIDDLE;
    p->flags.flag8 = 0;
    p->visible = kind != 0;
    p->left = s_planeLayouts[kind].left;
    p->menuLines = NULL;
    p->sourceRect.top = 0;
    p->sourceRect.left = 0;
    p->top = s_planeLayouts[kind].top;
    p->sourceRect.right = s_planeLayouts[kind].width;
    p->sourceRect.bottom = s_planeLayouts[kind].height;
    return plane;
}

RVA(0x00052400, 0x23)
void ResetTextPlanes(void) {
    int i;

    for (i = 0; i < TEXT_PLANE_COUNT; i++) {
        GetTextPlane(i)->kind = TEXT_PLANE_FREE;
        GetTextPlane(i)->visible = false;
        GetTextPlane(i)->surface = NULL;
        GetTextPlane(i)->glyphSurface = NULL;
    }
}

// Frees text plane `plane`: its rows, surfaces and record, which becomes a
// free slot. Returns -1.
RVA(0x00052430, 0xd3)
i16 FreeTextPlane(u16 plane) {
    i32 i;

    if (GetTextPlane(plane)->surface == NULL) {
        return -1;
    }
    for (i = 0; i < TEXT_PLANE_MAX_ROWS; i++) {
        delete GetTextPlane(plane)->text[i];
        delete GetTextPlane(plane)->attrs[i];
    }
    GetTextPlane(plane)->text = static_cast<u8**>(FreeBlock(GetTextPlane(plane)->text));
    GetTextPlane(plane)->attrs = static_cast<TextAttr**>(FreeBlock(GetTextPlane(plane)->attrs));
    ReleaseComObject(GetTextPlane(plane)->surface);
    ReleaseComObject(GetTextPlane(plane)->glyphSurface);
    memset(GetTextPlane(plane), 0, sizeof(TextPlane));
    GetTextPlane(plane)->kind = TEXT_PLANE_FREE;
    GetTextPlane(plane)->visible = false;
    return -1;
}

// Closes a window; window 0 stays allocated and is hidden.
RVA(0x00052510, 0x28)
i16 CloseTextWindow(i16 window) {
    if (window == TEXT_PLANE_NONE) {
        return window;
    }
    if (window == 0) {
        GetTextPlane(0)->visible = false;
        return window;
    }
    return FreeTextPlane(window);
}

RVA(0x00052540, 0x22)
i32 GetTextPlaneAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->attr.value;
    }
    return 0;
}

RVA(0x00052570, 0x2c)
u16 SetTextPlaneAttr(i16 plane, u16 attr) {
    u16 old = 0;

    if (plane != TEXT_PLANE_NONE) {
        old = GetTextPlane(plane)->attr.value;
        GetTextPlane(plane)->attr.value = attr;
    }
    return old;
}

RVA(0x000525a0, 0x1f)
void ResetTextPlaneNormalAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->normalAttr.value = TEXT_ATTR_NORMAL;
    }
}

RVA(0x000525c0, 0x1f)
void ResetTextPlaneAccentAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->accentAttr.value = TEXT_ATTR_ACCENT;
    }
}

RVA(0x000525e0, 0x2d)
void SaveTextPlaneAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        SaveCurrentTextAttr(GetTextPlane(plane));
    }
}

RVA(0x00052610, 0x42)
void SaveAndResetTextPlaneAttrs(i16 plane) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    SaveCurrentTextAttr(p);
    p->attr.value = TEXT_ATTR_DEFAULT;
    p->normalAttr.value = TEXT_ATTR_NORMAL;
    p->accentAttr.value = TEXT_ATTR_ACCENT;
}

RVA(0x00052660, 0x36)
void RestoreTextPlaneAttr(i16 plane) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    p->attr = p->savedAttr;
    p->flags.attrSaved = 0;
    p->savedAttr.value = 0;
}

RVA(0x000526a0, 0x1d)
void ForgetTextPlaneAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->flags.attrSaved = 0;
    }
}

RVA(0x000526c0, 0x2a)
i32 IsTextPlaneAttrSaved(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->flags.attrSaved;
    }
    return 0;
}

RVA(0x000526f0, 0x2c)
u16 SetTextPlaneNormalAttr(i16 plane, u16 attr) {
    u16 old = 0;

    if (plane != TEXT_PLANE_NONE) {
        old = GetTextPlane(plane)->normalAttr.value;
        GetTextPlane(plane)->normalAttr.value = attr;
    }
    return old;
}

RVA(0x00052720, 0x2c)
u16 SetTextPlaneAccentAttr(i16 plane, u16 attr) {
    u16 old = 0;

    if (plane != TEXT_PLANE_NONE) {
        old = GetTextPlane(plane)->accentAttr.value;
        GetTextPlane(plane)->accentAttr.value = attr;
    }
    return old;
}

RVA(0x00052750, 0x22)
i32 GetTextPlaneNormalAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->normalAttr.value;
    }
    return 0;
}

RVA(0x00052780, 0x22)
i32 GetTextPlaneAccentAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->accentAttr.value;
    }
    return 0;
}

RVA(0x000527b0, 0x1f)
void ResetTextPlaneAttr(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->attr.value = TEXT_ATTR_DEFAULT;
    }
}

RVA(0x000527d0, 0x27)
TextAttr* GetTextPlaneAttrRow(i16 plane, i16 y) {
    if (plane != TEXT_PLANE_NONE) {
        return TextPlaneAttrRow(GetTextPlane(plane), y);
    }
    return NULL;
}

RVA(0x00052800, 0x40)
void ReverseTextPlaneAttr(i16 plane) {
    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    GetTextPlane(plane)->attr.value = (GetTextPlane(plane)->attr.value & 0xf0f0)
                                      | ((GetTextPlane(plane)->attr.value >> 8) & 0x0f)
                                      | ((GetTextPlane(plane)->attr.value & 0x0f) << 8);
}

// Sets one colour of the plane's attribute: the glyph, the dim colour or the
// background (TEXT_COLOR_GLYPH/DIM/BG).
// @early-stop operand-order residue: for the dim colour retail shifts the
// colour first and ORs the masked attribute into it; cl canonicalizes both
// spellings to the attribute-first order here. The rest matches.
RVA(0x00052840, 0x67)
void SetTextPlaneColor(i16 plane, i16 which, u16 color) {
    u16 attr;

    color &= 0x0f;
    attr = GetTextPlane(plane)->attr.value;
    switch (which) {
        case TEXT_COLOR_GLYPH:
            attr = (attr & 0xf0ff) | (color << 8);
            break;
        case TEXT_COLOR_DIM:
            attr = (color << 4) | (attr & 0xff0f);
            break;
        case TEXT_COLOR_BG:
            attr = (attr & 0xfff0) | color;
            break;
    }
    GetTextPlane(plane)->attr.value = attr;
}

RVA(0x000528b0, 0x36)
void SetWindowOpaqueBg(i16 window, i32 on) {
    if (window == TEXT_PLANE_NONE) {
        return;
    }
    SetTextAttrFlag(&GetTextPlane(window)->attr, 1, on);
}

RVA(0x000528f0, 0x36)
void SetWindowAttrFlag1(i16 window, i32 on) {
    if (window == TEXT_PLANE_NONE) {
        return;
    }
    SetTextAttrFlag(&GetTextPlane(window)->attr, 2, on);
}

RVA(0x00052930, 0x36)
void SetWindowHalfWidth(i16 window, i32 on) {
    if (window == TEXT_PLANE_NONE) {
        return;
    }
    SetTextAttrFlag(&GetTextPlane(window)->attr, 8, on);
}

RVA(0x00052970, 0x36)
void SetWindowAttrFlag2(i16 window, i32 on) {
    if (window == TEXT_PLANE_NONE) {
        return;
    }
    SetTextAttrFlag(&GetTextPlane(window)->attr, 4, on);
}

RVA(0x000529b0, 0x20)
i32 GetTextPlaneCursorX(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->cursorX;
    }
    return 0;
}

RVA(0x000529d0, 0x20)
i32 GetTextPlaneCursorY(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->cursorY;
    }
    return 0;
}

RVA(0x000529f0, 0x46)
void SetTextPlaneCursor(i16 plane, i16 x, i16 y) {
    TextPlane* p;
    i16 v;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    v = p->cols;
    v = min(x, v);
    p->cursorX = v;
    v = p->rows;
    v = min(y, v);
    p->cursorY = v;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00052a40, 0x34)
void GetTextPlaneCursor(i16 plane, i16* x, i16* y) {
    if (plane != TEXT_PLANE_NONE) {
        *x = GetTextPlane(plane)->cursorX;
        *y = GetTextPlane(plane)->cursorY;
    }
}

RVA(0x00052a80, 0x22)
void MoveTextPlaneCursorX(i16 plane, i16 dx) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->cursorX += dx;
    }
}

RVA(0x00052ab0, 0x20)
i32 GetTextPlaneLineStep(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->lineStep;
    }
    return 0;
}

RVA(0x00052ad0, 0x36)
void SetTextPlaneCursorLine(i16 plane, i16 x, i16 line) {
    SetTextPlaneCursor(
        plane,
        x,
        (plane != TEXT_PLANE_NONE ? GetTextPlane(plane)->lineStep : 0) * line
    );
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
// The line step of `plane` when `on` is set and the plane is open, else 0.
RVA(0x00052b10, 0x2f)
i32 GetTextPlaneLineStepIf(i16 plane, i16 on) {
    if ((plane != TEXT_PLANE_NONE) * on) {
        return GetTextPlane(plane)->lineStep;
    }
    return 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00052b40, 0x39)
i16 TextPlaneRowToLine(i16 plane, i16 row) {
    return row / static_cast<i16>(plane != TEXT_PLANE_NONE ? GetTextPlane(plane)->lineStep : 1);
}

RVA(0x00052b80, 0x2b)
i16 ResetTextPlaneLineStep(i16 plane, i16 step) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->lineStep;
    GetTextPlane(plane)->lineStep = 1;
    return old;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00052bb0, 0x2a)
i32 IsTextPlaneIndentEnabled(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->flags.indentEnabled;
    }
    return 0;
}

RVA(0x00052be0, 0x41)
i16 SetTextPlaneIndentEnabled(i16 plane, i16 on) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->flags.indentEnabled;
    GetTextPlane(plane)->flags.indentEnabled = on;
    return old;
}

RVA(0x00052c30, 0x34)
void SaveTextPlaneIndentMode(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->flags.savedIndentEnabled = GetTextPlane(plane)->flags.indentEnabled;
    }
}

RVA(0x00052c70, 0x34)
void RestoreTextPlaneIndentMode(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->flags.indentEnabled = GetTextPlane(plane)->flags.savedIndentEnabled;
    }
}

RVA(0x00052cb0, 0x20)
i32 GetTextPlaneIndent(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->indent;
    }
    return 0;
}

RVA(0x00052cd0, 0x2e)
i16 SetTextPlaneIndent(i16 plane, i16 indent) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->indent;
    GetTextPlane(plane)->indent = indent;
    return old;
}

RVA(0x00052d00, 0x2b)
i16 GetActiveTextPlaneIndent(i16 plane) {
    if (plane != TEXT_PLANE_NONE && GetTextPlane(plane)->flags.indentEnabled) {
        return GetTextPlaneIndent(plane);
    }
    return 0;
}

// Moves the cursor right to the active indent; returns how far it moved.
RVA(0x00052d30, 0x49)
i16 ApplyTextPlaneIndent(i16 plane) {
    i16 indent;
    i16 x;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    indent = GetActiveTextPlaneIndent(plane);
    x = GetTextPlaneCursorX(plane);
    if (x < indent) {
        SetTextPlaneCursor(plane, indent, GetTextPlaneCursorY(plane));
    }
    return indent - x;
}

RVA(0x00052d80, 0x2a)
i32 IsTextPlaneCancelEnabled(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->flags.cancelEnabled;
    }
    return 0;
}

RVA(0x00052db0, 0x4b)
i16 SetTextPlaneCancelEnabled(i16 plane, i16 on) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->flags.cancelEnabled;
    GetTextPlane(plane)->flags.cancelEnabled = on != false;
    return old;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00052e00, 0x20)
i32 GetTextPlaneFirstSelectableRow(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->firstSelectableRow;
    }
    return 0;
}

RVA(0x00052e20, 0x4b)
i16 SetTextPlaneFirstSelectableRow(i16 plane, i16 pos, i16 inLines) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->firstSelectableRow;
    if (inLines) {
        pos *= GetTextPlane(plane)->lineStep;
    }
    GetTextPlane(plane)->firstSelectableRow = pos;
    return old;
}

RVA(0x00052e70, 0x40)
i16 SetTextPlaneHighlightMode(i16 plane, i16 mode) {
    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    GetTextPlane(plane)->flags.highlight = mode;
    return mode;
}

RVA(0x00052eb0, 0x20)
void ResetTextPlaneHighlight(i16 plane) {
    GetTextPlane(plane)->highlightX =
        GetTextPlane(plane)->highlightY = TEXT_MENU_HIGHLIGHT_NONE;
}

RVA(0x00052ed0, 0x5c)
void ResetTextPlaneMenu(i16 plane, i16 line, i16 cancelEnabled) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    if (cancelEnabled != -1) {
        p->flags.cancelEnabled = cancelEnabled != 0;
    }
    p->highlightY = TEXT_MENU_HIGHLIGHT_NONE;
    p->firstSelectableRow = p->lineStep * line;
    p->highlightX = TEXT_MENU_HIGHLIGHT_NONE;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00052f30, 0x3c)
void ToggleCurrentTextHighlight(i16 plane) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    if (p->highlightX >= 0 && p->highlightY >= 0) {
        ToggleTextHighlight(plane, p->highlightX, p->highlightY);
    }
}

RVA(0x00052f70, 0x4a)
void ClearTextPlaneHighlight(i16 plane) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    if (p->highlightX >= 0 && p->highlightY >= 0) {
        ToggleTextHighlight(plane, p->highlightX, p->highlightY);
    }
    p->highlightY = TEXT_MENU_HIGHLIGHT_NONE;
    p->highlightX = TEXT_MENU_HIGHLIGHT_NONE;
}

RVA(0x00052fc0, 0x42)
void SetTextPlaneHighlight(i16 plane, i16 x, i16 y) {
    TextPlane* p;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    p->highlightX = x;
    p->highlightY = y;
    if (x >= 0 && y >= 0) {
        ToggleTextHighlight(plane, x, y);
    }
}

// The number of text lines the plane shows, at least one.
RVA(0x00053010, 0x66)
i16 GetTextPlanePageLines(i16 plane) {
    i16 rows;
    i16 step;
    i16 lines;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    rows = GetTextPlane(plane)->rows - GetTextPlane(plane)->headerRows;
    step = GetTextPlane(plane)->lineStep;
    if (step == 0) {
        step = 1;
    }
    lines = rows / step;
    if (lines < 1) {
        lines = 1;
    }
    if (static_cast<i16>(rows % step) >= 2) {
        lines++;
    }
    return lines;
}

// Blanks `plane`'s rows from `row` for one line step; returns the row after
// them (the row count when the plane ends first).
// @early-stop register residue: the text[x] store encodes its SIB with base
// and index swapped ([x+text] here, [text+x] in retail), as in ReadTextChar;
// every other byte matches. Local order, pointer arithmetic, post-increment
// and unhoisted spellings are flat or worse.
RVA(0x00053080, 0xb5)
i16 ClearTextPlaneLine(i16 plane, i16 row) {
    TextPlane* p;
    i16 i;
    i16 x;
    u8* text;
    TextAttr* attrs;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    p = GetTextPlane(plane);
    for (i = 0; i < p->lineStep; i++) {
        if (row + i >= p->rows) {
            return p->rows;
        }
        text = TextPlaneTextRow(p, row + i);
        for (x = 0; x < p->cols; x++) {
            text[x] = 0;
        }
        attrs = TextPlaneAttrRow(p, row + i);
        for (x = 0; x < p->cols; x++) {
            attrs[x].value = 0x30;
        }
    }
    return row + p->lineStep;
}

// Moves the cursor to the start of the previous line.
RVA(0x00053140, 0x45)
void MoveTextPlaneCursorToPrevLine(i16 plane) {
    i16 y;

    if (plane != TEXT_PLANE_NONE) {
        y = GetTextPlane(plane)->cursorY - GetTextPlaneLineStep(plane);
        if (y < 0) {
            y = 0;
        }
        SetTextPlaneCursor(plane, GetActiveTextPlaneIndent(plane), y);
    }
}

// Moves the text rows below the header up by one line step, blanks the freed
// line and moves the cursor up with it.
// @early-stop register residue: the attribute rows' index (row + step) is
// formed in edi in retail and in edx here; every call matches. Size and
// clamp spellings tried.
RVA(0x00053190, 0x118)
void ScrollTextPlaneText(i16 plane) {
    TextPlane* p;
    i16 cols;
    i16 row;
    i16 attrBytes;
    u8 buffer[240];

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = &g_textPlanes[plane];
    cols = p->cols <= 80 ? p->cols : 80;
    for (row = p->headerRows; row < p->rows - p->lineStep; row++) {
        memmove(buffer, TextPlaneTextRow(p, row + p->lineStep), cols);
        memmove(TextPlaneTextRow(p, row), buffer, cols);
        memmove(buffer, TextPlaneAttrRow(p, row + p->lineStep), static_cast<i16>(cols * 2));
        memmove(TextPlaneAttrRow(p, row), buffer, static_cast<i16>(cols * 2));
    }
    ClearTextPlaneLine(plane, row);
    MoveTextPlaneCursorToPrevLine(plane);
}

// Scrolls the rendered glyph surface of `plane` up by one text line and
// clears the freed line.
RVA(0x000532b0, 0xbf)
void ScrollTextPlaneSurface(i16 plane) {
    TextPlane* p;
    i32 line;
    i32 y;
    RECT rect;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    p = GetTextPlane(plane);
    line = p->headerRows < 1 ? 0 : p->headerRows - 1;
    rect.right = p->sourceRect.right;
    rect.right -= 8;
    rect.bottom = p->sourceRect.bottom;
    rect.bottom -= 8;
    y = line * 16 + 8;
    LPDIRECTDRAWSURFACE surface = p->glyphSurface;
    rect.left = 8;
    rect.top = y + 16;
    surface->BltFast(8, y, surface, &rect, DDBLTFAST_NOCOLORKEY);
    rect.left = 8;
    rect.right = p->sourceRect.right - 8;
    rect.bottom = p->sourceRect.bottom - 8;
    rect.top = rect.bottom - 16;
    p->glyphSurface->Blt(&rect, NULL, &rect, DDBLT_COLORFILL, &g_clearBltFx);
}

RVA(0x00053370, 0x19)
void ScrollTextWindowLine(i16 plane) {
    ScrollTextPlaneSurface(plane);
    ScrollTextPlaneText(plane);
}

RVA(0x00053390, 0x20)
i32 GetTextPlaneHeaderRows(i16 plane) {
    if (plane != TEXT_PLANE_NONE) {
        return GetTextPlane(plane)->headerRows;
    }
    return 0;
}

RVA(0x000533b0, 0x2e)
i16 SetTextWindowScrollTop(i16 plane, i16 rows) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->headerRows;
    GetTextPlane(plane)->headerRows = rows;
    return old;
}

RVA(0x000533e0, 0x4f)
void FreeMenuLines(i16 plane) {
    MenuLine* line;

    if (plane != TEXT_PLANE_NONE) {
        while (GetTextPlane(plane)->menuLines != NULL) {
            line = static_cast<MenuLine*>(ListUnlink(GetTextPlane(plane)->menuLines));
            FreeBlock(line->text);
            FreeBlock(line);
        }
    }
}

// Appends a menu line to text plane `plane`'s list: a copy of `text` with
// its text attribute, selection value and flags. NULL for
// no plane.
RVA(0x00053430, 0x9b)
MenuLine* AddMenuLine(
    i16 plane,
    const char* text,
    i16 attr,
    i16 value,
    GZ_ENUM_PARAM(MenuLineFlags, i16) flags
) {
    MenuLine* line;

    if (plane == TEXT_PLANE_NONE) {
        return NULL;
    }
    line = static_cast<MenuLine*>(AllocCleared(1, sizeof(MenuLine)));
    line->text = static_cast<char*>(AllocCleared(strlen(text) + 1, 1));
    strcpy(line->text, text);
    line->attr = attr;
    line->value = value;
    line->flags = flags;
    ListAppend(&GetTextPlane(plane)->menuLines, line);
    return line;
}

RVA(0x000534d0, 0x3d)
MenuLine* FindMenuLineByValue(i16 plane, i16 value) {
    MenuLine* line;

    if (plane == TEXT_PLANE_NONE) {
        return NULL;
    }
    line = GetTextPlane(plane)->menuLines;
    while (line != NULL && line->value != value) {
        line = static_cast<MenuLine*>(ListNext(line));
    }
    return line;
}

// The menu line at `index`, or NULL past the end.
RVA(0x00053510, 0x3c)
MenuLine* GetMenuLine(i16 plane, i16 index) {
    MenuLine* line;

    if (plane == TEXT_PLANE_NONE) {
        return NULL;
    }
    line = GetTextPlane(plane)->menuLines;
    while (line != NULL && index != 0) {
        index--;
        line = static_cast<MenuLine*>(ListNext(line));
    }
    return line;
}

RVA(0x00053550, 0x2b)
i16 SetMenuLineAttr(i16 plane, i16 index, i16 attr) {
    MenuLine* line;
    i16 old;

    line = GetMenuLine(plane, index);
    if (line == NULL) {
        return 0;
    }
    old = line->attr;
    line->attr = attr;
    return old;
}

RVA(0x00053580, 0x1f)
i16 GetMenuLineAttr(i16 plane, i16 index) {
    MenuLine* line;

    line = GetMenuLine(plane, index);
    if (line == NULL) {
        return 0;
    }
    return line->attr;
}

RVA(0x000535a0, 0x69)
void SetMenuLineText(i16 plane, i16 index, const char* text) {
    MenuLine* line;

    line = GetMenuLine(plane, index);
    if (line != NULL) {
        FreeBlock(line->text);
        line->text = static_cast<char*>(AllocCleared(strlen(text) + 1, 1));
        strcpy(line->text, text);
    }
}

// @identity-TODO: the role of flags bit 8 is unrecovered.
RVA(0x00053610, 0x51)
i16 SetTextPlaneFlag8(i16 plane, i16 on) {
    i16 old;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    old = GetTextPlane(plane)->flags.flag8;
    GetTextPlane(plane)->flags.flag8 = on != false;
    return old;
}

// The index of the menu line under cell (x, y).
RVA(0x00053670, 0x50)
i16 GetMenuLineAt(i16 plane, i16 x, i16 y) {
    i16 index;

    index = (y - GetTextPlane(plane)->menuY) / GetTextPlane(plane)->lineStep;
    if (GetTextPlane(plane)->flags.twoColumns != false) {
        index *= 2;
        if (x >= GetTextPlane(plane)->cols / 2) {
            index++;
        }
    }
    return index;
}

// The cell where menu line `index` starts.
RVA(0x000536c0, 0xae)
TextPoint GetMenuLinePos(i16 plane, i16 index) {
    TextPoint pos;
    MenuLine* line;
    i16 i;

    i = 0;
    pos.x = 0;
    pos.y = 0;
    if (plane == TEXT_PLANE_NONE) {
        return pos;
    }
    line = GetTextPlane(plane)->menuLines;
    pos.x = GetTextPlane(plane)->menuX;
    pos.y = GetTextPlane(plane)->menuY;
    while (line != NULL && i != index) {
        i++;
        if (!GetTextPlane(plane)->flags.twoColumns || !(i & 1)) {
            pos.y += GetTextPlaneLineStep(plane);
        }
        line = static_cast<MenuLine*>(ListNext(line));
    }
    if (GetTextPlane(plane)->flags.twoColumns && (i & 1)) {
        pos.x += GetTextPlane(plane)->cols / 2;
    }
    return pos;
}

RVA(0x00053770, 0x30)
void SetTextPlaneMenuOrigin(i16 plane, i16 x, i16 y) {
    if (plane != TEXT_PLANE_NONE) {
        GetTextPlane(plane)->menuX = x;
        GetTextPlane(plane)->menuY = y;
    }
}

// Chooses the highlighted menu line: its value (-1 when disabled or
// hidden; the line index when the plane has no list), with the click sound.
RVA(0x000537a0, 0x82)
static i16 ChooseMenuLine(i16 plane) {
    TextPlane* p = GetTextPlane(plane);
    i16 index;
    i16 result;
    MenuLine* line;

    index = GetMenuLineAt(plane, p->highlightX, p->highlightY);
    SetHoveredObject(index);
    result = index;
    if (p->menuLines != NULL) {
        result = -1;
        line = GetMenuLine(plane, index);
        if (line != NULL && !(line->flags & (MENU_LINE_DISABLED | MENU_LINE_UNCHOOSABLE))) {
            result = line->value;
        }
    }
    ClearMouseClicks();
    PlaySoundEffect(1);
    SetSelectedObject(result);
    return result;
}

// The text cell of plane `plane` under screen point (x, y), inside its
// 8-pixel border: stores its column and row, and returns the column its word
// starts at (-1 off the text or on an empty cell).
RVA(0x00053830, 0xdc)
i16 TextPlaneCellAt(i16 plane, i16 x, i16 y, i16* col, i16* row) {
    TextPlane* p = &g_textPlanes[plane];
    i16 found = -1;
    i16 i;
    u8* text;

    if (static_cast<i16>(p->top + 8) <= y
        && static_cast<i16>(p->top + p->sourceRect.bottom - 8) >= y
        && static_cast<i16>(p->left + 8) <= x
        && static_cast<i16>(p->left + p->sourceRect.right - 8) >= x) {
        *row = static_cast<u32>(y - p->top - 8) >> 4;
        *col = static_cast<u32>(x - p->left - 8) >> 3;
        found = 0;
    }
    if (found == -1) {
        return found;
    }
    text = p->text[*row];
    if (text[*col] == 0) {
        return -1;
    }
    for (i = *col; i >= 0 && text[i] != 0; i--) {
    }
    i++;
    if (i > *col) {
        i = *col;
    }
    return i;
}

// Polls the mouse over text plane `plane`'s menu: an enabled cancel, a
// right or a left click on the highlighted line chooses it; otherwise the
// highlight follows the cursor. Returns -1 cancelled, 1 or 2 chosen (left,
// right), 0 otherwise.
RVA(0x00053910, 0x1a2)
GZ_ENUM_RETURN(TextEvent, i16) PollMenuInput(i16 plane) {
    TextPlane* p;
    i16 cancelEnabled;
    i16 x;
    i16 col;
    i16 row;
    MenuLine* line;

    if (plane == TEXT_PLANE_NONE) {
        return TEXT_EVENT_NONE;
    }
    p = GetTextPlane(plane);
    cancelEnabled = IsTextPlaneCancelEnabled(plane);
    if (cancelEnabled) {
        if (TakeMouseCancel(true)) {
            CallTextPlaneHook(plane, TEXT_EVENT_CANCEL, 0);
            return TEXT_EVENT_CANCEL;
        }
    } else if (GetTextPlane(plane)->highlightY >= 0 && GetMouseRightClick()) {
        ChooseMenuLine(plane);
        CallTextPlaneHook(plane, TEXT_EVENT_CHOOSE_RIGHT, 0);
        return TEXT_EVENT_CHOOSE_RIGHT;
    }
    if (GetTextPlane(plane)->highlightY >= 0 && GetMouseLeftClick()) {
        ChooseMenuLine(plane);
        CallTextPlaneHook(plane, TEXT_EVENT_CHOOSE, GetTextPlane(plane)->highlightY);
        return TEXT_EVENT_CHOOSE;
    }
    x = TextPlaneCellAt(plane, g_cursorPos.x, g_cursorPos.y, &col, &row);
    if (x < 0) {
        row = TEXT_MENU_HIGHLIGHT_NONE;
    }
    if (x < 0 || row < p->firstSelectableRow) {
        row = TEXT_MENU_HIGHLIGHT_NONE;
    }
    if (p->menuLines != NULL && row != TEXT_MENU_HIGHLIGHT_NONE) {
        line = GetMenuLine(plane, GetMenuLineAt(plane, x, row));
        if (line == NULL || (line->flags & MENU_LINE_DISABLED)) {
            row = TEXT_MENU_HIGHLIGHT_NONE;
        }
    }
    if (row != GetTextPlane(plane)->highlightY || x != p->highlightX) {
        ClearTextPlaneHighlight(plane);
        CallTextPlaneHook(plane, TEXT_EVENT_UNHIGHLIGHT, GetTextPlane(plane)->highlightY);
        if (row != TEXT_MENU_HIGHLIGHT_NONE) {
            SetTextPlaneHighlight(plane, x, row);
            CallTextPlaneHook(plane, TEXT_EVENT_HIGHLIGHT, GetTextPlane(plane)->highlightY);
        }
    }
    return TEXT_EVENT_NONE;
}

RVA(0x00053ac0, 0x4f)
void GetTextPlaneOrigin(i16 plane, i16* x, i16* y) {
    if (plane == TEXT_PLANE_NONE) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = GetTextPlane(plane)->left + 8;
    *y = GetTextPlane(plane)->top + 8;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
// @identity-TODO: the original role of this unreferenced duplicate is unknown.
RVA(0x00053b10, 0x4f)
void GetTextPlaneOrigin2(i16 plane, i16* x, i16* y) {
    if (plane == TEXT_PLANE_NONE) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = GetTextPlane(plane)->left + 8;
    *y = GetTextPlane(plane)->top + 8;
}

RVA(0x00053b60, 0x27)
u8* GetTextPlaneRowText(i16 plane, i16 row) {
    if (plane != TEXT_PLANE_NONE) {
        return TextPlaneTextRow(GetTextPlane(plane), row);
    }
    return NULL;
}

RVA(0x00053b90, 0x22)
void SetWindowDeferredChar(i16 window, u16 ch) {
    if (window != TEXT_PLANE_NONE) {
        GetTextPlane(window)->deferredChar = ch;
    }
}

RVA(0x00053bc0, 0x29)
u16 TakeWindowDeferredChar(i16 window) {
    u16 ch;

    ch = 0;
    if (window != TEXT_PLANE_NONE) {
        ch = GetTextPlane(window)->deferredChar;
        GetTextPlane(window)->deferredChar = 0;
    }
    return ch;
}

// Moves the cursor to the start of the next row; returns the rows left.
RVA(0x00053bf0, 0x5f)
i16 AdvanceWindowLine(i16 window) {
    TextPlane* p;
    i16 indent;
    i16 x;
    i16 y;

    if (window == TEXT_PLANE_NONE) {
        return 0;
    }
    p = GetTextPlane(window);
    y = p->cursorY + 1;
    indent = p->flags.indentEnabled != false ? p->indent : 0;
    x = p->cols;
    if (indent < x) {
        x = indent;
    }
    p->cursorX = x;
    if (y >= p->rows) {
        y = p->rows;
    }
    p->cursorY = y;
    return p->rows - p->cursorY;
}

// Wraps the cursor when `width` more cells overrun the row by more than
// `slack`; returns -1 once the cursor is past the last row.
RVA(0x00053c50, 0x5c)
i16 ReserveTextPlaneCells(i16 plane, i16 width, i16 slack) {
    TextPlane* p;
    i16 x;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    p = &g_textPlanes[plane];
    x = p->cursorX + width;
    if (x > p->cols + slack) {
        AdvanceWindowLine(plane);
    }
    return p->cursorY < p->rows ? 0 : -1;
}

// Moves the cursor to the start of the first row.
RVA(0x00053cb0, 0x1c)
void HomeTextPlaneCursor(i16 plane) {
    SetTextPlaneCursor(plane, GetActiveTextPlaneIndent(plane), 0);
}

RVA(0x00053cd0, 0x38)
void EraseTextPlaneText(i16 plane) {
    i16 row;

    if (plane != TEXT_PLANE_NONE) {
        for (row = 0; row < GetTextPlane(plane)->rows;) {
            row = ClearTextPlaneLine(plane, row);
        }
    }
}

// Clears text plane `plane`: its text, the cursor home, and its glyphs (a
// keypad plane only in its 134x32 top).
RVA(0x00053d10, 0xa9)
void ClearTextPlane(i16 plane) {
    RECT rect;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    EraseTextPlaneText(plane);
    SetTextPlaneCursor(plane, GetActiveTextPlaneIndent(plane), 0);
    if (GetTextPlane(plane)->glyphSurface == NULL) {
        return;
    }
    if (GetTextPlane(plane)->kind != TEXT_PLANE_KIND_KEYPAD) {
        GetTextPlane(plane)->glyphSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
        return;
    }
    rect.left = 0;
    rect.top = 0;
    rect.right = 134;
    rect.bottom = 32;
    GetTextPlane(plane)->glyphSurface->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
}

// The same, but an analyze name plane keeps its glyphs.
RVA(0x00053dc0, 0xb2)
void ClearTextPlaneText(i16 plane) {
    RECT rect;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    EraseTextPlaneText(plane);
    SetTextPlaneCursor(plane, GetActiveTextPlaneIndent(plane), 0);
    if (GetTextPlane(plane)->glyphSurface == NULL) {
        return;
    }
    if (GetTextPlane(plane)->kind == TEXT_PLANE_KIND_ANALYZE_NAME) {
        return;
    }
    if (GetTextPlane(plane)->kind != TEXT_PLANE_KIND_KEYPAD) {
        GetTextPlane(plane)->glyphSurface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
        return;
    }
    rect.left = 0;
    rect.top = 0;
    rect.right = 134;
    rect.bottom = 32;
    GetTextPlane(plane)->glyphSurface->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
}

// Clears text plane `plane` and prints its menu lines, each at its place and
// in its tag's attribute, followed by a newline.
RVA(0x00053e80, 0x90)
void PrintMenuLines(i16 plane) {
    MenuLine* line;
    i16 i;
    TextPoint pos;

    if (plane == TEXT_PLANE_NONE) {
        return;
    }
    ClearTextPlane(plane);
    i = 0;
    for (line = GetTextPlane(plane)->menuLines; line != NULL;
         line = static_cast<MenuLine*>(ListNext(line)), i++) {
        pos = GetMenuLinePos(plane, i);
        SetTextPlaneCursor(plane, pos.x, pos.y);
        PrintWindowText(plane, line->text, line->attr, 0, true);
        PrintWindowText(plane, "\n", line->attr, 0, true);
    }
}

// The plane's size in cells (x = columns, y = rows).
RVA(0x00053f10, 0x2b)
TextPoint GetTextPlaneSize(i16 plane) {
    TextPoint size;

    size.x = GetTextPlane(plane)->cols;
    size.y = GetTextPlane(plane)->rows;
    return size;
}

RVA(0x00053f40, 0x33)
i16 GetTextPlaneLineCount(i16 plane) {
    TextPoint size;

    if (plane == TEXT_PLANE_NONE) {
        return 0;
    }
    size = GetTextPlaneSize(plane);
    return size.y / static_cast<i16>(GetTextPlaneLineStep(plane));
}

// The highest plane of `kind`, or -1.
RVA(0x00053f80, 0x47)
i16 FindTextPlaneByKind(i16 kind) {
    i16 i;

    if (kind < 0 || kind > TEXT_PLANE_COUNT - 1) {
        return TEXT_PLANE_NONE;
    }
    for (i = TEXT_PLANE_COUNT - 1; i >= 0; i--) {
        if (GetTextPlane(i)->kind == kind) {
            break;
        }
    }
    if (i == TEXT_PLANE_COUNT) {
        i = TEXT_PLANE_NONE;
    }
    return i;
}

// The two title-menu commands drawn on g_titleMenuPicture.
#define TITLE_MENU_NEW_GAME_AREA 0x1e
#define TITLE_MENU_CONTINUE_AREA 0x1f

// The screen areas the panels' hotspots cover, the image pair each shows
// (up, highlighted) and, for the areas below AREA_PANEL_LIMIT, the kind of
// the text plane they are drawn on (none for 0xffff).
// @identity-TODO: recover the individual areas' caller-facing roles.
DATA(0x0006d3a8)
static HotspotArea s_hotspotAreas[88] = {
    {176, 192, 208, 208, HOTSPOT_IMAGES_ARROW_UP},
    {218, 192, 250, 208, HOTSPOT_IMAGES_ARROW_DOWN},
    {432, 232, 464, 248, HOTSPOT_IMAGES_ARROW_UP},
    {474, 232, 506, 248, HOTSPOT_IMAGES_ARROW_DOWN},
    {536, 62, 568, 78, HOTSPOT_IMAGES_ARROW_UP},
    {576, 62, 608, 78, HOTSPOT_IMAGES_ARROW_DOWN},
    {52, 186, 84, 202, HOTSPOT_IMAGES_ARROW_UP},
    {92, 186, 124, 202, HOTSPOT_IMAGES_ARROW_DOWN},
    {32, 208, 64, 224, HOTSPOT_IMAGES_ARROW_UP},
    {74, 208, 106, 224, HOTSPOT_IMAGES_ARROW_DOWN},
    {320, 208, 352, 224, HOTSPOT_IMAGES_ARROW_UP},
    {362, 208, 394, 224, HOTSPOT_IMAGES_ARROW_DOWN},
    {512, 304, 544, 320, HOTSPOT_IMAGES_ARROW_UP},
    {544, 304, 576, 320, HOTSPOT_IMAGES_ARROW_DOWN},
    {501, 295, 532, 311, HOTSPOT_IMAGES_ARROW_UP},
    {533, 295, 564, 311, HOTSPOT_IMAGES_ARROW_DOWN},
    {219, 295, 250, 311, HOTSPOT_IMAGES_ARROW_UP},
    {251, 295, 282, 311, HOTSPOT_IMAGES_ARROW_DOWN},
    {565, 295, 596, 311, HOTSPOT_IMAGES_ARROW_LEFT},
    {597, 295, 628, 311, HOTSPOT_IMAGES_ARROW_RIGHT},
    {501, 304, 532, 319, HOTSPOT_IMAGES_ARROW_UP},
    {597, 304, 628, 319, HOTSPOT_IMAGES_ARROW_RIGHT},
    {533, 304, 564, 319, HOTSPOT_IMAGES_ARROW_DOWN},
    {565, 304, 596, 319, HOTSPOT_IMAGES_ARROW_LEFT},
    {184, 318, 215, 333, HOTSPOT_IMAGES_ARROW_UP},
    {232, 318, 263, 333, HOTSPOT_IMAGES_ARROW_DOWN},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {240, 365, 366, 385, HOTSPOT_IMAGES_NEW_GAME},
    {246, 400, 360, 420, HOTSPOT_IMAGES_CONTINUE},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_BUY},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_SELL},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_LEAVE},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_HEAL},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_CURE},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_BUY},
    {560, 152, 639, 175, HOTSPOT_IMAGES_PANEL_LEAVE},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_CONSULT},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_BUY},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_LEAVE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_HEAL},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_CURE},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_LEAVE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_OK},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_EXIT},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_CANCEL},
    {560, 80, 639, 103, HOTSPOT_IMAGES_PANEL_OK},
    {560, 104, 639, 127, HOTSPOT_IMAGES_PANEL_ARM},
    {560, 128, 639, 151, HOTSPOT_IMAGES_PANEL_EXIT},
    {560, 152, 639, 175, HOTSPOT_IMAGES_PANEL_CANCEL},
    {560, 16, 624, 40, HOTSPOT_IMAGES_MODE_EXIT},
    {336, 16, 399, 40, HOTSPOT_IMAGES_MODE_ITEM},
    {16, 16, 79, 40, HOTSPOT_IMAGES_MODE_MAGIC},
    {80, 16, 143, 40, HOTSPOT_IMAGES_MODE_ABILITY},
    {432, 16, 495, 40, HOTSPOT_IMAGES_MODE_NEXT},
    {496, 16, 559, 40, HOTSPOT_IMAGES_MODE_QUIT},
    {144, 16, 207, 40, HOTSPOT_IMAGES_MODE_EQUIP},
    {208, 16, 271, 40, HOTSPOT_IMAGES_MODE_GEM},
    {272, 16, 335, 40, HOTSPOT_IMAGES_MODE_CHART},
    {545, 159, 569, 183, HOTSPOT_IMAGES_KEYPAD_0},
    {517, 75, 541, 99, HOTSPOT_IMAGES_KEYPAD_1},
    {545, 75, 569, 99, HOTSPOT_IMAGES_KEYPAD_2},
    {573, 75, 597, 99, HOTSPOT_IMAGES_KEYPAD_3},
    {517, 103, 541, 127, HOTSPOT_IMAGES_KEYPAD_4},
    {545, 103, 569, 127, HOTSPOT_IMAGES_KEYPAD_5},
    {573, 103, 597, 127, HOTSPOT_IMAGES_KEYPAD_6},
    {517, 131, 541, 155, HOTSPOT_IMAGES_KEYPAD_7},
    {545, 131, 569, 155, HOTSPOT_IMAGES_KEYPAD_8},
    {573, 131, 597, 155, HOTSPOT_IMAGES_KEYPAD_9},
    {517, 159, 541, 183, HOTSPOT_IMAGES_KEYPAD_CLEAR},
    {573, 159, 597, 183, HOTSPOT_IMAGES_KEYPAD_OK},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {0, 0, -1, -1, HOTSPOT_IMAGES_NONE},
    {264, 204, 271, 219, HOTSPOT_IMAGES_ARROW_UP},
    {256, 204, 263, 219, HOTSPOT_IMAGES_ARROW_UP},
    {248, 204, 255, 219, HOTSPOT_IMAGES_ARROW_UP},
    {240, 204, 247, 219, HOTSPOT_IMAGES_ARROW_UP},
    {232, 204, 239, 219, HOTSPOT_IMAGES_ARROW_UP},
    {224, 204, 231, 219, HOTSPOT_IMAGES_ARROW_UP},
    {216, 204, 223, 219, HOTSPOT_IMAGES_ARROW_UP},
    {208, 204, 215, 219, HOTSPOT_IMAGES_ARROW_UP},
};

DATA(0x0006da88)
static u16 s_hotspotImages[40][2] = {
    {0, 0},
    {IDB_BITMAP14, IDB_BITMAP148},
    {IDB_BITMAP15, IDB_BITMAP149},
    {IDB_BITMAP27, 0},
    {IDB_BITMAP28, 0},
    {IDB_BITMAP153, IDB_BITMAP163},
    {IDB_BITMAP161, IDB_BITMAP171},
    {IDB_BITMAP159, IDB_BITMAP169},
    {IDB_BITMAP158, IDB_BITMAP168},
    {IDB_BITMAP156, IDB_BITMAP166},
    {IDB_BITMAP155, IDB_BITMAP165},
    {IDB_BITMAP160, IDB_BITMAP170},
    {IDB_BITMAP157, IDB_BITMAP167},
    {IDB_BITMAP154, IDB_BITMAP164},
    {IDB_BITMAP152, IDB_BITMAP162},
    {IDB_BITMAP134, IDB_BITMAP135},
    {IDB_BITMAP138, IDB_BITMAP139},
    {IDB_BITMAP140, IDB_BITMAP141},
    {IDB_BITMAP128, IDB_BITMAP129},
    {IDB_BITMAP142, IDB_BITMAP143},
    {IDB_BITMAP144, IDB_BITMAP145},
    {IDB_BITMAP132, IDB_BITMAP133},
    {IDB_BITMAP136, IDB_BITMAP137},
    {IDB_BITMAP130, IDB_BITMAP131},
    {IDB_BITMAP146, IDB_BITMAP150},
    {IDB_BITMAP147, IDB_BITMAP151},
    {0, IDB_BITMAP345},
    {0, IDB_BITMAP335},
    {0, IDB_BITMAP336},
    {0, IDB_BITMAP337},
    {0, IDB_BITMAP338},
    {0, IDB_BITMAP339},
    {0, IDB_BITMAP340},
    {0, IDB_BITMAP341},
    {0, IDB_BITMAP342},
    {0, IDB_BITMAP343},
    {0, IDB_BITMAP344},
    {0, IDB_BITMAP334},
    {0, 0},
    {0, 0},
};

DATA(0x0006db28)
static u16 s_hotspotPlaneKinds[32] = {30, 30, 28,    28,    9,     9,     5,     5,     25, 25, 33,
                                      33, 26, 26,    3,     3,     4,     4,     3,     3,  31, 31,
                                      31, 31, 65535, 65535, 65535, 65535, 65535, 65535, 0,  0};

#define GetHotspotAreaImage(area, state) (s_hotspotImages[s_hotspotAreas[(area)].images][(state)])

// Draws hotspot area `id`'s image `on` (0 up, 1 highlighted) on plane
// `plane`, or, for plane 0, on the picture or layer the area lies on: the
// keypad plane (areas 0x3f..0x4b, highlight only), the command bar (from
// 0x36), the title menu (its two areas) or the character panel.
RVA(0x00053fd0, 0x257)
void HighlightHotspot(i16 plane, i16 id, i16 on) {
    RECT rect;
    i16 keypad;

    if (plane == TEXT_PLANE_NONE || id == HOTSPOT_AREA_NONE) {
        return;
    }
    if (plane == 0) {
        if (id >= AREA_KEYPAD_FIRST && id <= AREA_KEYPAD_LAST) {
            if (on == true) {
                keypad = FindTextPlaneByKind(TEXT_PLANE_KIND_KEYPAD);
                if (keypad != TEXT_PLANE_NONE) {
                    i32 x = s_hotspotAreas[id].left - GetTextPlane(keypad)->left;
                    i32 y = s_hotspotAreas[id].top - GetTextPlane(keypad)->top;
                    BlitImage(GetTextPlane(keypad)->glyphSurface, GetHotspotAreaImage(id, 1), x, y);
                }
            }
            return;
        }
        if (id >= AREA_MODE_FIRST) {
            BlitImage(
                g_commandBarPicture.surface,
                GetHotspotAreaImage(id, on),
                s_hotspotAreas[id].left - 16,
                0
            );
            return;
        }
        if (id == TITLE_MENU_NEW_GAME_AREA || id == TITLE_MENU_CONTINUE_AREA) {
            BlitImage(
                g_titleMenuPicture.surface,
                GetHotspotAreaImage(id, on),
                s_hotspotAreas[id].left - 240,
                s_hotspotAreas[id].top - 365
            );
            return;
        }
        BlitImage(
            g_screenLayers[SCREEN_LAYER_PANEL]->surface,
            GetHotspotAreaImage(id, on),
            0,
            s_hotspotAreas[id].top - 80
        );
        if (s_hotspotAreas[id].images == HOTSPOT_IMAGES_PANEL_LEAVE) {
            rect = g_screenLayers[SCREEN_LAYER_PANEL]->source;
            rect.top = s_hotspotAreas[id].bottom - 79;
            g_screenLayers[SCREEN_LAYER_PANEL]
                ->surface->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
        }
        g_screenLayers[SCREEN_LAYER_PANEL]->visible = true;
        return;
    }
    i32 x = s_hotspotAreas[id].left - GetTextPlane(plane)->left;
    i32 y = s_hotspotAreas[id].top - GetTextPlane(plane)->top;
    BlitImage(GetTextPlane(plane)->glyphSurface, GetHotspotAreaImage(id, on), x, y);
}

// Whether (x, y) lies in hotspot area `id` (1, else -1); a hit highlights
// the area (the panel areas on their plane; the others, unless `strict`,
// through HighlightHotspot's plane 0).
RVA(0x00054230, 0x92)
i16 HitTestHotspot(i16 id, i16 x, i16 y, i16 strict) {
    i16 hit = -1;
    i16 plane;

    if (id == HOTSPOT_AREA_NONE) {
        return id;
    }
    if (s_hotspotAreas[id].left <= x && x <= s_hotspotAreas[id].right && s_hotspotAreas[id].top <= y
        && y <= s_hotspotAreas[id].bottom) {
        hit = 1;
        if (id >= AREA_PANEL_LIMIT) {
            HighlightHotspot(0, id, strict == 0);
        } else {
            plane = FindTextPlaneByKind(s_hotspotPlaneKinds[id]);
            HighlightHotspot(plane, id, 1);
        }
    }
    return hit;
}

RVA(0x000542d0, 0x77)
void ErasePictureSurface(i16 picture) {
    if (picture == -1) {
        return;
    }
    if (picture >= AREA_KEYPAD_FIRST && picture <= AREA_KEYPAD_LAST) {
        return;
    }
    if (picture >= AREA_MODE_FIRST) {
        g_commandBarPicture.surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
    } else if (picture == TITLE_MENU_NEW_GAME_AREA || picture == TITLE_MENU_CONTINUE_AREA) {
        g_titleMenuPicture.surface->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
    } else {
        g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
        ClearPanelLayerSurface();
    }
}

RVA(0x00054350, 0x9)
i16 IsPanelLayerVisible(void) {
    return g_screenLayers[SCREEN_LAYER_PANEL]->visible;
}

RVA(0x00054360, 0x26)
void ShowScreenLayer(GZ_ENUM_PARAM(ScreenLayerSlot, i16) layer) {
    g_screenLayers[layer]->visible = true;
    if (layer > SCREEN_LAYER_NONPARTY_LAST) {
        GetTextPlane(0)->visible = false;
    }
}

RVA(0x00054390, 0x3a)
void HideScreenLayer(i16 layer) {
    g_screenLayers[layer]->visible = false;
    if (layer == SCREEN_LAYER_PANEL) {
        ClearPanelLayerSurface();
    }
}

RVA(0x000543d0, 0x26)
void ClearLayerSurface(GZ_ENUM_PARAM(ScreenLayerSlot, i16) layer) {
    ScreenLayer* screen = g_screenLayers[layer];

    screen->canvas->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
}

RVA(0x00054400, 0x1c)
void ClearStatusPicture(void) {
    ClearDisplaySurface(g_statusPicture.surface, NULL);
}

RVA(0x00054420, 0x3c)
void DrawStatusImage(i16 x, i16 y, i16 index) {
    BlitImage(
        g_statusPicture.surface,
        g_statusImages[index],
        static_cast<i16>(x * 8),
        static_cast<i16>(y * 8)
    );
}

RVA(0x00054460, 0x48)
void DrawPlaneImage(i16 plane, i16 x, i16 y, i16 index) {
    BlitImage(
        GetTextPlane(plane)->surface,
        g_statusImages[index],
        static_cast<i16>(x * 8),
        static_cast<i16>(y * 8)
    );
}

RVA(0x000544b0, 0x48)
void DrawStatBarMark(i16 x, i16 y, GZ_ENUM_PARAM(StatBarMark, i16) index, i16 plane) {
    BlitImage(
        GetTextPlane(plane)->glyphSurface,
        g_statBarMarkImages[index],
        static_cast<i16>(x * 8),
        static_cast<i16>(y * 8)
    );
}

RVA(0x00054500, 0x49)
void DrawPlaneIcon(i16 plane, i16 x, i16 y, i16 icon, i16 frame) {
    BlitImage(
        GetTextPlane(plane)->glyphSurface,
        g_fusionSummaryImages[icon][frame],
        static_cast<i16>(x * 8),
        y
    );
}

// Draws icon `icon` frame `frame` onto the glyph surface of `plane` at cell
// column `x`, pixel row `y`, through the staging surface so its colour key
// applies.
RVA(0x00054550, 0x88)
void DrawPlaneIconKeyed(i16 plane, i16 x, i16 y, i16 icon, i16 frame) {
    RECT source;

    BlitImage(g_iconStagingPicture.surface, g_fusionSummaryImages[icon][frame], 0, 0);
    source.top = 0;
    source.left = 0;
    source.right = 24;
    source.bottom = 16;
    GetTextPlane(plane)->glyphSurface->BltFast(
        static_cast<i16>(x * 8),
        y,
        g_iconStagingPicture.surface,
        &source,
        DDBLTFAST_SRCCOLORKEY
    );
}

// The automap's cells are 16 pixels square; on the automap layer the map
// starts half a cell in, on a text plane one cell in.
#define MAP_CELL 16

// Draws map tile `tile` in cell (x, y) of the automap layer's work surface.
RVA(0x000545e0, 0x3c)
void DrawMapTile(i16 tile, i16 x, i16 y) {
    BlitImage(
        g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas,
        g_mapTileImages[tile],
        static_cast<i16>(x * MAP_CELL + MAP_CELL / 2),
        static_cast<i16>(y * MAP_CELL + MAP_CELL / 2)
    );
}

// Draws map tile `tile` in cell (x, y) of text plane `plane` over a
// half-bright green cell (the cell under the party).
RVA(0x00054620, 0xfb)
void DrawPlaneMapTileLit(i16 tile, i16 x, i16 y, i16 plane) {
    RECT source;
    RECT cell;
    DDBLTFX fx;
    i16 px;
    i16 py;

    BlitImage(g_iconStagingPicture.surface, g_mapTileImages[tile], 0, 0);
    source.right = MAP_CELL;
    source.bottom = MAP_CELL;
    source.top = 0;
    px = static_cast<i16>((x + 1) * MAP_CELL);
    py = static_cast<i16>((y + 1) * MAP_CELL);
    cell.left = px + 1;
    cell.top = py + 1;
    cell.right = px + MAP_CELL - 1;
    cell.bottom = py + MAP_CELL - 1;
    source.left = 0;
    ZeroMemory(&fx, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = (g_greenMask >> 1) & g_greenMask;
    GetTextPlane(plane)->glyphSurface->Blt(&cell, NULL, NULL, DDBLT_COLORFILL, &fx);
    GetTextPlane(plane)->glyphSurface->BltFast(
        px,
        py,
        g_iconStagingPicture.surface,
        &source,
        DDBLTFAST_SRCCOLORKEY
    );
}

// Draws map tile `tile` in cell (x, y) of text plane `plane` over a cleared
// cell.
RVA(0x00054720, 0xea)
void DrawPlaneMapTile(i16 tile, i16 x, i16 y, i16 plane) {
    RECT source;
    RECT cell;
    DDBLTFX fx;

    BlitImage(g_iconStagingPicture.surface, g_mapTileImages[tile], 0, 0);
    source.right = MAP_CELL;
    source.bottom = MAP_CELL;
    source.top = 0;
    source.left = 0;
    cell.left = static_cast<i16>((x + 1) * MAP_CELL);
    cell.top = static_cast<i16>((y + 1) * MAP_CELL);
    cell.right = static_cast<i16>((x + 1) * MAP_CELL) + MAP_CELL;
    cell.bottom = static_cast<i16>((y + 1) * MAP_CELL) + MAP_CELL;
    ZeroMemory(&fx, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0;
    GetTextPlane(plane)->glyphSurface->Blt(&cell, NULL, NULL, DDBLT_COLORFILL, &fx);
    GetTextPlane(plane)->glyphSurface->BltFast(
        static_cast<i16>((x + 1) * MAP_CELL),
        static_cast<i16>((y + 1) * MAP_CELL),
        g_iconStagingPicture.surface,
        &source,
        DDBLTFAST_SRCCOLORKEY
    );
}

// Draws map mark `mark` (keyed) in cell (x, y) of the automap layer's work
// surface.
RVA(0x00054810, 0x7d)
void DrawMapMark(i16 mark, i16 x, i16 y) {
    RECT source;

    BlitImage(g_iconStagingPicture.surface, g_mapMarkImages[mark], 0, 0);
    source.top = 0;
    source.right = MAP_CELL;
    source.bottom = MAP_CELL;
    source.left = 0;
    g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas->BltFast(
        static_cast<i16>(x * MAP_CELL + MAP_CELL / 2),
        static_cast<i16>(y * MAP_CELL + MAP_CELL / 2),
        g_iconStagingPicture.surface,
        &source,
        DDBLTFAST_SRCCOLORKEY
    );
}

// Draws map mark `mark` (keyed) in cell (x, y) of text plane `plane`, if the
// plane is open.
RVA(0x00054890, 0x92)
void DrawPlaneMapMark(i16 mark, i16 x, i16 y, i16 plane) {
    RECT source;

    if (GetTextPlane(plane)->kind == TEXT_PLANE_FREE) {
        return;
    }
    BlitImage(g_iconStagingPicture.surface, g_mapMarkImages[mark], 0, 0);
    source.top = 0;
    source.right = MAP_CELL;
    source.bottom = MAP_CELL;
    source.left = 0;
    GetTextPlane(plane)->glyphSurface->BltFast(
        static_cast<i16>((x + 1) * MAP_CELL),
        static_cast<i16>((y + 1) * MAP_CELL),
        g_iconStagingPicture.surface,
        &source,
        DDBLTFAST_SRCCOLORKEY
    );
}

// Clears the part of text plane `plane`'s glyph surface right of x 264 and
// above y 264.
RVA(0x00054930, 0x62)
void ClearTextPlaneRight(i16 plane) {
    RECT rect;

    rect = GetTextPlane(plane)->sourceRect;
    rect.left = 264;
    rect.bottom = 264;
    GetTextPlane(plane)->glyphSurface->Blt(&rect, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
}

// The gauge's bar on its layer: x 56..136.
#define GAUGE_LEFT 56
#define GAUGE_WIDTH 80

// Draws an 80-pixel gauge of `value` out of `max` on layer `slot`'s work
// surface, in its upper or lower row: green for the part left, red for the
// part used, filled from the right.
RVA(0x000549a0, 0x18e)
void DrawLayerGauge(i16 slot, u16 value, u16 max, i16 upper) {
    RECT full;
    RECT used;
    DDBLTFX red;
    DDBLTFX green;
    ScreenLayer* layer;
    i32 width;

    used.left = GAUGE_LEFT;
    full.left = GAUGE_LEFT;
    used.right = GAUGE_LEFT + GAUGE_WIDTH;
    full.right = GAUGE_LEFT + GAUGE_WIDTH;
    if (upper) {
        used.top = 29;
        full.top = 29;
        used.bottom = 39;
        full.bottom = 39;
    } else {
        used.top = 45;
        full.top = 45;
        used.bottom = 55;
        full.bottom = 55;
    }
    ZeroMemory(&red, sizeof(red));
    red.dwSize = sizeof(red);
    red.dwFillColor = g_redMask;
    ZeroMemory(&green, sizeof(green));
    green.dwSize = sizeof(green);
    green.dwFillColor = g_greenMask;
    layer = g_screenLayers[slot];
    if (value <= 0) {
        layer->canvas->Blt(&used, NULL, NULL, DDBLT_COLORFILL, &red);
        return;
    }
    if (value >= max) {
        layer->canvas->Blt(&full, NULL, NULL, DDBLT_COLORFILL, &green);
        return;
    }
    if (max != 0) {
        width = value * GAUGE_WIDTH / max;
    } else {
        width = value * GAUGE_WIDTH;
    }
    full.left = used.right = GAUGE_LEFT + GAUGE_WIDTH - width;
    layer->canvas->Blt(&used, NULL, NULL, DDBLT_COLORFILL, &red);
    layer->canvas->Blt(&full, NULL, NULL, DDBLT_COLORFILL, &green);
}

#define PARTY_PANEL_WIDTH 192
#define PARTY_PANEL_HEIGHT 64

// The party panel (0..5) whose 192x64 area holds (x, y), -1 for none.
RVA(0x00054b30, 0x52)
static i16 FindPartyPanel(i16 x, i16 y) {
    i16 i;

    for (i = 0; i < PARTY_PANEL_COUNT; i++) {
        if (static_cast<i16>(s_layerOrigin[SCREEN_LAYER_FIRST_PANEL + i].x) <= x
            && static_cast<i16>(s_layerOrigin[SCREEN_LAYER_FIRST_PANEL + i].x + PARTY_PANEL_WIDTH)
                   >= x
            && static_cast<i16>(s_layerOrigin[SCREEN_LAYER_FIRST_PANEL + i].y) <= y
            && static_cast<i16>(s_layerOrigin[SCREEN_LAYER_FIRST_PANEL + i].y + PARTY_PANEL_HEIGHT)
                   >= y) {
            break;
        }
    }
    if (i >= PARTY_PANEL_COUNT) {
        i = -1;
    }
    return i;
}

RVA(0x00054b90, 0x13)
i16 PartyPanelAtPoint(i16 x, i16 y) {
    return FindPartyPanel(x, y);
}

// The layers' visibility and positions as a save file keeps them.
DATA(0x00090b18)
static SavedLayer s_savedScreenLayers[SCREEN_LAYER_COUNT];

// Writes the layers' visibility and positions to `file`; 0 on success.
RVA(0x00054bb0, 0x51)
i16 SaveScreenLayers(FILE* file) {
    i32 i;

    for (i = 0; i < SCREEN_LAYER_COUNT; i++) {
        s_savedScreenLayers[i].visible = g_screenLayers[i]->visible;
        s_savedScreenLayers[i].x = g_screenLayers[i]->x;
        s_savedScreenLayers[i].y = g_screenLayers[i]->y;
    }
    return 1 - fwrite(s_savedScreenLayers, sizeof(s_savedScreenLayers), 1, file);
}

// Reads the layers' visibility and positions from `file` (the character
// panel starts hidden); 0 on success.
RVA(0x00054c10, 0x64)
i16 LoadScreenLayers(FILE* file) {
    i16 result;
    i32 i;

    result = 1 - fread(s_savedScreenLayers, sizeof(s_savedScreenLayers), 1, file);
    for (i = 0; i < SCREEN_LAYER_COUNT; i++) {
        g_screenLayers[i]->visible = s_savedScreenLayers[i].visible;
        g_screenLayers[i]->x = s_savedScreenLayers[i].x;
        g_screenLayers[i]->y = s_savedScreenLayers[i].y;
    }
    g_screenLayers[SCREEN_LAYER_PANEL]->visible = false;
    return result;
}

// Paints the icon layer with its image `index`.
RVA(0x00054c80, 0x25)
void DrawIconLayerImage(i16 index) {
    BlitImage(g_screenLayers[SCREEN_LAYER_ICON]->surface, g_iconLayerImages[index], 0, 0);
}

// Draws a 256x256 portrait at (148, 20) on the status picture: bitmap `bmp`,
// or the NPC texture when there is none.
RVA(0x00054cb0, 0x77)
void DrawStatusPortrait(BmpFile* bmp) {
    RECT source;

    if (bmp == NULL) {
        CopySurfaceSquare(
            g_statusPortraitPicture.surface,
            g_npcTexture.surface,
            g_npcTexture.width
        );
    } else {
        LoadBitmapToSurface16(bmp, &g_statusPortraitPicture.surface, NULL);
    }
    source.left = 0;
    source.bottom = 256;
    source.right = 256;
    source.top = 0;
    g_statusPicture.surface
        ->BltFast(148, 20, g_statusPortraitPicture.surface, &source, DDBLTFAST_SRCCOLORKEY);
}

// The strip each scroll clears as well.
DATA(0x0006db68)
static RECT s_scrollStripRect = {0, 304, 640, 328};

// Scrolls text plane `plane`'s framed map area (a 16-pixel border, 24 pixels
// at the bottom) down by one cell and clears the row it opens.
RVA(0x00054d30, 0x9b)
void ScrollPlaneMapDown(i16 plane) {
    TextPlane* p = GetTextPlane(plane);
    RECT rect;

    rect.right = p->sourceRect.right - MAP_CELL;
    rect.left = MAP_CELL;
    rect.top = MAP_CELL;
    rect.bottom = p->sourceRect.bottom - 40;
    p->glyphSurface->BltFast(MAP_CELL, 2 * MAP_CELL, p->glyphSurface, &rect, DDBLTFAST_NOCOLORKEY);
    rect.bottom = 2 * MAP_CELL;
    p->glyphSurface->Blt(&rect, NULL, &rect, DDBLT_COLORFILL, &g_clearBltFx);
    p->glyphSurface
        ->Blt(&s_scrollStripRect, NULL, &s_scrollStripRect, DDBLT_COLORFILL, &g_clearBltFx);
}

// Scrolls it left by one cell and clears the column it opens.
RVA(0x00054dd0, 0xa5)
void ScrollPlaneMapLeft(i16 plane) {
    TextPlane* p = GetTextPlane(plane);
    RECT rect;

    rect.left = 2 * MAP_CELL;
    rect.top = MAP_CELL;
    rect.right = p->sourceRect.right - MAP_CELL;
    rect.bottom = p->sourceRect.bottom - 24;
    p->glyphSurface->BltFast(MAP_CELL, MAP_CELL, p->glyphSurface, &rect, DDBLTFAST_NOCOLORKEY);
    rect.left = rect.right - MAP_CELL;
    p->glyphSurface->Blt(&rect, NULL, &rect, DDBLT_COLORFILL, &g_clearBltFx);
    p->glyphSurface
        ->Blt(&s_scrollStripRect, NULL, &s_scrollStripRect, DDBLT_COLORFILL, &g_clearBltFx);
}

// Scrolls it up by one cell and clears the row it opens.
RVA(0x00054e80, 0xa5)
void ScrollPlaneMapUp(i16 plane) {
    TextPlane* p = GetTextPlane(plane);
    RECT rect;

    rect.left = MAP_CELL;
    rect.top = 2 * MAP_CELL;
    rect.right = p->sourceRect.right - MAP_CELL;
    rect.bottom = p->sourceRect.bottom - 24;
    p->glyphSurface->BltFast(MAP_CELL, MAP_CELL, p->glyphSurface, &rect, DDBLTFAST_NOCOLORKEY);
    rect.top = rect.bottom - MAP_CELL;
    p->glyphSurface->Blt(&rect, NULL, &rect, DDBLT_COLORFILL, &g_clearBltFx);
    p->glyphSurface
        ->Blt(&s_scrollStripRect, NULL, &s_scrollStripRect, DDBLT_COLORFILL, &g_clearBltFx);
}

// Scrolls it right by one cell and clears the column it opens.
RVA(0x00054f30, 0x9c)
void ScrollPlaneMapRight(i16 plane) {
    TextPlane* p = GetTextPlane(plane);
    RECT rect;

    rect.right = p->sourceRect.right - 2 * MAP_CELL;
    rect.left = MAP_CELL;
    rect.top = MAP_CELL;
    rect.bottom = p->sourceRect.bottom - 24;
    p->glyphSurface->BltFast(2 * MAP_CELL, MAP_CELL, p->glyphSurface, &rect, DDBLTFAST_NOCOLORKEY);
    rect.right = 2 * MAP_CELL;
    p->glyphSurface->Blt(&rect, NULL, &rect, DDBLT_COLORFILL, &g_clearBltFx);
    p->glyphSurface
        ->Blt(&s_scrollStripRect, NULL, &s_scrollStripRect, DDBLT_COLORFILL, &g_clearBltFx);
}

// The navigation pad buttons' positions on the pad (buttons 1..4) and the
// button under each 32x32 cell.
DATA(0x0006db78)
static POINT s_padPositions[4] = {{32, 0}, {32, 64}, {0, 32}, {64, 32}};

DATA(0x0006db98)
static GZ_ENUM_STORAGE(NavPadButton, i32) s_padGrid[10] = {
    PAD_NONE,
    PAD_FORWARD,
    PAD_NONE,
    PAD_LEFT,
    PAD_RELEASED,
    PAD_RIGHT,
    PAD_NONE,
    PAD_BACK,
    PAD_NONE,
    PAD_NONE,
};

// The character panel's commands: the ids shown on its eight lines (-1 for
// none), the character it shows, and the handlers by command id.
DATA(0x00090bd8)
static i16 s_panelCommandIds[PANEL_COMMAND_ROWS];

DATA(0x00090b10)
static i16 s_shownCharacter;

// the pun: DdsCommand, StatusCommand and SetEncounterPending take no
// argument; the table calls every command with the character. Id 8 (the
// encounter command FillCharacterCommands lists in render mode 6) is the
// ninth handler.
DATA(0x0006dbc0)
static void (*s_panelCommands[PANEL_COMMAND_COUNT])(i16 character) = {
    FightCommand,
    GunCommand,
    SkillCommand,
    ItemCommand,
    DefenceCommand,
    ReturnCommand,
    reinterpret_cast<void (*)(i16)>(DdsCommand),          // the pun: see the table
    reinterpret_cast<void (*)(i16)>(StatusCommand),       // the pun: see the table
    reinterpret_cast<void (*)(i16)>(SetEncounterPending), // the pun: see the table
};

// The pad button last pressed down.
DATA(0x00090bcc)
static GZ_ENUM_STORAGE(NavPadButton, i32) s_pressedPadButton;

// The party panels' last drawn states.
DATA(0x00090af8)
static i32 s_panelStates[PARTY_PANEL_COUNT];

// Frees every layer: its surfaces and the record.
RVA(0x00054fd0, 0x4e)
void FreeScreenLayers(void) {
    i32 i;

    for (i = 0; i < SCREEN_LAYER_COUNT; i++) {
        if (g_layerStack[i] != NULL) {
            ReleaseComObject(g_layerStack[i]->surface);
            ReleaseComObject(g_layerStack[i]->canvas);
            delete g_layerStack[i];
        }
    }
}

RVA(0x00055020, 0x3f)
b32 DrawPadButton(
    LPDIRECTDRAWSURFACE surface, GZ_ENUM_PARAM(NavPadButton, i32) button, b32 pressed) {
    if (button < PAD_FIRST || button > PAD_LAST) {
        return false;
    }
    return BlitImage(
        surface,
        g_padImages[button - PAD_FORWARD][pressed],
        s_padPositions[button - PAD_FORWARD].x,
        s_padPositions[button - PAD_FORWARD].y
    );
}

// Paints a new layer's surface for its slot: the menu bar and its buttons,
// the icon, the toggled layers' frames, the navigation pad with the compass,
// the party panels; FALSE when a blit fails.
RVA(0x00055060, 0x155)
static b32 PaintLayer(GZ_ENUM_PARAM(ScreenLayerSlot, i32) slot, ScreenLayer* layer) {
    i32 i;

    switch (slot) {
        case SCREEN_LAYER_MENU_BAR:
            if (!BlitImage(layer->surface, g_layerImages[SCREEN_LAYER_MENU_BAR], 0, 0)) {
                return false;
            }
            for (i = 0; i < MENU_BUTTON_COUNT; i++) {
                if (!BlitImage(
                        layer->surface,
                        g_menuButtonImages[i].up,
                        g_menuButtonX[i],
                        MENU_BAR_TOP
                    )) {
                    return false;
                }
            }
            return true;
        case SCREEN_LAYER_ICON:
            if (!BlitImage(layer->surface, g_iconLayerImages[0], 0, 0)) {
                return false;
            }
            break;
        case SCREEN_LAYER_MOON_PHASE:
        case SCREEN_LAYER_LOCATION:
        case SCREEN_LAYER_CURRENCY:
        case SCREEN_LAYER_AUTOMAP:
        case SCREEN_LAYER_TEXT:
            if (!BlitImage(layer->surface, g_layerImages[slot], 0, 0)) {
                return false;
            }
            break;
        case SCREEN_LAYER_FIRST_PANEL:
        case SCREEN_LAYER_FIRST_PANEL + 1:
        case SCREEN_LAYER_FIRST_PANEL + 2:
        case SCREEN_LAYER_FIRST_PANEL + 3:
        case SCREEN_LAYER_FIRST_PANEL + 4:
        case SCREEN_LAYER_FIRST_PANEL + 5:
            if (!BlitImage(layer->surface, g_panelImages[0], 0, 0)) {
                return false;
            }
            break;
        case SCREEN_LAYER_NAVIGATION:
            DrawPadButton(layer->surface, PAD_FORWARD, false);
            DrawPadButton(layer->surface, PAD_BACK, false);
            DrawPadButton(layer->surface, PAD_LEFT, false);
            DrawPadButton(layer->surface, PAD_RIGHT, false);
            if (!BlitImage(layer->surface, g_compassImages[VIEW_NORTH], 32, 32)) {
                return false;
            }
            break;
    }
    return true;
}

// Creates the layer of slot `slot`: a keyed system-memory surface of its size
// in the primary's pixel format (and a second one for the slots that have a
// work surface), painted for its slot; returns nonzero on failure.
RVA(0x000551c0, 0x206)
b32 CreateScreenLayer(GZ_ENUM_PARAM(ScreenLayerSlot, i32) slot) {
    DDSURFACEDESC primary;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    ScreenLayer* layer;

    layer = new ScreenLayer;
    if (layer == NULL) {
        return true;
    }
    primary.dwSize = sizeof(primary);
    primary.dwFlags = DDSD_ALL;
    if (g_primarySurface->GetSurfaceDesc(&primary) != DD_OK) {
        return false;
    }
    InitOffscreenSurfaceDesc(
        desc,
        s_layerSize[slot].cx,
        s_layerSize[slot].cy,
        primary.ddpfPixelFormat
    );
    if (g_ddraw->CreateSurface(&desc, &layer->surface, NULL) != DD_OK) {
        delete layer;
        return true;
    }
    ZeroMemory(&key, sizeof(key));
    layer->surface->SetColorKey(DDCKEY_SRCBLT, &key);
    if (!PaintLayer(slot, layer)) {
        ReleaseComObject(layer->surface);
        delete layer;
        return true;
    }
    if (s_layerHasWorkSurface[slot]) {
        if (g_ddraw->CreateSurface(&desc, &layer->canvas, NULL) != DD_OK) {
            ReleaseComObject(layer->surface);
            delete layer;
            return true;
        }
        layer->canvas->SetColorKey(DDCKEY_SRCBLT, &key);
    } else {
        layer->canvas = NULL;
    }
    layer->visible =
        slot > SCREEN_LAYER_DEFAULT_VISIBLE_BEGIN - 1 && slot < SCREEN_LAYER_DEFAULT_VISIBLE_END;
    if (slot == SCREEN_LAYER_TEXT) {
        layer->visible = true;
    }
    layer->slot = slot;
    layer->x = s_layerOrigin[slot].x;
    layer->y = s_layerOrigin[slot].y;
    layer->bltFlags = DDBLTFAST_SRCCOLORKEY;
    layer->source.left = 0;
    layer->source.top = 0;
    layer->source.right = s_layerSize[slot].cx;
    layer->source.bottom = s_layerSize[slot].cy;
    g_layerStack[slot] = layer;
    g_screenLayers[slot] = layer;
    return false;
}

// Whether (x, y) falls on `layer`.
#define LAYER_HIT(layer, px, py)                                                                   \
    ((layer)->visible && (layer)->x <= (px) && (layer)->x + (layer)->source.right > (px)           \
     && (layer)->y <= (py) && (layer)->y + (layer)->source.bottom > (py))

// The slot of the topmost layer at (x, y), SCREEN_LAYER_NONE for none; on
// the navigation pad only its opaque pixels count, and the pad button under
// them becomes the pressed and held one.
RVA(0x000553d0, 0x142)
GZ_ENUM_RETURN(ScreenLayerSlot, i32) LayerAtPoint(u32 x, u32 y) {
    i32 i;
    ScreenLayer* layer;
    LPDIRECTDRAWSURFACE surface;
    DDSURFACEDESC desc;
    i32 dx;
    i32 dy;
    u16 pixel;

    for (i = 0; i < SCREEN_LAYER_COUNT - 1; i++) {
        layer = g_layerStack[i];
        if (LAYER_HIT(layer, x, y)) {
            if (layer->slot != SCREEN_LAYER_NAVIGATION) {
                break;
            }
            dx = x - layer->x;
            dy = y - layer->y;
            ZeroMemory(&desc, sizeof(desc));
            surface = layer->surface;
            desc.dwSize = sizeof(desc);
            desc.dwFlags = DDSD_CAPS;
            desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
            surface->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
            pixel = static_cast<u16*>(desc.lpSurface)[dy * PAD_SIZE + dx];
            layer->surface->Unlock(desc.lpSurface);
            if (pixel) {
                g_heldPadButton = s_pressedPadButton = s_padGrid[dy / PAD_CELL * 3 + dx / PAD_CELL];
                break;
            }
        }
    }
    if (i < SCREEN_LAYER_COUNT - 1) {
        return g_layerStack[i]->slot;
    }
    return SCREEN_LAYER_NONE;
}

// The stack index of the topmost layer at (x, y), -1 for none.
RVA(0x00055520, 0x53)
i32 LayerIndexAtPoint(u32 x, u32 y) {
    i32 i;

    for (i = 0; i < SCREEN_LAYER_COUNT - 1; i++) {
        if (LAYER_HIT(g_layerStack[i], x, y)) {
            break;
        }
    }
    if (i >= SCREEN_LAYER_COUNT - 1) {
        i = -1;
    }
    return i;
}

// Whether (x, y) is still on the pad button that was pressed (updating the
// held button).
RVA(0x00055580, 0x112)
b32 PadButtonAtPoint(u32 x, u32 y) {
    ScreenLayer* layer = g_screenLayers[SCREEN_LAYER_NAVIGATION];
    LPDIRECTDRAWSURFACE surface;
    DDSURFACEDESC desc;
    i32 dx;
    i32 dy;
    u16 pixel;

    if (LAYER_HIT(layer, x, y) && layer->slot == SCREEN_LAYER_NAVIGATION) {
        dx = x - layer->x;
        dy = y - layer->y;
        ZeroMemory(&desc, sizeof(desc));
        surface = layer->surface;
        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS;
        desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
        surface->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
        pixel = static_cast<u16*>(desc.lpSurface)[dy * PAD_SIZE + dx];
        layer->surface->Unlock(desc.lpSurface);
        if (pixel) {
            g_heldPadButton = s_padGrid[dy / PAD_CELL * 3 + dx / PAD_CELL];
            return g_heldPadButton == s_pressedPadButton;
        }
    }
    return false;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x000556a0, 0x1)
void UpdateLayers(void) {}

RVA(0x000556b0, 0x16)
i16 GetShownPanelCharacter(void) {
    if (g_screenLayers[SCREEN_LAYER_PANEL]->visible == true) {
        return s_shownCharacter;
    }
    return -1;
}

// A click on the character panel at screen row `y`: runs the command on that
// line for the shown character; FALSE off the commands.
RVA(0x000556d0, 0x85)
b32 ClickPanelCommand(u32 y) {
    i32 line;
    u32 top;

    y -= g_screenLayers[SCREEN_LAYER_PANEL]->y;
    for (line = 0, top = 0; top < PANEL_LINES * PANEL_LINE_HEIGHT;
         line++, top += PANEL_LINE_HEIGHT) {
        if (y >= top && y < top + PANEL_LINE_HEIGHT) {
            if (s_panelCommandIds[line] < 0) {
                return false;
            }
            BlitImage(
                g_screenLayers[SCREEN_LAYER_PANEL]->surface,
                g_commandImages[s_panelCommandIds[line]].pressed,
                0,
                line * PANEL_LINE_HEIGHT
            );
            s_panelCommands[s_panelCommandIds[line]](s_shownCharacter);
            return true;
        }
    }
    return false;
}

// A party panel (slot `slot`) released: clicked in place (`dragged` clear) it
// opens the member's command panel; dropped elsewhere the member moves to the
// panel position under the drop point.
RVA(0x00055760, 0x174)
void ReleasePartyPanel(GZ_ENUM_PARAM(ScreenLayerSlot, i32) slot, b32 dragged) {
    ScreenLayer* layer = g_screenLayers[slot];
    i32 line;
    i32 x;
    i32 y;
    i32 target;

    if (!dragged) {
        if (GetFieldBattleActive() && !GetTickElapsed()) {
            return;
        }
        if (PickPartyMember(slot - SCREEN_LAYER_FIRST_PANEL) <= PARTY_MEMBER_UNAVAILABLE) {
            return;
        }
        s_shownCharacter = GetPartyMemberId(slot - SCREEN_LAYER_FIRST_PANEL);
        FillCharacterCommands(s_panelCommandIds, s_shownCharacter);
        for (line = 0; line < PANEL_LINES && s_panelCommandIds[line] >= 0; line++) {
            BlitImage(
                g_screenLayers[SCREEN_LAYER_PANEL]->surface,
                g_commandImages[s_panelCommandIds[line]].up,
                0,
                line * PANEL_LINE_HEIGHT
            );
        }
        g_screenLayers[SCREEN_LAYER_PANEL]->visible = true;
        return;
    }
    if (g_dragRect.left < 0) {
        g_dragRect.left = 0;
    } else if (g_dragRect.left + layer->source.right > SCREEN_WIDTH) {
        g_dragRect.left = SCREEN_WIDTH - layer->source.right;
    }
    if (g_dragRect.top < VIEW_HEIGHT) {
        g_dragRect.top = VIEW_HEIGHT;
    } else if (g_dragRect.top + layer->source.bottom > SCREEN_HEIGHT) {
        g_dragRect.top = SCREEN_HEIGHT - layer->source.bottom;
    }
    x = g_dragRect.left + g_dragOffset.x;
    y = g_dragRect.top;
    y += g_dragOffset.y;
    if (y < PANEL_ROW_SPLIT) {
        if (x < PANEL_COLUMN_SPLIT) {
            target = 0;
        } else {
            target = (x >= PANEL_COLUMN_SPLIT2) + 1;
        }
    } else if (x < PANEL_COLUMN_SPLIT) {
        target = 3;
    } else {
        target = (x >= PANEL_COLUMN_SPLIT2) + 4;
    }
    SwapPartySlots(slot - SCREEN_LAYER_FIRST_PANEL, target);
    if (slot - SCREEN_LAYER_FIRST_PANEL != target) {
        RedrawPartyStatus();
    }
}

// Moves the dragged layer to the drop position (kept on the 3D view area) and
// to the top of the dragged layers.
RVA(0x000558e0, 0xb1)
void PlaceDraggedLayer(GZ_ENUM_PARAM(ScreenLayerSlot, i32) slot) {
    ScreenLayer* layer = g_screenLayers[slot];
    i32 i;

    if (g_dragRect.left < 0) {
        g_dragRect.left = 0;
    } else if (g_dragRect.left + layer->source.right > SCREEN_WIDTH) {
        g_dragRect.left = SCREEN_WIDTH - layer->source.right;
    }
    if (g_dragRect.top < 0) {
        g_dragRect.top = 0;
    } else if (g_dragRect.top + layer->source.bottom > VIEW_HEIGHT) {
        g_dragRect.top = VIEW_HEIGHT - layer->source.bottom;
    }
    layer->x = g_dragRect.left;
    layer->y = g_dragRect.top;
    for (i = DRAGGED_LAYER_FIRST; i < DRAGGED_LAYER_END; i++) {
        if (g_layerStack[i]->slot == layer->slot) {
            break;
        }
    }
    for (; i > DRAGGED_LAYER_FIRST; i--) {
        g_layerStack[i] = g_layerStack[i - 1];
    }
    g_layerStack[DRAGGED_LAYER_FIRST] = layer;
}

// Redraws the party panels whose member state changed.
RVA(0x000559a0, 0x57)
void UpdateLayerPanels(void) {
    i32 i;
    i32 state;

    for (i = 0; i < PARTY_PANEL_COUNT; i++) {
        if (g_screenLayers[SCREEN_LAYER_FIRST_PANEL + i]->visible) {
            state = GetMemberPanelState(i);
            if (state >= 0 && s_panelStates[i] != state) {
                s_panelStates[i] = state;
                BlitImage(
                    g_screenLayers[SCREEN_LAYER_FIRST_PANEL + i]->surface,
                    g_panelImages[state],
                    0,
                    0
                );
            }
        }
    }
}
