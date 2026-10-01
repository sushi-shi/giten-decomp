// @identity-TODO: the owning TU is unproven; this unit holds the view
// occlusion pass until link-order evidence names it.

#include <rva.h>

#include <Game/AreaMap.h>
#include <Game/FieldHud.h>
#include <Game/FieldSight.h>
#include <Game/FieldView.h>
#include <Game/TreasureBox.h>
#include <Game/ViewCellAxis.h>
#include <Game/WorldMap.h>
#include <Input/Mouse.h>
#include <Platform/PlatformApi.h>
#include <Util/PixelMask.h>
#include <Util/Range.h>

#include <string.h>

// A view cell's column and row.
typedef struct ViewCell {
    i16 col;
    i16 row;
} ViewCell;

// Cells through which visibility may spread; the party's side cells are
// enabled only during the flood, then hidden from the rendered view.
DATA(0x000684e8)
static i16 s_viewFloodMask[4][7] = {
    {1, 1, 1, 1, 1, 1, 1},
    {0, 1, 1, 1, 1, 1, 0},
    {0, 0, 1, 1, 1, 0, 0},
    {0, 0, 0, 1, 0, 0, 0},
};

// @identity-TODO: no code in this image reads these 28 words; they hold
// seven zero-terminated runs of up to four cell numbers (see
// s_viewCellNumbers), but whether they form one table, and of which shape,
// is unproven.
DATA(0x00068520)
static i16 s_unusedViewCellLists[7][4] = {
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {2, 9, 12, 0},
    {2, 3, 4, 9},
    {5, 10, 11, 0},
    {5, 6, 7, 10},
    {8, 0, 0, 0},
};

// The number k of each view cell (zero off the flood mask): the inverse of
// s_viewCellOrder. No code in this image reads it.
DATA(0x00068558)
static i16 s_viewCellNumbers[4][7] = {
    {1, 2, 3, 4, 5, 6, 7},
    {0, 8, 9, 10, 11, 12, 0},
    {0, 0, 13, 14, 15, 0, 0},
    {0, 0, 0, 16, 0, 0, 0},
};

// The view cells in the order the occlusion pass visits them (cell k at
// k - 1): the far row left to right, then each nearer row. A seventeenth,
// zero entry fills the table's last four bytes.
DATA(0x00068590)
static ViewCell s_viewCellOrder[17] = {
    {0, 0},
    {1, 0},
    {2, 0},
    {3, 0},
    {4, 0},
    {5, 0},
    {6, 0},
    {1, 1},
    {2, 1},
    {3, 1},
    {4, 1},
    {5, 1},
    {2, 2},
    {3, 2},
    {4, 2},
    {3, 3},
};

// The draw-cell bitmap (0x1000 cells).
DATA(0x00078540)
static u8 s_drawTable[0x200];

DATA(0x00078740)
i16 g_viewCells[4][7] = {0};

// Clears the draw-cell bitmap.
RVA(0x0000bdd0, 0x11)
void ClearDrawTable(void) {
    memset(s_drawTable, 0, sizeof(s_drawTable));
}

// Sets draw cell `index` in the bitmap.
RVA(0x0000bdf0, 0x23)
void MarkDrawCell(i16 index) {
    s_drawTable[(i16)(index / 8)] |= GetPixelMask(index);
}

// Whether x/y is a drawn view cell seen from the party.
RVA(0x0000be20, 0x28)
b16 GetPartyView(i16 x, i16 y) {
    MapCoord party = GetMapCoord();
    return IsCellInViewCone(party.x, party.y, x, y);
}

// The grid distance of x/y from the party.
RVA(0x0000be50, 0x28)
i16 DistanceFromParty(i16 x, i16 y) {
    MapCoord party = GetMapCoord();
    return GridDistance(party.x, party.y, x, y);
}

