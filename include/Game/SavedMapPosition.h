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

#define SetSavedMapPosition(position, areaValue, levelValue, xValue, yValue, directionValue)       \
    do {                                                                                           \
        (position)->area = (areaValue);                                                            \
        (position)->level = (levelValue);                                                          \
        (position)->x = (xValue);                                                                  \
        (position)->y = (yValue);                                                                  \
        (position)->direction = (directionValue);                                                  \
    } while (0)

#endif // GITEN_GAME_SAVEDMAPPOSITION_H
