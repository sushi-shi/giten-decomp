// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of partyaction, field, attack and partypick interleave in a single run,
// and their initialized data and string literals form one .data run. The
// field-map routine at 0x407390 and the field state handler at 0x407aa0
// (about forty unclaimed callees) use the field statics and belong to it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Actor.h>
#include <Game/Analyze.h>
#include <Game/Attack.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/CombatantId.h>
#include <Game/Condition.h>
#include <Game/EquipEffect.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/Guest.h>
#include <Game/ItemEffect.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyStatus.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/TargetFlags.h>
#include <Game/WaitState.h>
#include <Game/WorldMap.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Script/ScriptVars.h>
#include <Script/TextState.h>
#include <Script/TextToken.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/FieldMenus.h>
#include <Ui/Hotspot.h>
#include <Ui/Menu.h>
#include <Ui/Message.h>
#include <Ui/PartySlotSelection.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// The temporary swap: the party position and the roster slot it held.
DATA(0x00068400)
i16 g_guestIndex = -1;

DATA(0x00068404)
static i16 s_swapSaved = -1;

// @identity-TODO: the party position the picker last targeted (-1: none).
DATA(0x00068408)
static i16 s_pickedIndex = -1;

DATA(0x0006840c)
static i16 s_fieldCountA = -1;

DATA(0x00068410)
static i16 s_fieldRateA = 100;

DATA(0x00068414)
static i16 s_fieldCountB = -1;

DATA(0x00068418)
static i16 s_fieldRateB = 100;

// -1 while no field map is active.
DATA(0x0006841c)
static i16 s_fieldMode = -1;

// The music track that was playing when the encounter started (or the field
// map was entered).
DATA(0x00068420)
static i16 s_fieldMusic = -1;

DATA(0x00068424)
static i16 s_fieldOption = 20;

DATA(0x00068428)
static i16 s_fieldParamFirst = 1;

DATA(0x0006842c)
static i16 s_fieldParamSecond = -1;

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

DATA(0x00078538)
char g_unavailableCommandText[8] = {0};

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

static __inline i16 GetReadyMemberPanelState(Character* member) {
    return IsCharacterHpLow(member) ? 3 : 0;
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
        state = GetReadyMemberPanelState(member);
    }
    if (!GetFieldBattleActive() && state == 1) {
        state = GetReadyMemberPanelState(member);
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

static __inline void SetBasicAttackPick(Character* actor, i16 target) {
    actor->mode = 1;
    actor->pickRole = 1;
    SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[5].item);
    actor->pickObject = target;
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
    SetBasicAttackPick(actor, target);
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
    SetBasicAttackPick(actor, target);
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
                DelayActionWait(GetCharacterActionWait(member), 50);
            }
        }
        MarkPickDone();
        return true;
    }
    for (i = 0; i < 16; i++) {
        object = GetLiveObject(i);
        if (object >= 0) {
            fieldObject = GetFieldObject(object);
            DelayActionWait(GetFieldObjectActionWait(fieldObject), 50);
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
    SetBasicAttackPick(actor, target);
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
                SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[5].item);
                return 1;
            case 2:
                SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[6].item);
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
                SetCharacterPickTarget(actor, 0);
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
                *condition = GetEquipmentInflictedCondition(
                    GetLoadedRecord(GetCharacterEquipment(actor)[5].item)
                );
                return GetEquipmentAttribute(GetLoadedRecord(GetCharacterEquipment(actor)[5].item));
            }
            break;
        case 4:
            *condition = GetSkillInflictedCondition(GetCachedSkill(actor->pickTarget));
            return GetSkillAttackAttribute(GetCachedSkill(actor->pickTarget));
        case 2:
        case 5:
            *condition = GetEquipmentInflictedCondition(GetLoadedRecord(actor->pickTarget));
            return GetEquipmentAttribute(GetLoadedRecord(actor->pickTarget));
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

DATA(0x000919f2)
i16 g_fieldBattleActive;

DATA(0x000784f4)
static i16 s_fieldMap;

DATA(0x000784f8)
static i16 s_fieldEntryState;

DATA(0x000784fc)
static b16 s_fieldMarker;

DATA(0x00078500)
static i16 s_fieldPairFirst;

DATA(0x00078504)
static i16 s_fieldPairSecond;

DATA(0x00078508)
static i16 s_fieldParamThird;

// Set when the map was left by abort or a proximity event; feeds script register 0.
DATA(0x0007850c)
static b16 s_fieldLeftEarly;

DATA(0x00078510)
static b16 s_fieldRefresh;

// The palette snapshot held while an encounter runs.
DATA(0x00078518)
static PaletteState* s_fieldPaletteState;

RVA(0x000070d0, 0xa)
void MarkFieldRefresh(void) {
    s_fieldRefresh = true;
}

RVA(0x000070e0, 0x12)
i16 ExchangeFieldOption(i16 option) {
    i16 old = s_fieldOption;
    s_fieldOption = option;
    return old;
}

RVA(0x00007100, 0x2a)
i16 SetFieldParams(i16 first, i16 second, i16 third) {
    i16 old = s_fieldParamFirst;
    s_fieldParamFirst = first;
    s_fieldParamSecond = second;
    s_fieldParamThird = third;
    return old;
}