// The column (axis 0) or row of view cell col/row stepped one cell toward
// side `dir` (east/west move the column, north/south the row).
RVA(0x0000be80, 0x46)
i16 StepViewCell(i16 col, i16 row, GZ_ENUM_PARAM(ViewDirection, i32) dir, i16 axis) {
    if (axis == VIEW_CELL_COLUMN) {
        switch (dir) {
            case VIEW_EAST:
                col++;
                break;
            case VIEW_WEST:
                col--;
                break;
        }
        return col;
    }
    switch (dir) {
        case VIEW_NORTH:
            row--;
            break;
        case VIEW_SOUTH:
            row++;
            break;
    }
    return row;
}

RVA(0x0000bed0, 0xe4)
b32 CanFloodViewCell(i16 col, i16 row, GZ_ENUM_PARAM(ViewDirection, i32) direction) {
    switch (direction & 3) {
        case VIEW_NORTH:
            if (row - 1 >= 0 && s_viewFloodMask[row - 1][col]) {
                return true;
            }
            break;
        case VIEW_EAST:
            if (col + 1 < 7 && s_viewFloodMask[row][col + 1]) {
                return true;
            }
            break;
        case VIEW_SOUTH:
            if (row + 1 < 4 && s_viewFloodMask[row + 1][col]) {
                return true;
            }
            break;
        case VIEW_WEST:
            if (col - 1 >= 0 && s_viewFloodMask[row][col - 1]) {
                return true;
            }
            break;
    }
    return false;
}

// Turns view cell *col/*row (party at column 3, row 3) seen from x/y facing
// `dir` into its map cell, -1 for a coordinate off the width x height map.
RVA(0x0000bfc0, 0xc8)
void ViewCellToMapCell(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i32) dir,
    i16* col,
    i16* row,
    i16 width,
    i16 height
) {
    *col += -3;
    *row += -3;
    switch (dir) {
        case VIEW_NORTH:
            x += *col;
            y += *row;
            break;
        case VIEW_EAST:
            x -= *row;
            y += *col;
            break;
        case VIEW_SOUTH:
            x -= *col;
            y -= *row;
            break;
        case VIEW_WEST:
            x += *row;
            y -= *col;
            break;
    }
    *col = x;
    *row = y;
    if (*col < 0 || *col >= width) {
        *col = -1;
    }
    if (*row < 0 || *row >= height) {
        *row = -1;
    }
}

// The wall on `side` of map cell (x, y), 0 for none.
RVA(0x0000c090, 0x2d)
i32 GetWallAt(i16 x, i16 y, i32 side, i16 width, i16 height) {
    i32 wall = GetWallCode(x, y, side, width, height);
    if (wall == WALL_KIND_INVISIBLE_BARRIER) {
        wall = WALL_KIND_NONE;
    }
    return wall;
}

// Whether the view cell (col, row) seen from (x, y) facing `dir` has a wall on
// `side`; a cell off the map counts as walled.
RVA(0x0000c0c0, 0x5d)
i32 ViewCellHasWall(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i32) dir,
    i16 col,
    i16 row,
    i32 side,
    i16 width,
    i16 height
) {
    ViewCellToMapCell(x, y, dir, &col, &row, width, height);
    if (col == -1 || row == -1) {
        return 1;
    }
    return GetWallAt(col, row, side, width, height);
}

RVA(0x0000c120, 0x208)
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
) {
    i32 side;
    if (x < 0) {
        x += width;
    } else if (x >= width) {
        x -= width;
    }
    if (y < 0) {
        y += height;
    } else if (y >= height) {
        y -= height;
    }
    if (row < 0 || row >= 4 || col < 0 || col >= 7) {
        return;
    }
    g_viewCells[row][col] = 1;
    side = (dir - 1) & 3;
    if (!GetWallAt(x, y, side, width, height) && CanFloodViewCell(col, row, VIEW_WEST)
        && previousStep != VIEW_EAST && blockedStep != VIEW_WEST) {
        FloodViewCells(
            StepViewCell(x, y, side, 0),
            StepViewCell(x, y, side, 1),
            dir,
            col - 1,
            row,
            VIEW_WEST,
            VIEW_EAST,
            width,
            height
        );
    }
    if (!GetWallAt(x, y, dir & 3, width, height) && CanFloodViewCell(col, row, VIEW_NORTH)) {
        FloodViewCells(
            StepViewCell(x, y, dir, VIEW_CELL_COLUMN),
            StepViewCell(x, y, dir, VIEW_CELL_ROW),
            dir,
            col,
            row - 1,
            VIEW_NORTH,
            blockedStep,
            width,
            height
        );
    }
    side = (dir + 1) & 3;
    if (!GetWallAt(x, y, side, width, height) && CanFloodViewCell(col, row, VIEW_EAST)
        && previousStep != VIEW_WEST && blockedStep != VIEW_EAST) {
        FloodViewCells(
            StepViewCell(x, y, side, 0),
            StepViewCell(x, y, side, 1),
            dir,
            col + 1,
            row,
            VIEW_EAST,
            VIEW_WEST,
            width,
            height
        );
    }
}

