#ifndef GITEN_GAME_ROOMREGION_H
#define GITEN_GAME_ROOMREGION_H

// The room-region grid: REGION_GRID_SIZE cells square, a region byte each.
// Rooms number their regions from 0 and doors from REGION_DOOR (a door's
// region has that bit set); REGION_NONE marks a cell in neither. Room-list
// entries are ROOM_ENTRY_SIZE bytes, door-list entries DOOR_ENTRY_SIZE.
#define REGION_GRID_SIZE 64
#define REGION_NONE 0xff
#define REGION_DOOR 0x80
#define ROOM_ENTRY_SIZE 0x1f
#define DOOR_ENTRY_SIZE 0xd

#endif // GITEN_GAME_ROOMREGION_H
