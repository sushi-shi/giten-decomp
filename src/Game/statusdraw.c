// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/FieldHud.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/PartyPick.h>
#include <Game/PartyStatus.h>
#include <Game/StatusDraw.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenLayer.h>
#include <Input/Mouse.h>
#include <Ints.h>
#include <Platform/GameCalls.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/PartySlotSelection.h>
#include <Util/Scratch.h>

#include <stdio.h>

DATA(0x00069f00)
static i16 s_selectedPartySlot = -1;

DATA(0x00083b20)
Character* g_panelMembers[6];

// @identity-TODO: while set, the party-status redraw requests are ignored.
DATA(0x00083b18)
static i16 s_statusRedrawLocked;

DATA(0x00083b1c)
static b16 s_statusRedrawPending;

static __inline i32 GetSelectedPartySlot(void) {
    i16 slot;
    if (s_selectedPartySlot == -1) {
        slot = -1;
    } else {
        slot = s_selectedPartySlot;
    }
    return slot;
}

static __inline void CommitPartySlotSelection(void) {
    g_hoveredObjectId = GetSelectedPartySlot();
    g_selectedObjectId = g_hoveredObjectId;
}

RVA(0x0003e160, 0x85)
void RedrawPartyStatus(void) {
    i16 slot;
    i16 swapped;
    ClearTextPlane(g_infoPlane);
    for (slot = 0; slot < 6; slot++) {
        if (PartySlotAt(slot) == -1) {
            DrawPartyStatusSlot(slot, NULL);
        } else {
            swapped = GetSwappedMember(slot);
            if (swapped == -1) {
                DrawPartyStatusSlot(slot, NULL);
            } else if (swapped == -2) {
                DrawPartyStatusSlot(slot, GetPartyEntry(slot));
            } else {
                DrawPartyStatusSlot(slot, GetRosterCharacter(swapped));
            }
        }
    }
    ResetTextPlaneHighlight(g_infoPlane);
    s_selectedPartySlot = -1;
    s_statusRedrawPending = false;
}

RVA(0x0003e1f0, 0x164)
void DrawPartyStatusSlot(i16 slot, Character* character) {
    char name[36];
    i16 partySlot = slot;
    slot += 8;
    if (character == NULL) {
        g_panelMembers[partySlot] = NULL;
        ClearLayerSurface(slot);
        HideScreenLayer(slot);
        return;
    }
    g_panelMembers[partySlot] = character;
    ShowScreenLayer(slot);
    sprintf(g_scratchBuffer, "%-16s", FormatFullName(name, character));
    DrawLayerText(slot, 8, 8, g_scratchBuffer, 0x3450);
    sprintf(g_scratchBuffer, "%5d", character->pools.hp.cur);
    DrawLayerText(slot, 144, 24, g_scratchBuffer, 0x9450);
    sprintf(g_scratchBuffer, "%5d", character->pools.mp.cur);
    DrawLayerText(slot, 144, 40, g_scratchBuffer, 0x9450);
    DrawLayerGauge(slot, character->pools.hp.cur, character->pools.hp.max, 1);
    DrawLayerGauge(slot, character->pools.mp.cur, character->pools.mp.max, 0);
    sprintf(g_scratchBuffer, "%6s", GetFirstConditionName(GetCharacterConditions(character)));
    DrawLayerText(slot, 8, 44, g_scratchBuffer, 0x1450);
}

static __inline void RefreshPartyStatusIfNeeded(i16 force) {
    if (!s_statusRedrawLocked) {
        if (force || s_statusRedrawPending) {
            RedrawPartyStatus();
        }
        s_statusRedrawPending = false;
    }
}

RVA(0x0003e360, 0x2b)
void FlushStatusRedraw(i16 force) {
    RefreshPartyStatusIfNeeded(force);
}

RVA(0x0003e390, 0x2b)
void RefreshStatusPanel(i16 force) {
    RefreshPartyStatusIfNeeded(force);
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
// @early-stop: column and selection mode exchange ebx/edi; calls and semantic
// edges match. A separate filter result regresses both callers; C-safe TU states
// retain the register exchange, with one also extending the line-step lifetime.
RVA(0x0003e3c0, 0x1c1)
i16 PollTextPartySlotSelection(i16 mode) {
    i16 oldStep = ResetTextPlaneLineStep(g_infoPlane, 3);
    i16 x;
    i16 y;
    i16 column;
    i16 lineStep;
    i16 slot;
    SetTextPlaneHighlightMode(0, 1);
    if (g_mouseLeftClick) {
        CommitPartySlotSelection();
        ClearMouseClicks();
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return 1;
    }
    if (g_mouseRightClick) {
        ClearMouseSelection();
        ClearMouseClicks();
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return -1;
    }
    column = TextPlaneCellAt(g_infoPlane, g_mousePosition.x, g_mousePosition.y, &x, &y);
    if (column == 0 || column == 39) {
        lineStep = GetTextPlaneLineStep(g_infoPlane);
        slot = y / lineStep + (column ? 3 : 0);
        slot = FilterPartySlotSelection(slot, mode);
    } else {
        slot = -1;
    }
    if (s_selectedPartySlot == slot) {
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return 0;
    }
    s_selectedPartySlot = slot;
    ClearTextPlaneHighlight(g_infoPlane);
    if (slot != -1) {
        SetTextPlaneHighlight(g_infoPlane, column, y);
    }
    ResetTextPlaneLineStep(g_infoPlane, oldStep);
    return 0;
}

RVA(0x0003e590, 0xc9)
i16 PollPartySlotSelection(i16 mode) {
    i16 slot;
    if (g_mouseLeftClick) {
        CommitPartySlotSelection();
        ClearMouseClicks();
        return 1;
    }
    if (g_mouseRightClick) {
        ClearMouseSelection();
        ClearMouseClicks();
        return -1;
    }
    slot = PartyPanelAtPoint(g_mousePosition.x, g_mousePosition.y);
    slot = FilterPartySlotSelection(slot, mode);
    s_selectedPartySlot = slot;
    return 0;
}

RVA(0x0003e660, 0x19)
void ClearPartySlotSelection(void) {
    s_selectedPartySlot = -1;
    ClearTextPlaneHighlight(g_infoPlane);
}

RVA(0x0003e680, 0x12)
i16 LockStatusRedraw(i16 lock) {
    i16 prev = s_statusRedrawLocked;
    s_statusRedrawLocked = lock;
    return prev;
}

RVA(0x0003e6a0, 0xa)
void RequestStatusRedraw(void) {
    s_statusRedrawPending = true;
}