static __inline void HideLeftViewEdge(int k) {
    if (k == 1) {
        g_viewCells[0][0] = 0;
    }
    if (k == 2) {
        g_viewCells[0][1] = 0;
    }
    if (k == 8) {
        g_viewCells[1][1] = 0;
        g_viewCells[0][0] = 0;
    }
    if (k == 13) {
        g_viewCells[2][2] = 0;
        g_viewCells[1][1] = 0;
        g_viewCells[0][1] = 0;
        g_viewCells[0][0] = 0;
    }
}

static __inline void HideLeftViewWedge(int k) {
    if (k == 1) {
        g_viewCells[0][0] = 0;
    }
    if (k == 2) {
        g_viewCells[0][1] = 0;
    }
    if (k == 8) {
        g_viewCells[1][1] = 0;
        g_viewCells[0][1] = 0;
        g_viewCells[0][0] = 0;
    }
    if (k == 13) {
        g_viewCells[2][2] = 0;
        g_viewCells[1][2] = 0;
        g_viewCells[1][1] = 0;
        g_viewCells[0][2] = 0;
        g_viewCells[0][1] = 0;
        g_viewCells[0][0] = 0;
    }
}

static __inline void HideRightViewEdge(int k) {
    if (k == 7) {
        g_viewCells[0][6] = 0;
    }
    if (k == 6) {
        g_viewCells[0][5] = 0;
    }
    if (k == 12) {
        g_viewCells[1][5] = 0;
        g_viewCells[0][6] = 0;
    }
    if (k == 15) {
        g_viewCells[2][4] = 0;
        g_viewCells[1][5] = 0;
        g_viewCells[0][5] = 0;
        g_viewCells[0][6] = 0;
    }
}

static __inline void HideRightViewWedge(int k) {
    if (k == 7) {
        g_viewCells[0][6] = 0;
    }
    if (k == 6) {
        g_viewCells[0][5] = 0;
    }
    if (k == 12) {
        g_viewCells[1][5] = 0;
        g_viewCells[0][5] = 0;
        g_viewCells[0][6] = 0;
    }
    if (k == 15) {
        g_viewCells[2][4] = 0;
        g_viewCells[1][4] = 0;
        g_viewCells[1][5] = 0;
        g_viewCells[0][4] = 0;
        g_viewCells[0][5] = 0;
        g_viewCells[0][6] = 0;
    }
}

