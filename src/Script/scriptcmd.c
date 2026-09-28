// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/BagItems.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/Familiarity.h>
#include <Game/FieldScreen.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/ItemPool.h>
#include <Game/Party.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Text/TextPlaneAttr.h>
#include <Text/WindowText.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <string.h>

DATA(0x00069130)
static i16 s_hoveredChoice = -1;

DATA(0x00081330)
static i16 s_choiceWindow;
DATA(0x00081334)
static i16 s_keepChoices;
DATA(0x00081338)
static i16 s_choiceCancelMode;
DATA(0x0008133c)
static ScriptChoice* s_highlightedChoice;
DATA(0x00081340)
static ScriptChoice* s_choiceMenu;
DATA(0x00081344)
static ScriptChoice* s_hitChoice;

RVA(0x00037950, 0x9f)
void OpFindMemberByPoolState(i16 all, i16 pools) {
    i16 index = ReadLongVarIndex();
    i16 state = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i16 slot = 0;
    i32 result;
    if (!all) {
        result = FindMemberByPoolState(slot, mode, state, pools);
    } else {
        result = 0;
        while (slot >= 0 && slot < 32) {
            slot = FindMemberByPoolState(slot, mode, state, pools);
            if (slot != -1) {
                result |= PowerOfTwo(slot);
                slot++;
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x000379f0, 0xde)
void OpFindMemberWithCondition(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 condition = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = RosterMemberAt(slot);
                if (character && HasCondition(GetCharacterConditions(character), condition)) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = RosterMemberAt(slot);
                if (character && HasCondition(GetCharacterConditions(character), condition)) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037ad0, 0xeb)
void OpFindMemberByAlignmentA(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 alignment = 1 - ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassB(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassB(character) == alignment) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037bc0, 0xeb)
void OpFindMemberByAlignmentB(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 alignment = 1 - ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassA(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassA(character) == alignment) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037cb0, 0xa3)
void OpCountItemOwned(void) {
    i32 count = 0;
    i16 index = ReadLongVarIndex();
    i16 item = ReadScriptValue();
    i16 mode = ReadScriptValue();
    GZ_ENUM_STORAGE(ItemCountScope, i16) scope = ReadScriptValue();
    i16 slot;
    Character* character;
    mode++;
    if (scope == ITEM_COUNT_EQUIPMENT || scope == ITEM_COUNT_BAG_AND_EQUIPMENT) {
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character) {
                    count += CountItemInSlots(item, GetCharacterEquipment(character));
                }
            }
        }
    }
    if (scope == ITEM_COUNT_BAG || scope == ITEM_COUNT_BAG_AND_EQUIPMENT) {
        count += CountHeldItem(item);
    }
    SetScriptLongVar(index, count);
}

RVA(0x00037d60, 0xa1)
i16 CountItemInSlots(i16 item, ItemSlot* slots) {
    i16 count = 0;
    if (slots[0].item == item) {
        count++;
    }
    if (slots[1].item == item) {
        count++;
    }
    if (slots[2].item == item) {
        count++;
    }
    if (slots[3].item == item) {
        count++;
    }
    if (slots[4].item == item) {
        count++;
    }
    if (slots[5].item == item) {
        count++;
    }
    if (slots[6].item == item) {
        count++;
    }
    if (slots[7].item == item) {
        count++;
    }
    return count;
}

// Jumps unless roster member `slot` has condition `condition` (or, with
// `expect` set, lacks it); no member counts as lacking it only with `expect`.
RVA(0x00037e10, 0x6e)
void OpIfMemberHasCondition(void) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 expect = ReadScriptValue();
    Character* character = GetRosterCharacter(ReadScriptValue());
    // The condition, then whether the member has it (no member: tested as
    // read).
    i16 has = ReadScriptValue();
    if (!character && expect) {
        jump = 1;
    } else {
        if (character) {
            has = HasCondition(GetCharacterConditions(character), has);
        }
        if (ScriptBooleanMatches(has, expect)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00037e80, 0x18)
void OpSaveObjectConditions(void) {
    Character* object = ReadScriptObject();
    if (object) {
        SetFlagTag(GetCharacterConditions(object));
    }
}

RVA(0x00037ea0, 0x28)
void OpApplyObjectCondition(void) {
    Character* object = ReadScriptObject();
    i16 condition = ReadScriptValue();
    if (object) {
        AddCondition(GetCharacterConditions(object), condition);
        RequestFieldRefresh();
    }
}

RVA(0x00037ed0, 0x28)
void OpClearObjectCondition(void) {
    Character* object = ReadScriptObject();
    i16 condition = ReadScriptValue();
    if (object) {
        ClearCondition(GetCharacterConditions(object), condition);
        RequestFieldRefresh();
    }
}

RVA(0x00037f00, 0x20)
i32 GetObjectStatTotal(i16 ref, i16 stat) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return GetStatTotal(object, stat);
}

