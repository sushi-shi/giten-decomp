#ifndef GITEN_TEXT_TEXTPLANE_H
#define GITEN_TEXT_TEXTPLANE_H

#include <rva.h>

#include <Win32.h>

#include <Enums.h>
#include <Ints.h>
#include <Text/TextAttr.h>
#include <Text/MenuLineFlags.h>
#include <Text/TextEvent.h>

#define TEXT_PLANE_COUNT 37

// No text plane: CreateTextPlane's failure result and a closed plane handle.
#define TEXT_PLANE_NONE (-1)

void SaveAndResetTextPlaneAttrs(i16 plane);
void ForgetTextPlaneAttr(i16 plane);

// The most rows and columns a plane's text grid holds.
#define TEXT_PLANE_MAX_ROWS 20
#define TEXT_PLANE_MAX_COLS 80

// Plane kinds with their own handling: the analyze screen's name plate
// (kept by ClearTextPlaneText; its image while exploring), the menu laid out
// in two columns, and the numeric keypad at the upper right (indent 1,
// with its display cleared only in the 134x32 top).
#define TEXT_PLANE_KIND_ANALYZE_NAME 0x0f
#define TEXT_PLANE_KIND_TWO_COLUMN_MENU 0x13
#define TEXT_PLANE_KIND_KEYPAD 0x23

// TextPlane.kind of a free slot.
#define TEXT_PLANE_FREE 0xffff

// TextPlaneFlags.highlight: the transform ToggleTextRunHighlight applies.
#define TEXT_HIGHLIGHT_MIDDLE 0
#define TEXT_HIGHLIGHT_OUTER 1

// @identity-TODO: the role of flag8 is unrecovered.
typedef struct TextPlaneFlags {
    i16 indentEnabled : 1;
    i16 savedIndentEnabled : 1;
    i16 attrSaved : 1;
    i16 cancelEnabled : 1;
    i16 twoColumns : 1; // menu lines fill two columns
    i16 highlight : 3;
    i16 flag8 : 1;
} TextPlaneFlags;

// One line of a text plane's menu, kept on a List.h list.
typedef struct MenuLine {
    struct MenuLine* next;
    struct MenuLine* prev;
    char* text;
    i16 attr;
    i16 value;
    GZ_ENUM_STORAGE(MenuLineFlags, i16) flags;
} MenuLine;

// A cell position or extent on a text plane, returned by value.
typedef struct TextPoint {
    i16 x;
    i16 y;
} TextPoint;

// One text window: a grid of Shift-JIS cells with per-cell attributes
// rendered through two DirectDraw surfaces.
typedef struct TextPlane {
    u16 kind; // 0xffff marks a free slot
    i16 arg;
    i16 cols;
    i16 rows;
    i16 cursorX;
    i16 cursorY;
    i16 indent;     // the wrap indent, applied while flags.indentEnabled is set
    i16 headerRows; // rows preserved above the scrolling text area
    i16 lineStep;
    TextAttr attr;
    TextAttr normalAttr;
    TextAttr accentAttr;
    TextAttr savedAttr;
    TextPlaneFlags flags;
    u16 deferredChar; // a character held back while the plane was full
    u8** text;
    TextAttr** attrs;
    i16 firstSelectableRow;
    i16 menuX; // where the first menu line starts
    i16 menuY;
    i16 highlightX; // -1 when no run is highlighted
    i16 highlightY;
    struct MenuLine* menuLines;
    b32 visible;
    i32 left;
    i32 top;
    RECT sourceRect;
    struct IDirectDrawSurface* surface;
    struct IDirectDrawSurface* glyphSurface;
} TextPlane;

#define TextPlaneTextRow(textPlane, rowIndex) ((textPlane)->text[(rowIndex)])

#define TextPlaneAttrRow(textPlane, rowIndex) ((textPlane)->attrs[(rowIndex)])

static __inline void StoreTextCell(u8* text, TextAttr* attrs, i16 column, u8 byte, u16 attr) {
    text[column] = byte;
    attrs[column].value = attr;
}

