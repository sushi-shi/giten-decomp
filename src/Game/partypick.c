// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/CombatantId.h>

#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/Guest.h>
#include <Game/ItemRecord.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/Skill.h>
#include <Game/TargetFlags.h>
#include <Game/StatusScreen.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Script/TextToken.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Ui/FieldMenus.h>
#include <Ui/Hotspot.h>
#include <Ui/Menu.h>
#include <Ui/Message.h>
#include <Ui/PartySlotSelection.h>

#include <stddef.h>

// @identity-TODO: the party position the picker last targeted (-1: none).
DATA(0x00068408)
static i16 s_pickedIndex = -1;

// The temporary swap: the party position and the roster slot it held.
DATA(0x00068400)
i16 g_guestIndex = -1;

DATA(0x00068404)
static i16 s_swapSaved = -1;

DATA(0x000784cc)
static i16 s_pickScreenSaved;

DATA(0x000784d0)
static i16 s_pickMode;

DATA(0x000784d4)
static i16 s_pickDone;

// The list menu a picked member acts through (NULL: none open).
DATA(0x00078520)
static MenuBox* s_pickMenu;

static __inline void ResetPartyCommandPick(void) {
    s_pickMode = 0;
    s_pickedIndex = -1;
}

RVA(0x000093e0, 0x36)
void CloseFieldWindows(void) {
    RunPartyPicker(-1);
    CancelFieldTargetMenu(-1);
    CancelItemTargetMenu(-1);
    ResetPartyCommandPick();
    s_pickDone = 0;
}

// @identity-TODO: whether party position `index` completes the pick; returns
// the done flag.
RVA(0x00009420, 0x90)
i16 CheckPickTarget(i16 index) {
    Character* character = GetPartyCharacter(index);
    i16 selection;
    if (character == NULL) {
        return 0;
    }
    switch (s_pickMode) {
        case 0:
        case 1:
            selection = GetPickerSelection();
            if (selection < 0) {
                return 0;
            }
            if (FindMenuLineByValue(selection, character->id)) {
                if (GetPickBlockingCondition(GetCharacterConditions(character))) {
                    s_pickDone = 1;
                }
            } else if (PickPartyMember(index) == 1) {
                s_pickDone = 1;
            }
    }
    return s_pickDone;
}

RVA(0x000094b0, 0xa)
void MarkPickDone(void) {
    s_pickDone = 1;
}

RVA(0x000094c0, 0x7)
i16 GetPickMode(void) {
    return s_pickMode;
}

// Runs the menu for picked member `id`'s role and stores the record it picks:
// 1 when the role needs no menu, -1 while open, -2 when cancelled or blocked.
RVA(0x000094d0, 0x118)
i16 RunMemberPickMenu(i16 id) {
    Character* character = GetCharacterById(id);
    i16 result;
    if (character == NULL || GetPickBlockingCondition(GetCharacterConditions(character))) {
        s_pickMenu = CloseListMenu(s_pickMenu);
        return -2;
    }
    if (s_pickMenu == NULL) {
        switch (character->pickRole) {
            case 3:
                return 1;
            case 4:
            case 6:
                s_pickMenu = OpenMemberSkillMenu(id);
                break;
            case 5:
                s_pickMenu = OpenItemListMenu();
                break;
            default:
                return 1;
        }
    }
    result = RunListMenu(s_pickMenu);
    if (result == -1) {
        return result;
    }
    if (result == -2) {
        s_pickMenu = CloseListMenu(s_pickMenu);
        return -2;
    }
    s_pickMenu = CloseListMenu(s_pickMenu);
    character->pickTarget = g_selectedObjectId;
    return g_selectedObjectId;
}

RVA(0x000095f0, 0x22)
void SetMemberPickRole(i16 id, i8 role) {
    Character* character;
    s_pickedIndex = id;
    character = GetCharacterById(id);
    if (character != NULL) {
        character->pickRole = role;
    }
}

static __inline i16 PickCurrentMemberAsTarget(Character* character) {
    character->pickObject = PartyCombatantId(FindPartyPositionOfId(s_pickedIndex));
    s_pickMode++;
    return g_tickElapsed;
}

static __inline i16 PickMemberActionTarget(Character* character, i16 flags, i16 range) {
    if (character->pickRole == 4) {
        if (character->pickTarget == 0x10e) {
            return RunPickTargetWindow(0, range, 1, 0);
        }
        if (character->pickTarget == 0x57) {
            return RunPickTargetWindow(0, range, 0x82, 0);
        }
    }
    if (flags == 1) {
        return RunPickTargetWindow(0, range, 0x12, 0);
    } else {
        return RunPickTargetWindow(0, range, 3, 0);
    }
}