RVA(0x00037f20, 0x1a)
i32 GetObjectLevel(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return object->level;
}

RVA(0x00037f40, 0x17)
i32 GetObjectAlignmentLevelB(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return object->alignmentLevelB;
}

RVA(0x00037f60, 0x17)
i32 GetObjectAlignmentLevelA(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return object->alignmentLevelA;
}

// @identity-TODO: maps a script mode operand 0/1/2 to the step +1/0/-1 the
// caller passes on; the mode's meaning is unrecovered.
RVA(0x00037f80, 0x22)
i16 StepForMode(i16 mode) {
    switch (mode) {
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return -1;
    }
    return 0;
}

// Shifts the player's alignment B by an amount, towards the side the mode
// operand (0, 1 or 2; 1 counts as 2) steps to.
RVA(0x00037fb0, 0x40)
void OpShiftPlayerAlignmentB(void) {
    Character* player = GetCharacter(0);
    i16 mode = ReadScriptValue();
    i16 amount;
    if (mode == 1) {
        mode = 2;
    }
    amount = ReadScriptValue();
    ShiftAlignmentB(player, amount, StepForMode(mode));
}

RVA(0x00037ff0, 0x40)
void OpShiftPlayerAlignmentA(void) {
    Character* player = GetCharacter(0);
    i16 mode = ReadScriptValue();
    i16 amount;
    if (mode == 1) {
        mode = 2;
    }
    amount = ReadScriptValue();
    ShiftAlignmentA(player, amount, StepForMode(mode));
}

RVA(0x00038030, 0x25)
b16 OpLevelUpMember(void) {
    i16 slot = ReadScriptValue();
    GainLevels(GetRosterCharacter(slot), ReadScriptValue());
    return false;
}

static __inline i16 ReadScriptDelta(i16 negate) {
    i16 delta = ReadScriptValue();
    if (negate) {
        delta = -delta;
    }
    return delta;
}

// The script actor's familiarity count (negated with `negate`).
RVA(0x00038060, 0x27)
void OpAddFamiliarityCount(i16 negate) {
    Character* actor = g_curScript->actor;
    i16 delta = ReadScriptDelta(negate);
    AddFamiliarityCount(actor->id, delta);
}

RVA(0x00038090, 0x22)
void OpAddActorFamiliarity(i16 negate) {
    i16 delta = ReadScriptDelta(negate);
    AddFamiliarity(g_curScript->actor, delta);
}

RVA(0x000380c0, 0x22)
void OpAddActorLevelGap(i16 negate) {
    i16 delta = ReadScriptDelta(negate);
    AddLevelGap(g_curScript->actor, delta);
}

RVA(0x000380f0, 0x18)
void OpSetActorFamiliarity(void) {
    SetFamiliarity(g_curScript->actor, ReadScriptValue());
}

RVA(0x00038110, 0x18)
void OpSetActorLevelGap(void) {
    SetLevelGap(g_curScript->actor, ReadScriptValue());
}

RVA(0x00038130, 0x15)
void OpSetActorAttitude(void) {
    g_curScript->actor->attitude = ReadScriptValue();
}

RVA(0x00038150, 0x15)
void OpSetActorFieldState(void) {
    g_curScript->actor->fieldState = ReadScriptValue();
}

