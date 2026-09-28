// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. Mouse input on menu panels.

#include <rva.h>

#include <Input/Mouse.h>
#include <Sound/Sound.h>
#include <Ui/Panel.h>

#include <stddef.h>

DATA(0x000919f4)
i16 g_panelClickY;

DATA(0x000919f8)
i16 g_panelClickX;

// The panel being polled (NULL outside a poll).
DATA(0x0007d610)
static Panel* s_activePanel;

// While set, a row click plays no sound.
DATA(0x0007d614)
static i16 s_panelSilent;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0001cae0, 0xc)
void SetPanelSilent(i16 silent) {
    s_panelSilent = silent;
}

RVA(0x0001caf0, 0xe)
b16 IsPanelActive(i16 value) {
    return s_activePanel != NULL;
}

RVA(0x0001cb00, 0x10)
Panel* ExchangeActivePanel(Panel* panel) {
    Panel* old = s_activePanel;
    s_activePanel = panel;
    return old;
}

// Latches a left click (or, when held-button input is enabled, the held button) and its
// position; clears the right-click mark.
RVA(0x0001cb10, 0x65)
b16 CheckPanelLeftClick(Panel* panel) {
    panel->input.rightClick = 0;
    g_panelClickX = g_mouseLeftClickX;
    g_panelClickY = g_mouseLeftClickY;
    if (!g_mouseLeftClick) {
        if (!(panel->flags & PANEL_HELD_BUTTON_INPUT)) {
            return false;
        }
        if (!(g_mousePosition.buttons & MOUSE_LEFT_DOWN)) {
            return false;
        }
        g_panelClickX = g_mousePosition.x;
        g_panelClickY = g_mousePosition.y;
    }
    return true;
}

// The same for the right button on panels that allow it;
// marks the panel (bit 0).
RVA(0x0001cb80, 0x7a)
b16 CheckPanelRightClick(Panel* panel) {
    if (!(panel->flags & PANEL_ALLOW_RIGHT_CLICK)) {
        return false;
    }
    if (panel->flags & PANEL_IGNORE_RIGHT_CLICK) {
        return false;
    }
    g_panelClickX = g_mouseRightClickX;
    g_panelClickY = g_mouseRightClickY;
    if (!g_mouseRightClick) {
        if (!(panel->flags & PANEL_HELD_BUTTON_INPUT)) {
            return false;
        }
        if (!(g_mousePosition.buttons & MOUSE_RIGHT_DOWN)) {
            return false;
        }
        g_panelClickX = g_mousePosition.x;
        g_panelClickY = g_mousePosition.y;
    }
    panel->input.rightClick = 1;
    return true;
}

// The row whose hotspot x/y hits (skipping hidden or disabled rows, and on a
// right-click the rows that refuse it), or -1.
RVA(0x0001cc00, 0x78)
i16 FindPanelRowAt(Panel* panel, i16 x, i16 y) {
    i16 i;
    u16 flags;
    if (panel->flags & (PANEL_HIDDEN | PANEL_INPUT_DISABLED | 0x0800)) {
        return -1;
    }
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        flags = GetPanelRow(panel, i)->flags;
        if (flags & (PANEL_HIDDEN | PANEL_INPUT_DISABLED | 0x0800)) {
            continue;
        }
        if (WasPanelRightClicked(panel) && (flags & PANEL_IGNORE_RIGHT_CLICK)) {
            continue;
        }
        if (HitTestPanelRow(panel, GetPanelRowId(GetPanelRow(panel, i)), x, y, flags) > 0) {
            return i;
        }
    }
    return -1;
}

// Calls row `row`'s handler with `op` unless the panel or the row is locked
// (PANEL_HANDLER_LOCKED); returns the handler's result, else `row`.
RVA(0x0001cc80, 0x37)
i16 RunPanelRow(Panel* panel, i16 row, i16 op) {
    if (!(panel->flags & PANEL_HANDLER_LOCKED)
        && !(GetPanelRow(panel, row)->flags & PANEL_HANDLER_LOCKED)) {
        row = GetPanelRow(panel, row)->handler(GetPanelRow(panel, row), row, op);
    }
    return row;
}

// Runs the row under the latched click (with the click sound); -1 when none.
RVA(0x0001ccc0, 0x69)
i16 ClickPanel(Panel* panel) {
    i16 row;
    s_activePanel = panel;
    row = FindPanelRowAt(panel, g_panelClickX, g_panelClickY);
    if (row == -1) {
        s_activePanel = NULL;
        return row;
    }
    if (!s_panelSilent) {
        PlaySoundEffect(1);
    }
    row = RunPanelRow(panel, row, -1);
    s_activePanel = NULL;
    return row;
}

RVA(0x0001cd30, 0x42)
i16 PollPanel(Panel* panel) {
    s_activePanel = panel;
    if (!CheckPanelLeftClick(panel) && !CheckPanelRightClick(panel)) {
        s_activePanel = NULL;
        return -1;
    }
    return ClickPanel(panel);
}

RVA(0x0001cd80, 0x5f)
i16 ApplyRowCheck(PanelRow* row, i16 value, i16 op) {
    i16 result = 0;
    switch (op) {
        case -1:
            g_mouseLeftClick = 0;
            result = ToggleFlagBits(&row->flags, PANEL_ROW_CHECKED);
            break;
        case 0:
            ClearPanelRowCheck(row);
            break;
        case 1:
            SetFlagBits(&row->flags, PANEL_ROW_CHECKED);
            result = 1;
            break;
    }
    return result;
}
