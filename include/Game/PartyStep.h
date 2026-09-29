#ifndef GITEN_GAME_PARTYSTEP_H
#define GITEN_GAME_PARTYSTEP_H

// StepParty's directions relative to the party's facing, and its results: no
// step, a plain step, or a step through a door.
#define STEP_FORWARD 0
#define STEP_RIGHT 1
#define STEP_BACK 2
#define STEP_LEFT 3
#define STEP_BLOCKED 0
#define STEP_WALK 1
#define STEP_DOOR 0x10

#endif // GITEN_GAME_PARTYSTEP_H