RVA(0x00007130, 0x23)
b16 IsFieldModeAtLeast(i16 anyMode) {
    if (anyMode == 0) {
        return s_fieldMode >= 1;
    }
    return s_fieldMode >= 0;
}

RVA(0x00007160, 0x7)
i16 GetFieldMarker(void) {
    return s_fieldMarker;
}

RVA(0x00007170, 0x18)
void SetFieldPair(i16 first, i16 second) {
    s_fieldPairFirst = first;
    s_fieldPairSecond = second;
}

RVA(0x00007190, 0x59)
void EnterFieldMap(i16 map, i16 countA, i16 rateA, i16 countB, i16 rateB, i16 mode) {
    s_fieldMap = map;
    s_fieldCountA = countA;
    s_fieldRateA = rateA;
    s_fieldCountB = countB;
    s_fieldRateB = rateB;
    s_fieldMode = mode;
    s_fieldEntryState = 0;
    PushGameState(0xb);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000071f0, 0x7)
i16 GetFieldMap(void) {
    return s_fieldMap;
}

RVA(0x00007200, 0x7)
i16 GetFieldEntryState(void) {
    return s_fieldEntryState;
}

RVA(0x00007210, 0x18)
void SetFieldCounts(i16 countA, i16 countB) {
    s_fieldCountA = countA;
    s_fieldCountB = countB;
}

// Counts the side's countdown down (unless held); returns 1 when inactive,
// -1 when the count is 0, else the count, with the exhausted marker -2 read
// back as 0.
RVA(0x00007230, 0x66)
i16 TickFieldCount(i16 side, i16 hold) {
    i16* count;
    if (s_fieldMode == -1) {
        return 1;
    }
    count = side < 0 ? &s_fieldCountA : &s_fieldCountB;
    if (*count == -1) {
        return 1;
    }
    if (*count == 0) {
        return -1;
    }
    if (hold == 0 && *count > 0) {
        if (--*count == 0) {
            *count = -2;
        }
    }
    if (*count == -2) {
        return 0;
    }
    return *count;
}

// Scales value by the rate of the side the operand signs select (A when the
// first is negative and the second not, B for the reverse).
RVA(0x000072a0, 0x60)
i32 ScaleByFieldRate(i16 first, i16 second, i32 value) {
    double scaled;
    if (s_fieldMode == -1) {
        return value;
    }
    scaled = value;
    if (first < 0 && second >= 0) {
        scaled *= s_fieldRateA;
    } else if (first >= 0 && second < 0) {
        scaled *= s_fieldRateB;
    } else {
        return value;
    }
    return scaled * 0.01;
}

RVA(0x00007300, 0x33)
void SpawnSecondGroupActor(i16 x, i16 y, i16 battle) {
    SpawnFieldObject(
        1,
        x,
        y,
        OppositeDirection(g_field.pos.direction),
        s_fieldParamSecond,
        battle,
        -1,
        0
    );
}

RVA(0x00007340, 0x42)
i16 GetFacingWall(i16 map) {
    i16 width;
    i16 height;
    GetMapSize(&width, &height);
    return GetWallAt(g_field.pos.x, g_field.pos.y, g_field.pos.direction, width, height);
}

