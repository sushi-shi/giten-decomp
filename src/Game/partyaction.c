// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/CombatantId.h>

#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/Field.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/ItemRecord.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyStatus.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Game/StatusDraw.h>
#include <Gfx/Render.h>
#include <Input/Mouse.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Script/ScriptVars.h>
#include <Script/TextState.h>
#include <Text/TextWindow.h>
#include <Ui/Message.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <string.h>

DATA(0x00068430)
static i16 (*s_commandLabels[])(Character*) = {
    FormatAttackCommand,
    FormatGunCommand,
    FormatCompCommand,
    FormatMagicCommand,
    FormatItemCommand,
    FormatExtraCommand,
    FormatReturnCommand,
    FormatDefenceCommand
};

DATA(0x00068450)
static i16 s_actionConditions[] = {7, 17, 30, 14, 18, 16, 27, 28, 29, 26, -1};

DATA(0x00078514)
static MenuBox* s_commandMenu;

RVA(0x00005a80, 0x86)
i16 PickPartyMember(i16 index) {
    Character* member;
    if (PartySlotAt(index) == -1) {
        return -1;
    }
    member = GetPartyCharacter(index);
    if (!GetPickBlockingCondition(GetCharacterConditions(member))
        && TickFieldCount(PartyCombatantId(index), 1) >= 1) {
        if (GetFieldBattleActive()) {
            if (IsActionWaitPickable(GetCharacterActionWait(member))) {
                return 1;
            }
        } else if (!IsActionWaitMarked(GetCharacterActionWait(member))) {
            return 1;
        }
    }
    return 0;
}

RVA(0x00005b10, 0x31)
i16 FindPickablePartyMember(i16 index) {
    for (; index < 6; index++) {
        if (PickPartyMember(index) > 0) {
            return index;
        }
    }
    return -1;
}

RVA(0x00005b50, 0x26)
i16 CountPickablePartyMembers(void) {
    i16 count = 0;
    i16 index = FindPickablePartyMember(0);
    while (index >= 0) {
        count++;
        index = FindPickablePartyMember(index + 1);
    }
    return count;
}

RVA(0x00005b80, 0xdf)
i16 GetMemberPanelState(i16 index) {
    Character* member = g_panelMembers[index];
    i16 state = 1;
    if (!member) {
        return -1;
    }
    if (GetFatalCondition(GetCharacterConditions(member))) {
        state = 4;
    } else if (GetPickBlockingCondition(GetCharacterConditions(member))) {
        state = 1;
    } else if (TickFieldCount(PartyCombatantId(index), 1) < 1) {
        state = 1;
    } else if (IsActionWaitPickable(GetCharacterActionWait(member))) {
        state = IsCharacterHpLow(member) ? 3 : 0;
    }
    if (!GetFieldBattleActive() && state == 1) {
        state = IsCharacterHpLow(member) ? 3 : 0;
    }
    return state;
}

RVA(0x00005c60, 0x50)
i16 ReadActionResultFlags(void) {
    i16 result = 0;
    i16 bit;
    for (bit = 31; bit >= 28; bit--) {
        result <<= 1;
        if (IsEventFlagSet(15, bit)) {
            result |= 1;
        }
    }
    if (IsEventFlagSet(15, 27)) {
        result |= 16;
    }
    if (IsEventFlagSet(15, 26)) {
        result |= 32;
    }
    return result;
}

RVA(0x00005cb0, 0x18)
i16 GetActionCondition(Character* actor) {
    return LastConditionIn(GetCharacterConditions(actor), s_actionConditions);
}

