#ifndef GITEN_GAME_PARTYACTION_H
#define GITEN_GAME_PARTYACTION_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/AttackMode.h>
#include <Game/Character.h>
#include <Ui/MenuBox.h>

// An empty party position, a member that cannot be picked now, or one ready
// for command input.
GZ_ENUM_BEGIN_SPLIT(PartyMemberPickResult, i16)
    PARTY_MEMBER_EMPTY = -1,
    PARTY_MEMBER_UNAVAILABLE = 0,
    PARTY_MEMBER_READY = 1
GZ_ENUM_END_SPLIT(PartyMemberPickResult)

GZ_ENUM_RETURN(PartyMemberPickResult, i16) PickPartyMember(i16 index);
i16 FindPickablePartyMember(i16 index);
i16 CountPickablePartyMembers(void);
// Image state of a party member's panel; the missing image slot 2 is not
// produced by this state reader.
GZ_ENUM_BEGIN_SPLIT(MemberPanelState, i16)
    MEMBER_PANEL_EMPTY = -1,
    MEMBER_PANEL_READY = 0,
    MEMBER_PANEL_UNAVAILABLE = 1,
    MEMBER_PANEL_LOW_HP = 3,
    MEMBER_PANEL_FALLEN = 4
GZ_ENUM_END_SPLIT(MemberPanelState)

GZ_ENUM_RETURN(MemberPanelState, i16) GetMemberPanelState(i16 member);
i16 ReadActionResultFlags(void);
i16 GetActionCondition(Character* actor);
i16 PickActorAction(Character* actor);

// The random-target picker searches living party members, visible field
// objects, or both. -100 means neither side supplied a candidate.
GZ_ENUM_FLAGS_BEGIN(CombatantSide, u8)
    COMBATANT_SIDE_PARTY = 1,
    COMBATANT_SIDE_FIELD = 2,
    COMBATANT_SIDE_BOTH = COMBATANT_SIDE_PARTY | COMBATANT_SIDE_FIELD
GZ_ENUM_FLAGS_END(CombatantSide)
#define RANDOM_COMBATANT_NONE (-100)

i16 PickRandomCombatant(GZ_ENUM_PARAM(CombatantSide, u8) sides);
// A condition action either does nothing, handles the actor immediately, or
// queues an attack against a picked target.
GZ_ENUM_BEGIN_SPLIT(ActorActionAdjustResult, i16)
    ACTOR_ACTION_NONE = 0,
    ACTOR_ACTION_HANDLED = 1,
    ACTOR_ACTION_ATTACK_QUEUED = 2
GZ_ENUM_END_SPLIT(ActorActionAdjustResult)

GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomOpponentAttack(i16 id);
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomAllyAttack(i16 id);
GZ_ENUM_RETURN(ActorActionAdjustResult, i16) PickRandomAttack(i16 id);

b16 PickActorDialogue(i16 id);
b16 DelayActionSide(i16 id);
b16 ResetActionWaits(void);
b16 SwapPartyRows(void);
// What a condition makes an actor do, in the low nibble of its action
// (AdjustActorAction): idle, attack an opponent or an ally, flee, nothing,
// talk, delay its side, reset the action waits, attack anyone, or swap the
// party rows; the _ALIAS values act as their base.
GZ_ENUM_BEGIN(ConditionAction)
    CONDITION_ACTION_IDLE = 0,
    CONDITION_ACTION_ATTACK_OPPONENT = 1,
    CONDITION_ACTION_ATTACK_ALLY = 2,
    CONDITION_ACTION_FLEE = 3,
    CONDITION_ACTION_NONE = 4,
    CONDITION_ACTION_TALK = 5,
    CONDITION_ACTION_DELAY_SIDE = 6,
    CONDITION_ACTION_RESET_WAITS = 7,
    CONDITION_ACTION_NONE_ALIAS = 8,
    CONDITION_ACTION_ATTACK_OPPONENT_ALIAS_1 = 9,
    CONDITION_ACTION_ATTACK_ALLY_ALIAS_1 = 10,
    CONDITION_ACTION_DELAY_SIDE_ALIAS = 11,
    CONDITION_ACTION_ATTACK_OPPONENT_ALIAS_2 = 12,
    CONDITION_ACTION_ATTACK_ALLY_ALIAS_2 = 13,
    CONDITION_ACTION_ATTACK_RANDOM = 14,
    CONDITION_ACTION_SWAP_ROWS = 15