// Runs one frame of a field encounter, by phase: 0 enters it (step 0 spawns the
// enemy groups at the party's position and starts the music), 1 runs the turns
// until the party or the enemies are beaten, 2 backs out, 3 grants the
// rewards, 4 shows the level-ups, 5 the analyze window and 6 tears it down.
// @identity-TODO: named from its phases (enemy spawns, turns, rewards,
// level-ups); the caller 0x417160 dispatches it as a game state.
RVA(0x00007390, 0x6b0)
b16 RunFieldEncounter(void) {
    i16 x;
    i16 y;
    i16 i;
    i16 allFallen;
    Character* actor;

    if (!(GetGameStep() | GetGamePhase())) {
        SaveScreenMode();
    }
    RefreshScreenMode();
    switch (GetGamePhase()) {
        case 0:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                    LockStatusRedraw(0);
                    SetFieldMenuMode(1);
                    s_fieldMarker = true;
                    g_fieldBattleActive = 1;
                    ResetFieldObjects();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    SaveFieldLayer(0);
                    SaveFieldLayer(1);
                    NotifyEncounterStart();
                    if (s_fieldMode == 0) {
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                0,
                                g_field.pos.x,
                                g_field.pos.y,
                                (g_field.pos.direction - 2) & 3,
                                s_fieldMap,
                                1,
                                -1,
                                0
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(g_field.pos.x, g_field.pos.y, 1);
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0x15, 1);
                        } else {
                            s_fieldMusic = PlayMusic(0xd, 1);
                        }
                    } else {
                        x = g_field.pos.x;
                        y = g_field.pos.y;
                        if (!GetFacingWall(s_fieldMap)) {
                            OffsetMapCoord(&x, &y, g_field.pos.direction, 0, -1);
                        }
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                0,
                                x,
                                y,
                                (g_field.pos.direction - 2) & 3,
                                s_fieldMap,
                                1,
                                -1,
                                0
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(x, y, 1);
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0xd, 1);
                        } else {
                            s_fieldMusic = PlayMusic(s_fieldOption, 1);
                        }
                    }
                    LoadEnemyGroupSlot(0, s_fieldMap);
                    if (s_fieldParamSecond >= 0) {
                        LoadEnemyGroupSlot(1, s_fieldParamSecond);
                    }
                    RequestFieldRefresh();
                    for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                        actor = GetFieldActor(i);
                        AlertActor(actor, 2);
                    }
                    break;
                case 1:
                    NextGamePhase();
                    ResetPartyTurnState();
                    RedrawFieldView();
                    break;
            }
            break;
        case 1:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims()) {
                AllowImmediateInput();
            }
            allFallen = 1;
            for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                allFallen &= GetFatalCondition(GetCharacterConditions(GetFieldActor(i)));
            }
            if (s_fieldMode >= 0 && CountFieldObjects() <= 0) {
                LeaveFieldMap(1);
                break;
            }
            if (s_fieldMode == 1 && allFallen) {
                LeaveFieldMap(1);
                break;
            }
            if (FindFirstAblePartyMember() == -1) {
                LeaveFieldMap(-1);
                break;
            }
            if (!TickFieldCount(-1, 1)) {
                LeaveFieldMap(0);
                break;
            }
            if (!TickFieldCount(1, 1)) {
                LeaveFieldMap(0);
                break;
            }
            if (!GetPickMode() && PickAnalyzeTarget() >= 0) {
                SetGamePhase(5);
                break;
            }
            SetFieldBusy(0);
            if (RunPartyTurn(g_tickElapsed)) {
                break;
            }
            if (TickFieldCount(0, 1) <= 0) {
                break;
            }
            for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                actor = GetFieldActor(i);
                if (actor != NULL) {
                    actor->fieldState = 6;
                }
            }
            RunFieldIdle();
            break;
        case 2:
            PrevGamePhase();
            break;
        case 3:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState <= 0) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, 1);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst != 0 || s_fieldPairSecond != 0) {
                ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, 1);
            }
            break;
        case 4:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(0x1b);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(2, 0xffff, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(6);
            break;
        case 5:
            if (RunAnalyzeWindow()) {
                SetGamePhase(1);
            }
            break;
        case 6:
            PlaySoundEffect(0x1b);
            RestoreScreenMode();
            ClearSelectedHotspot();
            ResetFieldObjects();
            ResetFieldLayer(1);
            ResetFieldLayer(0);
            CloseMessageWindow();
            NotifyEncounterEnd();
            RestoreFieldLayer(1);
            RestoreFieldLayer(0);
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, 1);
            if (s_fieldMode == 0) {
                RespawnAreaActors();
            }
            RequestFieldRefresh();
            s_fieldRefresh = false;
            ReturnFromGameState();
            s_fieldCountA = -1;
            s_fieldRateA = 100;
            s_fieldCountB = -1;
            s_fieldRateB = 100;
            s_fieldMode = -1;
            ResetRosterBattleState();
            SetFieldMenuMode(0);
            s_fieldMarker = false;
            s_fieldOption = 20;
            s_fieldParamFirst = 1;
            s_fieldParamSecond = -1;
            s_fieldParamThird = 0;
            break;
    }
    return FlushFieldScreen();
}

// Records how the field map ended and advances the owning state two phases.
RVA(0x00007a40, 0x23)
void LeaveFieldMap(i16 result) {
    g_fieldBattleActive = 0;
    s_fieldEntryState = result;
    CloseFieldWindows();
    NextGamePhase();
    NextGamePhase();
}

// Clears every roster member's field marks.
RVA(0x00007a70, 0x2a)
void ResetRosterFieldMarks(void) {
    Character* character;
    i16 slot;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character != NULL) {
            ClearActionWait(GetCharacterActionWait(character));
        }
    }
}