RVA(0x00005cd0, 0xe9)
i16 PickActorAction(Character* actor) {
    i16 result;
    if (!GetActionCondition(actor)) {
        actor->conditionActionTicks = 0;
        return -1;
    }
    ChangeCharacterFlag(actor, 17, 0);
    ChangeCharacterFlag(actor, 18, 0);
    ChangeCharacterFlag(actor, 19, 0);
    ChangeCharacterFlag(actor, 20, 0);
    ChangeCharacterFlag(actor, 21, 0);
    ChangeCharacterFlag(actor, 22, 0);
    ClearFlagBank(12);
    StartScript(0xdb, 0, NewScriptContext(0, NULL));
    ClearScriptLongVars();
    do {
        result = RunScriptStep(0);
    } while (result >= 0);
    RetakeDeferredChar(
        0,
        result,
        "\203R\203\223\203f\203B\203V\203\207\203\223\211e\213\277\203t\203\215\201["
    );
    FreeScriptContext(GetCurrentScript());
    result = ReadActionResultFlags();
    if (TestCharacterFlag(actor, 31)) {
        result |= 64;
    }
    return result;
}

RVA(0x00005dc0, 0x83)
i16 PickRandomOpponentAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target;
    if (id < 0) {
        target = PickRandomCombatant(2);
    } else {
        target = PickRandomCombatant(1);
    }
    if (target == -100) {
        return 0;
    }
    actor->mode = 1;
    actor->pickRole = 1;
    actor->pickTarget = GetCharacterEquipment(actor)[5].item;
    actor->pickTargetHigh = 0;
    actor->pickObject = target;
    return 2;
}

RVA(0x00005e50, 0xda)
i16 PickRandomCombatant(u8 sides) {
    i16 targets[22];
    MapCoord pos;
    Character* member;
    i16 count = 0;
    i16 index;
    i16 object;
    if (sides & 1) {
        for (index = 0; index < 6; index++) {
            member = GetPartyCharacter(index);
            if (member && !GetDisablingCondition(GetCharacterConditions(member))) {
                targets[count++] = PartyCombatantId(index);
            }
        }
    }
    if (sides & 2) {
        for (index = 0; index < 16; index++) {
            object = GetLiveObject(index);
            if (object >= 0
                && !GetDisablingCondition(GetFieldObjectConditions(GetFieldObject(object)))) {
                pos = GetObjectCoord(index);
                if (GetPartyView(pos.x, pos.y)) {
                    targets[count++] = object;
                }
            }
        }
    }
    if (!count) {
        return -100;
    }
    return targets[RandomAverage(0, count - 1, 0)];
}

RVA(0x00005f30, 0x83)
i16 PickRandomAllyAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target;
    if (id < 0) {
        target = PickRandomCombatant(1);
    } else {
        target = PickRandomCombatant(2);
    }
    if (target == -100) {
        return 0;
    }
    actor->mode = 1;
    actor->pickRole = 1;
    actor->pickTarget = GetCharacterEquipment(actor)[5].item;
    actor->pickTargetHigh = 0;
    actor->pickObject = target;
    return 2;
}

RVA(0x00005fc0, 0x22)
b16 PickActorDialogue(i16 id) {
    if (id < 0) {
        return false;
    }
    GetCombatant(id)->mode = 11;
    return true;
}

RVA(0x00005ff0, 0x6b)
b16 DelayActionSide(i16 id) {
    FieldObject* fieldObject;
    i16 i;
    i16 object;
    Character* member;
    if (id < 0) {
        for (i = 0; i < 6; i++) {
            member = GetPartyCharacter(i);
            if (member) {
                GetCharacterActionWait(member)->remaining =
                    50 + GetCharacterActionWait(member)->remaining;
            }
        }
        MarkPickDone();
        return true;
    }
    for (i = 0; i < 16; i++) {
        object = GetLiveObject(i);
        if (object >= 0) {
            fieldObject = GetFieldObject(object);
            GetFieldObjectActionWait(fieldObject)->remaining =
                50 + GetFieldObjectActionWait(fieldObject)->remaining;
        }
    }
    return true;
}