// Recomputes which view cells are drawn for the party at (x, y): every cell
// the flood reaches, minus the cells hidden behind the walls beside nearer
// cells.
RVA(0x0000c330, 0x870)
void UpdateViewCells(i16 x, i16 y) {
    int dir;
    i16 width;
    i16 height;
    i16 col;
    i16 row;
    int side;
    int k;

    memset(g_viewCells, 0, sizeof(g_viewCells));
    dir = GetViewDirection();
    GetMapSize(&width, &height);
    s_viewFloodMask[3][4] = 1;
    s_viewFloodMask[3][2] = 1;
    FloodViewCells(x, y, dir, 3, 3, 0, 0, width, height);
    s_viewFloodMask[3][4] = 0;
    s_viewFloodMask[3][2] = 0;
    g_viewCells[3][4] = 0;
    g_viewCells[3][2] = 0;
    for (k = 1; k < 17; k++) {
        col = s_viewCellOrder[k - 1].col;
        row = s_viewCellOrder[k - 1].row;
        switch (k) {
            case 1:
            case 2:
            case 8:
            case 13:
                side = (dir - 1) & 3;
                if (ViewCellHasWall(x, y, dir, col + 1, row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col, row + 1, dir, width, height)) {
                    HideLeftViewEdge(k);
                }
                if (ViewCellHasWall(x, y, dir, col + 1, row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col + 1, row + 1, side, width, height)) {
                    HideLeftViewEdge(k);
                }
                if (ViewCellHasWall(x, y, dir, col + 1, ++row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col + 1, row, dir, width, height)) {
                    HideLeftViewWedge(k);
                }
                if (ViewCellHasWall(x, y, dir, col, row, dir, width, height)
                    && ViewCellHasWall(x, y, dir, col + 1, row, dir, width, height)) {
                    HideLeftViewWedge(k);
                }
                break;
            case 3:
            case 9:
                if ((ViewCellHasWall(x, y, dir, col + 1, row, (dir - 1) & 3, width, height)
                     && ViewCellHasWall(x, y, dir, col, row + 1, dir, width, height))
                    || (ViewCellHasWall(x, y, dir, col, ++row, dir, width, height)
                        && ViewCellHasWall(x, y, dir, col + 1, row, dir, width, height))) {
                    if (k == 3) {
                        g_viewCells[0][2] = 0;
                    }
                    if (k == 9) {
                        g_viewCells[1][2] = 0;
                        g_viewCells[0][1] = 0;
                    }
                }
                break;
            case 6:
            case 7:
            case 12:
            case 15:
                side = (dir + 1) & 3;
                if (ViewCellHasWall(x, y, dir, col - 1, row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col, row + 1, dir, width, height)) {
                    HideRightViewEdge(k);
                }
                if (ViewCellHasWall(x, y, dir, col - 1, row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col - 1, row + 1, side, width, height)) {
                    HideRightViewEdge(k);
                }
                if (ViewCellHasWall(x, y, dir, col - 1, ++row, side, width, height)
                    && ViewCellHasWall(x, y, dir, col - 1, row, dir, width, height)) {
                    HideRightViewWedge(k);
                }
                if (ViewCellHasWall(x, y, dir, col, row, dir, width, height)
                    && ViewCellHasWall(x, y, dir, col - 1, row, dir, width, height)) {
                    HideRightViewWedge(k);
                }
                break;
            case 5:
            case 11:
                if ((ViewCellHasWall(x, y, dir, col - 1, row, (dir + 1) & 3, width, height)
                     && ViewCellHasWall(x, y, dir, col, row + 1, dir, width, height))
                    || (ViewCellHasWall(x, y, dir, col, ++row, dir, width, height)
                        && ViewCellHasWall(x, y, dir, col - 1, row, dir, width, height))) {
                    if (k == 5) {
                        g_viewCells[0][4] = 0;
                    }
                    if (k == 11) {
                        g_viewCells[1][4] = 0;
                        g_viewCells[0][5] = 0;
                    }
                }
                break;
            case 4:
            case 10:
            case 14:
                if (ViewCellHasWall(x, y, dir, col, ++row, dir, width, height)) {
                    if (k == 4) {
                        g_viewCells[0][3] = 0;
                    }
                    if (k == 10) {
                        g_viewCells[1][3] = 0;
                        g_viewCells[0][3] = 0;
                    }
                    if (k == 14) {
                        g_viewCells[2][3] = 0;
                        g_viewCells[1][3] = 0;
                        g_viewCells[0][3] = 0;
                    }
                }
                break;
            case 16:
                if (GetWallAt(x, y, dir, width, height)) {
                    g_viewCells[0][4] = 0;
                    g_viewCells[0][3] = 0;
                    g_viewCells[0][2] = 0;
                    g_viewCells[2][3] = 0;
                    g_viewCells[1][3] = 0;
                }
                break;
        }
    }
}

