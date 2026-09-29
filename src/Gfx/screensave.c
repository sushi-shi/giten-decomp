// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Automap.h>
#include <Game/FieldScreen.h>
#include <Game/InfoBar.h>
#include <Game/ModeFlags.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VramAccess.h>

#include <stddef.h>

// A refresh request transferred to the field redraw flag by RedrawScreen.
DATA(0x00075fec)
static b16 s_refreshRequested;

DATA(0x00091550)
i16 g_fieldRedrawRequest;

// Text screen offset of a cell on the 80-column grid.
RVA(0x000038e0, 0x1c)
void SetTextCursorOffset(i16* offset, i16 column, i16 row) {
    if (offset != NULL) {
        *offset = row * 80 + column;
    }
}

RVA(0x00003900, 0x15)
void RestoreSavedCursor(i16* offset) {
    if (offset != NULL) {
        i16 token = SaveDrawState();
        RestoreDrawState(token);
    }
}

RVA(0x00003920, 0x1)
void ApplyTextCursor() {}

RVA(0x00003930, 0x3)
b32 AllocScreenSave() {
    return false;
}

RVA(0x00003940, 0x3)
b32 FreeScreenSave() {
    return false;
}

RVA(0x00003950, 0x3)
b32 CaptureScreenSave() {
    return false;
}

RVA(0x00003960, 0x3)
b32 RestoreScreenSave() {
    return false;
}

RVA(0x00003970, 0xa)
void RequestRefresh(void) {
    s_refreshRequested = true;
}

RVA(0x00003980, 0x77)
b16 RedrawScreen(i16 drawView, i16 unused) {
    FieldScreenNop(0);
    if (!TestModeFlags(MODE_WORLD_MAP)) {
        UpdateInfoBar();
    } else {
        RefreshInfoBar(1);
    }
    if (drawView) {
        DrawMapOverlay(g_party.field.pos);
    }
    g_fieldRedrawRequest = s_refreshRequested;
    s_refreshRequested = false;
    return true;
}
