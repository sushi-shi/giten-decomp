#ifndef GITEN_GAME_FIELDVIEW_H
#define GITEN_GAME_FIELDVIEW_H

#include <rva.h>

#include <Game/GameState.h>

// The first-person view's cells: 4 rows ahead of the party (row 3 is the
// party's own) by 7 columns (column 3 is straight ahead); nonzero = drawn.
extern i16 g_viewCells[4][7];

// The directions the party faces; each steps one cell along its axis.
#define VIEW_NORTH 0 // y - 1
#define VIEW_EAST 1  // x + 1
#define VIEW_SOUTH 2 // y + 1
#define VIEW_WEST 3  // x - 1

// The map's width and height.
void GetMapSize(i16* width, i16* height);

// The direction the party faces (0..3).
// @identity-TODO: read from 0x4847ac; the owner of that word is unrecovered.
RVA_DECL(0x00049790)
i32 GetViewDirection(void);

// The draw-cell bitmap: clearing it and setting one cell.
void ClearDrawTable(void);
void MarkDrawCell(i16 index);

// Whether x/y is a drawn view cell seen from the party, and its grid distance
// from the party.
b16 GetPartyView(i16 x, i16 y);
i16 DistanceFromParty(i16 x, i16 y);

// A view cell's column (axis 0) or row stepped toward side `dir`, and the map
// cell of a view cell.
i16 StepViewCell(i16 col, i16 row, i32 dir, i16 axis);
b32 CanFloodViewCell(i16 col, i16 row, i32 direction);
void ViewCellToMapCell(i16 x, i16 y, i32 dir, i16* col, i16* row, i16 width, i16 height);

// The wall on `side` of map cell (x, y), 0 for none (kind 6 counts as none).
// @identity-TODO: what wall kind 6 is (an open door?) is unrecovered.
i32 GetWallAt(i16 x, i16 y, i32 side, i16 width, i16 height);

// Whether the view cell (col, row) seen from (x, y) facing `dir` has a wall on
// `side`; a cell off the map counts as walled.
i32 ViewCellHasWall(i16 x, i16 y, i32 dir, i16 col, i16 row, i32 side, i16 width, i16 height);

// Marks the view cells reachable from (col, row) without crossing a wall.
// previousStep prevents immediate backtracking; blockedStep keeps the flood
// from reversing its lateral direction after stepping forward.
void FloodViewCells(
    i16 x,
    i16 y,
    i32 dir,
    i16 col,
    i16 row,
    i16 previousStep,
    i16 blockedStep,
    i16 width,
    i16 height
);

// ABI: the C++ platform layer's prototypes of UpdateViewCells and
// IsCellInViewCone take int coordinates (its callers sign-extend every
// argument); the game's C callers pass words.
#ifdef __cplusplus
void UpdateViewCells(i32 x, i32 y);
#else
void UpdateViewCells(i16 x, i16 y);
#endif

// The map geometry helpers: view-cone cells, world-map blocks, turning,
// offsetting a coordinate in a direction's frame (wrapped, clamped or not),
// the walls of a cell word and how they stop a step or a sight line.
// Coordinate arguments occupy their low 16 bits; upper argument halves are unused.
#ifdef __cplusplus
b16 IsCellInViewCone(i32 x, i32 y, i32 cellX, i32 cellY);
#else
b16 IsCellInViewCone(i16 x, i16 y, i16 cellX, i16 cellY);
#endif
MapCoord GetLayerOrigin(i16 layer);
MapCoord GetWorldBlockOffset(i16 x, i16 y);
i16 GetWorldMapBlock(i16 x, i16 y);
i16 GetWorldBlockX(i16 x);
i16 GetWorldBlockY(i16 y);
MapCoord GetWorldCellAt(i16 x, i16 y);
MapCoord GetMouseWorldCell(void);
b16 IsWorldCellInMap(i16 x, i16 y);
#define OppositeDirection(direction) (((direction) - 2) & 3)

i16 TurnDirection(i16 direction, i16 turn);
static __inline void ApplyFacingOffset(i16* x, i16* y, i16 direction, i16 across, i16 along) {
    switch (direction & 3) {
        case VIEW_NORTH:
            *x += across;
            *y += along;
            break;
        case VIEW_EAST:
            *x -= along;
            *y += across;
            break;
        case VIEW_SOUTH:
            *x -= across;
            *y -= along;
            break;
        case VIEW_WEST:
            *x += along;
            *y -= across;
            break;
    }
}

MapCoord MoveMapCoord(MapCoord pos, i16 direction, i16 across, i16 along);
MapCoord OffsetCoordClamped(MapCoord pos, i16 direction, i16 across, i16 along);
void OffsetMapCoord(i16* x, i16* y, i16 dir, i16 across, i16 along);
i16 GetWallAtOffset(i16 x, i16 y, i16 dir, i16 across, i16 along);
void StepMapCoordBy(i16* x, i16* y, i16 direction, i16 across, i16 along);
i16 GetWallAtOffsetClamped(i16 x, i16 y, i16 dir, i16 across, i16 along);
i16 GetCellWall(i16 direction, i16 turn, u16 cell);
u16 GetRotatedWallAtOffset(i16 x, i16 y, i16 dir, i16 across, i16 along);
// Raw wall kind at x/y in an absolute direction; pass it to WallStops for a stop class.
i16 GetMapWallKind(i16 x, i16 y, i16 direction);
i16 StepMapCoord(i16* x, i16* y, i16 dir, i16 turn);
typedef enum WallStopMode {
    WALL_STOP_GEOMETRY = 0,
    WALL_STOP_MOVEMENT = 1
} WallStopMode;

u8 WallStops(i16 wall, i16 mode);
i16 GetCellWallStop(i16 direction, i16 turn, u16 cell);
i16 GetWallStopCode(u16 cell, i16 mode);
i32 GetFacingBit(void);
MapCoord RelativeOffset(i16 x0, i16 y0, i16 direction, i16 x1, i16 y1);
void OffsetMapCoordFacing(i16* x, i16* y, i16 direction, i16 across, i16 along);
i16 WallStopsToward(i16 x, i16 y, i16 direction, i16 turn);

// @identity-TODO: That 0x4f780 builds the 11x11 room geometry and 0x4f6e0 the camera/compass is
// read from their bodies only.
RVA_DECL(0x00049770)
void RebuildViewScene(void);

#endif // GITEN_GAME_FIELDVIEW_H
