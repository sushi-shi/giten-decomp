#ifndef GITEN_TEXT_FONT_H
#define GITEN_TEXT_FONT_H

#include <Win32.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Gfx/Bitmap.h>
#include <Text/TextAttr.h>
#include <Text/TextBand.h>
#include <Text/TextPlane.h>

#include <stdio.h>

u8* RenderGlyph(u16 code, u8* glyph);
void BlitGlyph(i32 x, i32 y, u16 attr, HDC dc, u8* glyph, i32 width);

// The text TU's (font.cpp) C-callable functions as it defines them
// (<Text/TextPlaneAttr.h> gives some of them i16 results for its callers).
i16 ApplyTextPlaneIndent(i16 plane);
void BlankTextRun(i16 plane, i16 x, i16 y, i16 count);
void SaveTextPlaneIndentMode(i16 plane);
void RestoreTextPlaneIndentMode(i16 plane);
void DrawPlaneIcon(i16 plane, i16 x, i16 y, i16 icon, i16 frame);
void DrawPlaneIconKeyed(i16 plane, i16 x, i16 y, i16 icon, i16 frame);
void DrawPlaneImage(i16 plane, i16 x, i16 y, i16 index);
// clang-format off
GZ_ENUM_BEGIN(StatBarMark)
    STAT_BAR_MARK_BASE = 0,
    STAT_BAR_MARK_BONUS = 1,
    STAT_BAR_MARK_EMPTY = 2,
    STAT_BAR_MARK_EQUIPMENT = 3,
    STAT_BAR_MARK_BASE_OVER_50 = 4
GZ_ENUM_END(StatBarMark);
// clang-format on

void DrawStatBarMark(i16 x, i16 y, i16 index, i16 plane);
void DrawStatusImage(i16 x, i16 y, i16 index);
i16 FindTextPlaneByKind(i16 kind);
i16 GetActiveTextPlaneIndent(i16 plane);
i16 GetMenuLineAttr(i16 plane, i16 index);
i32 GetTextPlaneAccentAttr(i16 plane);
i32 GetTextPlaneAttr(i16 plane);
TextAttr* GetTextPlaneAttrRow(i16 plane, i16 y);
void GetTextPlaneCursor(i16 plane, i16* x, i16* y);
i32 GetTextPlaneCursorX(i16 plane);
i32 GetTextPlaneCursorY(i16 plane);
i32 IsTextPlaneIndentEnabled(i16 plane);
i32 IsTextPlaneCancelEnabled(i16 plane);
i32 GetTextPlaneHeaderRows(i16 plane);
i32 GetTextPlaneIndent(i16 plane);
i16 GetTextPlaneLineCount(i16 plane);
i32 GetTextPlaneLineStep(i16 plane);
i32 GetTextPlaneNormalAttr(i16 plane);
void GetTextPlaneOrigin(i16 plane, i16* x, i16* y);
void GetTextPlaneOrigin2(i16 plane, i16* x, i16* y);
i16 GetTextPlanePageLines(i16 plane);
u8* GetTextPlaneRowText(i16 plane, i16 row);
i32 GetTextPlaneFirstSelectableRow(i16 plane);
i32 IsTextPlaneAttrSaved(i16 plane);
void MoveTextPlaneCursorX(i16 plane, i16 dx);
i16 ReserveTextPlaneCells(i16 plane, i16 width, i16 slack);
void ResetTextPlaneAccentAttr(i16 plane);
void ResetTextPlaneAttr(i16 plane);
void ResetTextPlaneHighlight(i16 plane);
void ResetTextPlaneNormalAttr(i16 plane);
void ResetTextPlanes(void);
void RestoreTextPlaneAttr(i16 plane);
void ReverseTextPlaneAttr(i16 plane);
void ReverseTextRun(i16 plane, i16 x, i16 y, i16 count);
void SaveTextPlaneAttr(i16 plane);
void ScrollTextPlaneSurface(i16 plane);
void ScrollTextPlaneText(i16 plane);
void ScrollTextWindowLine(i16 plane);
void SetMenuLineText(i16 plane, i16 index, const char* text);
u16 SetTextPlaneAccentAttr(i16 plane, u16 attr);
u16 SetTextPlaneAttr(i16 plane, u16 attr);
void SetTextPlaneColor(i16 plane, i16 which, u16 color);
void SetTextPlaneCursorLine(i16 plane, i16 x, i16 line);
i16 SetTextPlaneIndentEnabled(i16 plane, i16 on);
i16 SetTextPlaneCancelEnabled(i16 plane, i16 on);
i16 SetTextPlaneFlag8(i16 plane, i16 on);
void SetTextPlaneHighlight(i16 plane, i16 x, i16 y);
i16 SetTextPlaneIndent(i16 plane, i16 indent);
u16 SetTextPlaneNormalAttr(i16 plane, u16 attr);
void SetWindowDeferredChar(i16 window, u16 ch);
i16 TextPlaneRowToLine(i16 plane, i16 row);
void ToggleCurrentTextHighlight(i16 plane);

