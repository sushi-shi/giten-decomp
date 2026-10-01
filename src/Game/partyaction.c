// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of partyaction, field, attack and partypick interleave in a single
// run, and their initialized data and string literals form one .data run. The
// unreconstructed field-map routine and field state handler (about forty
// unclaimed callees) use the field statics and belong to it.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
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
#include <Game/EquipSlotIndex.h>
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
#include <Game/ItemId.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ObjectRecordId.h>
#include <Game/Party.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/PartyStatus.h>
#include <Game/Skill.h>
#include <Game/SkillId.h>
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
#include <stdlib.h>
#include <string.h>

// Distribution rows cover bursts of one through fifteen targets. The extra
// live slot lets SpendGunRounds shift the last row forward before clearing it.
#define GUN_BURST_MAX_TARGETS 15
#define GUN_BURST_SLOT_COUNT (GUN_BURST_MAX_TARGETS + 1)
#define GUN_BURST_LIMIT_SHIFT 4
#define GUN_BURST_LIMIT_MASK 0x0f

DATA(0x00078488)
static i16 s_gunPower[GUN_BURST_SLOT_COUNT] = {0};

DATA(0x000784a8)
static i16 s_gunRounds[GUN_BURST_SLOT_COUNT] = {0};

DATA(0x000784c8)
i16 g_commandPosition = 0;

DATA(0x000784cc)
static b16 s_pickScreenSaved = false;

DATA(0x000784d0)
static GZ_ENUM_STORAGE(PartyCommandPhase, i16) s_pickMode = PARTY_COMMAND_WAIT_MEMBER;

DATA(0x000784d4)
static b16 s_pickDone = false;

DATA(0x000784d8)
i16 g_attackResistance = 0;

DATA(0x000784dc)
GZ_ENUM_STORAGE(AttackAttribute, i16) g_attackAttribute = ATTACK_ATTRIBUTE_SWORD;

DATA(0x000784e0)
i16 g_attackCondition = 0;

// @identity-TODO: the penalties use total stats 8 and 6 respectively;
// the stat names are not yet recovered.
DATA(0x000784e4)
static i16 s_gunPenaltyA = 0;

DATA(0x000784e8)
static i16 s_gunPenaltyB = 0;

DATA(0x000784ec)
static i16 s_gunRoundPower = 0;

DATA(0x000784f0)
static i16 s_gunBasePower = 0;

DATA(0x000784f4)
static i16 s_fieldMap = 0;

DATA(0x000784f8)
static GZ_ENUM_STORAGE(FieldMapOutcome, i16) s_fieldEntryState = FIELD_MAP_ENDED;

DATA(0x000784fc)
static b16 s_fieldMarker = false;

DATA(0x00078500)
static i16 s_fieldPairFirst = 0;

DATA(0x00078504)
static i16 s_fieldPairSecond = 0;

DATA(0x00078508)
static i16 s_fieldParamThird = 0;

// Set when the map was left by abort or a proximity event; feeds script register 0.
DATA(0x0007850c)
static b16 s_fieldLeftEarly = false;

DATA(0x00078510)
static b16 s_fieldRefresh = false;

DATA(0x00078514)
static MenuBox* s_commandMenu = NULL;

// The palette snapshot held while an encounter runs.
DATA(0x00078518)
static PaletteState* s_fieldPaletteState = NULL;

DATA(0x0007851c)
static i32 s_gunDistribution = HANDLE_NONE;

// The list menu a picked member acts through (NULL: none open).
DATA(0x00078520)
static MenuBox* s_pickMenu = NULL;

DATA(0x00078528)
u8 g_pickScreenSave[16] = {0};

DATA(0x00078538)
char g_unavailableCommandText[8] = {0};

// The temporary swap: the party position and the roster slot it held.
DATA(0x00068400)
i16 g_guestIndex = PARTY_POSITION_NONE;

DATA(0x00068404)
static i16 s_swapSaved = PARTY_SLOT_EMPTY;

// @identity-TODO: the party position the picker last targeted (-1: none).
DATA(0x00068408)
static i16 s_pickedIndex = CHARACTER_ID_NONE;

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
static GZ_ENUM_STORAGE(FieldMapMode, i16) s_fieldMode = FIELD_MAP_INACTIVE;

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
static i16 (*s_commandLabels[ACTOR_COMMAND_COUNT])(Character*) = {
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
static i16 s_actionConditions[] = {
    CONDITION_POSSESSION,
    CONDITION_CHARM,
    CONDITION_DRUNK,
    CONDITION_PANIC,
    CONDITION_CONFUSION,
    CONDITION_HALLUCINATION,
    CONDITION_HIGH,
    CONDITION_HAPPY,
    CONDITION_TIPSY,
    CONDITION_BERSERK,
    CONDITION_LIST_END
};

RVA(0x00005a80, 0x86)
GZ_ENUM_RETURN(PartyMemberPickResult, i16) PickPartyMember(i16 index) {
    Character* member;
    if (PartySlotAt(index) == PARTY_SLOT_EMPTY) {
        return PARTY_MEMBER_EMPTY;
    }
    member = GetPartyCharacter(index);
    if (!GetPickBlockingCondition(GetCharacterConditions(member))
        && TickFieldCount(PartyCombatantId(index), true) >= 1) {
        if (GetFieldBattleActive()) {
            if (IsActionWaitPickable(GetCharacterActionWait(member))) {
                return PARTY_MEMBER_READY;
            }
        } else if (!IsActionWaitMarked(GetCharacterActionWait(member))) {
            return PARTY_MEMBER_READY;
        }
    }
    return PARTY_MEMBER_UNAVAILABLE;
}

RVA(0x00005b10, 0x31)
i16 FindPickablePartyMember(i16 index) {
    for (; index < PARTY_SIZE; index++) {
        if (PickPartyMember(index) > PARTY_MEMBER_UNAVAILABLE) {
            return index;
        }
    }
    return PARTY_POSITION_NONE;
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

static __inline GZ_ENUM_RETURN(MemberPanelState, i16) GetReadyMemberPanelState(Character* member) {
    return IsCharacterHpLow(member) ? MEMBER_PANEL_LOW_HP : MEMBER_PANEL_READY;
}

RVA(0x00005b80, 0xdf)
GZ_ENUM_RETURN(MemberPanelState, i16) GetMemberPanelState(i16 index) {
    Character* member = g_panelMembers[index];
    GZ_ENUM_LOCAL(MemberPanelState, i16) state = MEMBER_PANEL_UNAVAILABLE;
    if (!member) {
        return MEMBER_PANEL_EMPTY;
    }
    if (GetFatalCondition(GetCharacterConditions(member))) {
        state = MEMBER_PANEL_FALLEN;
    } else if (GetPickBlockingCondition(GetCharacterConditions(member))) {
        state = MEMBER_PANEL_UNAVAILABLE;
    } else if (TickFieldCount(PartyCombatantId(index), true) < 1) {
        state = MEMBER_PANEL_UNAVAILABLE;
    } else if (IsActionWaitPickable(GetCharacterActionWait(member))) {
        state = GetReadyMemberPanelState(member);
    }
    if (!GetFieldBattleActive() && state == MEMBER_PANEL_UNAVAILABLE) {
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
    ClearFlagBank(EVENT_FLAG_BANK_SCRATCH);
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
    actor->mode = ACTOR_MODE_ATTACK;
    actor->pickRole = PICK_ROLE_ATTACK;
    SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item);
    actor->pickObject = target;
}

RVA(0x00005dc0, 0x83)
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomOpponentAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target;
    if (id < 0) {
        target = PickRandomCombatant(COMBATANT_SIDE_FIELD);
    } else {
        target = PickRandomCombatant(COMBATANT_SIDE_PARTY);
    }
    if (target == RANDOM_COMBATANT_NONE) {
        return ACTOR_ACTION_NONE;
    }
    SetBasicAttackPick(actor, target);
    return ACTOR_ACTION_ATTACK_QUEUED;
}

RVA(0x00005e50, 0xda)
i16 PickRandomCombatant(GZ_ENUM_PARAM(CombatantSide, u8) sides) {
    i16 targets[PARTY_SIZE + FIELD_OBJECT_COUNT];
    MapCoord pos;
    Character* member;
    i16 count = 0;
    i16 index;
    i16 object;
    if (sides & COMBATANT_SIDE_PARTY) {
        for (index = 0; index < PARTY_SIZE; index++) {
            member = GetPartyCharacter(index);
            if (member && !GetDisablingCondition(GetCharacterConditions(member))) {
                targets[count++] = PartyCombatantId(index);
            }
        }
    }
    if (sides & COMBATANT_SIDE_FIELD) {
        for (index = 0; index < FIELD_OBJECT_COUNT; index++) {
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
        return RANDOM_COMBATANT_NONE;
    }
    return targets[RandomAverage(0, count - 1, 0)];
}

RVA(0x00005f30, 0x83)
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomAllyAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target;
    if (id < 0) {
        target = PickRandomCombatant(COMBATANT_SIDE_PARTY);
    } else {
        target = PickRandomCombatant(COMBATANT_SIDE_FIELD);
    }
    if (target == RANDOM_COMBATANT_NONE) {
        return ACTOR_ACTION_NONE;
    }
    SetBasicAttackPick(actor, target);
    return ACTOR_ACTION_ATTACK_QUEUED;
}

RVA(0x00005fc0, 0x22)
b16 PickActorDialogue(i16 id) {
    if (id < 0) {
        return false;
    }
    GetCombatant(id)->mode = ACTOR_MODE_TALK;
    return true;
}

RVA(0x00005ff0, 0x6b)
b16 DelayActionSide(i16 id) {
    FieldObject* fieldObject;
    i16 i;
    i16 object;
    Character* member;
    if (id < 0) {
        for (i = 0; i < PARTY_SIZE; i++) {
            member = GetPartyCharacter(i);
            if (member) {
                DelayActionWait(GetCharacterActionWait(member), 50);
            }
        }
        MarkPickDone();
        return true;
    }
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
    for (i = 0; i < PARTY_SIZE; i++) {
        member = GetPartyCharacter(i);
        if (member) {
            ResetActionWaitDelay(GetCharacterActionWait(member));
        }
    }
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        object = GetLiveObject(i);
        if (object >= 0) {
            ResetActionWaitDelay(GetFieldObjectActionWait(GetFieldObject(object)));
        }
    }
    MarkPickDone();
    return true;
}

