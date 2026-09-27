#ifndef GITEN_GAME_DOORREGION_H
#define GITEN_GAME_DOORREGION_H

#include <Ints.h>

// The payload following a door region's rectangle or default-region marker.
typedef struct DoorRegionData {
    u8 flagBank;
    u8 flagIndex;
    u8 invertFlag;
    i16 objects[2];
    i16 spawnInterval;
} DoorRegionData;

DoorRegionData* GetCellObjectTable(i16 code);
i16 LookupCellObject(DoorRegionData* table, i16 layer);

#endif // GITEN_GAME_DOORREGION_H
