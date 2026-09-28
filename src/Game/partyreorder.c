// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Party.h>
#include <Game/PartyPick.h>
#include <Game/PartyReorder.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/WorldMap.h>
#include <Input/Mouse.h>
#include <Text/Font.h>
#include <Ui/PartySlotSelection.h>

DATA(0x0007d634)
static i16 s_reorderFirst;

DATA(0x0007d638)
static i16 s_reorderSecond;

RVA(0x0001ab90, 0x168)
b16 RunPartyReorder(void) {
    i16 slot;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            NextGamePhase();
            break;
        case 1:
            ReturnFromGameState();
            FlushStatusRedraw(1);
            break;
        case 2:
            s_reorderFirst = PickReorderSlot();
            if (s_reorderFirst == -1) {
                break;
            }
            if (s_reorderFirst < 0) {
                ClearPartySlotSelection();
                PrevGamePhase();
            } else {
                NextGamePhase();
            }
            break;
        case 3:
            s_reorderSecond = PickReorderSlot();
            if (s_reorderSecond == -1) {
                break;
            }
            ClearPartySlotSelection();
            if (s_reorderSecond < 0) {
                PrevGamePhase();
                FlushStatusRedraw(1);
                break;
            }
            ExchangePartySlot(
                s_reorderFirst,
                ExchangePartySlot(s_reorderSecond, GetPartySlot(s_reorderFirst))
            );
            MarkPickDone();
            SetGamePhase(1);
            for (s_reorderFirst = 0; s_reorderFirst < 3; s_reorderFirst++) {
                if (GetPartySlot(s_reorderFirst) >= 0) {
                    return false;
                }
            }
            for (slot = 3; slot < 6; slot++) {
                s_reorderFirst = GetPartySlot(slot);
                if (s_reorderFirst >= 0) {
                    s_reorderFirst = ExchangePartySlot(slot - 3, s_reorderFirst);
                    ExchangePartySlot(slot, s_reorderFirst);
                }
            }
            break;
    }
    return false;
}

RVA(0x0001ad00, 0x39)
i16 PickReorderSlot(void) {
    if (!PollPartySlotSelection(1)) {
        return -1;
    }
    if (g_selectedObjectId < 0) {
        return -2;
    }
    ResetTextPlaneHighlight(g_infoPlane);
    return g_selectedObjectId;
}
