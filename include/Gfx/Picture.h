#ifndef GITEN_GFX_PICTURE_H
#define GITEN_GFX_PICTURE_H

#include <Win32.h>

#include <Enums.h>
#include <Ints.h>

// @identity-TODO: event image identities remain unknown; these exclusive
// range bounds and individual images control the cursor's activation delay.
// clang-format off
GZ_ENUM_BEGIN(EventCursorPicture)
    EVENT_CURSOR_DELAY_EXCLUSIVE_BEGIN = 0x50d6,
    EVENT_CURSOR_DELAY_END = 0x50e5,
    EVENT_CURSOR_DELAY_SINGLE = 0x5099,
    EVENT_CURSOR_ACTIVE_EXCLUSIVE_BEGIN = 0x50cf,
    EVENT_CURSOR_ACTIVE_END = 0x50d7,
    EVENT_CURSOR_ACTIVE_ALT_EXCLUSIVE_BEGIN = 0x507f,
    EVENT_CURSOR_ACTIVE_ALT_END = 0x5090,
    EVENT_CURSOR_ACTIVE_SINGLE = 0x50f6
GZ_ENUM_END(EventCursorPicture)

// A picture: an offscreen surface with the source rectangle blitted from it.
// CreatePicture (0x457380) zeroes the 0x24-byte record, sizes the surface and
// sets the rectangle's right and bottom; LoadScenePicture fills it from a
// bitmap.
typedef struct Picture {
    b32 visible;
    RECT rect;
    LPDIRECTDRAWSURFACE surface;
    u32 id;
    i32 surfaceWidth;
    i32 surfaceHeight;
} Picture;
// clang-format on

// The status screen's picture (640x440, blitted at y 40).
extern Picture g_statusPicture;

// The command bar shown at (16, 16) in the picture render mode (608x24).
extern Picture g_commandBarPicture;

// The 128x56 title-menu picture containing NEW GAME and CONTINUE.
extern Picture g_titleMenuPicture;

// The 24x16 staging picture keyed icons are drawn through.
extern Picture g_iconStagingPicture;

#endif // GITEN_GFX_PICTURE_H