RVA(0x00006060, 0x57)
b16 ResetActionWaits(void) {
    i16 i;
    i16 object;
    Character* member;
    for (i = 0; i < 6; i++) {
        member = GetPartyCharacter(i);
        if (member) {
            ResetActionWaitDelay(GetCharacterActionWait(member));
        }
    }
    for (i = 0; i < 16; i++) {
        object = GetLiveObject(i);
        if (object >= 0) {
            ResetActionWaitDelay(GetFieldObjectActionWait(GetFieldObject(object)));
        }
    }
    MarkPickDone();
    return true;
}

RVA(0x000060c0, 0x77)
i16 PickRandomAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target = PickRandomCombatant(3);
    if (target == -100) {
        return 0;
    }
    actor->mode = 1;
    actor->pickRole = 1;
    actor->pickTarget = GetCharacterEquipment(actor)[5].item;
    actor->pickTargetHigh = 0;
    actor->pickObject = target;
    return 2;
}

RVA(0x00006140, 0x34)
b16 SwapPartyRows(void) {
    i16 i;
    i16 member;
    for (i = 0; i < 3; i++) {
        member = PartySlotAt(i);
        g_party[i] = PartySlotAt(i + 3);
        g_party[i + 3] = member;
    }
    MarkPickDone();
    FlushStatusRedraw(1);
    return true;
}

RVA(0x00006180, 0x1bc)
i16 AdjustActorAction(i16 id, i16 action) {
    i16 hold;
    i16 window;
    i16 result;
    Character* actor;
    g_actorId = id;
    ClearFlagBank(12);
    hold = SetHold(1);
    window = OpenMessageWindow();
    StartScript(0xdb, 1, NewScriptContext(0, NULL));
    ClearScriptLongVars();
    RefreshMessageWindow();
    ResetTextStateDelayed();
    do {
        result = RunScriptStep(window);
        if (result == -2) {
            OpenMessageWindow();
            RefreshMessageWindow();
        }
    } while (result != -1);
    FreeScriptContext(GetCurrentScript());
    SetHold(hold);
    SetMessageLifetime(60);
    SetMessageHold(0);
    result = 0;
    actor = GetCombatant(id);
    switch (action & 15) {
        case 0:
            actor->mode = 10;
            result = 1;
            break;
        case 1:
        case 9:
        case 12:
            result = PickRandomOpponentAttack(id);
            break;
        case 2:
        case 10:
        case 13:
            result = PickRandomAllyAttack(id);
            break;
        case 3:
            actor->mode = 2;
            result = 1;
            break;
        case 4:
        case 8:
            result = 0;
            break;
        case 5:
            result = PickActorDialogue(id);
            break;
        case 6:
        case 11:
            result = DelayActionSide(id);
            actor->mode = 10;
            break;
        case 7:
            result = ResetActionWaits();
            actor->mode = 10;
            break;
        case 14:
            result = PickRandomAttack(id);
            break;
        case 15:
            SwapPartyRows();
            actor->mode = 10;
            result = 1;
            break;
    }
    return result;
}

RVA(0x00006340, 0x4f)
MenuBox* OpenActorCommandMenu(i16 id) {
    Character* actor = GetCharacterById(id);
    MenuBox* menu;
    if (!actor) {
        return NULL;
    }
    menu = CreateMenuBox(NULL, 5, 2);
    SetMenuItems(menu, 9, actor, 8, ActorCommandMenuHandler);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    return menu;
}

RVA(0x00006390, 0xaf)
void ActorCommandMenuHandler(MenuBox* menu, i16 index, i16 event) {
    Character* actor = menu->items.character;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            FormatFullName(g_scratchBuffer, actor);
            AddMenuLine(menu->plane, g_scratchBuffer, 0x2460, -1, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            if (index < 8) {
                if (!s_commandLabels[index](actor)) {
                    AddMenuLine(menu->plane, g_unavailableCommandText, 0x2500, index + 1, 1);
                } else {
                    AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, index + 1, 0);
                }
            }
            break;
    }
}

RVA(0x00006440, 0x1b)
i16 PollActorCommandMenu(MenuBox* menu) {
    i16 result = RunMenu(menu);
    if (result <= 0) {
        return result - 1;
    }
    return g_selectedObjectId;
}

