#ifndef GITEN_GAME_PARTYCOMMAND_H
#define GITEN_GAME_PARTYCOMMAND_H

#include <rva.h>

#include <Ints.h>

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

// Selects a target within the inclusive range; kind bits 0/1 permit field
// objects/party slots, and bit 2 opens the roster list. Returns 1 on selection,
// 0 while waiting, or -1 on cancellation; the target is g_selectedObjectId.
i16 RunPickTargetWindow(i16 minimumRange, i16 maximumRange, i16 kind, i16 id);
b16 PickFieldObjectTarget(i16 minimumRange, i16 maximumRange);
i16 PickPartySlotTarget(i16 minimumRange, i16 mode);

// The sixteen-byte screen-save area used while picking a target.
extern u8 g_pickScreenSave[];

i16 RunPartyCommandInput(void);

#endif // GITEN_GAME_PARTYCOMMAND_H
