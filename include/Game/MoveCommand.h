#ifndef GITEN_GAME_MOVECOMMAND_H
#define GITEN_GAME_MOVECOMMAND_H

#include <EnumDomain.h>

// A party move as RunMoveCommand runs it; a queue of moves ends at
// MOVE_QUEUE_END.
GZ_ENUM_BEGIN_SPLIT(MoveCommand, i16)
    MOVE_UNAVAILABLE = -1,
    MOVE_FORWARD = 0,
    MOVE_RIGHT = 1,
    MOVE_BACK = 2,
    MOVE_LEFT = 3,
    MOVE_TURN_RIGHT = 4,
    MOVE_TURN_AROUND = 5,
    MOVE_TURN_LEFT = 6,
    MOVE_NONE = 7,
    MOVE_QUEUE_END = 0xff
GZ_ENUM_END_SPLIT(MoveCommand)

#endif // GITEN_GAME_MOVECOMMAND_H
