// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/AreaMap.h>
#include <Game/BagItems.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/DropTable.h>
#include <Game/Familiarity.h>
#include <Game/Field.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldView.h>
#include <Game/FusionMenu.h>
#include <Game/FusionScreen.h>
#include <Game/GameLoop.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/InfoBar.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemMenu.h>
#include <Game/ItemRecord.h>
#include <Game/ModeFlags.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/SkillList.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/WorldMap.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptVars.h>
#include <Ui/Panel.h>
#include <Ui/PartySlotSelection.h>
#include <Util/Range.h>

#include <math.h>
#include <stddef.h>

RVA(0x00034690, 0x1c)
i16 CompareInt(i32 a, i32 b) {
    if (a < b) {
        return -1;
    }
    return a > b;
}

// Applies a comparison operator to CompareInt(a, b): 0 !=, 1 ==, 2 <=, 3 >=,
// 4 <, 5 >; 1 when it holds, 0 otherwise (also for an unknown operator).
RVA(0x000346b0, 0x8c)
i32 CompareByOp(ComparisonOperator op, i32 a, i32 b) {
    i16 order = CompareInt(a, b);
    i32 holds = 0;
    switch (op) {
        case COMPARE_EQUAL:
            if (order == 0) {
                holds = 1;
            }
            break;
        case COMPARE_NOT_EQUAL:
            if (order != 0) {
                holds = 1;
            }
            break;
        case COMPARE_LESS_EQUAL:
            if (order <= 0) {
                holds = 1;
            }
            break;
        case COMPARE_GREATER_EQUAL:
            if (order >= 0) {
                holds = 1;
            }
            break;
        case COMPARE_LESS:
            if (order < 0) {
                holds = 1;
            }
            break;
        case COMPARE_GREATER:
            if (order > 0) {
                holds = 1;
            }
            break;
    }
    return holds;
}

RVA(0x00034740, 0x3c)
void OpJumpUnlessCompare(ComparisonOperator op, i32 withRhs) {
    i16 target = ReadBranchTarget();
    i32 lhs = ReadScriptValue();
    i32 rhs = 0;
    if (withRhs == 1) {
        rhs = ReadScriptValue();
    }
    ScriptJumpUnless(target, CompareByOp(op, lhs, rhs));
}

RVA(0x00034780, 0xc8)
i32 OpApplyEventFlag(ScriptFlagAction action, i32 expect) {
    u16 bank;
    u16 index;
    i32 result;
    ReadFlagOperand(&bank, &index);
    result = 0;
    switch (action) {
        case SCRIPT_FLAG_TEST:
            result = TestEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_SET:
            result = SetEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_CLEAR:
            result = ClearEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_TOGGLE:
            result = ToggleEventFlag(bank, index);
            break;
    }
    return result == expect;
}

RVA(0x00034850, 0x26)
void OpJumpUnlessEventFlag(ScriptFlagAction action, i32 expect) {
    i16 target = ReadBranchTarget();
    ScriptJumpUnless(target, OpApplyEventFlag(action, expect));
}