static __inline void CloseActorCommandMenu(void) {
    s_commandMenu = DestroyMenuBox(s_commandMenu);
}

// @dead-code
// Zero-ref: no direct call/jmp, relocated reference or data slot reaches it.
RVA(0x00006460, 0x9e)
i16 RunActorCommandMenu(i16 id) {
    Character* actor = GetCharacterById(id);
    i16 result;
    if (actor && !GetPickBlockingCondition(GetCharacterConditions(actor))) {
        if (!s_commandMenu) {
            s_commandMenu = OpenActorCommandMenu(id);
        }
        result = PollActorCommandMenu(s_commandMenu);
        if (result == -1) {
            return result;
        }
        if (result != -2) {
            CloseActorCommandMenu();
            actor->pickRole = g_selectedObjectId;
            return g_selectedObjectId;
        }
    }
    CloseActorCommandMenu();
    return -2;
}

RVA(0x00006500, 0xaa)
b16 FormatAttackCommand(Character* actor) {
    ItemRecord record;
    if (GetCharacterEquipment(actor)[5].item < 1) {
        strcpy(g_scratchBuffer, "ATTACK");
        return true;
    }
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[5].item);
    if (!record.kind) {
        strcpy(g_scratchBuffer, "ATTACK");
        return true;
    }
    strcpy(g_scratchBuffer, "SWORD");
    return true;
}

RVA(0x000065b0, 0xb0)
b16 FormatGunCommand(Character* actor) {
    ItemRecord record;
    if (GetCharacterEquipment(actor)[6].item < 1) {
        return false;
    }
    if (HasObjectInReach(0, -1, 0)) {
        return false;
    }
    strcpy(g_scratchBuffer, "GUN");
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[6].item);
    if (!record.kind) {
        return false;
    }
    if (GetCharacterEquipment(actor)[7].item < 1) {
        return false;
    }
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[7].item);
    return record.kind != 0;
}

RVA(0x00006660, 0x77)
b16 FormatMagicCommand(Character* actor) {
    i16 index;
    strcpy(g_scratchBuffer, "MAGIC");
    if (!GetWordCount(GetCharacterSkills(actor))) {
        return false;
    }
    for (index = 0; index < GetWordCount(GetCharacterSkills(actor)); index++) {
        if (GetSkillCost(GetWord(GetCharacterSkills(actor), index)) >= 0) {
            CanUseSkill(GetWord(GetCharacterSkills(actor), index), actor);
        }
    }
    return true;
}

RVA(0x000066e0, 0x39)
b16 FormatItemCommand(Character* actor) {
    strcpy(g_scratchBuffer, "ITEM");
    if (!actor) {
        return false;
    }
    if (!IsHumanCharacter(actor)) {
        return false;
    }
    return CountBagEntries() != 0;
}

RVA(0x00006720, 0x2a)
b16 FormatCompCommand(Character* actor) {
    strcpy(g_scratchBuffer, "COMP");
    return actor->compState == 1;
}

RVA(0x00006750, 0x76)
b16 FormatExtraCommand(Character* actor) {
    i16 index;
    strcpy(g_scratchBuffer, "EXTRA");
    if (!GetWordCount(GetCharacterSkills(actor))) {
        return false;
    }
    for (index = 0; index < GetWordCount(GetCharacterSkills(actor)); index++) {
        if (GetSkillCost(GetWord(GetCharacterSkills(actor), index)) <= 0) {
            CanUseSkill(GetWord(GetCharacterSkills(actor), index), actor);
        }
    }
    return false;
}

RVA(0x000067d0, 0x40)
b16 FormatReturnCommand(Character* actor) {
    if (!g_fieldBattleActive) {
        return false;
    }
    strcpy(g_scratchBuffer, "RETURN");
    return !IsHumanCharacter(actor);
}

