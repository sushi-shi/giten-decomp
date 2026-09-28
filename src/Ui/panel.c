// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. Flag-word and menu-panel
// helpers.

#include <rva.h>

#include <Game/FieldHud.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Ui/Panel.h>

#include <stddef.h>

DATA(0x00068ec0)
static const i16 s_kindFirstRow[] = {-1, 18, -1, 14, 16, 6,  -1, -1, -1, 4,  16, -1, -1, -1,
                                     -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, 8,  12, -1,
                                     2,  -1, 0,  20, -1, 10, -1, -1, -1, 0,  0,  0};

DATA(0x00068f10)
static const i16 s_imageFirstRow[] =
    {30, 63, -1, 32, 50, -1, 35, 47, 80, -1, -1, -1, -1, -1, 39, 43};

DATA(0x00068f30)
static i16 s_panelImage = -1;

RVA(0x000226c0, 0xc)
void SetPanelImage(i16 image) {
    s_panelImage = image;
}

static __inline Panel* InitPanelImage(Panel* panel, i16 image, i16 count) {
    if (panel == NULL) {
        panel = AllocCleared(1, offsetof(Panel, rows) + count * sizeof(PanelRow));
        panel->image = 0;
    }
    panel->count = count;
    if (panel->image) {
        panel->image = FreeImageHandle(panel->image);
    }
    panel->image = LoadMenuImage(image);
    panel->flags = 0;
    SetPanelPosition(panel, 0, 0);
    return panel;
}

RVA(0x000226d0, 0xc0)
Panel* CreateImagePanel(Panel* panel, i16 image, i16 count, i16 unused) {
    i16 first;
    i16 i;
    panel = InitPanelImage(panel, image, count);
    if (image == 0x12) {
        image = 0;
    } else if (image == 0x110 && count == 13) {
        image = 1;
    }
    first = s_imageFirstRow[image & 15];
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, first + i, PanelRowHandlerDefault);
    }
    return panel;
}

RVA(0x00022790, 0x26)
i16 PanelRowHandlerDefault(PanelRow* row, i16 value, i16 op) {
    ApplyRowCheck(row, value, op);
    IsPanelActive(value);
    return value;
}

RVA(0x000227c0, 0xa6)
Panel* CreateKindPanel(Panel* panel, i16 image, i16 count, i16 kind) {
    i16 first;
    i16 i;
    panel = InitPanelImage(panel, image, count);
    first = s_kindFirstRow[kind];
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, first + i, PanelRowHandlerDefault);
    }
    return panel;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x00022870, 0x99)
Panel* CreateSequentialPanel(Panel* panel, i16 image, i16 count) {
    i16 i;
    panel = InitPanelImage(panel, image, count);
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, image + i, PanelRowHandlerDefault);
    }
    return panel;
}

RVA(0x00022910, 0x70)
Panel* CreatePositionedPanel(Panel* panel, i16 x, i16 y, i16 count, i16 kind) {
    if (s_panelImage == -1) {
        s_panelImage = 0x112;
    }
    panel = CreateKindPanel(panel, s_panelImage, count, kind);
    SetPanelPosition(panel, x == -1 ? 0 : x, y == -1 ? 0 : y);
    s_panelImage = -1;
    return panel;
}

RVA(0x00022980, 0x3c)
Panel* ReleasePanel(Panel* panel, i16 freePanel) {
    if (panel == NULL) {
        return NULL;
    }
    ClearPanelChecks(panel);
    panel->image = FreeImageHandle(panel->image);
    if (freePanel) {
        panel = FreeBlock(panel);
    }
    return panel;
}

RVA(0x000229c0, 0x22)
void SetPanelRowState(Panel* panel, i16 index, u16 flags) {
    if (index < GetPanelRowCount(panel)) {
        AssignPanelRowState(panel, index, flags);
    }
}

RVA(0x000229f0, 0x25)
void PaintPanel(Panel* panel, i16 mode) {
    i16 token = SaveDrawState();
    DrawPanel(panel, NULL, mode);
    RestoreDrawState(token);
}

