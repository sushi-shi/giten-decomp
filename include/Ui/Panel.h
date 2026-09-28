#ifndef GITEN_UI_PANEL_H
#define GITEN_UI_PANEL_H

#include <rva.h>

#include <Ints.h>

// Panel input policy; hidden, locked and disabled also apply to rows.
typedef enum PanelFlags {
    PANEL_ALLOW_RIGHT_CLICK = 0x0002,
    PANEL_IGNORE_RIGHT_CLICK = 0x0004,
    PANEL_HIDDEN = 0x1000,
    PANEL_HANDLER_LOCKED = 0x2000,
    PANEL_HELD_BUTTON_INPUT = 0x4000,
    PANEL_INPUT_DISABLED = 0x8000
} PanelFlags;

typedef enum PanelRowFlags {
    PANEL_ROW_CHECKED = 0x0001,
    PANEL_ROW_SAVED_CHECK = 0x0010,
    PANEL_ROW_SAVED_INPUT_DISABLED = 0x0080
} PanelRowFlags;

// A menu panel: a flags word, a row count and a picture handle, then a table
// of 10-byte rows. Each row has a flags word (bit 0 checked, bit 4 saved
// check, bit 7 saved bit 15), an id, and a handler called as
// (row, value, op). The two panels at 0x468858 (2 rows) and 0x4686a8
// (7 rows) are initialized data.
// x/y place the panel, left/top offset its text origin (a cell index is
// (top + y) * 80 + left + x).
// @identity-TODO: the header bytes +0x10..+0x13 and the row word +4 are
// unread in the claimed code.
struct PanelRow;
typedef i16 (*PanelRowHandler)(struct PanelRow* row, i16 value, i16 op);

typedef struct PanelRow {
    u16 flags;
    i16 id;
    i16 word04;
    PanelRowHandler handler;
} PanelRow;

typedef struct Panel {
    union {
        u16 flags;
        struct {
            u16 rightClick : 1;
            u16 : 15;
        } input;
    };
    i16 x;
    i16 y;
    i16 count;
    u32 image;
    i16 left;
    i16 top;
    u8 pad10[4];
    PanelRow rows[1];
} Panel;

#define GetPanelTextCell(panel) (((panel)->top + (panel)->y) * 80 + (panel)->left + (panel)->x)

static __inline i16 GetPanelRowCount(const Panel* panel) {
    return panel->count;
}

static __inline PanelRow* GetPanelRow(Panel* panel, i16 index) {
    return &panel->rows[index];
}

static __inline i32 IsPanelRowHidden(Panel* panel, i16 index) {
    return GetPanelRow(panel, index)->flags & PANEL_HIDDEN;
}

void SetPanelImage(i16 image);
Panel* CreateImagePanel(Panel* panel, i16 image, i16 count, i16 unused);
Panel* CreateKindPanel(Panel* panel, i16 image, i16 count, i16 kind);
Panel* CreateSequentialPanel(Panel* panel, i16 image, i16 count);
Panel* CreatePositionedPanel(Panel* panel, i16 x, i16 y, i16 count, i16 kind);
Panel* ReleasePanel(Panel* panel, i16 freePanel);
i16 PanelRowHandlerDefault(PanelRow* row, i16 value, i16 op);
void SetPanelRowState(Panel* panel, i16 index, u16 flags);
void PaintPanel(Panel* panel, i16 mode);
i16 RunPanelInput(Panel* panel);

// Keep separate panel evaluation and coordinate-store order.
#define SetPanelPosition(panel, xValue, yValue) ((panel)->x = (xValue), (panel)->y = (yValue))

static __inline i16 WasPanelRightClicked(const Panel* panel) {
    return panel->input.rightClick;
}

static __inline void AssignPanelRowState(Panel* panel, i16 index, u16 flags) {
    GetPanelRow(panel, index)->flags = flags;
}

static __inline void InitPanelRow(Panel* panel, i16 index, i16 id, PanelRowHandler handler) {
    GetPanelRow(panel, index)->id = id;
    GetPanelRow(panel, index)->word04 = 0;
    GetPanelRow(panel, index)->handler = handler;
    AssignPanelRowState(panel, index, 0);
}

void SetFlagBits(u16* flags, u16 mask);
void ClearFlagBits(u16* flags, u16 mask);

static __inline void ClearPanelRowCheck(PanelRow* row) {
    ClearFlagBits(&row->flags, PANEL_ROW_CHECKED);
}

i16 ToggleFlagBits(u16* flags, u16 mask);
b32 TestFlagBits(u16* flags, u16 mask);
b32 TestPanelRowFlags(Panel* panel, i16 row, u16 mask);
void SetPanelRowFlags(Panel* panel, i16 row, u16 mask, i16 on);
b32 IsPanelRowChecked(Panel* panel, i16 row);
void ClearPanelChecks(Panel* panel);
void ClearPanelChecksAgain(Panel* panel);
void SavePanelChecks(Panel* panel);
void RestorePanelChecks(Panel* panel);
void SetPanelFlags(Panel* panel, u16 mask);
void ClearPanelFlags(Panel* panel, u16 mask);

// The shared row handler body: op -1 toggles the row's check, 0 clears it,
// 1 sets it; returns whether it is now set.
i16 ApplyRowCheck(PanelRow* row, i16 value, i16 op);

// @identity-TODO: the hotspot helpers the panels use (0x453fd0 draws or
// highlights hotspot `id` in `mode`; 0x454230 tests x/y against it).
void HighlightHotspot(i16 mode, i16 id, i16 on);

i16 HitTestHotspot(i16 id, i16 x, i16 y, i16 strict);

i16 GetPanelRowId(PanelRow* row);
void DrawPanel(Panel* panel, i16* cell, i16 mode);
void DrawPanelRow(Panel* panel, i16 x, i16 y, i16 row, i16 mode);
void ClearPanel(Panel* panel, i16* cell);
void ClearPanelRow(Panel* panel, i16 x, i16 y, i16 row);
i16 HitTestPanelRow(Panel* panel, i16 id, i16 x, i16 y, u8 flags);

// The panel input layer (Ui/panelinput.c).
void SetPanelSilent(i16 silent);
// @identity-TODO: the sole caller passes a row value that this Windows body ignores.
b16 IsPanelActive(i16 value);
Panel* ExchangeActivePanel(Panel* panel);
b16 CheckPanelLeftClick(Panel* panel);
b16 CheckPanelRightClick(Panel* panel);
i16 FindPanelRowAt(Panel* panel, i16 x, i16 y);
i16 RunPanelRow(Panel* panel, i16 row, i16 op);
i16 ClickPanel(Panel* panel);
i16 PollPanel(Panel* panel);

// The panel input position, taken from a latched click or the held-button cursor.
extern i16 g_panelClickX;
extern i16 g_panelClickY;

#endif // GITEN_UI_PANEL_H