// Geometry (mode 0) and movement (mode 1) class of each wall kind: 0 open,
// 1/2 a door or a partial wall, 3 a solid wall. Kind 6 is invisible but blocks
// movement; kind 12 is drawn but passable.
DATA(0x000642f8)
static const u8 s_wallStops[16][2] = {
    {0, 0},
    {1, 1},
    {1, 2},
    {3, 3},
    {3, 3},
    {3, 3},
    {0, 3},
    {3, 3},
    {3, 3},
    {3, 3},
    {3, 3},
    {1, 2},
    {3, 0},
    {3, 3},
    {3, 3},
    {3, 3},
};

// The weight of each side's stop code in GetWallStopCode (base 3, first side
// most significant).
DATA(0x000685d8)
static i16 s_sideWeights[4] = {27, 9, 3, 1};

// Whether cell (cellX, cellY) is a drawn view cell seen from (x, y) facing
// the view direction.
RVA(0x0000cba0, 0xc0)
b16 IsCellInViewCone(i16 x, i16 y, i16 cellX, i16 cellY) {
    i16 row;
    i16 col;
    switch (GetViewDirection()) {
        case VIEW_NORTH:
            row = cellY - y + 3;
            col = cellX - x + 3;
            break;
        case VIEW_EAST:
            row = x - cellX + 3;
            col = cellY - y + 3;
            break;
        case VIEW_SOUTH:
            row = y - cellY + 3;
            col = x - cellX + 3;
            break;
        case VIEW_WEST:
            row = cellX - x + 3;
            col = y - cellY + 3;
            break;
    }
    if (row > 3) {
        return false;
    }
    return g_viewCells[row][col] != 0;
}

// The origin of world-map layer `layer` (a 288x200 cell of an 8-wide sheet;
// two layers per cell).
RVA(0x0000cc60, 0x5c)
MapCoord GetLayerOrigin(i16 layer) {
    MapCoord origin;
    i16 cell = layer / 2;
    origin.x = cell % WORLD_BLOCK_COLUMNS;
    origin.y = cell / WORLD_BLOCK_COLUMNS;
    origin.x *= WORLD_BLOCK_WIDTH;
    origin.y *= WORLD_BLOCK_HEIGHT;
    return origin;
}

// A world position within its block.
RVA(0x0000ccc0, 0x29)
MapCoord GetWorldBlockOffset(i16 x, i16 y) {
    MapCoord offset;
    offset.x = x % WORLD_BLOCK_WIDTH;
    offset.y = y % WORLD_BLOCK_HEIGHT;
    return offset;
}

// The world-map block (8 across) holding a world position.
RVA(0x0000ccf0, 0x32)
i16 GetWorldMapBlock(i16 x, i16 y) {
    i16 row = y / WORLD_BLOCK_HEIGHT;
    return x / WORLD_BLOCK_WIDTH + row * WORLD_BLOCK_COLUMNS;
}

RVA(0x0000cd30, 0x11)
i16 GetWorldBlockX(i16 x) {
    return x % WORLD_BLOCK_WIDTH;
}

RVA(0x0000cd50, 0x11)
i16 GetWorldBlockY(i16 y) {
    return y % WORLD_BLOCK_HEIGHT;
}

#define IsPointInWorldView(x, y) ((x) >= 0 && (x) < 0x280 && (y) >= 0 && (y) < 0x148)

// The world cell under view point x/y (-1/-1 outside the 640x328 view).
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0000cd70, 0x4b)
MapCoord GetWorldCellAt(i16 x, i16 y) {
    MapCoord cell;
    cell.y = MAP_COORD_NONE;
    cell.x = MAP_COORD_NONE;
    if (IsPointInWorldView(x, y)) {
        cell = GetWorldViewOrigin();
        cell.x += x;
        cell.y += y;
    }
    return cell;
}

