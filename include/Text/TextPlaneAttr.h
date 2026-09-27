#ifndef GITEN_TEXT_TEXTPLANEATTR_H
#define GITEN_TEXT_TEXTPLANEATTR_H

#include <rva.h>

#include <Ints.h>

// The text-plane attribute and cursor calls the script's window colour ops
// make (defined in the font TU). Codegen constraint: kept out of
// <Text/TextPlane.h>, which the font TU itself includes.
void ResetTextPlaneAttr(i16 plane);
void ReverseTextPlaneAttr(i16 plane);
void SetTextPlaneColor(i16 plane, i16 which, u16 color);
void SaveTextPlaneAttr(i16 plane);
void RestoreTextPlaneAttr(i16 plane);
// Callers test only the low word (the font TU defines it returning i32).
i16 IsTextPlaneAttrSaved(i16 plane);
void RestoreTextPlaneIndentMode(i16 plane);
u16 SetTextPlaneAttr(i16 plane, u16 attr);
void SaveTextPlaneIndentMode(i16 plane);
i16 SetTextPlaneIndentEnabled(i16 plane, i16 on);
// Callers of these getters use only the low word (the font TU defines them
// returning i32).
i16 GetTextPlaneCursorX(i16 plane);
i16 GetTextPlaneCursorY(i16 plane);
i16 GetTextPlaneHeaderRows(i16 plane);
i16 GetTextPlaneIndent(i16 plane);
i16 GetTextPlaneLineStep(i16 plane);
i16 SetTextPlaneIndent(i16 plane, i16 indent);
i16 ApplyTextPlaneIndent(i16 plane);
i16 GetActiveTextPlaneIndent(i16 plane);
void GetTextPlaneOrigin(i16 plane, i16* x, i16* y);
void ReverseTextRun(i16 plane, i16 x, i16 y, i16 count);
void BlankTextRun(i16 plane, i16 x, i16 y, i16 count);
i32 GetTextPlaneNormalAttr(i16 plane);
i32 GetTextPlaneAccentAttr(i16 plane);
void ResetTextPlaneNormalAttr(i16 plane);
void ResetTextPlaneAccentAttr(i16 plane);
u16 SetTextPlaneNormalAttr(i16 plane, u16 attr);
u16 SetTextPlaneAccentAttr(i16 plane, u16 attr);

// Scrolls a window up one line.
RVA_DECL(0x00053370)
void ScrollTextWindowLine(i16 plane);

#endif // GITEN_TEXT_TEXTPLANEATTR_H
