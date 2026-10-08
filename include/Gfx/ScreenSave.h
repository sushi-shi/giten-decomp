#ifndef GITEN_GFX_SCREENSAVE_H
#define GITEN_GFX_SCREENSAVE_H

#include <rva.h>

#include <Gfx/VramAccess.h>
#include <Ints.h>

// Redraw request consumed by RedrawScreen and field state updates.
extern i16 g_fieldRedrawRequest;

void RestoreSavedCursor(i16* offset);

void SetTextCursorOffset(i16* offset, i16 column, i16 row);

// @identity-TODO: ApplyTextCursor is an empty Windows body; its argument
// type is unrecovered.
// Screen saves contain four handles, one per 100-scanline strip.
void ApplyTextCursor();
b32 AllocScreenSave(i32* save);
b32 FreeScreenSave(i32* save);
b32 CaptureScreenSave(i32* save);
b32 RestoreScreenSave(i32* save);

static __inline void CaptureScreenSaveWithState(i32* save) {
    i16 state = SaveDrawState();
    CaptureScreenSave(save);
    RestoreDrawState(state);
}

static __inline void RestoreScreenSaveWithState(i32* save) {
    i16 state = SaveDrawState();
    RestoreScreenSave(save);
    RestoreDrawState(state);
}

void RequestRefresh(void);

// @identity-TODO: The second argument is never read (callers push 0 or 1); confirm whether it
// is a PC-98 leftover parameter when the unit is matched.
b16 RedrawScreen(i16 drawView, i16 unused);

#endif // GITEN_GFX_SCREENSAVE_H