// Sets an object's familiarity (0..255; negated with `negate`) and its
// personal flag 0.
RVA(0x00038170, 0x44)
void OpSetObjectFamiliarity(i16 negate) {
    Character* object = ReadScriptObject();
    i32 value = ReadScriptValue();
    u8 familiarity;
    if (negate) {
        value = -value;
    }
    familiarity = ClampInt(value, 0, 0xff);
    if (object) {
        object->familiarity = familiarity;
        SetCharacterFlag(object, 0);
    }
}

// Jumps unless the pooled items fit in the bag (with `invert`, unless they
// do not).
RVA(0x000381c0, 0x2a)
void OpBranchOnItemsFit(i16 invert) {
    i16 target = ReadBranchTarget();
    i32 fit = PooledItemsFit();
    if (invert) {
        fit = !fit;
    }
    ScriptJumpUnless(target, fit);
}

RVA(0x000381f0, 0x19)
void OpRemovePendingItem(void) {
    i16 item = ReadScriptValue();
    TakeFromPool(item, ReadScriptValue());
}

RVA(0x00038210, 0x19)
void OpAddPendingItem(void) {
    i16 item = ReadScriptValue();
    AddToPool(item, ReadScriptValue());
}

// Stores in a long variable the mask of roster slots (passing
// FilterPartyMember with `mode` + 1) whose demon race is `race`.
RVA(0x00038230, 0x7e)
void OpMaskRosterByKind(void) {
    i16 index = ReadLongVarIndex();
    i16 race = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    u32 mask = 0;
    i16 i;
    for (i = 0; i < 32; i++) {
        if (FilterPartyMember(i, mode) != -1 && RosterMemberAt(i)
            && race == GetDemonRace(RosterMemberAt(i)->id)) {
            mask |= PowerOfTwo(i);
        }
    }
    SetScriptLongVar(index, mask);
}

// Fills pool `pool` (1: HP, else MP) by `amount` for each roster slot in the
// mask.
// @early-stop: retail loads the mask into a register before testing it
// against the bit; no spelling of the test reproduces that (the permuter's
// search is flat).
RVA(0x000382b0, 0x67)
void OpRecoverRosterPool(i16 pool) {
    u32 bit = 1;
    u32 mask = ReadScriptValue();
    i16 amount = ReadScriptValue();
    i16 i;
    for (i = 0; i < 32; i++) {
        if (RosterMemberAt(i) && (mask & bit)) {
            if (pool == 1) {
                FillPool(&RosterMemberAt(i)->pools.hp, amount, POOL_FILL_TO_MAX);
            } else {
                FillPool(&RosterMemberAt(i)->pools.mp, amount, POOL_FILL_TO_MAX);
            }
        }
        bit <<= 1;
    }
}

// Clears condition `condition` from each roster slot in the mask that has it.
// @early-stop: the same mask load as OpRecoverRosterPool.
RVA(0x00038320, 0x5f)
void OpCureRosterCondition(void) {
    u32 bit = 1;
    u32 mask = ReadScriptValue();
    i16 condition = ReadScriptValue();
    i16 i;
    for (i = 0; i < 32; i++) {
        if (mask & bit) {
            Character* character = GetRosterCharacter(i);
            if (character && HasCondition(GetCharacterConditions(character), condition)) {
                ClearCondition(GetCharacterConditions(character), condition);
            }
        }
        bit <<= 1;
    }
}

RVA(0x00038380, 0xf)
ScriptChoice* ListTail(ScriptChoice* node) {
    while (1) {
        if (node->next == NULL) {
            break;
        }
        node = node->next;
    }
    return node;
}

RVA(0x00038390, 0x2e)
ScriptChoice* AppendScriptChoice(ScriptChoice** head) {
    ScriptChoice* choice = AllocCleared(1, sizeof(ScriptChoice));
    if (*head == NULL) {
        *head = choice;
    } else {
        ListTail(*head)->next = choice;
    }
    return choice;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000383c0, 0x7b)
ScriptChoice*
PrintScriptChoice(i16 window, ScriptChoice** head, const char* text, i16 value, i16 disabled) {
    ScriptChoice* choice = AppendScriptChoice(head);
    choice->x = GetTextPlaneCursorX(window);
    choice->y = GetTextPlaneCursorY(window);
    choice->width = strlen(text);
    choice->value = value;
    choice->disabled = disabled;
    PrintWindowText(window, text, 0x400, 0, 1);
    return choice;
}