GZ_ENUM_END(ConditionAction)

// The action nibble and the flag nibble above it.
#define CONDITION_ACTION_MASK 0xf
#define CONDITION_ACTION_FLAGS_MASK 0xf0

GZ_ENUM_RETURN(ActorActionAdjustResult, i16) AdjustActorAction(i16 id, i16 action);

// @identity-TODO: text shown for unavailable commands; storage extent is unproven.
extern char g_unavailableCommandText[8];

MenuBox* OpenActorCommandMenu(i16 id);
void ActorCommandMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
i16 PollActorCommandMenu(MenuBox* menu);
i16 RunActorCommandMenu(i16 id);
b16 FormatAttackCommand(Character* actor);
b16 FormatGunCommand(Character* actor);
b16 FormatMagicCommand(Character* actor);
b16 FormatItemCommand(Character* actor);
b16 FormatCompCommand(Character* actor);
b16 FormatExtraCommand(Character* actor);
b16 FormatReturnCommand(Character* actor);
b16 FormatDefenceCommand(Character* actor);

// The command image and handler selected by each row of the character panel.
// The panel holds eight rows and the ninth handler starts an encounter.
GZ_ENUM_BEGIN_SPLIT(PanelCommandId, i16)
    PANEL_COMMAND_NONE = -1,
    PANEL_COMMAND_FIGHT = 0,
    PANEL_COMMAND_GUN = 1,
    PANEL_COMMAND_SKILL = 2,
    PANEL_COMMAND_ITEM = 3,
    PANEL_COMMAND_DEFENCE = 4,
    PANEL_COMMAND_RETURN = 5,
    PANEL_COMMAND_DDS = 6,
    PANEL_COMMAND_STATUS = 7,
    PANEL_COMMAND_ENCOUNTER = 8
GZ_ENUM_END_SPLIT(PanelCommandId)
#define PANEL_COMMAND_ROWS 8
#define PANEL_COMMAND_COUNT 9

void FillCharacterCommands(i16* list, i16 id);

i32 ScaleActionValue(i32 value, i16 resistance, i16 multiplier);
// @identity-TODO: attribute and mode are the skill/item attack domains;
// negative results encode special resistance outcomes whose names are unproven.
// The protection check either blocks the attack or allows normal resistance
// handling. Its negative special outcomes still need the battle message table.
GZ_ENUM_BEGIN_SPLIT(BattleProtectionResult, i16)
    BATTLE_PROTECTION_BLOCKED = 0,
    BATTLE_PROTECTION_NORMAL = 1
GZ_ENUM_END_SPLIT(BattleProtectionResult)

GZ_ENUM_RETURN(BattleProtectionResult, i16) CheckBattleProtection(
    Character* actor,
    i16 attribute,
    GZ_ENUM_PARAM(AttackMode, i16) mode,
    b16 report
);
i16 GetActionResistance(
    Character* actor,
    i16 attribute,
    GZ_ENUM_PARAM(AttackMode, i16) mode,
    b16 report,
    b16 sameSide
);

i16 GetSkillResistance(Character* actor, i16 skill, b16 report, b16 sameSide, i16* attribute);
i16 GetItemResistance(Character* actor, i16 item, b16 report, b16 sameSide, i16* attribute);
i16 GetPickedAttackAttribute(Character* actor, i16* condition);
void ApplyResistanceOutcome(Character* actor, i16 resistance, i32 amount);

// -1 for two party combatants, 1 for two field objects, otherwise zero.
// Which sides the current actor and target occupy: two party members, a
// mixed pair, or two field actors.
GZ_ENUM_BEGIN_SPLIT(CombatantSideRelation, i16)
    COMBATANT_RELATION_PARTY_PAIR = -1,
    COMBATANT_RELATION_MIXED = 0,
    COMBATANT_RELATION_FIELD_PAIR = 1
GZ_ENUM_END_SPLIT(CombatantSideRelation)

GZ_ENUM_RETURN(CombatantSideRelation, i16) GetCombatantSideRelation(void);

#endif // GITEN_GAME_PARTYACTION_H