RVA(0x000060c0, 0x77)
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomAttack(i16 id) {
    Character* actor = GetCombatant(id);
    i16 target = PickRandomCombatant(COMBATANT_SIDE_BOTH);
    if (target == RANDOM_COMBATANT_NONE) {
        return ACTOR_ACTION_NONE;
    }
    SetBasicAttackPick(actor, target);
    return ACTOR_ACTION_ATTACK_QUEUED;
}

RVA(0x00006140, 0x34)
b16 SwapPartyRows(void) {
    i16 i;
    i16 member;
    for (i = 0; i < PARTY_ROW_SIZE; i++) {
        member = PartySlotAt(i);
        g_party.slots[i] = PartySlotAt(i + PARTY_ROW_SIZE);
        g_party.slots[i + PARTY_ROW_SIZE] = member;
    }
    MarkPickDone();
    FlushStatusRedraw(true);
    return true;
}

RVA(0x00006180, 0x1bc)
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) AdjustActorAction(i16 id, i16 action) {
    b16 hold;
    i16 window;
    i16 result;
    Character* actor;
    g_actorId = id;
    ClearFlagBank(EVENT_FLAG_BANK_SCRATCH);
    hold = SetHold(true);
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
    SetMessageHold(false);
    result = ACTOR_ACTION_NONE;
    actor = GetCombatant(id);
    switch (action & CONDITION_ACTION_MASK) {
        case CONDITION_ACTION_IDLE:
            actor->mode = ACTOR_MODE_IDLE;
            result = ACTOR_ACTION_HANDLED;
            break;
        case CONDITION_ACTION_ATTACK_OPPONENT:
        case CONDITION_ACTION_ATTACK_OPPONENT_ALIAS_1:
        case CONDITION_ACTION_ATTACK_OPPONENT_ALIAS_2:
            result = PickRandomOpponentAttack(id);
            break;
        case CONDITION_ACTION_ATTACK_ALLY:
        case CONDITION_ACTION_ATTACK_ALLY_ALIAS_1:
        case CONDITION_ACTION_ATTACK_ALLY_ALIAS_2:
            result = PickRandomAllyAttack(id);
            break;
        case CONDITION_ACTION_FLEE:
            actor->mode = ACTOR_MODE_FLEE;
            result = ACTOR_ACTION_HANDLED;
            break;
        case CONDITION_ACTION_NONE:
        case CONDITION_ACTION_NONE_ALIAS:
            result = ACTOR_ACTION_NONE;
            break;
        case CONDITION_ACTION_TALK:
            result = PickActorDialogue(id);
            break;
        case CONDITION_ACTION_DELAY_SIDE:
        case CONDITION_ACTION_DELAY_SIDE_ALIAS:
            result = DelayActionSide(id);
            actor->mode = ACTOR_MODE_IDLE;
            break;
        case CONDITION_ACTION_RESET_WAITS:
            result = ResetActionWaits();
            actor->mode = ACTOR_MODE_IDLE;
            break;
        case CONDITION_ACTION_ATTACK_RANDOM:
            result = PickRandomAttack(id);
            break;
        case CONDITION_ACTION_SWAP_ROWS:
            SwapPartyRows();
            actor->mode = ACTOR_MODE_IDLE;
            result = ACTOR_ACTION_HANDLED;
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
    SetMenuItems(
        menu,
        ACTOR_COMMAND_MENU_ROWS,
        actor,
        ACTOR_COMMAND_COUNT,
        ActorCommandMenuHandler
    );
    SetTextPlaneFirstSelectableRow(menu->plane, 0, true);
    return menu;
}

RVA(0x00006390, 0xaf)
void ActorCommandMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    Character* actor = menu->items.character;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            FormatFullName(g_scratchBuffer, actor);
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                -1,
                MENU_LINE_DISABLED
            );
            break;
        case MENU_EVENT_ADD_ROW:
            if (index < ACTOR_COMMAND_COUNT) {
                if (!s_commandLabels[index](actor)) {
                    AddMenuLine(
                        menu->plane,
                        g_unavailableCommandText,
                        TEXT_ATTR_FLAG1
                            | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK),
                        index + 1,
                        MENU_LINE_DISABLED
                    );
                } else {
                    AddMenuLine(
                        menu->plane,
                        g_scratchBuffer,
                        TEXT_ATTR_FLAG1
                            | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK),
                        index + 1,
                        MENU_LINE_NORMAL
                    );
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
        if (result == LIST_MENU_OPEN) {
            return result;
        }
        if (result == LIST_MENU_CANCELLED) {
            CloseActorCommandMenu();
            return result;
        }
        CloseActorCommandMenu();
        actor->pickRole = g_selectedObjectId;
        return g_selectedObjectId;
    }
    CloseActorCommandMenu();
    return LIST_MENU_CANCELLED;
}

RVA(0x00006500, 0xaa)
b16 FormatAttackCommand(Character* actor) {
    ItemRecord record;
    if (GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item < 1) {
        strcpy(g_scratchBuffer, "ATTACK");
        return true;
    }
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item);
    if (record.kind == ITEM_KIND_NONE) {
        strcpy(g_scratchBuffer, "ATTACK");
        return true;
    }
    strcpy(g_scratchBuffer, "SWORD");
    return true;
}

