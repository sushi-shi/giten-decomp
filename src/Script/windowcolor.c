// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Script/ScriptOps.h>
#include <Script/TextState.h>
#include <Script/WindowReverseMode.h>
#include <Text/TextPlane.h>
#include <Text/TextPlaneAttr.h>
#include <Text/TextWindow.h>

// Mode 1 resets the window's attribute and reverses it, 0 only resets it, -1
// only reverses it.
RVA(0x0002ea80, 0x46)
void SetWindowReverse(i16 window, GZ_ENUM_PARAM(WindowReverseMode, i16) mode) {
    switch (mode) {
        case WINDOW_ATTR_RESET_AND_REVERSE:
            ResetTextPlaneAttr(window);
        case WINDOW_ATTR_REVERSE:
            ReverseTextPlaneAttr(window);
            break;
        case WINDOW_ATTR_RESET:
            ResetTextPlaneAttr(window);
            break;
    }
}

RVA(0x0002ead0, 0x19)
void OpSetWindowColor(i16 window, GZ_ENUM_PARAM(TextColorPart, i16) part) {
    SetTextPlaneColor(window, part, ReadScriptValue());
}

RVA(0x0002eaf0, 0x23)
void StashWindowColor(i16 window, i32 save) {
    if (save == 1) {
        SaveTextPlaneAttr(window);
    } else {
        RestoreTextPlaneAttr(window);
    }
}

// Saves the window's attribute (restoring an earlier save first), switches to
// `attr` and restarts the line.
RVA(0x0002eb20, 0x67)
void SwitchWindowAttr(i16 window, u16 attr) {
    if (IsTextPlaneAttrSaved(window)) {
        RestoreTextPlaneAttr(window);
        RestoreTextPlaneIndentMode(window);
    }
    SaveTextPlaneAttr(window);
    SetTextPlaneAttr(window, attr);
    SaveTextPlaneIndentMode(window);
    SetTextPlaneIndentEnabled(window, 0);
    SetTextPlaneCursor(window, 0, GetTextPlaneCursorY(window));
}

// Returns the window to its saved attribute.
RVA(0x0002eb90, 0x22)
void RestoreWindowAttr(i16 window) {
    RestoreTextPlaneAttr(window);
    RestoreTextPlaneIndentMode(window);
    ApplyTextPlaneIndent(window);
}

// @identity-TODO: what the alternate color at +0x14 is used for in play (emphasis vs. choice
// text) is unrecovered
RVA(0x0002ebc0, 0x25)
void BeginWindowAltText(i16 window) {
    SwitchWindowAttr(window, GetTextPlaneNormalAttr(window));
    SetTextWindowScrollTop(window, 0);
}

RVA(0x0002ebf0, 0x1b)
void EndWindowAltText(i16 window) {
    RestoreWindowAttr(window);
    SetTextWindowScrollTop(window, 2);
}

// @identity-TODO: the role of the second alternate color +0x16 is unrecovered
RVA(0x0002ec10, 0x48)
void BeginWindowInstantText(i16 window) {
    SwitchWindowAttr(window, GetTextPlaneAccentAttr(window));
    SetTextWindowScrollTop(window, 2);
    ClearTextPlane(window);
    RepaintTextPlane(window, 3);
    ClearTextPeriod();
    PushTextDelay(0);
}

RVA(0x0002ec60, 0x1e)
void EndWindowInstantText(i16 window) {
    RestoreWindowAttr(window);
    PopTextDelay();
    AdvanceWindowLine(window);
}

RVA(0x0002ec80, 0xe)
void ResetWindowAltColor(i16 window) {
    ResetTextPlaneNormalAttr(window);
}

RVA(0x0002ec90, 0x14)
void OpSetWindowAltColor(i16 window) {
    SetTextPlaneNormalAttr(window, ReadScriptValue());
}

RVA(0x0002ecb0, 0xe)
void ResetWindowInstantColor(i16 window) {
    ResetTextPlaneAccentAttr(window);
}

RVA(0x0002ecc0, 0x14)
void OpSetWindowInstantColor(i16 window) {
    SetTextPlaneAccentAttr(window, ReadScriptValue());
}