// @identity-TODO: the party command-input machine, one step per call (steps
// in s_pickMode): wait for a picked member, settle its role, run its action
// menu, pick the target, then confirm. Returns the tick flag.
RVA(0x00009620, 0x600)
i16 RunPartyCommandInput(void) {
    Character* character;
    i16 result;
    i16 kind;
    i16 range;
    i16 reach = 0;
    i16 flags = -1;
    switch (s_pickMode) {
        case 0:
            if (IsPanelLayerVisible()) {
                g_tickElapsed = 0;
            }
            s_pickDone = 0;
            if (s_pickedIndex < 0) {
                break;
            }
            SetFieldBusy(1);
            s_pickMode++;
            g_tickElapsed = 0;
            HideScreenLayer(SCREEN_LAYER_PANEL);
            return g_tickElapsed;
        case 1:
            GetCharacterById(s_pickedIndex)->conditionActionTicks = 0;
            SetFieldBusy(1);
            s_pickMode++;
            g_tickElapsed = 0;
            s_pickMode += PrepareMemberPickTarget(s_pickedIndex);
            if (s_pickMode > 4) {
                s_pickMode = 4;
            }
            break;
        case 2:
            GetCharacterById(s_pickedIndex)->conditionActionTicks = 0;
            SetFieldBusy(1);
            g_tickElapsed = 0;
            result = RunMemberPickMenu(s_pickedIndex);
            if (result == -2) {
                ResetPartyCommandPick();
            }
            if (result < 0) {
                break;
            }
            s_pickMode++;
            return g_tickElapsed;
        case 3:
            character = GetCharacterById(s_pickedIndex);
            character->conditionActionTicks = 0;
            if (GetPickBlockingCondition(GetCharacterConditions(character))) {
                ResetPartyCommandPick();
                return g_tickElapsed;
            }
            if (character->pickRole == 5) {
                kind = GetLoadedRecord(character->pickTarget)->kind;
                if (kind == 0xb || kind == 0x13) {
                    character->pickFlags |= 4;
                    character->pickItem = character->pickTarget;
                    character->pickRole = 4;
                    character->pickTarget = GetItemSkillId(GetLoadedRecord(character->pickTarget));
                }
            }
            if (character->pickRole == 4) {
                flags = GetSkillTargetFlags(character->pickTarget);
                if (TargetFlagsSelectSelf(flags)) {
                    return PickCurrentMemberAsTarget(character);
                } else if (TargetFlagsSelectActorGroup(flags)) {
                    return PickCurrentMemberAsTarget(character);
                } else if (flags & TARGET_ACTOR_SIDE) {
                    reach = 1;
                }
            } else if (character->pickRole == 5) {
                flags = GetItemTargetFlags(GetLoadedRecord(character->pickTarget));
                if ((TargetFlagsSelectSelf(flags)) || TargetFlagsSelectActorGroup(flags)) {
                    return PickCurrentMemberAsTarget(character);
                }
                if (character->pickTarget == 0x71) {
                    flags = TARGET_ACTOR_SIDE;
                }
                if (flags & TARGET_ACTOR_SIDE) {
                    reach = 1;
                }
            }
            if (flags == 0x10 || flags == 0x11 || flags == 0x30) {
                reach = 1;
            }
            if (reach == 0 && HasObjectInReach(0, -1, 0)) {
                character->pickObject = FindObjectAtParty();
                s_pickMode++;
                return g_tickElapsed;
            }
            range = GetMemberPickRange(s_pickedIndex);
            if (flags == 0x10) {
                result = RunPickTargetWindow(0, range, 5, 0);
            } else if (flags == 0x11) {
                result = RunPickTargetWindow(0, range, 4, 0);
            } else if (flags == 0x30) {
                result = RunPickTargetWindow(0, range, 6, 0);
            } else {
                result = PickMemberActionTarget(character, flags, range);
                flags = 0;
            }
            if (flags != 0) {
                g_tickElapsed = 0;
            }
            if (result == -1) {
                s_pickMode--;
                s_pickMode -= PrepareMemberPickTarget(s_pickedIndex);
                if (s_pickMode < 1) {
                    s_pickMode = 1;
                }
                if (character->pickFlags & 4) {
                    character->pickRole = 5;
                }
            }
            if (result < 1) {
                if (character->pickRole != 1) {
                    break;
                }
                ResetPartyCommandPick();
                // "攻撃がとどかない！"
                ShowMessage(
                    "\215U\214\202\202\252\202\306\202\307\202\251\202\310\202\242\201I",
                    -1
                );
                return g_tickElapsed;
            }
            character = GetCharacterById(s_pickedIndex);
            if (flags != 0) {
                g_selectedObjectId =
                    SwapInForPick(FindPartyPositionOfId(s_pickedIndex), g_selectedObjectId);
            }
            character->pickObject = g_selectedObjectId;
            s_pickMode++;
            g_tickElapsed = 0;
            return g_tickElapsed;
        case 4:
            character = GetCharacterById(s_pickedIndex);
            if (character != NULL) {
                if (!GetPickBlockingCondition(GetCharacterConditions(character))) {
                    QueueActionWait(GetCharacterActionWait(character));
                }
                if (character->pickRole == 4 && character->pickTarget == 0x7d) {
                    s_pickMode++;
                    g_tickElapsed = 0;
                    return g_tickElapsed;
                }
            }
            ResetPartyCommandPick();
            return g_tickElapsed;
        case 5:
            character = GetCharacterById(s_pickedIndex);
            g_tickElapsed = 0;
            result = RunPickTargetWindow(0, 0, 2, 0);
            if (result == -1) {
                RestoreSwappedMember();
                s_pickMode = 3;
                if (character->pickFlags & 4) {
                    character->pickRole = 5;
                }
            }
            if (result < 1) {
                break;
            }
            g_pickHoveredObject = g_hoveredObjectId;
            if (!GetPickBlockingCondition(GetCharacterConditions(character))) {
                QueueActionWait(GetCharacterActionWait(character));
            }
            ResetPartyCommandPick();
            break;
    }
    return g_tickElapsed;
}