RVA(0x00022a20, 0x6d)
i16 RunPanelInput(Panel* panel) {
    i16 result;
    if (panel == NULL) {
        return -1;
    }
    ExchangeActivePanel(panel);
    if (!(panel->flags & 2) && TakeMouseCancelSound()) {
        ClearMouseSelection();
        ExchangeActivePanel(NULL);
        return -2;
    }
    result = PollPanel(panel);
    // Codegen constraint: preserve the explicit no-selection result assignment.
    if (result == -1) {
        result = -1;
    }
    ExchangeActivePanel(NULL);
    return result;
}

// @identity-TODO: a second entry of ClearPanelChecks (its callers are the
// menu code).
RVA(0x00022a90, 0xe)
void ClearPanelChecksAgain(Panel* panel) {
    ClearPanelChecks(panel);
}

RVA(0x00022aa0, 0xd)
void SetFlagBits(u16* flags, u16 mask) {
    *flags |= mask;
}

RVA(0x00022ab0, 0xe)
void ClearFlagBits(u16* flags, u16 mask) {
    *flags &= ~mask;
}

RVA(0x00022ac0, 0x11)
i16 ToggleFlagBits(u16* flags, u16 mask) {
    *flags ^= mask;
    mask &= *flags;
    return mask;
}

RVA(0x00022ae0, 0x15)
b32 TestFlagBits(u16* flags, u16 mask) {
    return (*flags & mask) != 0;
}

RVA(0x00022b00, 0x21)
b32 TestPanelRowFlags(Panel* panel, i16 row, u16 mask) {
    return TestFlagBits(&GetPanelRow(panel, row)->flags, mask);
}

RVA(0x00022b30, 0x4a)
void SetPanelRowFlags(Panel* panel, i16 row, u16 mask, i16 on) {
    if (on == 0) {
        ClearFlagBits(&GetPanelRow(panel, row)->flags, mask);
        return;
    }
    SetFlagBits(&GetPanelRow(panel, row)->flags, mask);
}

RVA(0x00022b80, 0x15)
b32 IsPanelRowChecked(Panel* panel, i16 row) {
    return TestPanelRowFlags(panel, row, PANEL_ROW_CHECKED);
}

RVA(0x00022ba0, 0x2f)
void ClearPanelChecks(Panel* panel) {
    i16 i;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        ClearPanelRowCheck(GetPanelRow(panel, i));
    }
}

// Copies each row's check (bit 0) to bit 4 and bit 15 to bit 7, then clears
// the checks.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00022bd0, 0x84)
void SavePanelChecks(Panel* panel) {
    i16 i;
    PanelRow* row;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        row = GetPanelRow(panel, i);
        ClearFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK);
        if (TestFlagBits(&row->flags, PANEL_ROW_CHECKED) == 1) {
            SetFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK);
        }
        ClearFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED);
        if (TestFlagBits(&row->flags, PANEL_INPUT_DISABLED) == 1) {
            SetFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED);
        }
    }
    ClearPanelChecks(panel);
}

// Brings the saved bits back (bit 4 to the check, bit 7 to bit 15).
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00022c60, 0x7a)
void RestorePanelChecks(Panel* panel) {
    i16 i;
    PanelRow* row;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        row = GetPanelRow(panel, i);
        ClearPanelRowCheck(row);
        if (TestFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK) == 1) {
            SetFlagBits(&row->flags, PANEL_ROW_CHECKED);
        }
        ClearFlagBits(&row->flags, PANEL_INPUT_DISABLED);
        if (TestFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED) == 1) {
            SetFlagBits(&row->flags, PANEL_INPUT_DISABLED);
        }
    }
}

RVA(0x00022ce0, 0xd)
void SetPanelFlags(Panel* panel, u16 mask) {
    panel->flags |= mask;
}

RVA(0x00022cf0, 0xe)
void ClearPanelFlags(Panel* panel, u16 mask) {
    panel->flags &= ~mask;
}
