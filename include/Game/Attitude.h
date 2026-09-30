#ifndef GITEN_GAME_ATTITUDE_H
#define GITEN_GAME_ATTITUDE_H

#include <Enums.h>

// An actor's attitude toward the party, named by the analyze window's five
// attitude names.
GZ_ENUM_BEGIN(Attitude)
    ATTITUDE_PLEADING = 0,
    ATTITUDE_FRIENDLY = 1,
    ATTITUDE_VERY_HOSTILE = 2,
    ATTITUDE_HOSTILE = 3,
    ATTITUDE_NORMAL = 4,
    ATTITUDE_COUNT = 5
GZ_ENUM_END(Attitude)

#endif // GITEN_GAME_ATTITUDE_H
