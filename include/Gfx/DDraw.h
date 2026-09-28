#ifndef GITEN_GFX_DDRAW_H
#define GITEN_GFX_DDRAW_H

#include <Win32.h>

#define IsPalettizedSurface(desc) ((desc).ddpfPixelFormat.dwRGBBitCount < 16)

// The DirectDraw interface owned by the display device setup.
extern IDirectDraw* g_ddraw;

// Black colour-fill parameters initialized by the display setup and shared
// by surface clears.
extern DDBLTFX g_clearBltFx;

#define ClearDisplaySurface(surface, rect)                                                         \
    IDirectDrawSurface_Blt(surface, rect, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &g_clearBltFx)

// The 16-bit surface pixel format: each 8-bit channel is shifted right by
// its loss and left by its shift to form a pixel.
extern u8 g_redShift;
extern u8 g_greenShift;
extern u8 g_blueShift;
extern u8 g_redLoss;
extern u8 g_greenLoss;
extern u8 g_blueLoss;

#define WriteSurfacePixel16(destination, offset, pixel)                                            \
    (((destination)[offset] = LOBYTE(pixel)), ((destination)[(offset) + 1] = HIBYTE(pixel)))

static __inline DWORD PackSurfaceColor(u8 red, u8 green, u8 blue) {
    DWORD blueValue = blue >> g_blueLoss;
    DWORD greenValue = green >> g_greenLoss;
    DWORD redValue = red >> g_redLoss;
    return (blueValue << g_blueShift) | (greenValue << g_greenShift) | (redValue << g_redShift);
}

#endif // GITEN_GFX_DDRAW_H