// The text drawn onto a plane, the status picture and the band pictures,
// and an unreferenced stub.
void DrawPlaneText(i16 plane, i16 x, i16 y, const char* text, i32 attr);
void DrawStatusText(i16 x, i16 y, const char* text, i32 attr);
b16 DrawBandText(i16 x, i16 y, const char* text, i32 attr, i16 band);
b16 ReturnZero(void);
void ClearTextPlaneText(i16 plane);
void PrintMenuLines(i16 plane);
i32 GetTextPlaneLineStepIf(i16 plane, i16 on);

// The automap and panel helpers (0x4545e0..0x454f30) the C game calls.
void DrawMapTile(i16 tile, i16 x, i16 y);
void DrawPlaneMapTileLit(i16 tile, i16 x, i16 y, i16 plane);
void DrawPlaneMapTile(i16 tile, i16 x, i16 y, i16 plane);
void DrawMapMark(i16 mark, i16 x, i16 y);
void DrawPlaneMapMark(i16 mark, i16 x, i16 y, i16 plane);
void ClearTextPlaneRight(i16 plane);
void DrawLayerGauge(i16 slot, u16 value, u16 max, i16 upper);
i16 PartyPanelAtPoint(i16 x, i16 y);
i16 TextPlaneCellAt(i16 plane, i16 x, i16 y, i16* col, i16* row);
i16 SaveScreenLayers(FILE* file);
i16 LoadScreenLayers(FILE* file);
void DrawIconLayerImage(i16 index);
void DrawStatusPortrait(BmpFile* bmp);
void ScrollPlaneMapDown(i16 plane);
void ScrollPlaneMapLeft(i16 plane);
void ScrollPlaneMapUp(i16 plane);
void ScrollPlaneMapRight(i16 plane);

// A layer's visibility and position as a save file keeps them
// (SaveScreenLayers).
typedef struct SavedLayer {
    b32 visible;
    i32 x;
    i32 y;
} SavedLayer;