// The field state's per-frame handler, one case per phase.
RVA(0x00007aa0, 0x4d4)
b16 RunFieldState(void) {
    i16 key;
    SetFieldRenderMode();
    SetInfoBarLayout(0);
    switch ((u16)GetGamePhase()) {
        case 0:
            switch ((u16)GetGameStep()) {
                case 0:
                    ClearSceneSurfaces();
                    NextGameStep();
                    s_fieldLeftEarly = false;
                    SetFieldStatusBit0(0);
                    SetFieldStatusBit11(0);
                    SetFieldMenuMode(3);
                    g_fieldBattleActive = 1;
                    ResetFieldScene();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    ResetFieldObjects();
                    LoadFieldTable();
                    PrepareFieldRandom();
                    s_fieldMusic = PlayMusic(13, 1);
                    ResetRosterFieldMarks();
                    RequestFieldRefresh();
                    return FlushFieldScreen();
                case 1:
                    NextGamePhase();
                    StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
                    break;
            }
            break;
        case 1:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims() && !GetPickMode()) {
                AllowImmediateInput();
            }
            key = CountFieldObjects();
            if (key <= 0) {
                LeaveFieldMap(0);
                if (key >= 0) {
                    break;
                }
                s_fieldLeftEarly = true;
                return FlushFieldScreen();
            }
            if (FindFirstAblePartyMember() == -1) {
                LeaveFieldMap(-1);
                return FlushFieldScreen();
            }
            if (!GetPickMode()) {
                if (PickAnalyzeTarget() >= 0) {
                    SetGamePhase(5);
                    return FlushFieldScreen();
                }
                if (g_pendingTalk) {
                    RunPendingTalk();
                    return FlushFieldScreen();
                }
            }
            SetFieldBusy(0);
            if (!RunPartyTurn(g_tickElapsed)) {
                RunFieldIdle();
            }
            UpdateFieldObjects();
            if (GetFieldBusy()) {
                break;
            }
            if (!GetEncounterPending()) {
                break;
            }
            HideScreenLayer(1);
            if (RollProximityEvent() > 0) {
                LeaveFieldMap(0);
                s_fieldLeftEarly = true;
                RunMessageScene(0x7f04, 0x10, -1);
                PlaySoundEffect(4);
                ClearEncounterPending();
                return UpdateFieldScreen(0);
            }
            NextGamePhase();
            RunMessageScene(0x7f04, 0x11, -1);
            PlaySoundEffect(3);
            PushWaitState(WAIT_FRAMES, 0x3c, 0x3c, 0);
            ClearEncounterPending();
            return UpdateFieldScreen(0);
        case 2:
            SetFieldStatusBit0(0);
            CloseMessageWindow();
            RestoreDrawState(SaveDrawState());
            PrevGamePhase();
            return UpdateFieldScreen(0);
        case 3:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState < 0) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, 1);
            AccessScriptReg(1, 0, 1 - s_fieldLeftEarly);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst == 0 && s_fieldPairSecond == 0) {
                break;
            }
            ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, 1);
            return FlushFieldScreen();
        case 4:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(0x1b);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(WAIT_INPUT_OR_FRAMES, -1, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(6);
            return FlushFieldScreen();
        case 5:
            if (RunAnalyzeWindow()) {
                SetGamePhase(1);
                return FlushFieldScreen();
            }
            break;
        case 6:
            PlaySoundEffect(0x1b);
            CloseMessageWindow();
            ResetFieldObjects();
            ResetFieldLayer(1);
            ResetFieldLayer(0);
            RequestFieldRefresh();
            ReturnFromGameState();
            SetFieldStatusBit11(1);
            ResetRosterBattleState();
            SetFieldMenuMode(2);
            ReleaseFieldImage();
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, 1);
            StartScreenFadeAndWait(SCREEN_FADE_TO_BLACK, 1);
            break;
    }
    return FlushFieldScreen();
}

RVA(0x00007f80, 0xcf)
b16 RollProximityEvent(void) {
    i16 nearest = 0x7fff;
    i16 object = -1;
    i16 index;
    i16 distance;
    MapCoord pos;
    for (index = 15; index >= 0; index--) {
        if (GetLiveObject(index) >= 0) {
            pos = GetObjectCoord(index);
            distance = GridDistance(g_field.pos.x, g_field.pos.y, pos.x, pos.y);
            if (distance <= nearest) {
                nearest = distance;
                object = index;
            }
        }
    }
    if (object < 0) {
        return true;
    }
    switch (nearest) {
        case 0:
            index = 0x7fff;
            break;
        case 1:
            index = RandomAverage(20, 40, 0);
            break;
        case 2:
            index = RandomAverage(12, 22, 0);
            break;
        default:
            index = RandomAverage(5, 15, 0);
            break;
    }
    distance = GetStatTotal(GetRosterLeader(), STAT_FORTUNE);
    return distance >= index;
}

RVA(0x00008050, 0x10)
void UpdatePartyActionWaits(void) {
    if (g_tickElapsed) {
        TickPartyActionWaits();
    }
}

RVA(0x00008060, 0xd5)
b16 HasObjectInReach(i16 mode, i16 first, i16 second) {
    MapCoord pos = GetMapCoord();
    FieldObject* object;
    switch (mode) {
        case 0:
            if (first >= 0) {
                object = GetFieldObject(first);
                if (pos.x != object->pos.x || pos.y == object->pos.y) {
                    return false;
                }
            } else if (!CountObjectsAt(pos.x, pos.y, 0, 0)) {
                return false;
            }
            break;
        case 1:
            if (first >= 0 && second >= 0) {
                return false;
            }
            if (first < 0 && second < 0) {
                break;
            }
            if (first >= 0) {
                object = GetFieldObject(first);
            } else {
                object = GetFieldObject(second);
            }
            if (pos.x != object->pos.x || pos.y != object->pos.y) {
                return false;
            }
            break;
        default:
            return false;
    }
    return true;
}

RVA(0x00008140, 0x2b)
b32 IsPartyAt(i32 x, i32 y) {
    MapCoord pos = GetMapCoord();
    return pos.x == x && pos.y == y;
}

