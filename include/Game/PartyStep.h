#ifndef GITEN_GAME_PARTYSTEP_H
#define GITEN_GAME_PARTYSTEP_H

#include <EnumDomain.h>

// StepParty's result: no step, a plain step, or a step through a door.
GZ_ENUM_BEGIN_SPLIT(PartyStepResult, i16)
    STEP_BLOCKED = 0,
    STEP_WALK = 1,
    STEP_DOOR = 0x10
GZ_ENUM_END_SPLIT(PartyStepResult)

#endif // GITEN_GAME_PARTYSTEP_H