// The targeting range for member `id`'s pending action.
RVA(0x00009c20, 0xcb)
i16 GetMemberPickRange(i16 id) {
    Character* character = GetCharacterById(id);
    i8 role = character->pickRole;
    i16 item;
    if (role == 0) {
        return 0;
    }
    if (role == 8) {
        return 0;
    }
    if (role == 7) {
        return 0;
    }
    if (role == 1) {
        item = GetCharacterEquipment(character)[5].item;
        if (item == 0 || item == -1) {
            return 1;
        }
        return GetItemAttackRange(GetLoadedRecord(item));
    }
    if (role == 2) {
        item = GetCharacterEquipment(character)[6].item;
        if (item == 0 || item == -1) {
            return 1;
        }
        return 3;
    }
    if (role == 5) {
        if (character->pickTarget < 1) {
            return 1;
        }
        return GetItemAttackRange(GetLoadedRecord(character->pickTarget));
    }
    if (character->pickTarget < 1) {
        return 3;
    }
    return GetSkillAttackRange(character->pickTarget);
}

// @identity-TODO: a second getter of the pick mode, called from another
// module; whether it once differed is unknown.
RVA(0x00009cf0, 0x7)
i16 QueryPickMode(void) {
    return s_pickMode;
}

RVA(0x00009d00, 0x7)
i16 GetTickElapsed(void) {
    return g_tickElapsed;
}

static __inline i16 FindPickReplacementSlot(i16 keep) {
    i16 index;
    index = FindEmptySlot(1);
    if (index >= 0) {
        return index;
    }
    for (index = 0; index < 6; index++) {
        if (IsPartyMemberFallen(index)) {
            return index;
        }
    }
    for (index = 0; index < 6; index++) {
        if (index != keep && GetPartyRosterId(index) >= 32) {
            return index;
        }
    }
    for (index = 0; index < 6; index++) {
        if (index != keep) {
            return index;
        }
    }
    return index;
}

// Swaps roster slot `slot` into the party for a pick (unless it is already in
// it) and returns -1 - the position it occupies. The position is the first
// empty one, else the first fallen member, else the first non-human member
// other than `keep`, else the first position other than `keep`.
RVA(0x00009d10, 0xad)
i16 SwapInForPick(i16 keep, i16 slot) {
    i16 index;
    if (slot < 0) {
        return slot;
    }
    index = FindPartySlot(slot);
    if (index >= 0) {
        return PartyCombatantId(index);
    }
    index = FindPickReplacementSlot(keep);
    g_guestIndex = index;
    s_swapSaved = GetPartySlot(index);
    ExchangePartySlot(index, slot);
    return PartyCombatantId(index);
}