RVA(0x00008170, 0x35)
i16 FindFirstAblePartyMember(void) {
    i16 index;
    Character* member;
    for (index = 0; index < 6; index++) {
        member = GetPartyEntry(index);
        if (member && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return -1;
}

RVA(0x000081b0, 0x4b)
i16 FindAbleHumanMember(void) {
    i16 index;
    Character* member;
    for (index = 0; index < 6; index++) {
        member = GetPartyCharacter(index);
        if (member && (member->id == 38 || member->id == 399 || IsHumanCharacter(member))
            && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return -1;
}

RVA(0x00008200, 0xbc)
void TickPartyConditionActions(void) {
    i16 index;
    i16 action;
    Character* actor;
    for (index = 0; index < 6; index++) {
        actor = GetPartyCharacter(index);
        if (actor && !GetPickState(actor) && !IsActionWaitMarked(GetCharacterActionWait(actor))) {
            if (!GetActionCondition(actor)) {
                actor->conditionActionTicks = 0;
            } else {
                actor->conditionActionTicks++;
                if (actor->conditionActionTicks >= 36) {
                    actor->conditionActionTicks = 0;
                    g_actorId = PartyCombatantId(index);
                    action = PickActorAction(actor);
                    if (action >= 1) {
                        if ((action & 15) == 4) {
                            action = (action & 0xf0) | 1;
                        }
                        action = AdjustActorAction(PartyCombatantId(index), action);
                        if (action != 0) {
                            ChangeCharacterFlag(actor, 32, 1);
                            MarkActorActionReady(actor);
                        }
                        return;
                    }
                }
            }
        }
    }
}

RVA(0x000082c0, 0x67)
void MarkActorActionReady(Character* actor) {
    GetCharacterActionWait(actor)->ready = 1;
    switch (actor->mode) {
        case 1:
            MarkPickDone();
            break;
        case 2:
            if (!IsHumanCharacter(actor)) {
                actor->pickRole = 7;
                MarkPickDone();
                break;
            }
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            actor->pickRole = 8;
            MarkPickDone();
            break;
    }
}

RVA(0x00008330, 0x11e)
i16 RunPartyTurn(i16 ticks) {
    i16 index;
    i16 id;
    i16 action;
    Character* actor;
    if (!RunPartyCommandInput()) {
        return -1;
    }
    TickPartyConditionActions();
    UpdatePartyActionWaits();
    index = FindReadyMember(1);
    if (index < 0) {
        return 0;
    }
    actor = GetPartyCharacter(index);
    id = PartyCombatantId(index);
    actor->conditionActionTicks = 0;
    g_actorId = id;
    ApplyEquipmentEffects(actor, EQUIP_EFFECT_ACTION);
    if (!TestCharacterFlag(actor, 32)) {
        action = PickActorAction(actor);
        if (action > 0 && AdjustActorAction(id, action) > 0) {
            MarkActorActionReady(actor);
        }
    }
    ChangeCharacterFlag(actor, 32, 0);
    if (PushPromptState(0, 0, 200, 450, 0)) {
        return 0;
    }
    g_actionId = 1;
    g_actorId = id;
    g_targetId = actor->pickObject;
    ResetActionWait(GetCharacterActionWait(actor));
    CheckPickTarget(index);
    g_tickElapsed = 0;
    return index + 1;
}

DATA(0x00078488)
static i16 s_gunPower[16];
DATA(0x000784a8)
static i16 s_gunRounds[16];
DATA(0x000784d8)
i16 g_attackResistance = 0;
DATA(0x000784dc)
i16 g_attackAttribute = 0;
DATA(0x000784e0)
i16 g_attackCondition = 0;
// @identity-TODO: the penalties use total stats 8 and 6 respectively;
// the stat names are not yet recovered.
DATA(0x000784e4)
static i16 s_gunPenaltyA;
DATA(0x000784e8)
static i16 s_gunPenaltyB;
DATA(0x000784ec)
static i16 s_gunRoundPower;
DATA(0x000784f0)
static i16 s_gunBasePower;
DATA(0x0007851c)
static i32 s_gunDistribution;

RVA(0x00008450, 0x2f)
i16 GetCombatantSideRelation(void) {
    if (g_targetId < 0 && g_actorId < 0) {
        return -1;
    }
    if (g_targetId >= 0 && g_actorId >= 0) {
        return 1;
    }
    return 0;
}

static __inline void AddArmorSlotHitModifier(ItemSlot* slot, i16* modifier) {
    if (slot->item != -1) {
        *modifier += GetArmorHitModifier(GetLoadedRecord(slot->item));
    }
}

RVA(0x00008480, 0x11b)
i16 GetEquipmentHitModifier(Character* attacker, Character* target) {
    i16 modifier = 0;
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[0], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[1], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[2], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[3], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[4], &modifier);
    modifier = -modifier;
    if (attacker->pickTarget >= 1) {
        modifier += GetLoadedRecord(attacker->pickTarget)->params[0x1c];
    }
    return modifier;
}

static __inline i16 GetCombatantFacing(i16 id) {
    if (id < 0) {
        return g_field.pos.direction;
    }
    return GetFieldActor(id)->facing;
}

RVA(0x000085a0, 0x65)
i16 GetCombatantFacingDifference(i16 first, i16 second) {
    i16 direction;
    i16 facingDifference;
    if (first < 0 && second < 0) {
        return 0;
    }
    direction = GetCombatantFacing(first);
    facingDifference = OppositeDirection(GetCombatantFacing(second) - direction);
    return facingDifference;
}

RVA(0x00008610, 0x59)
i16 GetCombatantDistance(i16 first, i16 second) {
    MapCoord from;
    MapCoord to;
    from = GetFieldTargetCoord(first);
    to = GetFieldTargetCoord(second);
    return GridDistance(from.x, from.y, to.x, to.y);
}

RVA(0x00008670, 0x51)
i16 GetCombatantAttackRange(i16 id) {
    Character* actor = GetCombatant(id);
    if (actor->pickTarget < 1) {
        return 1;
    }
    if (!actor->pickTargetHigh) {
        return GetItemAttackRange(GetLoadedRecord(actor->pickTarget));
    }
    return GetSkillAttackRange(actor->pickTarget);
}

RVA(0x000086d0, 0x27)
i16 GetAttackRangeExcess(i16 first, i16 second) {
    i16 distance = GetCombatantDistance(first, second);
    return distance - GetCombatantAttackRange(first);
}

RVA(0x00008700, 0x209)
i16 RollExceptionalAttack(Character* attacker, Character* target, i16 mode, i16 resistance) {
    i32 phase = (g_clock.moonPhase + 13) % 14 + 1;
    i16 modifier;
    i32 attack;
    i32 defense;
    double value;
    if (phase * phase / 4 > RandomUpTo(255) && resistance != 0 && resistance != -6) {
        modifier = GetEquipmentHitModifier(attacker, target);
        if (!mode) {
            value = GetExceptionalAttackLuck(attacker);
            value *= RandomAverage(80, 120, 0);
            value *= 0.01;
            value += modifier;
            attack = RoundToInt(value);
            value = GetExceptionalAttackLuck(target);
            defense = RoundToInt(value * RandomAverage(100, 200, 0) * 0.01);
            if (attack > defense) {
                SetActionResult(attacker, 5);
                return 5;
            }
        }
        attack = GetExceptionalAttackBase(attacker);
        attack += RandomUpTo(7);
        defense = GetExceptionalAttackBase(target);
        defense += RandomUpTo(31);
        if (modifier + attack > defense) {
            SetActionResult(attacker, 4);
            return 4;
        }
        SetActionResult(attacker, 0);
    }
    return 0;
}

RVA(0x00008910, 0x207)
b16 RollGunHit(Character* attacker, Character* target, i16 resistance) {
    i32 accuracy;
    i32 evasion;
    i32 attack;
    i32 defense;
    i32 roll;
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        SetActionResult(attacker, 3);
        return true;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == 2) {
        SetActionResult(attacker, 3);
        return true;
    }
    accuracy = GetBattleStatShown(attacker, BATTLE_STAT_GUN_ACCURACY);
    ApplyAttackAccuracyConditions(attacker, accuracy);
    evasion = GetBattleStatShown(target, BATTLE_STAT_GUN_EVASION);
    if (HasCondition(GetCharacterConditions(target), 19)) {
        evasion *= 2;
    }
    if (GetAttackRangeExcess(g_actorId, g_targetId) < 0) {
        attack = accuracy * 50;
    } else {
        attack = accuracy * 100;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        attack *= 2;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) != 0) {
        defense = evasion * 75;
    } else {
        defense = evasion * 100;
    }
    if (attack >= defense) {
        roll = defense * RandomAverage(-2, 12, 1);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, 3);
            return true;
        }
    } else {
        roll = defense * RandomAverage(0, 15, 0);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, 3);
            return true;
        }
    }
    roll = defense * RandomAverage(0, 7, 0);
    if (attack >= roll) {
        SetActionResult(attacker, 2);
        return true;
    }
    SetActionResult(attacker, 0);
    return false;
}