RVA(0x000065b0, 0xb0)
b16 FormatGunCommand(Character* actor) {
    ItemRecord record;
    if (GetCharacterEquipment(actor)[EQUIP_SLOT_GUN].item < 1) {
        return false;
    }
    if (HasObjectInReach(REACH_VERTICAL_OR_OCCUPIED, -1, 0)) {
        return false;
    }
    strcpy(g_scratchBuffer, "GUN");
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[EQUIP_SLOT_GUN].item);
    if (!record.kind) {
        return false;
    }
    if (GetCharacterEquipment(actor)[EQUIP_SLOT_AMMO].item < 1) {
        return false;
    }
    DecodeItemRecord(&record, GetCharacterEquipment(actor)[EQUIP_SLOT_AMMO].item);
    return record.kind != ITEM_KIND_NONE;
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
            case PICK_ROLE_ATTACK:
                SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item);
                return 1;
            case PICK_ROLE_GUN:
                SetCharacterPickTarget(actor, GetCharacterEquipment(actor)[EQUIP_SLOT_GUN].item);
                return 1;
            case PICK_ROLE_MAGIC:
            case PICK_ROLE_EXTRA:
                actor->pickTargetHigh = -1;
                return 0;
            case PICK_ROLE_COMP:
            case PICK_ROLE_ITEM:
                actor->pickTargetHigh = 0;
                return 0;
            case PICK_ROLE_RETURN:
            case PICK_ROLE_DEFENCE:
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
        list[count++] = PANEL_COMMAND_FIGHT;
        if (FormatGunCommand(actor)) {
            list[count++] = PANEL_COMMAND_GUN;
        }
        if (FormatMagicCommand(actor)) {
            list[count++] = PANEL_COMMAND_SKILL;
        }
        if (FormatItemCommand(actor)) {
            list[count++] = PANEL_COMMAND_ITEM;
        }
        if (FormatDefenceCommand(actor)) {
            list[count++] = PANEL_COMMAND_DEFENCE;
        }
        if (FormatReturnCommand(actor)) {
            list[count++] = PANEL_COMMAND_RETURN;
        }
        if (CanCharacterOpenAutomap(actor)) {
            list[count++] = PANEL_COMMAND_DDS;
        }
        list[count++] = PANEL_COMMAND_STATUS;
        if (GetRenderMode() == RENDER_MODE_FIELD) {
            list[count++] = PANEL_COMMAND_ENCOUNTER;
        }
        while (count < PANEL_COMMAND_ROWS) {
            list[count++] = PANEL_COMMAND_NONE;
        }
    }
}

RVA(0x00006a00, 0x39)
b16 ResetPartyTurnState(void) {
    i16 index;
    Character* actor;
    for (index = 0; index < PARTY_SIZE; index++) {
        if (PartySlotAt(index) != PARTY_SLOT_EMPTY) {
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
GZ_ENUM_RETURN(BattleProtectionResult, i16) CheckBattleProtection(
    Character* actor,
    i16 attribute,
    GZ_ENUM_PARAM(AttackMode, i16) mode,
    b16 report
) {
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_ALL_BLOCK]) {
        ReportBattleTally(actor, BATTLE_TALLY_ALL_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_MAGIC_GUN_BLOCK]
        && (mode == ATTACK_MAGIC || mode == ATTACK_GUN)) {
        ReportBattleTally(actor, BATTLE_TALLY_MAGIC_GUN_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_MAGIC_SEAL] && mode == ATTACK_MAGIC) {
        ReportBattleTally(actor, BATTLE_TALLY_MAGIC_SEAL, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_GUN_BLOCK] && mode == ATTACK_GUN) {
        ReportBattleTally(actor, BATTLE_TALLY_GUN_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_FIRE_BLOCK]
        && attribute == ATTACK_ATTRIBUTE_FIRE) {
        ReportBattleTally(actor, BATTLE_TALLY_FIRE_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_ICE_BLOCK]
        && attribute == ATTACK_ATTRIBUTE_ICE) {
        ReportBattleTally(actor, BATTLE_TALLY_ICE_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_ELECTRIC_BLOCK]
        && attribute == ATTACK_ATTRIBUTE_ELECTRIC) {
        ReportBattleTally(actor, BATTLE_TALLY_ELECTRIC_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_EXPEL_BLOCK]
        && attribute == ATTACK_ATTRIBUTE_EXPEL) {
        ReportBattleTally(actor, BATTLE_TALLY_EXPEL_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_DARK_BLOCK]
        && attribute == ATTACK_ATTRIBUTE_DARK) {
        ReportBattleTally(actor, BATTLE_TALLY_DARK_BLOCK, report);
        return BATTLE_PROTECTION_BLOCKED;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_MAGIC_REFLECT] && mode == ATTACK_MAGIC) {
        ReportBattleTally(actor, BATTLE_TALLY_MAGIC_REFLECT, report);
        return BATTLE_PROTECTION_REFLECT;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_MAGIC_REFLECT_HALF] && mode == ATTACK_MAGIC) {
        ReportBattleTally(actor, BATTLE_TALLY_MAGIC_REFLECT_HALF, report);
        return BATTLE_PROTECTION_REFLECT_HALF;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_TETRAKARN]
        && attribute == ATTACK_ATTRIBUTE_SWORD) {
        ReportBattleTally(actor, BATTLE_TALLY_TETRAKARN, report);
        return BATTLE_PROTECTION_REFLECT;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_PHYSICAL_REFLECT]
        && attribute == ATTACK_ATTRIBUTE_PHYSICAL) {
        ReportBattleTally(actor, BATTLE_TALLY_PHYSICAL_REFLECT, report);
        return BATTLE_PROTECTION_REFLECT;
    }
    if (GetCharacterBattleTallies(actor)[BATTLE_TALLY_MAGIC_MP_ABSORB] && mode == ATTACK_MAGIC) {
        ReportBattleTally(actor, BATTLE_TALLY_MAGIC_MP_ABSORB, report);
        return BATTLE_PROTECTION_ABSORB_MP;
    }
    return BATTLE_PROTECTION_NORMAL;
}

RVA(0x00006ca0, 0xcd)
i16 GetActionResistance(
    Character* actor,
    i16 attribute,
    GZ_ENUM_PARAM(AttackMode, i16) mode,
    b16 report,
    b16 sameSide
) {
    i16 result;
    if (!actor) {
        return ATTACK_RESIST_IMMUNE;
    }
    if (sameSide && GetCombatantSideRelation()) {
        return 50;
    }
    result = CheckBattleProtection(actor, attribute, mode, report);
    if (!result) {
        return ATTACK_RESIST_PROTECTED;
    }
    if (result >= 1) {
        if (HasCondition(GetCharacterConditions(actor), CONDITION_ZOMBIE)
            && attribute == ATTACK_ATTRIBUTE_EXPEL) {
            return 100;
        }
        result = attribute == ATTACK_ATTRIBUTE_FIXED_HALF_RESISTANCE
                     ? 50
                     : actor->resistance[attribute];
        if (result == ATTACK_RESIST_BYTE_REFLECT_HALF) {
            return ATTACK_RESIST_REFLECT_HALF;
        }
        if (result == ATTACK_RESIST_BYTE_REFLECT) {
            return ATTACK_RESIST_REFLECT;
        }
        if (result == ATTACK_RESIST_BYTE_ABSORB_MP) {
            return ATTACK_RESIST_ABSORB_MP;
        }
        if (result == ATTACK_RESIST_BYTE_ABSORB_HP_HALF) {
            return ATTACK_RESIST_ABSORB_HP_HALF;
        }
        if (result == ATTACK_RESIST_BYTE_ABSORB_HP) {
            return ATTACK_RESIST_ABSORB_HP;
        }
    }
    return result;
}