RVA(0x00038440, 0x1e)
ScriptChoice* FreeScriptChoices(ScriptChoice* head) {
    while (head) {
        ScriptChoice* choice = head;
        head = head->next;
        FreeBlock(choice);
    }
    return head;
}

static __inline void
SetScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    s_choiceMenu = choices;
    s_choiceWindow = window;
    s_keepChoices = keep;
    s_choiceCancelMode = cancelMode;
}

RVA(0x00038460, 0x39)
ScriptChoice* PushScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    SetScriptChoiceMenu(choices, window, keep, cancelMode);
    PushGameState(6);
    return NULL;
}

RVA(0x000384a0, 0x4c)
void InitScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    SetScriptChoiceMenu(choices, window, keep, cancelMode);
    if (g_mouseLeftClick) {
        g_mouseLeftClick = 0;
    }
    s_hoveredChoice = -1;
    s_highlightedChoice = NULL;
}

static __inline void ToggleScriptChoiceHighlight(void) {
    if (s_highlightedChoice) {
        ReverseTextRun(
            s_choiceWindow,
            s_highlightedChoice->x,
            s_highlightedChoice->y,
            s_highlightedChoice->width
        );
        RedrawTextRun(
            s_choiceWindow,
            s_highlightedChoice->x,
            s_highlightedChoice->y,
            s_highlightedChoice->width
        );
    }
}

RVA(0x000384f0, 0x1d2)
i16 PollScriptChoiceMenu(void) {
    i16 index = 0;
    i16 hovered;
    if (TakeMouseCancel(s_choiceCancelMode)) {
        if (!s_keepChoices) {
            s_choiceMenu = FreeScriptChoices(s_choiceMenu);
        }
        PlaySoundEffect(2);
        return -1;
    }
    hovered = FindScriptChoiceAtMouse();
    if (hovered != s_hoveredChoice) {
        ToggleScriptChoiceHighlight();
        s_hoveredChoice = hovered;
        s_highlightedChoice = s_hitChoice;
        ToggleScriptChoiceHighlight();
    }
    if (!TakeMouseLeftClick()) {
        return 0;
    }
    if (s_hoveredChoice == -1) {
        return 0;
    }
    g_hoveredObjectId = s_hoveredChoice;
    g_selectedObjectId = s_highlightedChoice->value;
    if (!s_keepChoices) {
        s_hitChoice = s_choiceMenu;
        while (s_hitChoice) {
            if (index != s_hoveredChoice) {
                BlankTextRun(s_choiceWindow, s_hitChoice->x, s_hitChoice->y, s_hitChoice->width);
                RedrawTextRun(s_choiceWindow, s_hitChoice->x, s_hitChoice->y, s_hitChoice->width);
            }
            s_hitChoice = s_hitChoice->next;
            index++;
        }
        s_choiceMenu = FreeScriptChoices(s_choiceMenu);
    }
    PlaySoundEffect(1);
    return 1;
}

RVA(0x000386d0, 0xbf)
i16 FindScriptChoiceAtMouse(void) {
    i16 originX, originY;
    i16 index;
    ScriptChoice* choice;
    GetTextPlaneOrigin(s_choiceWindow, &originX, &originY);
    choice = s_choiceMenu;
    index = 0;
    while (choice) {
        if (!choice->disabled) {
            i16 x = choice->x * 8;
            i16 y = choice->y * 16;
            x = g_mousePosition.x - x - originX;
            y = g_mousePosition.y - y - originY;
            if (x >= 0 && x < choice->width * 8 && y >= 0 && y < 16) {
                s_hitChoice = choice;
                return index;
            }
        }
        choice = choice->next;
        index++;
    }
    s_hitChoice = NULL;
    return -1;
}

RVA(0x00038790, 0x53)
b16 RunScriptChoiceState(void) {
    switch (GetGameSub()) {
        case 0:
            NextGameSub();
            InitScriptChoiceMenu(s_choiceMenu, s_choiceWindow, s_keepChoices, s_choiceCancelMode);
        case 1:
            if (PollScriptChoiceMenu()) {
                ReturnFromGameState();
            }
            break;
    }
    return false;
}