// The world cell under the mouse (-1/-1 outside the 640x328 view).
RVA(0x0000cdc0, 0x83)
MapCoord GetMouseWorldCell(void) {
    i16 block = GetWorldBlock();
    MapCoord cell;
    cell.y = MAP_COORD_NONE;
    cell.x = MAP_COORD_NONE;
    if (IsPointInWorldView(g_mousePosition.x, g_mousePosition.y)) {
        cell.x = g_mousePosition.x + 0x70 + block % WORLD_BLOCK_COLUMNS * WORLD_BLOCK_WIDTH;
        cell.y = g_mousePosition.y + 0x24 + block / WORLD_BLOCK_COLUMNS * WORLD_BLOCK_HEIGHT;
    }
    return cell;
}

// Whether a world cell lies inside the map.
RVA(0x0000ce50, 0x29)
b16 IsWorldCellInMap(i16 x, i16 y) {
    if (x >= 0 && x < WORLD_BLOCK_COLUMNS * WORLD_BLOCK_WIDTH && y >= 0
        && y < WORLD_BLOCK_ROWS * WORLD_BLOCK_HEIGHT) {
        return true;
    }
    return false;
}

// `direction` turned by `turn` quarter turns.
RVA(0x0000ce80, 0xe)
GZ_ENUM_RETURN(ViewDirection, i16) TurnDirection(GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 turn) {
    return (u8)(direction + turn) & 3;
}

// `pos` moved by `across`/`along` in the frame of `direction`, wrapped into
// the level.
RVA(0x0000ce90, 0x90)
MapCoord
MoveMapCoord(MapCoord pos, GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 across, i16 along) {
    ApplyFacingOffset(&pos.x, &pos.y, direction, across, along);
    WrapMapPosition(&pos.x, &pos.y);
    return pos;
}

// The same, clamped into the level.
RVA(0x0000cf20, 0x90)
MapCoord OffsetCoordClamped(
    MapCoord pos,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16 across,
    i16 along
) {
    ApplyFacingOffset(&pos.x, &pos.y, direction, across, along);
    ClampMapPosition(&pos.x, &pos.y);
    return pos;
}

// Moves x/y by `across`/`along` in the frame of `dir` (wrapped).
RVA(0x0000cfb0, 0x4d)
void OffsetMapCoord(i16* x, i16* y, GZ_ENUM_PARAM(ViewDirection, i16) dir, i16 across, i16 along) {
    MapCoord pos;
    pos.x = *x;
    pos.y = *y;
    pos = MoveMapCoord(pos, dir, across, along);
    *x = pos.x;
    *y = pos.y;
}

// The wall word of the cell `across`/`along` from x/y (wrapped).
RVA(0x0000d000, 0x49)
i16 GetWallAtOffset(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) dir, i16 across, i16 along) {
    MapCoord pos;
    pos.x = x;
    pos.y = y;
    pos = MoveMapCoord(pos, dir, across, along);
    return RevealAreaMapAt(pos.x, pos.y);
}

// Moves x/y by `across`/`along` in the frame of `direction` (clamped).
RVA(0x0000d050, 0x4d)
void StepMapCoordBy(
    i16* x,
    i16* y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16 across,
    i16 along
) {
    MapCoord pos;
    pos.x = *x;
    pos.y = *y;
    pos = OffsetCoordClamped(pos, direction, across, along);
    *x = pos.x;
    *y = pos.y;
}

// The wall word of the cell `across`/`along` from x/y (clamped).
RVA(0x0000d0a0, 0x49)
i16 GetWallAtOffsetClamped(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) dir,
    i16 across,
    i16 along
) {
    MapCoord pos;
    pos.x = x;
    pos.y = y;
    pos = OffsetCoordClamped(pos, dir, across, along);
    return RevealAreaMapAt(pos.x, pos.y);
}

// The wall kind on side `turn` (relative to `direction`) of a map cell word:
// one nibble per side.
RVA(0x0000d0f0, 0x60)
GZ_ENUM_RETURN(WallKind, i16)
GetCellWall(GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 turn, u16 cell) {
    switch (TurnDirection(direction, turn)) {
        case VIEW_NORTH:
            return cell & 0xf;
        case VIEW_EAST:
            return cell >> 4 & 0xf;
        case VIEW_SOUTH:
            return cell >> 8 & 0xf;
        case VIEW_WEST:
            return cell >> 12;
    }
    return 0;
}