RVA(0x00006810, 0x27)
b16 FormatDefenceCommand(Character* actor) {
    if (!actor) {
        return false;
    }
    strcpy(g_scratchBuffer, "DEFENCE");
    return true;
}

RVA(0x00006840, 0xb4)
i16 PrepareMemberPickTarget(i16 id) {
    Character* actor = GetCharacterById(id);
    if (actor) {
        switch (actor->pickRole) {
            case 1:
                actor->pickTarget = GetCharacterEquipment(actor)[5].item;
                actor->pickTargetHigh = 0;
                return 1;
            case 2:
                actor->pickTarget = GetCharacterEquipment(actor)[6].item;
                actor->pickTargetHigh = 0;
                return 1;
            case 4:
            case 6:
                actor->pickTargetHigh = -1;
                return 0;
            case 3:
            case 5:
                actor->pickTargetHigh = 0;
                return 0;
            case 7:
            case 8:
                actor->pickTarget = 0;
                actor->pickTargetHigh = 0;
                return 4;
        }
    }
    return -1;
}

RVA(0x00006900, 0xf9)
void FillCharacterCommands(i16* list, i16 id) {
    Character* actor = GetCharacterById(id);
    i16 count = 0;
    if (actor) {
        list[count++] = 0;
        if (FormatGunCommand(actor)) {
            list[count++] = 1;
        }
        if (FormatMagicCommand(actor)) {
            list[count++] = 2;
        }
        if (FormatItemCommand(actor)) {
            list[count++] = 3;
        }
        if (FormatDefenceCommand(actor)) {
            list[count++] = 4;
        }
        if (FormatReturnCommand(actor)) {
            list[count++] = 5;
        }
        if (CanCharacterOpenAutomap(actor)) {
            list[count++] = 6;
        }
        list[count++] = 7;
        if (GetRenderMode() == 6) {
            list[count++] = 8;
        }
        while (count < 8) {
            list[count++] = -1;
        }
    }
}

RVA(0x00006a00, 0x39)
b16 ResetPartyTurnState(void) {
    i16 index;
    Character* actor;
    for (index = 0; index < 6; index++) {
        if (PartySlotAt(index) != -1) {
            actor = GetPartyEntry(index);
            ClearActionWait(GetCharacterActionWait(actor));
        }
    }
    return false;
}

RVA(0x00006a40, 0x32)
i32 ScaleActionValue(i32 value, i16 resistance, i16 multiplier) {
    i32 scale;
    if (resistance < 0) {
        resistance = 50;
    }
    scale = resistance * multiplier;
    return scale * value / 100;
}

