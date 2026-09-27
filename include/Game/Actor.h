#ifndef GITEN_GAME_ACTOR_H
#define GITEN_GAME_ACTOR_H

#include <rva.h>

#include <Game/Character.h>

// Map actors' behaviour state.

// Sets the actor's attitude when state is nonnegative, then activates it.
// @identity-TODO: personalFlags bits 8/10 and mode 6 are unproven;
// decode 0xf890's reads of them.
void AlertActor(Character* actor, i16 state);

#endif // GITEN_GAME_ACTOR_H
