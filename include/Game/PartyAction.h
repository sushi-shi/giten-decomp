#ifndef GITEN_GAME_PARTYACTION_H
#define GITEN_GAME_PARTYACTION_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/Character.h>
#include <Ui/MenuBox.h>

i16 PickPartyMember(i16 index);
i16 FindPickablePartyMember(i16 index);
i16 CountPickablePartyMembers(void);
i16 GetMemberPanelState(i16 member);
i16 ReadActionResultFlags(void);
i16 GetActionCondition(Character* actor);
i16 PickActorAction(Character* actor);

i16 PickRandomCombatant(u8 sides);
i16 PickRandomOpponentAttack(i16 id);
i16 PickRandomAllyAttack(i16 id);
i16 PickRandomAttack(i16 id);

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

i16 AdjustActorAction(i16 id, i16 action);

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

void FillCharacterCommands(i16* list, i16 id);

i32 ScaleActionValue(i32 value, i16 resistance, i16 multiplier);
// @identity-TODO: attribute and mode are the skill/item attack domains;
// negative results encode special resistance outcomes whose names are unproven.
i16 CheckBattleProtection(Character* actor, i16 attribute, i16 mode, i16 report);
i16 GetActionResistance(Character* actor, i16 attribute, i16 mode, i16 report, i16 sameSide);

i16 GetSkillResistance(Character* actor, i16 skill, i16 report, i16 sameSide, i16* attribute);
i16 GetItemResistance(Character* actor, i16 item, i16 report, i16 sameSide, i16* attribute);
i16 GetPickedAttackAttribute(Character* actor, i16* condition);
void ApplyResistanceOutcome(Character* actor, i16 resistance, i32 amount);

// -1 for two party combatants, 1 for two field objects, otherwise zero.
i16 GetCombatantSideRelation(void);

#endif // GITEN_GAME_PARTYACTION_H