RVA(0x00008b20, 0x17)
i16 GetGunAttackPower(Character* attacker) {
    if (s_gunPower[0]) {
        return s_gunPower[0];
    }
    return GetBattleStatShown(attacker, BATTLE_STAT_GUN_POWER);
}

RVA(0x00008b40, 0x1e9)
i32 ComputeGunDamage(Character* attacker, Character* target, i16 result) {
    i16 power;
    i16 defense;
    double ratio;
    double amount;
    i32 facing;
    i32 damage;
    if (!result) {
        return 0;
    }
    power = GetGunAttackPower(attacker);
    defense = GetBattleStatShown(target, BATTLE_STAT_GUN_DEFENSE);
    amount = defense;
    amount *= 0.2;
    amount = -amount;
    amount += power;
    if (amount < 0.0) {
        amount = 0.0;
    }
    ratio = power;
    if (defense) {
        ratio /= defense;
    }
    if (power >= defense) {
        ratio += 2.2;
    } else {
        ratio += 1.0;
    }
    amount = sqrt(amount) * ratio;
    if (result == 4) {
        amount += attacker->level + 5;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        amount *= 1.2;
    }
    facing = GetCombatantFacingDifference(g_actorId, g_targetId);
    if (facing == 2) {
        amount *= 1.5;
    } else if (facing != 0) {
        amount *= 1.2;
    }
    if (result == 2) {
        amount *= 0.25;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        amount *= 1.5;
    }
    damage = RoundToInt(amount * 100.0);
    damage = ScaleActionValue(damage, g_attackResistance, 2);
    damage = ScaleByMoonValue(damage, attacker->moonRow, 2);
    damage = RandomPercent(damage, -20, 20);
    damage = ClampInt(damage / 100, 0, 0x7fffffff);
    if (damage == 0) {
        SetActionResult(attacker, 1);
    }
    return damage;
}

