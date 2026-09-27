#ifndef GITEN_GFX_SCREENSAVE_H
#define GITEN_GFX_SCREENSAVE_H

#include <rva.h>

#include <Gfx/VramAccess.h>
#include <Ints.h>

void RestoreSavedCursor(i16* offset);

void SetTextCursorOffset(i16* offset, i16 column, i16 row);

// @identity-TODO: empty Windows bodies. Text drawing calls the first after
// computing a cursor offset; the others bracket a screen area kept in an
// object: allocate, capture (inside a draw-state pair), and on close restore
// (inside a draw-state pair) and free. Their argument types are not
// recovered, so they are declared without a prototype.
void ApplyTextCursor();
i32 AllocScreenSave();
i32 FreeScreenSave();
i32 CaptureScreenSave();
i32 RestoreScreenSave();

static __inline void CaptureScreenSaveWithState(void* save) {
    i16 state = SaveDrawState();
    CaptureScreenSave(save);
    RestoreDrawState(state);
}

static __inline void RestoreScreenSaveWithState(void* save) {
    i16 state = SaveDrawState();
    RestoreScreenSave(save);
    RestoreDrawState(state);
}

void RequestRefresh(void);

// @identity-TODO: The second argument is never read (callers push 0 or 1); confirm whether it
// is a PC-98 leftover parameter when the unit is matched.
i16 RedrawScreen(i16 drawView, i16 unused);

#endif // GITEN_GFX_SCREENSAVE_H