RVA(0x00006d70, 0x4d)
i16 GetSkillResistance(Character* actor, i16 skill, b16 report, b16 sameSide, i16* attribute) {
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
i16 GetItemResistance(Character* actor, i16 item, b16 report, b16 sameSide, i16* attribute) {
    *attribute = GetItemAttackAttribute(GetLoadedRecord(item));
    return GetActionResistance(actor, *attribute, ATTACK_MAGIC, report, sameSide);
}

RVA(0x00006e00, 0x114)
GZ_ENUM_RETURN(AttackAttribute, i16) GetPickedAttackAttribute(Character* actor, i16* condition) {
    switch (actor->pickRole) {
        case PICK_ROLE_ATTACK:
            *condition = 0;
            if (GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item != ITEM_ID_EMPTY) {
                *condition = GetEquipmentInflictedCondition(
                    GetLoadedRecord(GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item)
                );
                return GetEquipmentAttribute(
                    GetLoadedRecord(GetCharacterEquipment(actor)[EQUIP_SLOT_WEAPON].item)
                );
            }
            break;
        case PICK_ROLE_MAGIC:
            *condition = GetSkillInflictedCondition(GetCachedSkill(actor->pickTarget));
            return GetSkillAttackAttribute(GetCachedSkill(actor->pickTarget));
        case PICK_ROLE_GUN:
        case PICK_ROLE_ITEM:
            *condition = GetEquipmentInflictedCondition(GetLoadedRecord(actor->pickTarget));
            return GetEquipmentAttribute(GetLoadedRecord(actor->pickTarget));
    }
    return ATTACK_ATTRIBUTE_SWORD;
}

RVA(0x00006f20, 0x1a4)
void ApplyResistanceOutcome(Character* actor, i16 resistance, i32 amount) {
    switch (resistance) {
        case ATTACK_RESIST_PROTECTED:
            SetCharacterChanges(actor, 0, 0);
            SetResistanceResult(actor, -6, BATTLE_ACTION_PROTECTED);
            break;
        case ATTACK_RESIST_REFLECT_HALF:
            SetCharacterChanges(actor, amount / 2, amount / 2);
            SetResistanceResult(actor, -4, BATTLE_ACTION_REFLECTED);
            break;
        case ATTACK_RESIST_REFLECT:
            SetCharacterChanges(actor, 0, amount);
            SetResistanceResult(actor, -5, BATTLE_ACTION_REFLECTED);
            break;
        case ATTACK_RESIST_ABSORB_MP:
            SetCharacterChanges(actor, amount, 0);
            SetResistanceResult(actor, -3, BATTLE_ACTION_MP_ABSORBED);
            break;
        case ATTACK_RESIST_ABSORB_HP_HALF:
            SetCharacterChanges(actor, amount / 2, amount / 2);
            SetResistanceResult(actor, -1, BATTLE_ACTION_HP_ABSORBED);
            break;
        case ATTACK_RESIST_ABSORB_HP:
            SetCharacterChanges(actor, amount, 0);
            SetResistanceResult(actor, -2, BATTLE_ACTION_HP_ABSORBED);
            break;
        case ATTACK_RESIST_IMMUNE:
            SetCharacterChanges(actor, 0, 0);
            actor->result = BATTLE_ACTION_IMMUNE;
            break;
        default:
            SetCharacterChanges(actor, amount, 0);
            break;
    }
}

DATA(0x000919f2)
i16 g_fieldBattleActive;

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
b16 IsFieldModeAtLeast(b16 anyMode) {
    if (!anyMode) {
        return s_fieldMode >= FIELD_MAP_SCRIPT_EVENT;
    }
    return s_fieldMode >= FIELD_MAP_CELL_EVENT;
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
void EnterFieldMap(
    i16 map,
    i16 countA,
    i16 rateA,
    i16 countB,
    i16 rateB,
    GZ_ENUM_PARAM(FieldMapMode, i16) mode
) {
    s_fieldMap = map;
    s_fieldCountA = countA;
    s_fieldRateA = rateA;
    s_fieldCountB = countB;
    s_fieldRateB = rateB;
    s_fieldMode = mode;
    s_fieldEntryState = FIELD_MAP_ENDED;
    PushGameState(GAME_STATE_FIELD_ENCOUNTER);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000071f0, 0x7)
i16 GetFieldMap(void) {
    return s_fieldMap;
}

RVA(0x00007200, 0x7)
GZ_ENUM_RETURN(FieldMapOutcome, i16) GetFieldEntryState(void) {
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
i16 TickFieldCount(i16 side, b16 hold) {
    i16* count;
    if (s_fieldMode == FIELD_MAP_INACTIVE) {
        return 1;
    }
    count = side < 0 ? &s_fieldCountA : &s_fieldCountB;
    if (*count == -1) {
        return 1;
    }
    if (*count == 0) {
        return -1;
    }
    if (!hold && *count > 0) {
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
    if (s_fieldMode == FIELD_MAP_INACTIVE) {
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
void SpawnSecondGroupActor(i16 x, i16 y, b16 alternate) {
    SpawnFieldObject(
        FIELD_LAYER_SECOND,
        x,
        y,
        OppositeDirection(g_party.field.pos.direction),
        s_fieldParamSecond,
        alternate,
        FIELD_OBJECT_NO_EVENT,
        false
    );
}

RVA(0x00007340, 0x42)
i16 GetFacingWall(i16 map) {
    i16 width;
    i16 height;
    GetMapSize(&width, &height);
    return GetWallAt(
        g_party.field.pos.x,
        g_party.field.pos.y,
        g_party.field.pos.direction,
        width,
        height
    );
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
        case FIELD_ENCOUNTER_PHASE_ENTER:
            switch (GetGameStep()) {
                case FIELD_ENCOUNTER_STEP_SETUP:
                    NextGameStep();
                    LockStatusRedraw(false);
                    SetFieldMenuMode(FIELD_MENU_NO_SKILL_ITEM_FIGHT);
                    s_fieldMarker = true;
                    g_fieldBattleActive = true;
                    ResetFieldObjects();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    SaveFieldLayer(FIELD_LAYER_FIRST);
                    SaveFieldLayer(FIELD_LAYER_SECOND);
                    NotifyEncounterStart();
                    if (s_fieldMode == FIELD_MAP_CELL_EVENT) {
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                FIELD_LAYER_FIRST,
                                g_party.field.pos.x,
                                g_party.field.pos.y,
                                OppositeDirection(g_party.field.pos.direction),
                                s_fieldMap,
                                true,
                                FIELD_OBJECT_NO_EVENT,
                                false
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(
                                    g_party.field.pos.x,
                                    g_party.field.pos.y,
                                    true
                                );
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0x15, true);
                        } else {
                            s_fieldMusic = PlayMusic(0xd, true);
                        }
                    } else {
                        x = g_party.field.pos.x;
                        y = g_party.field.pos.y;
                        if (!GetFacingWall(s_fieldMap)) {
                            OffsetMapCoord(&x, &y, g_party.field.pos.direction, 0, -1);
                        }
                        for (i = 0; i < s_fieldParamFirst; i++) {
                            SpawnFieldObject(
                                FIELD_LAYER_FIRST,
                                x,
                                y,
                                OppositeDirection(g_party.field.pos.direction),
                                s_fieldMap,
                                true,
                                FIELD_OBJECT_NO_EVENT,
                                false
                            );
                        }
                        if (s_fieldParamSecond >= 0) {
                            for (i = 0; i < s_fieldParamThird; i++) {
                                SpawnSecondGroupActor(x, y, true);
                            }
                        }
                        if (s_fieldRefresh) {
                            s_fieldMusic = PlayMusic(0xd, true);
                        } else {
                            s_fieldMusic = PlayMusic(s_fieldOption, true);
                        }
                    }
                    LoadEnemyGroupSlot(FIELD_LAYER_FIRST, s_fieldMap);
                    if (s_fieldParamSecond >= 0) {
                        LoadEnemyGroupSlot(FIELD_LAYER_SECOND, s_fieldParamSecond);
                    }
                    RequestFieldRefresh();
                    for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                        actor = GetFieldActor(i);
                        AlertActor(actor, ATTITUDE_VERY_HOSTILE);
                    }
                    break;
                case FIELD_ENCOUNTER_STEP_START:
                    NextGamePhase();
                    ResetPartyTurnState();
                    RedrawFieldView();
                    break;
            }
            break;
        case FIELD_ENCOUNTER_PHASE_TURNS:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims()) {
                AllowImmediateInput();
            }
            allFallen = true;
            for (i = 0; i < s_fieldParamThird + s_fieldParamFirst; i++) {
                allFallen &= GetFatalCondition(GetCharacterConditions(GetFieldActor(i)));
            }
            if (s_fieldMode >= FIELD_MAP_CELL_EVENT && CountFieldObjects() <= 0) {
                LeaveFieldMap(FIELD_MAP_WON);
                break;
            }
            if (s_fieldMode == FIELD_MAP_SCRIPT_EVENT && allFallen) {
                LeaveFieldMap(FIELD_MAP_WON);
                break;
            }
            if (FindFirstAblePartyMember() == PARTY_POSITION_NONE) {
                LeaveFieldMap(FIELD_MAP_LOST);
                break;
            }
            if (!TickFieldCount(-1, true)) {
                LeaveFieldMap(FIELD_MAP_ENDED);
                break;
            }
            if (!TickFieldCount(1, true)) {
                LeaveFieldMap(FIELD_MAP_ENDED);
                break;
            }
            if (!GetPickMode() && PickAnalyzeTarget() >= 0) {
                SetGamePhase(FIELD_ENCOUNTER_PHASE_ANALYZE);
                break;
            }
            SetFieldBusy(false);
            if (RunPartyTurn(g_tickElapsed)) {
                break;
            }
            if (TickFieldCount(0, true) <= 0) {
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
        case FIELD_ENCOUNTER_PHASE_BACK_OUT:
            PrevGamePhase();
            break;
        case FIELD_ENCOUNTER_PHASE_REWARDS:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState <= FIELD_MAP_ENDED) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, true);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst != 0 || s_fieldPairSecond != 0) {
                ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, BIT_CHANGE_SET);
            }
            break;
        case FIELD_ENCOUNTER_PHASE_LEVEL_UPS:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(GAME_STATE_LEVEL_UP);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(WAIT_INPUT_OR_FRAMES, WAIT_ON_ANY_INPUT, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(FIELD_ENCOUNTER_PHASE_TEAR_DOWN);
            break;
        case FIELD_ENCOUNTER_PHASE_ANALYZE:
            if (RunAnalyzeWindow()) {
                SetGamePhase(FIELD_ENCOUNTER_PHASE_TURNS);
            }
            break;
        case FIELD_ENCOUNTER_PHASE_TEAR_DOWN:
            PlaySoundEffect(0x1b);
            RestoreScreenMode();
            ClearSelectedHotspot();
            ResetFieldObjects();
            ResetFieldLayer(FIELD_LAYER_SECOND);
            ResetFieldLayer(FIELD_LAYER_FIRST);
            CloseMessageWindow();
            NotifyEncounterEnd();
            RestoreFieldLayer(FIELD_LAYER_SECOND);
            RestoreFieldLayer(FIELD_LAYER_FIRST);
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, true);
            if (s_fieldMode == FIELD_MAP_CELL_EVENT) {
                RespawnAreaActors();
            }
            RequestFieldRefresh();
            s_fieldRefresh = false;
            ReturnFromGameState();
            s_fieldCountA = -1;
            s_fieldRateA = 100;
            s_fieldCountB = -1;
            s_fieldRateB = 100;
            s_fieldMode = FIELD_MAP_INACTIVE;
            ResetRosterBattleState();
            SetFieldMenuMode(FIELD_MENU_ALL);
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
void LeaveFieldMap(GZ_ENUM_PARAM(FieldMapOutcome, i16) result) {
    g_fieldBattleActive = false;
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
    for (slot = 0; slot < ROSTER_SIZE; slot++) {
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
        case FIELD_ENCOUNTER_PHASE_ENTER:
            switch ((u16)GetGameStep()) {
                case FIELD_ENCOUNTER_STEP_SETUP:
                    ClearSceneSurfaces();
                    NextGameStep();
                    s_fieldLeftEarly = false;
                    SetFieldStatusBit0(false);
                    SetFieldStatusBit11(false);
                    SetFieldMenuMode(FIELD_MENU_NO_SKILL_ITEM_FIGHT_MAPPING);
                    g_fieldBattleActive = true;
                    ResetFieldScene();
                    s_fieldPaletteState = SavePaletteState(s_fieldPaletteState, 3);
                    ResetFieldObjects();
                    LoadFieldTable();
                    PrepareFieldRandom();
                    s_fieldMusic = PlayMusic(13, true);
                    ResetRosterFieldMarks();
                    RequestFieldRefresh();
                    return FlushFieldScreen();
                case FIELD_ENCOUNTER_STEP_START:
                    NextGamePhase();
                    StartScreenFadeAndWait(SCREEN_FADE_FROM_BLACK, 1);
                    break;
            }
            break;
        case FIELD_ENCOUNTER_PHASE_TURNS:
            if (HasTurnElapsed() && TickPartyConditions()) {
                RequestFieldRefresh();
            }
            if (!AdvanceObjectAnims() && !GetPickMode()) {
                AllowImmediateInput();
            }
            key = CountFieldObjects();
            if (key <= 0) {
                LeaveFieldMap(FIELD_MAP_ENDED);
                if (key >= 0) {
                    break;
                }
                s_fieldLeftEarly = true;
                return FlushFieldScreen();
            }
            if (FindFirstAblePartyMember() == PARTY_POSITION_NONE) {
                LeaveFieldMap(FIELD_MAP_LOST);
                return FlushFieldScreen();
            }
            if (!GetPickMode()) {
                if (PickAnalyzeTarget() >= 0) {
                    SetGamePhase(FIELD_ENCOUNTER_PHASE_ANALYZE);
                    return FlushFieldScreen();
                }
                if (g_pendingTalk) {
                    RunPendingTalk();
                    return FlushFieldScreen();
                }
            }
            SetFieldBusy(false);
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
            HideScreenLayer(SCREEN_LAYER_PANEL);
            if (RollProximityEvent() > 0) {
                LeaveFieldMap(FIELD_MAP_ENDED);
                s_fieldLeftEarly = true;
                RunMessageScene(0x7f04, 0x10, -1);
                PlaySoundEffect(4);
                ClearEncounterPending();
                return UpdateFieldScreen(false);
            }
            NextGamePhase();
            RunMessageScene(0x7f04, 0x11, -1);
            PlaySoundEffect(3);
            PushWaitState(WAIT_FRAMES, 0x3c, 0x3c, 0);
            ClearEncounterPending();
            return UpdateFieldScreen(false);
        case FIELD_ENCOUNTER_PHASE_BACK_OUT:
            SetFieldStatusBit0(false);
            CloseMessageWindow();
            RestoreDrawState(SaveDrawState());
            PrevGamePhase();
            return UpdateFieldScreen(false);
        case FIELD_ENCOUNTER_PHASE_REWARDS:
            PlaySoundEffect(0x1b);
            NextGamePhase();
            ResetRosterStatModifiers();
            if (s_fieldEntryState < FIELD_MAP_ENDED) {
                break;
            }
            MarkRewardsPending();
            PlayMusic(s_fieldMusic, true);
            AccessScriptReg(1, 0, 1 - s_fieldLeftEarly);
            RunMessageScene(0xdd, 0x59, -1);
            if (s_fieldPairFirst == 0 && s_fieldPairSecond == 0) {
                break;
            }
            ModifyEventFlag(s_fieldPairFirst, s_fieldPairSecond, BIT_CHANGE_SET);
            return FlushFieldScreen();
        case FIELD_ENCOUNTER_PHASE_LEVEL_UPS:
            if (GrantBattleRewards()) {
                CloseMessageWindow();
                PushScreenFade(SCREEN_FADE_FROM_BLACK, 1);
                PushGameState(GAME_STATE_LEVEL_UP);
                PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                PushWaitState(WAIT_INPUT_OR_FRAMES, -1, 0x50, -1);
                MarkRewardsPending();
                FormatLevelUpMessage(g_scratchBuffer, FindLevelUpSlot());
                ShowMessage(g_scratchBuffer, 0x3c);
                return false;
            }
            s_fieldPairFirst = 0;
            s_fieldPairSecond = 0;
            SetGamePhase(FIELD_ENCOUNTER_PHASE_TEAR_DOWN);
            return FlushFieldScreen();
        case FIELD_ENCOUNTER_PHASE_ANALYZE:
            if (RunAnalyzeWindow()) {
                SetGamePhase(FIELD_ENCOUNTER_PHASE_TURNS);
                return FlushFieldScreen();
            }
            break;
        case FIELD_ENCOUNTER_PHASE_TEAR_DOWN:
            PlaySoundEffect(0x1b);
            CloseMessageWindow();
            ResetFieldObjects();
            ResetFieldLayer(FIELD_LAYER_SECOND);
            ResetFieldLayer(FIELD_LAYER_FIRST);
            RequestFieldRefresh();
            ReturnFromGameState();
            SetFieldStatusBit11(true);
            ResetRosterBattleState();
            SetFieldMenuMode(FIELD_MENU_NO_FIGHT_TALK_MAPPING);
            ReleaseFieldImage();
            s_fieldPaletteState = RestorePaletteState(s_fieldPaletteState, true);
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
            distance = GridDistance(g_party.field.pos.x, g_party.field.pos.y, pos.x, pos.y);
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
b16 HasObjectInReach(GZ_ENUM_PARAM(ReachTestMode, i16) mode, i16 first, i16 second) {
    MapCoord pos = GetMapCoord();
    FieldObject* object;
    switch (mode) {
        case REACH_VERTICAL_OR_OCCUPIED:
            if (first >= 0) {
                object = GetFieldObject(first);
                if (pos.x != object->pos.x || pos.y == object->pos.y) {
                    return false;
                }
            } else if (!CountObjectsAt(pos.x, pos.y, OBJECT_MATCH_ANY, 0)) {
                return false;
            }
            break;
        case REACH_SHARED_PARTY_CELL:
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
    for (index = 0; index < PARTY_SIZE; index++) {
        member = GetPartyEntry(index);
        if (member && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return PARTY_POSITION_NONE;
}

RVA(0x000081b0, 0x4b)
i16 FindAbleHumanMember(void) {
    i16 index;
    Character* member;
    for (index = 0; index < PARTY_SIZE; index++) {
        member = GetPartyCharacter(index);
        if (member
            && (member->id == OBJECT_RECORD_ISHTAR || member->id == OBJECT_RECORD_HELL_DOG
                || IsHumanCharacter(member))
            && !GetDisablingCondition(GetCharacterConditions(member))) {
            return index;
        }
    }
    return PARTY_POSITION_NONE;
}

RVA(0x00008200, 0xbc)
void TickPartyConditionActions(void) {
    i16 index;
    i16 action;
    Character* actor;
    for (index = 0; index < PARTY_SIZE; index++) {
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
                        if ((action & CONDITION_ACTION_MASK) == CONDITION_ACTION_NONE) {
                            action = (action & CONDITION_ACTION_FLAGS_MASK)
                                     | CONDITION_ACTION_ATTACK_OPPONENT;
                        }
                        action = AdjustActorAction(PartyCombatantId(index), action);
                        if (action != ACTOR_ACTION_NONE) {
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
    GetCharacterActionWait(actor)->ready = true;
    switch (actor->mode) {
        case ACTOR_MODE_ATTACK:
            MarkPickDone();
            break;
        case ACTOR_MODE_FLEE:
            if (!IsHumanCharacter(actor)) {
                actor->pickRole = PICK_ROLE_RETURN;
                MarkPickDone();
                break;
            }
        case ACTOR_MODE_DEFEND:
        case ACTOR_MODE_APPROACH:
        case ACTOR_MODE_STEP_INTO_RANGE:
        case ACTOR_MODE_STEP_CLOSER:
        case ACTOR_MODE_CIRCLE_AROUND:
        case ACTOR_MODE_RECOVER:
        case ACTOR_MODE_WANDER:
        case ACTOR_MODE_IDLE:
        case ACTOR_MODE_TALK:
            actor->pickRole = PICK_ROLE_DEFENCE;
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

RVA(0x00008450, 0x2f)
GZ_ENUM_RETURN(CombatantSideRelation, i16) GetCombatantSideRelation(void) {
    if (g_targetId < 0 && g_actorId < 0) {
        return COMBATANT_RELATION_PARTY_PAIR;
    }
    if (g_targetId >= 0 && g_actorId >= 0) {
        return COMBATANT_RELATION_FIELD_PAIR;
    }
    return COMBATANT_RELATION_MIXED;
}

static __inline void AddArmorSlotHitModifier(ItemSlot* slot, i16* modifier) {
    if (slot->item != ITEM_ID_EMPTY) {
        *modifier += GetArmorHitModifier(GetLoadedRecord(slot->item));
    }
}

RVA(0x00008480, 0x11b)
i16 GetEquipmentHitModifier(Character* attacker, Character* target) {
    i16 modifier = 0;
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[EQUIP_SLOT_HEAD], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[EQUIP_SLOT_BODY], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[EQUIP_SLOT_ARMS], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[EQUIP_SLOT_LEGS], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[EQUIP_SLOT_ACCESSORY], &modifier);
    modifier = -modifier;
    if (attacker->pickTarget >= 1) {
        modifier += GetWeaponHitModifier(GetLoadedRecord(attacker->pickTarget));
    }
    return modifier;
}

static __inline i16 GetCombatantFacing(i16 id) {
    if (id < 0) {
        return g_party.field.pos.direction;
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
                SetActionResult(attacker, BATTLE_ACTION_LETHAL);
                return BATTLE_ACTION_LETHAL;
            }
        }
        attack = GetExceptionalAttackBase(attacker);
        attack += RandomUpTo(7);
        defense = GetExceptionalAttackBase(target);
        defense += RandomUpTo(31);
        if (modifier + attack > defense) {
            SetActionResult(attacker, BATTLE_ACTION_CRITICAL);
            return BATTLE_ACTION_CRITICAL;
        }
        SetActionResult(attacker, BATTLE_ACTION_MISSED);
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
        SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
        return true;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == FACING_FROM_BEHIND) {
        SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
        return true;
    }
    accuracy = GetBattleStatShown(attacker, BATTLE_STAT_GUN_ACCURACY);
    ApplyAttackAccuracyConditions(attacker, accuracy);
    evasion = GetBattleStatShown(target, BATTLE_STAT_GUN_EVASION);
    if (HasCondition(GetCharacterConditions(target), CONDITION_DANCE)) {
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
    if (GetCombatantFacingDifference(g_actorId, g_targetId) != FACING_FACE_TO_FACE) {
        defense = evasion * 75;
    } else {
        defense = evasion * 100;
    }
    if (attack >= defense) {
        roll = defense * RandomAverage(-2, 12, 1);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
            return true;
        }
    } else {
        roll = defense * RandomAverage(0, 15, 0);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
            return true;
        }
    }
    roll = defense * RandomAverage(0, 7, 0);
    if (attack >= roll) {
        SetActionResult(attacker, BATTLE_ACTION_GRAZED);
        return true;
    }
    SetActionResult(attacker, BATTLE_ACTION_MISSED);
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
    if (result == BATTLE_ACTION_CRITICAL) {
        amount += attacker->level + 5;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        amount *= 1.2;
    }
    facing = GetCombatantFacingDifference(g_actorId, g_targetId);
    if (facing == FACING_FROM_BEHIND) {
        amount *= 1.5;
    } else if (facing != FACING_FACE_TO_FACE) {
        amount *= 1.2;
    }
    if (result == BATTLE_ACTION_GRAZED) {
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
        SetActionResult(attacker, BATTLE_ACTION_NO_EFFECT);
    }
    return damage;
}

RVA(0x00008d30, 0x124)
b16 RollGunCondition(Character* attacker, Character* target, i16 resistance, i16 condition) {
    i16 luck;
    i16 roll;
    i16 defense;
    g_statusCondition = INFLICT_NONE;
    if (!condition) {
        return false;
    }
    if (attacker->lastChange < GetConditionDamageThreshold(target)) {
        return false;
    }
    if (g_actionResult >= BATTLE_ACTION_REFLECTED) {
        return false;
    }
    if (g_targetId >= 0 && IsFieldModeAtLeast(false) && IsFieldConditionRestricted(condition)) {
        return false;
    }
    roll = RandomAverage(0, 20, 0);
    luck = GetStatTotal(attacker, STAT_FORTUNE);
    luck += roll;
    if (luck <= GetStatTotal(target, STAT_FORTUNE)) {
        return false;
    }
    roll = RandomAverage(0, 40, 0);
    defense = GetBattleStatShown(target, BATTLE_STAT_GUN_DEFENSE);
    defense *= roll;
    if (ScaleActionValue(GetBattleStatShown(attacker, BATTLE_STAT_GUN_POWER) * 10, resistance, 2)
            - defense
        <= 0) {
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
    g_attackResistance = GetActionResistance(target, g_attackAttribute, ATTACK_GUN, true, false);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, g_attackAttribute);
    result = RollExceptionalAttack(attacker, target, mode, g_attackResistance);
    if (result != 0) {
        AddTrainingPoints(attacker, BATTLE_GROUP_GUN, 1);
    }
    if (result < 5) {
        if (result == 0) {
            result = RollGunHit(attacker, target, g_attackResistance);
            attacker->resultFlag = result;
            if (result != 0) {
                AddTrainingPoints(attacker, BATTLE_GROUP_GUN, 1);
            }
        } else {
            SetCharacterResult(attacker, result, 1);
        }
        amount = ComputeGunDamage(attacker, target, attacker->result);
        SetCharacterChanges(attacker, amount, 0);
    } else if (result == BATTLE_ACTION_LETHAL) {
        amount = 0x7fff;
        SetCharacterChanges(attacker, amount, 0);
        SetFlaggedActionResult(attacker, BATTLE_ACTION_LETHAL);
        AddTrainingPoints(attacker, BATTLE_GROUP_GUN, 1);
    }
    ApplyResistanceOutcome(attacker, g_attackResistance, amount);
    return RollGunCondition(attacker, target, g_attackResistance, g_attackCondition);
}

RVA(0x00008fe0, 0x2a)
void LoadGunDistributionTable(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_GUN_DISTRIBUTION, DATA_FILE_TABLE, 0);
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
    if (GetCharacterEquipment(attacker)[EQUIP_SLOT_GUN].item < 1) {
        return 0;
    }
    if (GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].item < 1) {
        return 0;
    }
    rounds = GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].quantity;
    if (g_actorId >= 0) {
        rounds = 255;
    }
    limit = GetGunBurstLimit(GetLoadedRecord(GetCharacterEquipment(attacker)[EQUIP_SLOT_GUN].item));
    if (limit > rounds) {
        limit = rounds;
    }
    return limit;
}

RVA(0x000090c0, 0xe7)
i16 PrepareGunBurst(Character* attacker, i16 count) {
    ItemRecord* record = GetLoadedRecord(GetCharacterEquipment(attacker)[EQUIP_SLOT_GUN].item);
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
    s_gunRoundPower =
        GetItemAttackPower(GetLoadedRecord(GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].item));
    maximum = limits & GUN_BURST_LIMIT_MASK;
    minimum = limits >> GUN_BURST_LIMIT_SHIFT;
    if (minimum < 1) {
        minimum = 1;
    } else if (minimum > GUN_BURST_MAX_TARGETS) {
        minimum = GUN_BURST_MAX_TARGETS;
    }
    if (maximum < minimum) {
        maximum = minimum;
    } else if (maximum > GUN_BURST_MAX_TARGETS) {
        maximum = GUN_BURST_MAX_TARGETS;
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

static __inline u8 GetGunRoundPercent(u8 (*table)[GUN_BURST_MAX_TARGETS], i16 count, i16 index) {
    return table[count - 1][index];
}

RVA(0x000091f0, 0xe2)
i16 DistributeGunRounds(i16 rounds, i16 count) {
    u8(*table)[GUN_BURST_MAX_TARGETS];
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
        if (rounds > GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].quantity) {
            rounds = GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].quantity;
        }
        GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].quantity -= rounds;
        if (GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].quantity <= 0) {
            ClearItemSlot(&GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO]);
        }
    }
    for (index = 0; index < GUN_BURST_MAX_TARGETS; index++) {
        s_gunRounds[index] = s_gunRounds[index + 1];
        s_gunPower[index] = s_gunPower[index + 1];
    }
    s_gunPower[GUN_BURST_MAX_TARGETS] = 0;
    s_gunRounds[GUN_BURST_MAX_TARGETS] = 0;
}

