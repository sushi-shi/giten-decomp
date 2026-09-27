#ifndef GITEN_GAME_SAVEDMAPPOSITION_H
#define GITEN_GAME_SAVEDMAPPOSITION_H

#include <Ints.h>

typedef struct SavedMapPosition {
    u8 area;
    u8 level;
    u8 x;
    u8 y;
    u8 direction;
} SavedMapPosition;

#endif // GITEN_GAME_SAVEDMAPPOSITION_H
