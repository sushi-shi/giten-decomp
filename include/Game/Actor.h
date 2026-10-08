#ifndef GITEN_GAME_ACTOR_H
#define GITEN_GAME_ACTOR_H

#include <rva.h>

#include <Game/Character.h>

// Map actors' behaviour state.

// Sets the actor's attitude when state is nonnegative, then activates it.
// @identity-TODO: personalFlags bits 8/10 are unproven; decode RunObjectStep's reads of them.
void AlertActor(CharacterCore* actor, GZ_ENUM_PARAM(Attitude, i16) state);

#endif // GITEN_GAME_ACTOR_H