RVA(0x00008d30, 0x124)
b16 RollGunCondition(Character* attacker, Character* target, i16 resistance, i16 condition) {
    i16 luck;
    i16 roll;
    i16 defense;
    g_statusCondition = 0;
    if (!condition) {
        return false;
    }
    if (attacker->lastChange < GetConditionDamageThreshold(target)) {
        return false;
    }
    if (g_actionResult >= 7) {
        return false;
    }
    if (g_targetId >= 0 && IsFieldModeAtLeast(0) && IsFieldConditionRestricted(condition)) {
        return false;
    }
    roll = RandomAverage(0, 20, 0);
    luck = GetStatTotal(attacker, STAT_FORTUNE);
    luck += roll;
    if (luck <= GetStatTotal(target, STAT_FORTUNE)) {
        return false;
    }
    roll = RandomAverage(0, 40, 0);
    defense = GetBattleStatShown(target, 11);
    defense *= roll;
    if (ScaleActionValue(GetBattleStatShown(attacker, 9) * 10, resistance, 2) - defense <= 0) {
        return false;
    }
    if (IsConditionResisted(target, g_attackCondition)) {
        return false;
    }
    g_statusCondition = g_attackCondition;
    InflictCondition(g_attackCondition, target);
    return true;
}

RVA(0x00008e60, 0x176)
b16 ResolveGunAttack(Character* attacker, Character* target, i16 mode) {
    i16 result;
    i32 amount = 0;
    ResetActionOutcome();
    g_hpChange = 0;
    g_attackAttribute = GetPickedAttackAttribute(attacker, &g_attackCondition);
    g_attackResistance = GetActionResistance(target, g_attackAttribute, 2, 1, 0);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, g_attackAttribute);
    result = RollExceptionalAttack(attacker, target, mode, g_attackResistance);
    if (result != 0) {
        AddTrainingPoints(attacker, 1, 1);
    }
    if (result < 5) {
        if (result == 0) {
            result = RollGunHit(attacker, target, g_attackResistance);
            attacker->resultFlag = result;
            if (result != 0) {
                AddTrainingPoints(attacker, 1, 1);
            }
        } else {
            SetCharacterResult(attacker, result, 1);
        }
        amount = ComputeGunDamage(attacker, target, attacker->result);
        SetCharacterChanges(attacker, amount, 0);
    } else if (result == 5) {
        amount = 0x7fff;
        SetCharacterChanges(attacker, amount, 0);
        SetFlaggedActionResult(attacker, 5);
        AddTrainingPoints(attacker, 1, 1);
    }
    ApplyResistanceOutcome(attacker, g_attackResistance, amount);
    return RollGunCondition(attacker, target, g_attackResistance, g_attackCondition);
}

RVA(0x00008fe0, 0x2a)
void LoadGunDistributionTable(void) {
    FILE* fp = OpenDataFile(18, 12, 0);
    s_gunDistribution = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00009010, 0x38)
i16 FilterGunTargets(Character* attacker, i16 count) {
    i16 rounds = GetGunBurstRounds(attacker);
    if (rounds < 1) {
        return 0;
    }
    return DistributeGunRounds(rounds, PrepareGunBurst(attacker, count));
}

RVA(0x00009050, 0x62)
i16 GetGunBurstRounds(Character* attacker) {
    i16 rounds;
    i16 limit;
    if (GetCharacterEquipment(attacker)[6].item < 1) {
        return 0;
    }
    if (GetCharacterEquipment(attacker)[7].item < 1) {
        return 0;
    }
    rounds = GetCharacterEquipment(attacker)[7].quantity;
    if (g_actorId >= 0) {
        rounds = 255;
    }
    limit = GetGunBurstLimit(GetLoadedRecord(GetCharacterEquipment(attacker)[6].item));
    if (limit > rounds) {
        limit = rounds;
    }
    return limit;
}

RVA(0x000090c0, 0xe7)
i16 PrepareGunBurst(Character* attacker, i16 count) {
    ItemRecord* record = GetLoadedRecord(GetCharacterEquipment(attacker)[6].item);
    u8 limits;
    i16 minimum;
    i16 maximum;
    s_gunPenaltyA = GetGunRequirementPenalty(
        GetStatTotal(attacker, STAT_DEXTERITY),
        GetItemRequiredDexterity(record)
    );
    s_gunPenaltyB = GetGunRequirementPenalty(
        GetStatTotal(attacker, STAT_VITALITY),
        GetItemRequiredVitality(record)
    );
    s_gunBasePower = GetItemAttackPower(record);
    limits = GetGunTargetLimits(record);
    s_gunRoundPower = GetItemAttackPower(GetLoadedRecord(GetCharacterEquipment(attacker)[7].item));
    maximum = limits & 15;
    minimum = limits >> 4;
    if (minimum < 1) {
        minimum = 1;
    } else if (minimum > 15) {
        minimum = 15;
    }
    if (maximum < minimum) {
        maximum = minimum;
    } else if (maximum > 15) {
        maximum = 15;
    }
    if (count < minimum) {
        return minimum;
    }
    if (count > maximum) {
        return maximum;
    }
    return count;
}

RVA(0x000091b0, 0x32)
i16 GetGunRequirementPenalty(i16 stat, i16 requirement) {
    if (requirement < 1) {
        requirement = 1;
    }
    return ClampShort(5 - stat / requirement, 1, 0x7fff);
}