// A row of s_hotspotImages, named from the button bitmaps it shows.
GZ_ENUM_BEGIN(HotspotImageGroup)
    HOTSPOT_IMAGES_NONE = 0,
    HOTSPOT_IMAGES_ARROW_UP = 1,
    HOTSPOT_IMAGES_ARROW_DOWN = 2,
    HOTSPOT_IMAGES_NEW_GAME = 3,
    HOTSPOT_IMAGES_CONTINUE = 4,
    HOTSPOT_IMAGES_PANEL_BUY = 5,
    HOTSPOT_IMAGES_PANEL_SELL = 6,
    HOTSPOT_IMAGES_PANEL_LEAVE = 7,
    HOTSPOT_IMAGES_PANEL_HEAL = 8,
    HOTSPOT_IMAGES_PANEL_CURE = 9,
    HOTSPOT_IMAGES_PANEL_CONSULT = 10,
    HOTSPOT_IMAGES_PANEL_OK = 11,
    HOTSPOT_IMAGES_PANEL_EXIT = 12,
    HOTSPOT_IMAGES_PANEL_CANCEL = 13,
    HOTSPOT_IMAGES_PANEL_ARM = 14,
    HOTSPOT_IMAGES_MODE_EXIT = 15,
    HOTSPOT_IMAGES_MODE_ITEM = 16,
    HOTSPOT_IMAGES_MODE_MAGIC = 17,
    HOTSPOT_IMAGES_MODE_ABILITY = 18,
    HOTSPOT_IMAGES_MODE_NEXT = 19,
    HOTSPOT_IMAGES_MODE_QUIT = 20,
    HOTSPOT_IMAGES_MODE_EQUIP = 21,
    HOTSPOT_IMAGES_MODE_GEM = 22,
    HOTSPOT_IMAGES_MODE_CHART = 23,
    HOTSPOT_IMAGES_ARROW_LEFT = 24,
    HOTSPOT_IMAGES_ARROW_RIGHT = 25,
    HOTSPOT_IMAGES_KEYPAD_0 = 26,
    HOTSPOT_IMAGES_KEYPAD_1 = 27,
    HOTSPOT_IMAGES_KEYPAD_2 = 28,
    HOTSPOT_IMAGES_KEYPAD_3 = 29,
    HOTSPOT_IMAGES_KEYPAD_4 = 30,
    HOTSPOT_IMAGES_KEYPAD_5 = 31,
    HOTSPOT_IMAGES_KEYPAD_6 = 32,
    HOTSPOT_IMAGES_KEYPAD_7 = 33,
    HOTSPOT_IMAGES_KEYPAD_8 = 34,
    HOTSPOT_IMAGES_KEYPAD_9 = 35,
    HOTSPOT_IMAGES_KEYPAD_CLEAR = 36,
    HOTSPOT_IMAGES_KEYPAD_OK = 37
GZ_ENUM_END(HotspotImageGroup)

// A hotspot area: its screen rectangle and image pair (HighlightHotspot).
typedef struct HotspotArea {
    i32 left;
    i32 top;
    i32 right;
    i32 bottom;
    GZ_ENUM_STORAGE(HotspotImageGroup, i32) images;
} HotspotArea;

// Hotspot areas by where they are drawn: below AREA_PANEL_LIMIT on their
// text plane, the two title-menu areas, from AREA_MODE_FIRST on the command
// bar, AREA_KEYPAD_FIRST..AREA_KEYPAD_LAST on the keypad plane.
#define AREA_PANEL_LIMIT 0x1e
#define AREA_MODE_FIRST 0x36
#define AREA_KEYPAD_FIRST 0x3f
#define AREA_KEYPAD_LAST 0x4b

// A text palette colour, stored red-first.
typedef struct TextColor {
    u8 r;
    u8 g;
    u8 b;
} TextColor;

// The colours SetTextPlaneColor sets: the glyph, the dim colour and the
// background.
#define TEXT_COLOR_GLYPH 0
#define TEXT_COLOR_DIM 1
#define TEXT_COLOR_BG 2

// A text plane kind's window: screen position, size and text grid.
typedef struct TextPlaneLayout {
    i32 left;
    i32 top;
    i32 width;
    i32 height;
    i32 cols;
    i32 rows;
} TextPlaneLayout;

// The frame image of the analyze name plate while the party explores.
#define ANALYZE_NAME_EXPLORING_IMAGE 0x117

// A double-byte Shift-JIS character's length (drawn 16 pixels wide).
#define SJIS_WIDE_BYTES 2

// Shift-JIS 0x8151, the full-width low line, drawn as a solid bottom row.
#define SJIS_LOW_LINE 0x8151

#endif // GITEN_TEXT_FONT_H