RVA(0x00034880, 0x27)
void OpJumpUnlessFlagSet(void) {
    i16 target = ReadBranchTarget();
    i32 matches = 0;
    if (ReadAndMatchEventFlag()) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

#define RollFixedContestValue(value, level)                                                        \
    do {                                                                                           \
        switch (level) {                                                                           \
            case 0:                                                                                \
                break;                                                                             \
            case 1:                                                                                \
                (value) = RandomAverage(5, 15, 0);                                                 \
                break;                                                                             \
            case 2:                                                                                \
                (value) = RandomAverage(12, 22, 0);                                                \
                break;                                                                             \
            case 3:                                                                                \
                (value) = RandomAverage(20, 40, 0);                                                \
                break;                                                                             \
        }                                                                                          \
    } while (0)

#define RollRelativeContestValue(value, level)                                                     \
    do {                                                                                           \
        switch (level) {                                                                           \
            case 0:                                                                                \
                break;                                                                             \
            case 1:                                                                                \
                (value) = RandomPercent((value), -20, 20);                                         \
                break;                                                                             \
            case 2:                                                                                \
                (value) = RandomPercent((value), 0, 30);                                           \
                break;                                                                             \
            case 3:                                                                                \
                (value) = RandomPercent((value), 10, 40);                                          \
                break;                                                                             \
        }                                                                                          \
    } while (0)

// Jumps unless the actor wins a contest of `stat` (the next operand) against
// the target, or with `invert` unless it loses: the target's side is its own
// value, a random spread around it, or a fixed random range chosen by the
// stat and the contest `level` (0..3); `swap` exchanges the sides.
RVA(0x000348b0, 0x520)
void OpJumpUnlessStatContest(i16 level, i16 invert, i16 swap) {
    i16 target = ReadBranchTarget();
    i16 stat = ReadScriptValue();
    i32 own;
    i32 other;
    i16 order;
    i32 won;

    ReadContestValues(stat, &own, &other, swap);
    won = 0;
    switch (stat) {
        case 0:
            RollFixedContestValue(other, level);
            break;
        case 1:
            RollFixedContestValue(other, level);
            break;
        case 2:
            RollRelativeContestValue(other, level);
            break;
        case 3:
            switch (level) {
                case 0:
                    break;
                case 1:
                    other = RandomPercent(other, -20, 20);
                    break;
                case 2:
                    other = RandomPercent(other, 10, 30);
                    break;
                case 3:
                    other = RandomPercent(other, 10, 40);
                    break;
            }
            break;
        case 4:
            RollFixedContestValue(other, level);
            break;
        case 5:
            switch (level) {
                case 0:
                case 1:
                case 2:
                case 3:
                    other = RandomAverage(0, 40, 2);
                    break;
            }
            break;
        case 6:
            RollRelativeContestValue(other, level);
            break;
        case 7:
            RollRelativeContestValue(other, level);
            break;
        case 8:
            RollFixedContestValue(other, level);
            break;
        case 9:
            RollFixedContestValue(other, level);
            break;
        case 10:
            RollFixedContestValue(other, level);
            break;
        case 11:
            switch (level) {
                case 0: {
                    i32 ownAgility;
                    i32 otherAgility;

                    ReadContestValues(4, &ownAgility, &otherAgility, swap);
                    other = -sqrt(otherAgility);
                    break;
                }
                case 1:
                    other = RandomPercent(other, 10, 25);
                    break;
                case 2:
                    other = RandomPercent(other, 25, 50);
                    break;
                case 3:
                    other = RandomPercent(other, -20, 20);
                    break;
            }
            break;
        case 12:
            switch (level) {
                case 0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case 1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case 2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case 3:
                    other = RandomAverage(10, 17, 0);
                    break;
            }
            break;
        case 13:
            switch (level) {
                case 0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case 1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case 2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case 3:
                    other = RandomAverage(11, 18, 0);
                    break;
            }
            break;
        case 14:
            switch (level) {
                case 0:
                    other = RandomAverage(35, 70, 0);
                    break;
                case 1:
                    other = RandomAverage(60, 91, 0);
                    break;
                case 2:
                    other = RandomAverage(85, 116, 0);
                    break;
                case 3:
                    other = RandomAverage(120, 135, 0);
                    break;
            }
            break;
    }
    order = CompareInt(own, other);
    if (!invert && order >= 0) {
        won = 1;
    }
    if (invert && order < 0) {
        won = 1;
    }
    ScriptJumpUnless(target, won);
}

// Jumps unless the party is in the script actor's sight (with `invert`,
// unless it is not).
RVA(0x00034dd0, 0x81)
void OpJumpUnlessPlayerInView(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 seen;
    BuildSightGrid(
        ((FieldActor*)g_curScript->actor)->pos.x,
        ((FieldActor*)g_curScript->actor)->pos.y,
        ((FieldActor*)g_curScript->actor)->direction
    );
    seen = IsPartyInSight(
        ((FieldActor*)g_curScript->actor)->pos.x,
        ((FieldActor*)g_curScript->actor)->pos.y
    );
    if ((seen == 1 && invert == 0) || (seen == 0 && invert == 1)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Compares (operator `op`) a roll of 0..100 with the actor's HP percentage
// plus its byte +0x1f0 and a roll of 0..15.
// @identity-TODO: what the byte at +0x1f0 adds is unrecovered.
RVA(0x00034e60, 0x7f)
void OpJumpUnlessHpPercentRoll(ComparisonOperator op) {
    i16 target = ReadBranchTarget();
    Character* actor = g_curScript->actor;
    i32 roll = RandomAverage(0, 100, 0);
    i32 value = actor->pools.hp.cur * 100 / actor->pools.hp.max + actor->hpRollBonus;
    value += RandomAverage(0, 15, 0);
    ScriptJumpUnless(target, CompareByOp(op, roll, value));
}

// Compares (operator `op`) the actor's HP with a quarter of its maximum plus
// a roll up to that quarter.
RVA(0x00034ee0, 0x56)
void OpJumpUnlessHpQuarterRoll(ComparisonOperator op) {
    i16 target = ReadBranchTarget();
    Character* actor = g_curScript->actor;
    i32 quarter = actor->pools.hp.max;
    i32 hp = actor->pools.hp.cur;
    quarter >>= 2;
    quarter += RandomAverage(0, quarter, 0);
    ScriptJumpUnless(target, CompareByOp(op, hp, quarter));
}

// Jumps unless the party stands within 4 cells in front of the actor.
RVA(0x00034f40, 0xab)
void OpJumpUnlessPlayerNearFront(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    MapCoord coord = GetMapCoord();
    if (GridDistance(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y
        )
        > 4) {
        if (invert != 0) {
            jump = 1;
        }
    } else {
        i16 side = RelativeDirection(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y,
            ((FieldActor*)g_curScript->actor)->direction
        );
        if ((side == 0 && invert == 0) || (side != 0 && invert == 1)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party is at the actor's trigger range (always, unless
// inverted, while the field marker is set).
RVA(0x00034ff0, 0x7a)
void OpJumpUnlessPlayerAtRange(i16 invert) {
    i32 jump = 0;
    i16 range = g_curScript->actor->triggerRange;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == 0;
    } else {
        i16 distance = DistanceToParty((FieldActor*)g_curScript->actor);
        if ((range != distance && invert) || (range == distance && !invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035070, 0x7b)
void OpJumpUnlessActorVisible(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == 0;
    } else {
        i16 view = GetPartyView(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y
        );
        if ((invert == 0 && view) || (invert == 1 && !view)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000350f0, 0x44)
void OpJumpUnlessInRoster(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 slot = RosterSlotOfId(ReadObjectId());
    if ((slot >= 0 && !invert) || (slot < 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the roster holds the roster capacity less 6 entries or more.
RVA(0x00035140, 0x4c)
void OpJumpUnlessRosterFull(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountRosterEntries(1);
    i16 limit = GetRosterCapacity() - 6;
    if ((count >= limit && !invert) || (count < limit && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object's alignment agrees with the leader's.
RVA(0x00035190, 0x44)
void OpJumpUnlessAlignmentMatch(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 conflict = AlignmentConflicts(ReadScriptObject());
    if (ScriptBooleanMatches(!conflict, invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party's macca covers the object's rank score.
RVA(0x000351e0, 0x5b)
void OpJumpUnlessCanAfford(i16 invert) {
    i32 price = 0x7fffffff;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (object) {
        price = GetRankScore(object);
    }
    price -= GetObjectMacca(-1);
    if ((price <= 0 && !invert) || (price > 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035240, 0x44)
void OpJumpUnlessInParty(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 position = FindPartyPositionOfId(ReadObjectId());
    if ((position >= 0 && !invert) || (position < 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035290, 0x3f)
void OpJumpUnlessRosterHasNoDemons(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(0);
    if (ScriptBooleanMatches(!demons, invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has no condition (no object counts as healthy
// only when inverted).
RVA(0x000352d0, 0x62)
void OpJumpUnlessHealthy(i16 invert) {
    i16 conditions = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (!object && invert) {
        jump = 1;
    } else {
        if (object) {
            AccumulateConditionBits(GetCharacterConditions(object), conditions);
        }
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

static __inline Character* GetResolvedPartyCharacter(i16 id) {
    return GetRosterCharacterById(ResolveObjectId(id), 1);
}

// The same for the first of the companions -2, -3 and -7 in the roster.
// @early-stop: with no companion and no invert, retail re-zeroes the jump
// flag in its register before the call; every spelling here passes the
// known-zero pointer instead.
RVA(0x00035340, 0xb5)
void OpJumpUnlessCompanionHealthy(i16 invert) {
    i16 conditions = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* companion = GetResolvedPartyCharacter(-2);
    if (!companion) {
        companion = GetResolvedPartyCharacter(-3);
    }
    if (!companion) {
        companion = GetResolvedPartyCharacter(-7);
    }
    if (!companion && invert) {
        jump = 1;
    } else {
        if (!companion) {
            ScriptJumpUnless(target, jump);
            return;
        }
        AccumulateConditionBits(GetCharacterConditions(companion), conditions);
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the player has an item in equipment slot 6.
RVA(0x00035400, 0x4d)
void OpJumpUnlessHeroEquipped(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* player = ResolveScriptObject(-1);
    if ((GetCharacterEquipment(player)[6].item != -1 && !invert)
        || (GetCharacterEquipment(player)[6].item == -1 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the script has no actor.
// @identity-TODO: the state is 3 with an actor and 0 without; what the
// values 0..2 against which it is tested stood for is unrecovered.
RVA(0x00035450, 0x4a)
void OpIfNoActor(i16 negate) {
    i16 state = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (GetScriptActor()) {
        state = 3;
    }
    if ((state < 2 && !negate) || (state > 2 && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party faces the operand's direction.
RVA(0x000354a0, 0x45)
void OpIfFacing(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if ((direction == g_field.pos.direction && !negate)
        || (direction != g_field.pos.direction && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// The same against the direction saved with the return position (with none
// saved, only when negated).
RVA(0x000354f0, 0x64)
void OpIfReturnFacing(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if (g_savedDirection == -1) {
        if (negate) {
            jump = 1;
        }
    } else if ((direction == g_savedDirection && !negate)
               || (direction != g_savedDirection && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has condition `condition` (no object counts as
// lacking it).
RVA(0x00035560, 0x63)
void OpIfObjectHasCondition(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    i16 has = ReadScriptValue();
    if (!object && negate) {
        jump = 1;
    } else {
        if (object) {
            has = HasCondition(GetCharacterConditions(object), has);
        }
        if (ScriptBooleanMatches(has, negate)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds the item.
RVA(0x000355d0, 0x44)
void OpIfHasItem(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 held = CountHeldItem(ReadScriptValue());
    if (ScriptBooleanMatches(held, negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds every item of the -1-terminated list (with
// `negate`, unless it holds none of them).
RVA(0x00035620, 0x67)
void OpIfHasAllItems(i16 negate) {
    i16 all = -1;
    i16 any = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 item;
    for (item = ReadScriptValue(); item != -1; item = ReadScriptValue()) {
        i16 held = CountHeldItem(item) ? -1 : 0;
        all &= held;
        any |= held;
    }
    if ((!negate && all) || (negate && !any)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Puts one of the item into the bag (without the full-bag prompt).
RVA(0x00035690, 0x33)
void OpGiveItem(void) {
    i16 item = ReadScriptValue();
    if (item > 0) {
        i16 quiet = SetBagQuiet(0);
        StoreBagItem(item, 1, -1);
        SetBagQuiet(quiet);
    }
}

RVA(0x000356d0, 0x16)
void OpTakeItem(void) {
    i16 item = ReadScriptValue();
    if (item > 0) {
        TakeBagItems(item, 1);
    }
}

RVA(0x000356f0, 0x55)
void OpOpenItemListWindow(void) {
    i16 totalVar = ReadScriptValue();
    i16 selling = ReadScriptValue();
    ScriptPanel* node = CreateScriptPanel(0x118, 8, 0, 0);
    i16 i;
    node->panel->flags |= PANEL_ALLOW_RIGHT_CLICK;
    node->panel->flags &= ~PANEL_IGNORE_RIGHT_CLICK;
    OpenScriptItemMenu(totalVar, selling);
    for (i = 0; i < 8; i++) {
        SetLastPanelRowState(i, PANEL_HANDLER_LOCKED);
    }
}

RVA(0x00035750, 0x12)
void OpCloseItemListWindow(void) {
    CloseScriptPanelByImage(0x115);
    CloseItemMenu();
}

RVA(0x00035770, 0x5)
void OpRedrawItemListTotal(void) {
    RefreshScriptItemMenuTotal();
}

RVA(0x00035780, 0x3b)
void OpIfPoolHasItems(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountPoolEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000357c0, 0x3b)
void OpIfBagHasEntries(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountBagEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035800, 0x26)
void OpGetItemPrice(void) {
    i16 item = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetItemPrice(item));
}

// Saves (0) or restores the kind-9 item table and the bag through the
// script variables from the operand on (capped at 176): the table at the
// operand, the bag 16 words on. Saving also resets the table and empties
// the bag.
RVA(0x00035830, 0x7d)
void OpStashItemLists(void) {
    i16 restore = ReadScriptValue();
    i16 var = ReadScriptValue();
    if (var >= 0xb0) {
        var = 0xb0;
    }
    if (!restore) {
        SaveGemItems((ItemStack*)&g_scriptVars[var]);
        ResetGemItems(GetGemItemBase());
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], 0);
        ClearBag();
    } else {
        RestoreGemItems((ItemStack*)&g_scriptVars[var]);
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], 1);
    }
}

// Stores (a positive count) or takes (a negative one) the item quietly and
// sets a long variable to how many were moved (never below 0 for a store).
RVA(0x000358b0, 0x6d)
void OpAdjustItemCount(void) {
    i16 item = ReadScriptValue();
    i16 count = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    i16 quiet = SetBagQuiet(1);
    i16 moved;
    if (count < 0) {
        moved = count - TakeBagItems(item, count);
    } else {
        moved = count - StoreBagItem(item, count, -1);
    }
    if (count > 0 && moved < 0) {
        moved = 0;
    }
    SetBagQuiet(quiet);
    SetScriptLongVar(index, moved);
}

// Lists the bag entries holding items of `category` (0: any; 1..19 an item
// kind; 20 excludes scenario items, 21 also requires a price, 22 is priceless items)
// into a new array handle, with `spare` extra entries; stores the handle and
// the count.
RVA(0x00035920, 0x11a)
void OpListBagByCategory(void) {
    i16 listVar = ReadLongVarIndex();
    i16 countVar = ReadLongVarIndex();
    i16 category = ReadScriptValue();
    i16 spare = ReadScriptValue();
    // Retail's frame holds more than the bag's 64 entries (65 words fit).
    i16 entries[65];
    i16 count = 0;
    i16 i;
    i32 handle;
    i32* list;
    for (i = 0; i < 64; i++) {
        i16 item = GetBagItem(i);
        if (item < 1) {
            continue;
        }
        if (category >= 0 && category <= 19) {
            if (category != 0 && GetItemKind(item) != category) {
                continue;
            }
        } else {
            if ((category == 20 || category == 21) && GetItemKind(item) == ITEM_KIND_SCENARIO) {
                continue;
            }
            if (category == 21 && GetItemPrice(item) == 0) {
                continue;
            }
            if (category == 22 && GetItemPrice(item) != 0) {
                continue;
            }
        }
        entries[count++] = i;
    }
    count += spare;
    handle = CreateArrayHandle(count + spare, 4);
    SetScriptLongVar(listVar, handle);
    SetScriptLongVar(countVar, count);
    list = HandleWritePtr(handle);
    for (i = 0; i < count; i++) {
        list[i] = entries[i];
    }
}

// Stores a whole bag entry (ItemStack) in a long variable.
RVA(0x00035a40, 0x28)
void OpGetBagEntry(void) {
    i16 index = ReadScriptValue();
    i16 var = ReadLongVarIndex();
    SetScriptLongVar(var, GetBagEntry(index)->value);
}

RVA(0x00035a70, 0x1a)
void OpClearBagEntry(void) {
    ItemStack* entry = GetBagEntry(ReadScriptValue());
    entry->count = 0;
    entry->hasAttachment = 0;
    entry->item = -1;
    entry->attachment = 0;
}

// @early-stop: the item and its remapped id swap registers (esi/edi); the
// permuter's search is flat.
RVA(0x00035a90, 0x88)
void OpTakeDropSlot(void) {
    i16 slot = ReadScriptValue();
    i16 itemVar = ReadLongVarIndex();
    i16 amountVar = ReadLongVarIndex();
    i16 item = GetDropSlot(slot)->item;
    i16 amount = GetDropSlot(slot)->amount;
    i16 remapped;
    ClearDropSlot(slot);
    remapped = RemapItem(item);
    if (remapped) {
        amount = RollDropAmount(item, amount);
    } else {
        remapped = item;
    }
    SetScriptLongVar(itemVar, remapped);
    SetScriptLongVar(amountVar, amount);
}

RVA(0x00035b20, 0xf)
i16 OpCallSubScene(void) {
    PushGameState(0x26);
    return -3;
}

// Stores in a long variable a handle to a copy of the item's decoded record
// (without its name and description pointers).
RVA(0x00035b30, 0x52)
void OpCopyItemRecord(void) {
    i16 index = ReadLongVarIndex();
    ItemRecord* record = GetLoadedRecord(ReadScriptValue());
    i32 handle = AllocHandle(sizeof(ItemRecord));
    ItemRecord* copy = HandleWritePtr(handle);
    *copy = *record;
    copy->name = NULL;
    copy->description = NULL;
    SetScriptLongVar(index, handle);
}

RVA(0x00035b90, 0x14)
void OpOpenFusionScreen(i16 kind) {
    PushFusionMenu(kind, ReadLongVarIndex());
}

RVA(0x00035bb0, 0x1c)
void OpRunFusion(i16 triple) {
    SetBlankStep(1);
    if (!triple) {
        RunPairFusion();
    } else {
        RunTripleFusion();
    }
}

RVA(0x00035bd0, 0x5)
void OpEndFusion(void) {
    EndFusion();
}

// Jumps unless `cond` is zero.
RVA(0x00035be0, 0x20)
void OpJumpIf(i16 cond) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (cond == 0) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035c00, 0x11)
void OpSkipJumpTarget(i16 unused) {
    i16 target = ReadBranchTarget();
    ScriptJumpUnless(target, 1);
}

// Jumps unless the roster's demon count is above `limit` (mode 0) or at most
// `limit` (mode 1).
RVA(0x00035c20, 0x45)
void OpIfDemonCount(i16 mode, i16 limit) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(0);
    if ((mode == 0 && demons > limit) || (mode == 1 && demons <= limit)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035c70, 0x1c)
void OpGetFusionResult(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetFusionResultKind());
}

RVA(0x00035c90, 0x2c)
void OpAddMagnetite(i16 sign) {
    AddMagnetite(ResolveScriptObject(-1), ReadScriptValue() * sign);
    DrawMoneyCounters(1);
}

RVA(0x00035cc0, 0x2c)
void OpAddMacca(i16 sign) {
    AddMacca(ResolveScriptObject(-1), ReadScriptValue() * sign);
    DrawMoneyCounters(1);
}

// Queues the 0xff-terminated operand bytes as automatic moves and holds the
// scene. At area 0x82 level 5, cell 4/9 facing 3, a first move of 3 becomes 1.
// @identity-TODO: why that one spot's first move is rewritten is unrecovered.
RVA(0x00035cf0, 0xb8)
void OpQueueAutoMoves(void) {
    u16 pc = g_curScript->pc;
    i16 count = 0;
    i32 i;
    i16 move;
    while (ReadScriptByte() != 0xff) {
        count++;
    }
    GrowAutoMoves(count);
    g_curScript->pc = pc;
    i = 0;
    move = ReadScriptByte();
    while (move != 0xff) {
        if (i == 0 && g_field.pos.area == 0x82 && g_field.pos.level == 5 && g_field.pos.x == 4
            && g_field.pos.y == 9 && g_field.pos.direction == 3 && move == 3) {
            move = 1;
        }
        PushAutoMove(move);
        i++;
        move = ReadScriptByte();
    }
    ExchangeSceneHold(1);
}

// Sets the return point (area, level, x, y, direction) the field leaves to.
// @identity-TODO: the operand order is read from SetReturnPoint's stores.
RVA(0x00035db0, 0x40)
void OpChangeMap(void) {
    i16 area = ReadScriptValue();
    i16 level = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    SetReturnPoint(area, level, x, y, ReadScriptValue());
    g_worldMapRequest = -1;
}

// Sets the world-map layer and spot the world map opens at.
RVA(0x00035df0, 0x2c)
void OpSetWorldMapSpot(void) {
    i16 layer = ReadScriptValue();
    i16 x = ReadScriptValue();
    SetWorldMapSpot(layer, x, ReadScriptValue());
    g_worldMapRequest = 1;
}

// Adds a world-map route point: layer `layer`'s origin plus an x/y offset
// (layer 0 drops the route instead).
RVA(0x00035e20, 0x4c)
void OpAddRoutePoint(void) {
    MapCoord point;
    i16 layer = ReadScriptValue();
    if (!layer) {
        ReadScriptValue();
        ReadScriptValue();
        FreeRoute();
        return;
    }
    point = GetLayerOrigin(layer);
    point.x += ReadScriptValue();
    point.y += ReadScriptValue();
    PushRoutePoint(point);
}

// Stores the party's area, level, x, y and direction in five long variables.
RVA(0x00035e70, 0x75)
void OpGetPlayerLocation(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.area);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.level);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.x);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.y);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.direction);
}

RVA(0x00035ef0, 0x23)
void OpSetPlayerPosition(void) {
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    MovePartyTo(x, y, ReadScriptValue());
}

RVA(0x00035f20, 0xe8)
void OpIfBlockedToward(i16 negate, i16 turn) {
    i32 matches = 0;
    i16 target = ReadBranchTarget();
    i16 x = g_field.pos.x;
    i16 y = g_field.pos.y;
    i16 direction = g_field.pos.direction;
    i16 blocked;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        blocked = 1;
    } else {
        blocked = GetMapWallKind(x, y, (direction + turn) & 3);
    }
    if (!blocked) {
        StepMapCoord(&x, &y, direction, turn);
        blocked = IsCellBlocked(g_field.pos.level, 1, x, y);
        if (!blocked && g_curScript->actor != NULL) {
            blocked = DistanceToParty((FieldActor*)g_curScript->actor) == 0;
        }
    }
    if ((!blocked && !negate) || (blocked && negate)) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

// Runs move command `effect` as a screen transition and refreshes the field.
RVA(0x00036010, 0x19)
i16 PlayScreenTransition(i16 effect) {
    RunMoveCommand(effect, 0);
    RequestFieldRefresh();
    return -3;
}

// Plays a screen transition, then redraws the field screen in one long frame
// with the status redraw locked.
RVA(0x00036030, 0x41)
i16 OpScreenTransition(void) {
    i16 result = PlayScreenTransition(ReadScriptValue());
    i16 lock = LockStatusRedraw(1);
    UpdateFieldScreen(0);
    SetLongFrame(1);
    LockStatusRedraw(lock);
    return result;
}

static __inline void LoadScriptCharacterToRoster(i16 id) {
    Character* character = LoadCharacterCore(id, NULL);
    SetAnalyzed(character->id, 1);
    AddScriptCharacterToRoster(character, 3);
}

RVA(0x00036080, 0xd7)
i16 OpAddToRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    Character* character;
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id != -1) {
            DespawnScriptActor();
            LoadScriptCharacterToRoster(id);
        }
        return -1;
    }
    if (id >= 0) {
        LoadScriptCharacterToRoster(id);
        return 0;
    }
    character = GetCharacter(ObjectSlotOfId(ref));
    if (RosterSlotOfId(character->id) == -1) {
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    return 0;
}

RVA(0x00036160, 0x50)
void AddScriptCharacterToRoster(Character* character, i16 unused) {
    if (AddToRoster(character) < 0) {
        g_rosterPendingMember = character;
        g_rosterReturnState = GetGameState();
        g_rosterReturnPhase = GetGamePhase();
        g_rosterReturnStep = GetGameStep();
        SetGameState(0x28);
        SetGamePhase(0);
    }
}

RVA(0x000361b0, 0x97)
i16 OpRemoveFromRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (id >= 3000) {
        id = ref - 3000;
    } else if (id >= 2000) {
        RemoveFromRoster(id - 2000);
        return 0;
    } else if (id >= 1000) {
        i16 slot = GetPartySlot(id - 1000);
        if (slot != -1) {
            RemoveFromRoster(slot);
        }
        return 0;
    } else {
        if (id == -17 || id == -19) {
            return -1;
        }
        if (id == -18) {
            id = GetScriptActorId();
            if (id == -1) {
                return -1;
            }
        }
        if (id < 0) {
            id = ResolveObjectId(id);
        }
    }
    RemoveFromRoster(RosterSlotOfId(id));
    return 0;
}

RVA(0x00036250, 0xdd)
i16 OpJoinActiveParty(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    i16 slot;
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (id >= 2000) {
        id = GetRosterId(id - 2000);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id == -1) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    if (id < 32 && RosterSlotOfId(id) == -1) {
        Character* character = FindCharacterById(id);
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    slot = RosterSlotOfId(id);
    if (id >= 32 || FindPartyPositionOfId(id) == -1) {
        AddToParty(slot);
        RequestFieldRefresh();
    }
    return 0;
}

RVA(0x00036330, 0xa0)
i16 OpLeaveActiveParty(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (id >= 2000) {
        id = GetRosterId(id - 2000);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id == -1) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    RemoveFromParty(FindRosterSlotById(id));
    if (id < 32) {
        RemoveFromRoster(RosterSlotOfId(id));
    }
    RequestFieldRefresh();
    return 0;
}

RVA(0x000363d0, 0x28)
i16 OpSelectPartySlot(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, PollPartySlotSelection(ReadScriptValue()));
    return 0;
}

RVA(0x00036400, 0x9)
i16 OpEndPartySlotSelect(void) {
    ClearPartySlotSelection();
    return 0;
}

RVA(0x00036410, 0x2c)
i16 OpCountActiveParty(void) {
    i16 index = ReadLongVarIndex();
    i16 count = CountPartyMembers(ReadScriptValue());
    SetScriptLongVar(index, count);
    return count;
}

RVA(0x00036440, 0x7b)
i16 OpGetCombatantId(void) {
    i16 index = ReadLongVarIndex();
    i16 id = ReadObjectRef();
    if (id == -20) {
        id = g_actorId;
    } else if (id == -21) {
        id = g_targetId;
    } else {
        id = -1;
    }
    SetScriptLongVar(index, id);
    return id;
}

RVA(0x000364c0, 0x92)
void OpIfObjectIsAlly(i16 negate) {
    i16 target = ReadBranchTarget();
    i16 id = ReadObjectRef();
    i32 matches;
    if (id == -20) {
        id = g_actorId;
    } else if (id == -21) {
        id = g_targetId;
    } else if (id == -16) {
        id = GetPartySlot(FindFavouredMember());
        if (id >= 0) {
            id = -1 - id;
        }
    }
    matches = 0;
    if ((id < 0 && !negate) || (id >= 0 && negate)) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x00036560, 0x79)
void OpRebalanceMemberStats(void) {
    Character* character = GetRosterCharacter(ReadScriptValue());
    if (character) {
        i16 i;
        for (i = 0; i < 11; i++) {
            i16 sum = character->stats.bonus[i] + character->stats.equipment[i]
                      + GetBaseStat(character, i) + character->stats.modifiers[i];
            if (HasCondition(GetCharacterConditions(character), 8)) {
                sum /= 2;
            }
            character->stats.base[i] += GetStatTotal(character, i) - sum;
        }
        FullyRestoreCharacter(character);
    }
}

RVA(0x000365e0, 0x2f)
void OpAddMemberSkill(void) {
    i16 index = ReadScriptValue();
    i16 skill = ReadScriptValue();
    Character* character = GetRosterCharacter(index);
    if (character != NULL) {
        AddSkill(GetCharacterSkills(character), skill);
    }
}