RVA(0x00006a80, 0x21a)
i16 CheckBattleProtection(Character* actor, i16 attribute, i16 mode, i16 report) {
    if (GetCharacterBattleTallies(actor)[13]) {
        ReportBattleTally(actor, 13, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[14] && (mode == 0 || mode == 2)) {
        ReportBattleTally(actor, 14, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[0] && mode == 0) {
        ReportBattleTally(actor, 0, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[7] && mode == 2) {
        ReportBattleTally(actor, 7, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[8] && attribute == 2) {
        ReportBattleTally(actor, 8, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[9] && attribute == 3) {
        ReportBattleTally(actor, 9, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[10] && attribute == 5) {
        ReportBattleTally(actor, 10, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[11] && attribute == 6) {
        ReportBattleTally(actor, 11, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[12] && attribute == 8) {
        ReportBattleTally(actor, 12, report);
        return 0;
    }
    if (GetCharacterBattleTallies(actor)[1] && mode == 0) {
        ReportBattleTally(actor, 1, report);
        return -4;
    }
    if (GetCharacterBattleTallies(actor)[2] && mode == 0) {
        ReportBattleTally(actor, 2, report);
        return -5;
    }
    if (GetCharacterBattleTallies(actor)[5] && attribute == 0) {
        ReportBattleTally(actor, 5, report);
        return -4;
    }
    if (GetCharacterBattleTallies(actor)[6] && attribute == 1) {
        ReportBattleTally(actor, 6, report);
        return -4;
    }
    if (GetCharacterBattleTallies(actor)[3] && mode == 0) {
        ReportBattleTally(actor, 3, report);
        return -3;
    }
    return 1;
}

RVA(0x00006ca0, 0xcd)
i16 GetActionResistance(Character* actor, i16 attribute, i16 mode, i16 report, i16 sameSide) {
    i16 result;
    if (!actor) {
        return 0;
    }
    if (sameSide && GetCombatantSideRelation()) {
        return 50;
    }
    result = CheckBattleProtection(actor, attribute, mode, report);
    if (!result) {
        return -6;
    }
    if (result >= 1) {
        if (HasCondition(GetCharacterConditions(actor), CONDITION_ZOMBIE) && attribute == 6) {
            return 100;
        }
        result = attribute == 10 ? 50 : actor->resistance[attribute];
        if (result == 255) {
            return -5;
        }
        if (result == 254) {
            return -4;
        }
        if (result == 253) {
            return -3;
        }
        if (result == 252) {
            return -2;
        }
        if (result == 251) {
            return -1;
        }
    }
    return result;
}

RVA(0x00006d70, 0x4d)
i16 GetSkillResistance(Character* actor, i16 skill, i16 report, i16 sameSide, i16* attribute) {
    *attribute = GetSkillAttackAttribute(GetCachedSkill(skill));
    return GetActionResistance(
        actor,
        *attribute,
        GetCachedSkill(skill)->parameters.mode,
        report,
        sameSide
    );
}

RVA(0x00006dc0, 0x34)
i16 GetItemResistance(Character* actor, i16 item, i16 report, i16 sameSide, i16* attribute) {
    *attribute = GetLoadedRecord(item)->params[0xf];
    return GetActionResistance(actor, *attribute, 0, report, sameSide);
}

RVA(0x00006e00, 0x114)
i16 GetPickedAttackAttribute(Character* actor, i16* condition) {
    switch (actor->pickRole) {
        case 1:
            *condition = 0;
            if (GetCharacterEquipment(actor)[5].item != -1) {
                *condition = GetLoadedRecord(GetCharacterEquipment(actor)[5].item)->params[0x24];
                return GetLoadedRecord(GetCharacterEquipment(actor)[5].item)->params[0x21];
            }
            break;
        case 4:
            *condition = GetSkillInflictedCondition(GetCachedSkill(actor->pickTarget));
            return GetSkillAttackAttribute(GetCachedSkill(actor->pickTarget));
        case 2:
        case 5:
            *condition = GetLoadedRecord(actor->pickTarget)->params[0x24];
            return GetLoadedRecord(actor->pickTarget)->params[0x21];
    }
    return 0;
}

RVA(0x00006f20, 0x1a4)
void ApplyResistanceOutcome(Character* actor, i16 resistance, i32 amount) {
    switch (resistance) {
        case -6:
            SetCharacterChanges(actor, 0, 0);
            SetResistanceResult(actor, -6, 10);
            break;
        case -5:
            SetCharacterChanges(actor, amount / 2, amount / 2);
            SetResistanceResult(actor, -4, 7);
            break;
        case -4:
            SetCharacterChanges(actor, 0, amount);
            SetResistanceResult(actor, -5, 7);
            break;
        case -3:
            SetCharacterChanges(actor, amount, 0);
            SetResistanceResult(actor, -3, 9);
            break;
        case -2:
            SetCharacterChanges(actor, amount / 2, amount / 2);
            SetResistanceResult(actor, -1, 8);
            break;
        case -1:
            SetCharacterChanges(actor, amount, 0);
            SetResistanceResult(actor, -2, 8);
            break;
        case 0:
            SetCharacterChanges(actor, 0, 0);
            actor->result = 6;
            break;
        default:
            SetCharacterChanges(actor, amount, 0);
            break;
    }
}