static __inline u8 GetGunRoundPercent(u8 (*table)[15], i16 count, i16 index) {
    return table[count - 1][index];
}

RVA(0x000091f0, 0xe2)
i16 DistributeGunRounds(i16 rounds, i16 count) {
    u8(*table)[15];
    i16 index;
    i16 remaining;
    i16 share;
    memset(s_gunRounds, 0, sizeof(s_gunRounds));
    memset(s_gunPower, 0, sizeof(s_gunPower));
    table = HandleReadPtr(s_gunDistribution);
    remaining = rounds;
    for (index = 0; index < count; index++) {
        if (rounds * GetGunRoundPercent(table, count, index) == 0 || remaining < 1) {
            break;
        }
        share = rounds * GetGunRoundPercent(table, count, index);
        share /= 100;
        if (share < 1) {
            share = 1;
        }
        if (share > remaining) {
            share = remaining;
        }
        s_gunRounds[index] = share;
        s_gunPower[index] = ComputeGunBurstPower(share);
        remaining -= share;
    }
    return index;
}

RVA(0x000092e0, 0x3e)
i16 ComputeGunBurstPower(i16 rounds) {
    i16 penalty = s_gunPenaltyB;
    i16 power = s_gunRoundPower;
    i16 total = 0;
    i16 index;
    penalty += s_gunPenaltyA;
    for (index = 0; index < rounds; index++) {
        total += power;
        power -= penalty;
        power = max(1, power);
    }
    return total + s_gunBasePower;
}

RVA(0x00009320, 0x85)
void SpendGunRounds(Character* attacker) {
    i16 rounds;
    i16 index;
    if (g_actorId < 0) {
        rounds = s_gunRounds[0];
        if (rounds > GetCharacterEquipment(attacker)[7].quantity) {
            rounds = GetCharacterEquipment(attacker)[7].quantity;
        }
        GetCharacterEquipment(attacker)[7].quantity -= rounds;
        if (GetCharacterEquipment(attacker)[7].quantity <= 0) {
            ClearItemSlot(&GetCharacterEquipment(attacker)[7]);
        }
    }
    for (index = 0; index < 15; index++) {
        s_gunRounds[index] = s_gunRounds[index + 1];
        s_gunPower[index] = s_gunPower[index + 1];
    }
    s_gunPower[15] = 0;
    s_gunRounds[15] = 0;
}

RVA(0x000093b0, 0x28)
void SpendAllGunRounds(Character* attacker) {
    if (attacker) {
        while (s_gunRounds[0]) {
            SpendGunRounds(attacker);
        }
    }
}

DATA(0x000784c8)
i16 g_commandPosition = 0;

DATA(0x000784cc)
static b16 s_pickScreenSaved;

DATA(0x000784d0)
static i16 s_pickMode;

DATA(0x000784d4)
static b16 s_pickDone;

// The list menu a picked member acts through (NULL: none open).
DATA(0x00078520)
static MenuBox* s_pickMenu;

DATA(0x00078528)
u8 g_pickScreenSave[16] = {0};

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
    s_pickDone = false;
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
                    s_pickDone = true;
                }
            } else if (PickPartyMember(index) == 1) {
                s_pickDone = true;
            }
    }
    return s_pickDone;
}

RVA(0x000094b0, 0xa)
void MarkPickDone(void) {
    s_pickDone = true;
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

static __inline i16 CurrentMemberCombatantId(void) {
    return PartyCombatantId(FindPartyPositionOfId(s_pickedIndex));
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
    b16 reach = false;
    i16 flags = -1;
    switch (s_pickMode) {
        case 0:
            if (IsPanelLayerVisible()) {
                g_tickElapsed = 0;
            }
            s_pickDone = false;
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
                    result = CurrentMemberCombatantId();
                    goto target_selected;
                } else if (TargetFlagsSelectActorGroup(flags)) {
                    result = CurrentMemberCombatantId();
                    goto target_selected;
                } else if (flags & TARGET_ACTOR_SIDE) {
                    reach = true;
                }
            } else if (character->pickRole == 5) {
                flags = GetItemTargetFlags(GetLoadedRecord(character->pickTarget));
                if ((TargetFlagsSelectSelf(flags)) || TargetFlagsSelectActorGroup(flags)) {
                    result = CurrentMemberCombatantId();
                    goto target_selected;
                }
                if (character->pickTarget == 0x71) {
                    flags = TARGET_ACTOR_SIDE;
                }
                if (flags & TARGET_ACTOR_SIDE) {
                    reach = true;
                }
            }
            if (flags == 0x10 || flags == 0x11 || flags == 0x30) {
                reach = true;
            }
            if (reach == 0 && HasObjectInReach(0, -1, 0)) {
                result = FindObjectAtParty();
            target_selected:
                character->pickObject = result;
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
            g_commandPosition = g_hoveredObjectId;
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
                s_pickScreenSaved = false;
            }
            return -1;
        }
        if (!s_pickScreenSaved) {
            AllocScreenSave(g_pickScreenSave);
            CaptureScreenSaveWithState(g_pickScreenSave);
            s_pickScreenSaved = true;
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
            s_pickScreenSaved = false;
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
