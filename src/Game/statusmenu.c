// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The status screen's
// command menu panel.

#include <rva.h>

#include <Game/CharacterStat.h>
#include <Game/StatusScreen.h>
#include <Input/Mouse.h>
#include <Text/Font.h>
#include <Ui/Panel.h>

#include <stddef.h>

static i16 StatusMenuRowHandler(PanelRow* row, i16 value, i16 op);

// The status screen's command menu: two rows without an id (flag PANEL_HIDDEN),
// then items 0x36..0x3e.
DATA(0x0006a070)
static struct {
    Panel panel;
    PanelRow more[10];
} s_statusMenu = {
    {0, 0, 0, 11, 0, 0, 0, {0}, {{PANEL_HIDDEN, -1, 0, StatusMenuRowHandler}}},
    {
        {PANEL_HIDDEN, -1, 0, StatusMenuRowHandler},
        {0, 0x36, 0, StatusMenuRowHandler},
        {0, 0x37, 0, StatusMenuRowHandler},
        {0, 0x38, 0, StatusMenuRowHandler},
        {0, 0x39, 0, StatusMenuRowHandler},
        {0, 0x3a, 0, StatusMenuRowHandler},
        {0, 0x3b, 0, StatusMenuRowHandler},
        {0, 0x3c, 0, StatusMenuRowHandler},
        {0, 0x3d, 0, StatusMenuRowHandler},
        {0, 0x3e, 0, StatusMenuRowHandler},
    },
};

// @identity-TODO: the items SetStatusMenuItemsHidden switches together.
DATA(0x00064988)
static const i16 s_switchedItems[] = {4, 5, 6, 7, 8, 9, 10, 3, -1};

RVA(0x00041910, 0x22)
i16 DrawStatusLine(i16 x, i16 y, const char* text, i32 attr) {
    DrawStatusText(x, y, text, attr);
    return y + 2;
}

RVA(0x00041940, 0x1d)
i16 DrawStatusLabel(i16 x, i16 y, const char* text) {
    return DrawStatusLine(x, y, text, TEXT_ATTR_DEFAULT);
}

RVA(0x00041960, 0x1d)
static i16 StatusMenuRowHandler(PanelRow* row, i16 value, i16 op) {
    ApplyRowCheck(row, value, op);
    return value;
}

RVA(0x00041980, 0x10)
void ClearStatusMenu(void) {
    ClearPanel(&s_statusMenu.panel, NULL);
}

// The item picked in the status menu; -2 on a cancel click, -1 for none.
RVA(0x00041990, 0x26)
i16 PollStatusMenu(void) {
    i16 item;
    if (TakeMouseCancelSound()) {
        return -2;
    }
    item = PollPanel(&s_statusMenu.panel);
    if (item == -1) {
        return -1;
    }
    return item;
}

RVA(0x000419c0, 0x1d)
void SetStatusMenuItemFlag(i16 item, i16 flag, i16 on) {
    SetPanelRowFlags(&s_statusMenu.panel, item, flag, on);
}

RVA(0x000419e0, 0x52)
void ResetStatusMenu(void) {
    ClearPanelChecks(&s_statusMenu.panel);
    SetStatusMenuItemFlag(6, PANEL_HIDDEN, false);
    SetStatusMenuItemFlag(7, PANEL_HIDDEN, false);
    SetStatusMenuItemFlag(8, PANEL_HIDDEN, false);
    SetStatusMenuItemFlag(9, PANEL_HIDDEN, false);
}

// Checks item `item` and unchecks the others.
RVA(0x00041a40, 0x29)
void CheckStatusMenuItem(i16 item) {
    i16 i;
    for (i = 0; i < STAT_COUNT; i++) {
        if (i != item) {
            SetStatusMenuItemFlag(i, PANEL_ROW_CHECKED, false);
        } else {
            SetStatusMenuItemFlag(i, PANEL_ROW_CHECKED, true);
        }
    }
}

RVA(0x00041a70, 0x37)
void SetStatusMenuItemsHidden(i16 on) {
    i16 i;
    for (i = 0; s_switchedItems[i] != -1; i++) {
        SetStatusMenuItemFlag(s_switchedItems[i], PANEL_HIDDEN, on);
    }
}