#define SaveCurrentTextAttr(textPlane)                                                             \
    do {                                                                                           \
        (textPlane)->savedAttr = (textPlane)->attr;                                                \
        (textPlane)->flags.attrSaved = 1;                                                          \
    } while (0)

extern TextPlane g_textPlanes[TEXT_PLANE_COUNT];

#define GetTextPlane(index) (&g_textPlanes[(index)])

// Called on text-plane events: (plane, event, value).
typedef void (*TextPlaneHook)(i16 plane, i16 event, i16 value);

#define IsTwoByteTextChar(code) ((code) >= 0x100)

i16 ReadTextChar(u16* code, const u8* text, i16 pos);

i16 DrawTextCell(i16 plane, u16 code, u16 attr, i16 px, i16 y);
// Advances a byte-column cursor and the drawn pixel cursor independently.
#define DrawNextTextCell(plane, textPlane, column, rowIndex, pixelX)                               \
    do {                                                                                           \
        u16 cellCode;                                                                              \
        u16 cellAttr = TextPlaneAttrRow(textPlane, rowIndex)[(column)].value;                      \
        if (TextPlaneTextRow(textPlane, rowIndex)[(column)] == 0) {                                \
            (column)++;                                                                            \
        } else {                                                                                   \
            i16 nextColumn =                                                                       \
                ReadTextChar(&cellCode, TextPlaneTextRow(textPlane, rowIndex), (column));          \
            (pixelX) = DrawTextCell((plane), cellCode, cellAttr, (pixelX), (rowIndex));            \
            (column) = nextColumn;                                                                 \
        }                                                                                          \
    } while (0)

void RedrawTextPlane(i16 plane);
void RedrawTextRun(i16 plane, i16 x, i16 y, i16 count);
i16 ToggleTextRunHighlight(i16 plane, i16 x, i16 y);
i16 SetTextRunAttr(i16 plane, i16 x, i16 y, u16 attr);

void RefreshTextPlane(i16 plane);
void ToggleTextHighlight(i16 plane, i16 x, i16 y);
void PaintTextRun(i16 plane, i16 x, i16 y, u16 attr);
TextPlaneHook SetTextPlaneHook(TextPlaneHook hook);
void CallTextPlaneHook(i16 plane, i16 event, i16 value);

i16 ClearTextPlaneLine(i16 plane, i16 row);
void MoveTextPlaneCursorToPrevLine(i16 plane);
void HomeTextPlaneCursor(i16 plane);
void FreeMenuLines(i16 plane);
MenuLine* GetMenuLine(i16 plane, i16 index);
i16 GetMenuLineAt(i16 plane, i16 x, i16 y);
i16 SetMenuLineAttr(i16 plane, i16 index, i16 attr);
TextPoint GetMenuLinePos(i16 plane, i16 index);
TextPoint GetTextPlaneSize(i16 plane);

// @identity-TODO: Why only code 0x0a cells get a glyph and other codes step a row is
// unexplained; a closer read of the loop vs RedrawTextPlane would confirm the name.
// @identity-TODO: every caller passes a second word (1, 3, 4 or -2) that the
// body does not appear to read.
// @identity-TODO: every caller passes a line step (2 or 3), but the body resets
// the step to 1 and returns the old one.
i16 ResetTextPlaneLineStep(i16 plane, i16 step);

void RepaintTextPlane(i16 plane, i16 mode);

// Finds a menu line by its selection value, or NULL when absent.
RVA_DECL(0x000534d0)
MenuLine* FindMenuLineByValue(i16 plane, i16 value);

// Writes one character to `plane` with the message-text state; returns its
// width while the text delay is on (else 0), or -2 when the plane has no room
// (the character is then deferred on the plane).
// `choosing` bypasses punctuation-dependent line-breaking slack.
i16 PutTextChar(i16 plane, u16 ch, struct TextState* state, i16 choosing);

#endif // GITEN_TEXT_TEXTPLANE_H
