#ifndef GITEN_TEXT_TEXTWINDOW_H
#define GITEN_TEXT_TEXTWINDOW_H

#include <rva.h>

// Per-window text attributes and cursor of the window table.

// Keeps a zero background colour opaque when enabled.
RVA_DECL(0x000528b0)
void SetWindowOpaqueBg(i16 window, i32 on);

// @identity-TODO: The role of attribute flag bit 1 (bit 13) is unrecovered.
RVA_DECL(0x000528f0)
void SetWindowAttrFlag1(i16 window, i32 on);

// Draws glyphs eight pixels wide instead of sixteen when enabled.
RVA_DECL(0x00052930)
void SetWindowHalfWidth(i16 window, i32 on);

// @identity-TODO: The role of attribute flag bit 2 (bit 14) is unrecovered.
RVA_DECL(0x00052970)
void SetWindowAttrFlag2(i16 window, i32 on);

// Closes a window; window 0 stays allocated and is hidden.
i16 CloseTextWindow(i16 window);

// Sets the first shown row of a window; returns the previous one (0 for no
// window).
RVA_DECL(0x000533b0)
i16 SetTextWindowScrollTop(i16 window, i16 top);

RVA_DECL(0x00053bc0)
u16 TakeWindowDeferredChar(i16 window);

RVA_DECL(0x00053bf0)
i16 AdvanceWindowLine(i16 window);

i16 CreateTextPlane(u16 kind, i16 arg);

RVA_DECL(0x000529f0)
void SetTextPlaneCursor(i16 plane, i16 x, i16 y);

// Sets the first selectable row, converting pos from lines when inLines is set.
i16 SetTextPlaneFirstSelectableRow(i16 plane, i16 pos, i16 inLines);

// @identity-TODO: The return (0 for -1, else mode left in ax) may be an artefact of a void
// source.
i16 SetTextPlaneHighlightMode(i16 plane, i16 mode);

// Resets the menu; cancelEnabled of -1 preserves its cancellation mode.
void ResetTextPlaneMenu(i16 plane, i16 line, i16 cancelEnabled);

void ClearTextPlaneHighlight(i16 plane);

struct MenuLine;
struct MenuLine* AddMenuLine(i16 plane, const char* text, i16 attr, i16 value, i16 flags);

void SetTextPlaneMenuOrigin(i16 plane, i16 x, i16 y);
i16 SetTextPlaneCancelEnabled(i16 plane, i16 on);
void DrawStatusImage(i16 x, i16 y, i16 index);
void DrawPlaneImage(i16 plane, i16 x, i16 y, i16 index);

i16 PollMenuInput(i16 plane);

RVA_DECL(0x00053cd0)
void EraseTextPlaneText(i16 plane);

void ClearTextPlane(i16 plane);

#endif // GITEN_TEXT_TEXTWINDOW_H
