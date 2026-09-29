#ifndef GITEN_GAME_PARTYCOMMAND_H
#define GITEN_GAME_PARTYCOMMAND_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

// The command picker waits for a member, prepares its role, runs its action
// menu, picks a target, confirms the action, then may pick a summon position.
GZ_ENUM_BEGIN_SPLIT(PartyCommandPhase, i16)
    PARTY_COMMAND_WAIT_MEMBER = 0,
    PARTY_COMMAND_PREPARE_MEMBER = 1,
    PARTY_COMMAND_PICK_ACTION = 2,
    PARTY_COMMAND_PICK_TARGET = 3,
    PARTY_COMMAND_CONFIRM = 4,
    PARTY_COMMAND_PICK_SUMMON_POSITION = 5
GZ_ENUM_END_SPLIT(PartyCommandPhase)

// Eight action rows are shown beneath the actor's name.
#define ACTOR_COMMAND_COUNT 8
#define ACTOR_COMMAND_MENU_ROWS (ACTOR_COMMAND_COUNT + 1)

// Callees of the party command-input machine (0x409620), label-only until
// their TUs are claimed. Kept out of the field headers: field.c's codegen is
// sensitive to their declaration count.

// Sets member `id`'s pick target from its role (the equipped weapon or item
// for roles 1/2); returns how many command-input steps that settles.
i16 PrepareMemberPickTarget(i16 id);

// The party position of roster member `id`, or -1.
i16 FindPartyPositionOfId(i16 id);

// @identity-TODO: tests combatant placement against the party cell; negative
// ids select the party side. Mode zero's object test requires equal x and unequal y.
b16 HasObjectInReach(i16 mode, i16 first, i16 second);

// Target sources selected by RunPickTargetWindow's low three kind bits.
GZ_ENUM_FLAGS_BEGIN(TargetPickKind, i16)
    TARGET_PICK_FIELD_OBJECT = 1,
    TARGET_PICK_PARTY_SLOT = 2,
    TARGET_PICK_ROSTER_LIST = 4
GZ_ENUM_FLAGS_END(TargetPickKind)

// The target picker runs over frames until a target is selected or cancelled.
GZ_ENUM_BEGIN_SPLIT(TargetPickResult, i16)
    TARGET_PICK_CANCELLED = -1,
    TARGET_PICK_WAITING = 0,
    TARGET_PICK_SELECTED = 1
GZ_ENUM_END_SPLIT(TargetPickResult)

// Selects a target within the inclusive range; kind bits 0/1 permit field
// objects/party slots, and bit 2 opens the roster list. Returns 1 on selection,
// 0 while waiting, or -1 on cancellation; the target is g_selectedObjectId.
GZ_ENUM_RETURN(TargetPickResult, i16) RunPickTargetWindow(i16 minimumRange, i16 maximumRange, i16 kind, i16 id);
b16 PickFieldObjectTarget(i16 minimumRange, i16 maximumRange);
GZ_ENUM_RETURN(TargetPickResult, i16) PickPartySlotTarget(i16 minimumRange, i16 mode);

// The sixteen-byte screen-save area used while picking a target.
extern u8 g_pickScreenSave[];

i16 RunPartyCommandInput(void);

#endif // GITEN_GAME_PARTYCOMMAND_H