// The wall word `across`/`along` from x/y, turned into the frame of `dir`.
RVA(0x0000d150, 0x2c)
u16 GetRotatedWallAtOffset(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) dir,
    i16 across,
    i16 along
) {
    return RotateByDirection(GetWallAtOffset(x, y, dir, across, along), dir);
}

// The raw wall kind on the map cell's `direction` side (zero means no wall).
RVA(0x0000d180, 0x23)
GZ_ENUM_RETURN(WallKind, i16)
GetMapWallKind(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction) {
    return GetCellWall(direction, 0, RevealAreaMapAt(x, y));
}

// Steps x/y one cell toward `turn` of `dir` (wrapped); the direction taken.
RVA(0x0000d1b0, 0x3f)
i16 StepMapCoord(
    i16* x,
    i16* y,
    GZ_ENUM_PARAM(ViewDirection, i16) dir,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
) {
    i16 facing = TurnDirection(dir, turn);
    OffsetMapCoord(x, y, facing, 0, -1);
    WrapMapPosition(x, y);
    return facing;
}

// The rendered geometry or movement-blocking class of a wall kind.
RVA(0x0000d1f0, 0x14)
u8 WallStops(i16 wall, GZ_ENUM_PARAM(WallStopMode, i16) mode) {
    return s_wallStops[(u8)wall & 0xf][mode];
}

// The rendered geometry class on side `turn` of `direction` in a cell word.
RVA(0x0000d210, 0x27)
i16 GetCellWallStop(GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 turn, u16 cell) {
    i16 wall = GetCellWall(direction, turn, cell);
    return WallStops(wall, WALL_STOP_GEOMETRY);
}

// The four sides' stop codes (mode `mode`; doors and partial walls folded to
// 1/2) as one base-3 number.
RVA(0x0000d240, 0x5d)
i16 GetWallStopCode(u16 cell, GZ_ENUM_PARAM(WallStopMode, i16) mode) {
    i16 code = 0;
    i16 i;
    i16 stop;
    for (i = 0; i < 4; i++) {
        cell = RotateByDirection(cell, 3);
        stop = WallStops(cell, mode);
        if (stop == 2) {
            stop = true;
        }
        if (stop == WALL_STOP_SOLID) {
            stop = 2;
        }
        code += stop * s_sideWeights[i];
    }
    return code;
}

// 1 << the party's facing direction.
// @early-stop retail loads the direction word (mov cx, word) as the shift
// count, here only its byte; i16/u16 locals and casts tried.
RVA(0x0000d2a0, 0xf)
i32 GetFacingBit(void) {
    return 1 << g_party.field.pos.direction;
}

// x1/y1 relative to x0/y0 in the frame of `direction`.
RVA(0x0000d2b0, 0x68)
MapCoord
RelativeOffset(i16 x0, i16 y0, GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 x1, i16 y1) {
    i16 dx = x1 - x0;
    i16 dy = y1 - y0;
    MapCoord offset;
    switch (direction & 3) {
        case VIEW_EAST:
            offset.x = dy;
            offset.y = -dx;
            break;
        case VIEW_SOUTH:
            offset.x = -dx;
            offset.y = -dy;
            break;
        case VIEW_WEST:
            offset.x = -dy;
            offset.y = dx;
            break;
        default:
            offset.x = dx;
            offset.y = dy;
            break;
    }
    return offset;
}

// Moves x/y by `across`/`along` in the frame of `direction` (unwrapped).
RVA(0x0000d320, 0x88)
void OffsetMapCoordFacing(
    i16* x,
    i16* y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16 across,
    i16 along
) {
    ApplyFacingOffset(x, y, direction, across, along);
}

// The movement-blocking class from x/y toward `turn` of `direction`.
RVA(0x0000d3b0, 0x35)
i16 WallStopsToward(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
) {
    i16 wall = GetCellWall(direction, turn, RevealAreaMapAt(x, y));
    return WallStops(wall, WALL_STOP_MOVEMENT);
}
