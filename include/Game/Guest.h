#ifndef GITEN_GAME_GUEST_H
#define GITEN_GAME_GUEST_H

#include <Ints.h>

// The party position a roster member was swapped into for a pick (-1: none);
// defined by the party-pick TU (Game/partypick.c), which also keeps the slot it
// held and swaps it back through 0x43f910.
extern i16 g_guestIndex;

// @identity-TODO: unreferenced placeholder; its owner is unrecovered.
extern i16 g_guestValue;

i16 IsGuestIndex(i16 index);

#endif // GITEN_GAME_GUEST_H
