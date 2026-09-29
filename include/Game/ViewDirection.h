#ifndef GITEN_GAME_VIEWDIRECTION_H
#define GITEN_GAME_VIEWDIRECTION_H

#include <Enums.h>

// The directions the party and the map objects face; each steps one cell
// along its axis.
GZ_ENUM_BEGIN(ViewDirection)
    VIEW_NORTH = 0, // y - 1
    VIEW_EAST = 1,  // x + 1
    VIEW_SOUTH = 2, // y + 1
    VIEW_WEST = 3
        // x - 1
GZ_ENUM_END(ViewDirection)

#endif // GITEN_GAME_VIEWDIRECTION_H