RVA(0x00009dc0, 0x2c)
void RestoreSwappedMember(void) {
    if (g_guestIndex >= 0) {
        ExchangePartySlot(g_guestIndex, s_swapSaved);
        s_swapSaved = -1;
        g_guestIndex = -1;
    }
}

// The slot held before the swap at `index`, or -2 when `index` is not the
// swapped position.
RVA(0x00009df0, 0x19)
i16 GetSwappedMember(i16 index) {
    if (index != g_guestIndex) {
        return -2;
    }
    return s_swapSaved;
}

// `index` when it is the guest index, -2 otherwise.
RVA(0x00009e10, 0x12)
i16 IsGuestIndex(i16 index) {
    if (index != g_guestIndex) {
        return -2;
    }
    return g_guestIndex;
}

RVA(0x00009e30, 0x214)
i16 RunPickTargetWindow(i16 minimumRange, i16 maximumRange, i16 kind, i16 id) {
    Character* character = GetCharacterById(id);
    i16 result;
    if (kind & 4) {
        if (id && (!character || GetPickBlockingCondition(GetCharacterConditions(character)))) {
            RunStatusListPicker(1);
            ClearMouseSelection();
            if (s_pickScreenSaved) {
                RestoreScreenSaveWithState(g_pickScreenSave);
                FreeScreenSave(g_pickScreenSave);
                s_pickScreenSaved = 0;
            }
            return -1;
        }
        if (!s_pickScreenSaved) {
            AllocScreenSave(g_pickScreenSave);
            CaptureScreenSaveWithState(g_pickScreenSave);
            s_pickScreenSaved = 1;
        }
        if (kind & 1) {
            SetStatusColumn(4);
        } else if (kind & 2) {
            SetStatusColumn(5);
        } else {
            SetStatusColumn(0);
        }
        result = RunStatusListPicker(0);
        if (result == -1) {
            return 0;
        }
        RunStatusListPicker(1);
        PlaySoundEffect(1);
        if (s_pickScreenSaved) {
            RestoreScreenSaveWithState(g_pickScreenSave);
            FreeScreenSave(g_pickScreenSave);
            s_pickScreenSaved = 0;
        }
        return result == -2 ? -1 : 1;
    }
    if ((id && (!character || GetPickBlockingCondition(GetCharacterConditions(character))))
        || TakeMouseCancelSound()) {
        ClearPartySlotSelection();
        RunStatusListPicker(1);
        ClearMouseSelection();
        return -1;
    }
    if (kind & 1) {
        result = PickFieldObjectTarget(minimumRange, maximumRange);
        if (result) {
            ClearPartySlotSelection();
            PlaySoundEffect(1);
            return result;
        }
    }
    if (kind & 2) {
        if (kind == 2) {
            result = PickPartySlotTarget(minimumRange, 2);
        } else {
            result = PickPartySlotTarget(minimumRange, 0);
        }
        if (result) {
            ClearPartySlotSelection();
            PlaySoundEffect(1);
            return result;
        }
    }
    return 0;
}

RVA(0x0000a050, 0x53)
b16 PickFieldObjectTarget(i16 minimumRange, i16 maximumRange) {
    i16 distance;
    void* actor;
    g_hoveredObjectId = GetSelectedHotspotValue();
    if (g_hoveredObjectId != -1) {
        // The field object's character prefix starts at its kind member.
        actor = &GetFieldObject(g_hoveredObjectId)->kind;
        distance = DistanceToParty(actor);
        if (distance >= minimumRange && distance <= maximumRange) {
            g_selectedObjectId = g_hoveredObjectId;
            ClearMouseClicks();
            return true;
        }
        ClearMouseClicks();
    }
    return false;
}

RVA(0x0000a0b0, 0x69)
i16 PickPartySlotTarget(i16 minimumRange, i16 mode) {
    i16 result = PollPartySlotSelection(mode);
    if (result == 0) {
        ClearMouseClicks();
        return 0;
    }
    if (result == -1) {
        ClearMouseClicks();
        return -1;
    }
    if (g_hoveredObjectId == -1) {
        ClearMouseClicks();
        return 0;
    }
    if (minimumRange > 0) {
        ClearMouseClicks();
        return 0;
    }
    g_selectedObjectId = PartyCombatantId(g_hoveredObjectId);
    ClearMouseClicks();
    return 1;
}