RVA(0x000093b0, 0x28)
void SpendAllGunRounds(Character* attacker) {
    if (attacker) {
        while (s_gunRounds[0]) {
            SpendGunRounds(attacker);
        }
    }
}

static __inline void ResetPartyCommandPick(void) {
    s_pickMode = PARTY_COMMAND_WAIT_MEMBER;
    s_pickedIndex = CHARACTER_ID_NONE;
}

RVA(0x000093e0, 0x36)
void CloseFieldWindows(void) {
    RunPartyPicker(PARTY_PICKER_COMMAND_CLOSE);
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
        case PARTY_COMMAND_WAIT_MEMBER:
        case PARTY_COMMAND_PREPARE_MEMBER:
            selection = GetPickerSelection();
            if (selection < 0) {
                return 0;
            }
            if (FindMenuLineByValue(selection, character->id)) {
                if (GetPickBlockingCondition(GetCharacterConditions(character))) {
                    s_pickDone = true;
                }
            } else if (PickPartyMember(index) == PARTY_MEMBER_READY) {
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
GZ_ENUM_RETURN(PartyCommandPhase, i16) GetPickMode(void) {
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
        return LIST_MENU_CANCELLED;
    }
    if (s_pickMenu == NULL) {
        switch (character->pickRole) {
            case PICK_ROLE_COMP:
                return 1;
            case PICK_ROLE_MAGIC:
            case PICK_ROLE_EXTRA:
                s_pickMenu = OpenMemberSkillMenu(id);
                break;
            case PICK_ROLE_ITEM:
                s_pickMenu = OpenItemListMenu();
                break;
            default:
                return 1;
        }
    }
    result = RunListMenu(s_pickMenu);
    if (result == LIST_MENU_OPEN) {
        return result;
    }
    if (result == LIST_MENU_CANCELLED) {
        s_pickMenu = CloseListMenu(s_pickMenu);
        return LIST_MENU_CANCELLED;
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
    if (character->pickRole == PICK_ROLE_MAGIC) {
        if (character->pickTarget == SKILL_FUSION) {
            return RunPickTargetWindow(0, range, TARGET_PICK_FIELD_OBJECT, 0);
        }
        if (character->pickTarget == SKILL_MAHOROGI) {
            return RunPickTargetWindow(0, range, 0x82, 0);
        }
    }
    if (flags == TARGET_ACTOR_SIDE) {
        return RunPickTargetWindow(0, range, 0x12, 0);
    } else {
        return RunPickTargetWindow(0, range, TARGET_PICK_FIELD_OBJECT | TARGET_PICK_PARTY_SLOT, 0);
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
        case PARTY_COMMAND_WAIT_MEMBER:
            if (IsPanelLayerVisible()) {
                g_tickElapsed = 0;
            }
            s_pickDone = false;
            if (s_pickedIndex < 0) {
                break;
            }
            SetFieldBusy(true);
            s_pickMode++;
            g_tickElapsed = 0;
            HideScreenLayer(SCREEN_LAYER_PANEL);
            return g_tickElapsed;
        case PARTY_COMMAND_PREPARE_MEMBER:
            GetCharacterById(s_pickedIndex)->conditionActionTicks = 0;
            SetFieldBusy(true);
            s_pickMode++;
            g_tickElapsed = 0;
            s_pickMode += PrepareMemberPickTarget(s_pickedIndex);
            if (s_pickMode > PARTY_COMMAND_CONFIRM) {
                s_pickMode = PARTY_COMMAND_CONFIRM;
            }
            break;
        case PARTY_COMMAND_PICK_ACTION:
            GetCharacterById(s_pickedIndex)->conditionActionTicks = 0;
            SetFieldBusy(true);
            g_tickElapsed = 0;
            result = RunMemberPickMenu(s_pickedIndex);
            if (result == LIST_MENU_CANCELLED) {
                ResetPartyCommandPick();
            }
            if (result < 0) {
                break;
            }
            s_pickMode++;
            return g_tickElapsed;
        case PARTY_COMMAND_PICK_TARGET:
            character = GetCharacterById(s_pickedIndex);
            character->conditionActionTicks = 0;
            if (GetPickBlockingCondition(GetCharacterConditions(character))) {
                ResetPartyCommandPick();
                return g_tickElapsed;
            }
            if (character->pickRole == PICK_ROLE_ITEM) {
                kind = GetLoadedRecord(character->pickTarget)->kind;
                if (kind == ITEM_KIND_WEAPON || kind == ITEM_KIND_ACCESSORY) {
                    character->pickFlags |= PICK_ITEM_SKILL;
                    character->pickItem = character->pickTarget;
                    character->pickRole = PICK_ROLE_MAGIC;
                    character->pickTarget = GetItemSkillId(GetLoadedRecord(character->pickTarget));
                }
            }
            if (character->pickRole == PICK_ROLE_MAGIC) {
                flags = GetSkillTargetFlags(character->pickTarget);
                if (TargetFlagsSelectSelf(flags)) {
                    result = CurrentMemberCombatantId();
                    character->pickObject = result;
                    goto target_selected;
                } else if (TargetFlagsSelectActorGroup(flags)) {
                    result = CurrentMemberCombatantId();
                    character->pickObject = result;
                    goto target_selected;
                } else if (flags & TARGET_ACTOR_SIDE) {
                    reach = true;
                }
            } else if (character->pickRole == PICK_ROLE_ITEM) {
                flags = GetItemTargetFlags(GetLoadedRecord(character->pickTarget));
                if (TargetFlagsSelectSelf(flags)) {
                    result = CurrentMemberCombatantId();
                    character->pickObject = result;
                    goto target_selected;
                } else if (TargetFlagsSelectActorGroup(flags)) {
                    result = CurrentMemberCombatantId();
                    character->pickObject = result;
                    goto target_selected;
                }
                if (character->pickTarget == ITEM_CORE_SHIELD) {
                    flags = TARGET_ACTOR_SIDE;
                }
                if (flags & TARGET_ACTOR_SIDE) {
                    reach = true;
                }
            }
            if (flags == TARGET_SELECT_FIELD_OR_ROSTER || flags == TARGET_SELECT_ROSTER_ONLY
                || flags == TARGET_SELECT_PARTY_OR_ROSTER) {
                reach = true;
            }
            if (reach == false && HasObjectInReach(REACH_VERTICAL_OR_OCCUPIED, -1, 0)) {
                character->pickObject = FindObjectAtParty();
            target_selected:
                s_pickMode++;
                return g_tickElapsed;
            }
            range = GetMemberPickRange(s_pickedIndex);
            if (flags == TARGET_SELECT_FIELD_OR_ROSTER) {
                result = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_FIELD_OBJECT | TARGET_PICK_ROSTER_LIST,
                    0
                );
            } else if (flags == TARGET_SELECT_ROSTER_ONLY) {
                result = RunPickTargetWindow(0, range, TARGET_PICK_ROSTER_LIST, 0);
            } else if (flags == TARGET_SELECT_PARTY_OR_ROSTER) {
                result = RunPickTargetWindow(
                    0,
                    range,
                    TARGET_PICK_PARTY_SLOT | TARGET_PICK_ROSTER_LIST,
                    0
                );
            } else {
                result = PickMemberActionTarget(character, flags, range);
                flags = 0;
            }
            if (flags != 0) {
                g_tickElapsed = 0;
            }
            if (result == TARGET_PICK_CANCELLED) {
                s_pickMode--;
                s_pickMode -= PrepareMemberPickTarget(s_pickedIndex);
                if (s_pickMode < PARTY_COMMAND_PREPARE_MEMBER) {
                    s_pickMode = PARTY_COMMAND_PREPARE_MEMBER;
                }
                if (character->pickFlags & PICK_ITEM_SKILL) {
                    character->pickRole = PICK_ROLE_ITEM;
                }
            }
            if (result < TARGET_PICK_SELECTED) {
                if (character->pickRole != PICK_ROLE_ATTACK) {
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
            break;
        case PARTY_COMMAND_CONFIRM:
            character = GetCharacterById(s_pickedIndex);
            if (character != NULL) {
                if (!GetPickBlockingCondition(GetCharacterConditions(character))) {
                    QueueActionWait(GetCharacterActionWait(character));
                }
                if (character->pickRole == PICK_ROLE_MAGIC
                    && character->pickTarget == SKILL_SABBATMA) {
                    s_pickMode++;
                    g_tickElapsed = 0;
                    break;
                }
            }
            ResetPartyCommandPick();
            return g_tickElapsed;
        case PARTY_COMMAND_PICK_SUMMON_POSITION:
            character = GetCharacterById(s_pickedIndex);
            g_tickElapsed = 0;
            result = RunPickTargetWindow(0, 0, TARGET_PICK_PARTY_SLOT, 0);
            if (result == TARGET_PICK_CANCELLED) {
                RestoreSwappedMember();
                s_pickMode = PARTY_COMMAND_PICK_TARGET;
                if (character->pickFlags & PICK_ITEM_SKILL) {
                    character->pickRole = PICK_ROLE_ITEM;
                }
            }
            if (result < TARGET_PICK_SELECTED) {
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
    GZ_ENUM_LOCAL(PickRole, i8) role = character->pickRole;
    i16 item;
    if (role == PICK_ROLE_NONE) {
        return 0;
    }
    if (role == PICK_ROLE_DEFENCE) {
        return 0;
    }
    if (role == PICK_ROLE_RETURN) {
        return 0;
    }
    if (role == PICK_ROLE_ATTACK) {
        item = GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item;
        if (item == ITEM_ID_NONE || item == ITEM_ID_EMPTY) {
            return 1;
        }
        return GetItemAttackRange(GetLoadedRecord(item));
    }
    if (role == PICK_ROLE_GUN) {
        item = GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item;
        if (item == ITEM_ID_NONE || item == ITEM_ID_EMPTY) {
            return 1;
        }
        return 3;
    }
    if (role == PICK_ROLE_ITEM) {
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
GZ_ENUM_RETURN(PartyCommandPhase, i16) QueryPickMode(void) {
    return s_pickMode;
}

RVA(0x00009d00, 0x7)
i16 GetTickElapsed(void) {
    return g_tickElapsed;
}

static __inline i16 FindPickReplacementSlot(i16 keep) {
    i16 index;
    index = FindEmptySlot(true);
    if (index >= 0) {
        return index;
    }
    for (index = 0; index < PARTY_SIZE; index++) {
        if (IsPartyMemberFallen(index)) {
            return index;
        }
    }
    for (index = 0; index < PARTY_SIZE; index++) {
        if (index != keep && GetPartyRosterId(index) >= HUMAN_ID_LIMIT) {
            return index;
        }
    }
    for (index = 0; index < PARTY_SIZE; index++) {
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
        s_swapSaved = PARTY_SLOT_EMPTY;
        g_guestIndex = PARTY_POSITION_NONE;
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
GZ_ENUM_RETURN(TargetPickResult, i16) RunPickTargetWindow(i16 minimumRange, i16 maximumRange, i16 kind, i16 id) {
    Character* character = GetCharacterById(id);
    i16 result;
    if (kind & TARGET_PICK_ROSTER_LIST) {
        if (id && (!character || GetPickBlockingCondition(GetCharacterConditions(character)))) {
            RunStatusListPicker(true);
            g_hoveredObjectId = g_selectedObjectId = -1;
            if (s_pickScreenSaved) {
                RestoreScreenSaveWithState(g_pickScreenSave);
                FreeScreenSave(g_pickScreenSave);
                s_pickScreenSaved = false;
            }
            return TARGET_PICK_CANCELLED;
        }
        if (!s_pickScreenSaved) {
            AllocScreenSave(g_pickScreenSave);
            CaptureScreenSaveWithState(g_pickScreenSave);
            s_pickScreenSaved = true;
        }
        if (kind & TARGET_PICK_FIELD_OBJECT) {
            SetStatusColumn(STATUS_LIST_RESERVE);
        } else if (kind & TARGET_PICK_PARTY_SLOT) {
            SetStatusColumn(STATUS_LIST_DEMONS);
        } else {
            SetStatusColumn(STATUS_LIST_ALL);
        }
        result = RunStatusListPicker(false);
        if (result == LIST_MENU_OPEN) {
            return TARGET_PICK_WAITING;
        }
        RunStatusListPicker(true);
        PlaySoundEffect(1);
        if (s_pickScreenSaved) {
            RestoreScreenSaveWithState(g_pickScreenSave);
            FreeScreenSave(g_pickScreenSave);
            s_pickScreenSaved = false;
        }
        return result == LIST_MENU_CANCELLED ? TARGET_PICK_CANCELLED : TARGET_PICK_SELECTED;
    }
    if ((id && (!character || GetPickBlockingCondition(GetCharacterConditions(character))))
        || TakeMouseCancelSound()) {
        ClearPartySlotSelection();
        RunStatusListPicker(true);
        g_hoveredObjectId = g_selectedObjectId = -1;
        return TARGET_PICK_CANCELLED;
    }
    if (kind & TARGET_PICK_FIELD_OBJECT) {
        result = PickFieldObjectTarget(minimumRange, maximumRange);
        if (result) {
            ClearPartySlotSelection();
            PlaySoundEffect(1);
            return result;
        }
    }
    if (kind & TARGET_PICK_PARTY_SLOT) {
        if (kind == TARGET_PICK_PARTY_SLOT) {
            result = PickPartySlotTarget(minimumRange, PARTY_SLOT_EXCLUDE_HUMANS);
        } else {
            result = PickPartySlotTarget(minimumRange, PARTY_SLOT_REQUIRE_OCCUPIED);
        }
        if (result) {
            ClearPartySlotSelection();
            PlaySoundEffect(1);
            return result;
        }
    }
    return TARGET_PICK_WAITING;
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
GZ_ENUM_RETURN(TargetPickResult, i16) PickPartySlotTarget(i16 minimumRange, i16 mode) {
    GZ_ENUM_LOCAL(PartySlotPollResult, i16) result = PollPartySlotSelection(mode);
    if (result == PARTY_SLOT_POLL_WAITING) {
        ClearMouseClicks();
        return TARGET_PICK_WAITING;
    }
    if (result == PARTY_SLOT_POLL_CANCELLED) {
        ClearMouseClicks();
        return TARGET_PICK_CANCELLED;
    }
    if (g_hoveredObjectId == -1) {
        ClearMouseClicks();
        return TARGET_PICK_WAITING;
    }
    if (minimumRange > 0) {
        ClearMouseClicks();
        return TARGET_PICK_WAITING;
    }
    g_selectedObjectId = PartyCombatantId(g_hoveredObjectId);
    ClearMouseClicks();
    return TARGET_PICK_SELECTED;
}
