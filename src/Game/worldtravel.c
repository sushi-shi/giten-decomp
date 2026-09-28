// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldMain.h>
#include <Game/FieldScreen.h>
#include <Game/FieldView.h>
#include <Game/WorldMap.h>
#include <Gfx/Scene.h>
#include <Input/Mouse.h>

#include <stdlib.h>

DATA(0x000912fc)
static i16 s_destinationY;

DATA(0x000912fe)
static i16 s_destinationX;

DATA(0x0007b720)
static i16 s_travelHistoryCount;

DATA(0x0007b500)
static u8 s_travelScores[5][5];

DATA(0x0007b520)
MapCoord g_worldTravelHistory[128];

DATA(0x000644a8)
static const u8 s_travelWeights[3][5][5] = {
    {{4, 12, 20, 16, 8},
     {36, 44, 52, 48, 40},
     {68, 76, 0, 80, 72},
     {100, 108, 116, 112, 104},
     {132, 140, 148, 144, 136}},
    {{12, 20, 16, 8, 4},
     {44, 52, 48, 40, 36},
     {76, 84, 0, 72, 68},
     {108, 116, 112, 104, 100},
     {140, 148, 144, 136, 132}},
    {{20, 16, 12, 8, 4},
     {52, 48, 44, 40, 36},
     {84, 80, 0, 72, 68},
     {116, 112, 108, 104, 100},
     {148, 144, 140, 136, 132}},
};

DATA(0x000644f8)
const u8 g_worldTravelTerrainFlags[16] = {0, 0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};

DATA(0x00068630)
static i16 s_travelHistoryLimit = 64;

DATA(0x00068a48)
i16 g_worldMapX = 286;

DATA(0x00068a4c)
i16 g_worldMapY = 192;

static __inline void SetWorldTravelDestination(MapCoord destination) {
    s_destinationX = destination.x;
    s_destinationY = destination.y;
    s_travelHistoryCount = 0;
}

RVA(0x00011660, 0xe2)
i16 PickWorldMapDestination(i16 layer) {
    MapCoord destination = PopRoutePoint();
    i16 hit;
    if (destination.x != -1 && destination.y != -1) {
        SetWorldTravelDestination(destination);
        return 1;
    }
    if (!TakeMouseLeftClick()) {
        return 0;
    }
    destination = GetMouseWorldCell();
    if (destination.x == -1 && destination.y == -1) {
        return 0;
    }
    hit = HitTestWorldMap(g_mousePosition.x, g_mousePosition.y, layer);
    if (!hit) {
        return 0;
    }
    if (hit == 2) {
        SetWorldTravelDestination(destination);
        return 1;
    }
    destination = GetMouseTravelCell();
    SetWorldTravelDestination(destination);
    return 1;
}

RVA(0x00011750, 0x122)
i16 StepWorldMapTravel(i16 layer, i16 speed) {
    MapCoord step;
    MapCoord origin;
    i16 distance;
    i16 i;
    g_worldTravelHistory[s_travelHistoryCount].x = g_worldMapX;
    g_worldTravelHistory[s_travelHistoryCount].y = g_worldMapY;
    step = ComputeWorldTravelStep(
        layer,
        g_worldMapX,
        g_worldMapY,
        s_destinationX,
        s_destinationY,
        speed
    );
    g_worldMapX += step.x;
    g_worldMapY += step.y;
    distance = abs(step.x) >= abs(step.y) ? abs(step.x) : abs(step.y);
    if (IsWorldMapMarkerNearEdge(g_worldMapX, g_worldMapY)) {
        origin = GetWorldMapViewOrigin(g_worldMapX, g_worldMapY);
        ScrollWorldMapView(origin.x, origin.y);
    }
    s_travelHistoryCount++;
    if (s_travelHistoryCount >= s_travelHistoryLimit) {
        for (i = 1; i < s_travelHistoryCount; i++) {
            g_worldTravelHistory[i - 1] = g_worldTravelHistory[i];
        }
        s_travelHistoryCount--;
    }
    if (g_mouseLeftClick && !IsRouteActive()) {
        return 0;
    }
    return distance;
}

// The Windows implementation does not use the requested speed.
RVA(0x00011880, 0x22)
MapCoord ComputeWorldTravelStep(i16 layer, i16 x, i16 y, i16 destX, i16 destY, i16 speed) {
    return FindWorldTravelStep(layer, x, y, destX, destY);
}

RVA(0x000118b0, 0xcb)
MapCoord FindWorldTravelStep(i16 layer, i16 x, i16 y, i16 destX, i16 destY) {
    MapCoord delta;
    i16 direction;
    i16 i;
    delta.x = destX - x;
    delta.y = destY - y;
    direction = GetWorldTravelDirection(delta.x, delta.y);
    LoadWorldTravelCandidates(layer, x, y, direction);
    MarkReachableWorldTravelCells(2, 2);
    WeightWorldTravelCandidates(GetWorldTravelLateralDelta(delta.x, delta.y, direction));
    for (i = 0; i < s_travelHistoryCount; i++) {
        ExcludeWorldTravelStep(
            g_worldTravelHistory[i].x - x,
            g_worldTravelHistory[i].y - y,
            direction
        );
    }
    PreferWorldTravelDestination(delta.x, delta.y, direction);
    return GetBestWorldTravelStep(direction);
}

RVA(0x00011980, 0x43)
i16 GetWorldTravelDirection(i16 x, i16 y) {
    if (abs(x) < abs(y)) {
        return y < 0 ? VIEW_NORTH : VIEW_SOUTH;
    }
    return x < 0 ? VIEW_WEST : VIEW_EAST;
}

RVA(0x000119d0, 0x40)
i16 GetWorldTravelLateralDelta(i16 x, i16 y, i16 direction) {
    switch (direction) {
        case VIEW_NORTH:
            return x;
        case VIEW_EAST:
            return y;
        case VIEW_SOUTH:
            return -x;
        case VIEW_WEST:
            return -y;
        default:
            return 0;
    }
}

RVA(0x00011a10, 0x120)
void LoadWorldTravelCandidates(i16 layer, i16 x, i16 y, i16 direction) {
    i16 row;
    i16 column;
    u8 code = 0;
    for (row = -2; row <= 2; row++) {
        for (column = -2; column <= 2; column++) {
            switch (direction) {
                case VIEW_NORTH:
                    code = GetWorldMapCellCode(layer, x + column, y - row);
                    break;
                case VIEW_EAST:
                    code = GetWorldMapCellCode(layer, x + row, y + column);
                    break;
                case VIEW_SOUTH:
                    code = GetWorldMapCellCode(layer, x - column, y + row);
                    break;
                case VIEW_WEST:
                    code = GetWorldMapCellCode(layer, x - row, y - column);
                    break;
            }
            code = g_worldTravelTerrainFlags[code];
            s_travelScores[row + 2][column + 2] = code;
        }
    }
}

RVA(0x00011b30, 0x10e)
void MarkReachableWorldTravelCells(i16 row, i16 column) {
    i16 scan;
    if (s_travelScores[row][column] & 2) {
        return;
    }
    if (!(s_travelScores[row][column] & 1)) {
        return;
    }
    for (scan = column; scan >= 0; scan--) {
        if ((s_travelScores[row][scan] & 3) != 1) {
            break;
        }
        s_travelScores[row][scan] |= 2;
        if (row < 4 && (s_travelScores[row + 1][scan] & 3) == 1) {
            MarkReachableWorldTravelCells(row + 1, scan);
        }
        if (row > 0 && (s_travelScores[row - 1][scan] & 3) == 1) {
            MarkReachableWorldTravelCells(row - 1, scan);
        }
    }
    for (scan = column + 1; scan <= 4; scan++) {
        if ((s_travelScores[row][scan] & 3) != 1) {
            break;
        }
        s_travelScores[row][scan] |= 2;
        if (row < 4 && (s_travelScores[row + 1][scan] & 3) == 1) {
            MarkReachableWorldTravelCells(row + 1, scan);
        }
        if (row > 0 && (s_travelScores[row - 1][scan] & 3) == 1) {
            MarkReachableWorldTravelCells(row - 1, scan);
        }
    }
}

RVA(0x00011c40, 0xad)
void WeightWorldTravelCandidates(i16 lateral) {
    i16 magnitude;
    i16 row;
    i16 column;
    if (lateral < -2) {
        lateral = -2;
    } else if (lateral > 2) {
        lateral = 2;
    }
    magnitude = abs(lateral);
    lateral = lateral >= 0 ? -1 : 1;
    for (row = -2; row <= 2; row++) {
        u8* scores = s_travelScores[row + 2] + 2;
        for (column = -2; column <= 2; column++) {
            if (scores[column] & 2) {
                scores[column] |= s_travelWeights[magnitude][row + 2][column * lateral + 2];
            }
        }
    }
}

RVA(0x00011cf0, 0x84)
void PreferWorldTravelDestination(i16 x, i16 y, i16 direction) {
    i16 row = 0;
    i16 column = 0;
    GetWorldTravelGridOffset(x, y, direction, &row, &column);
    if (row >= -2 && row <= 2 && column >= -2 && column <= 2) {
        if (s_travelScores[row + 2][column + 2] & 2) {
            s_travelScores[row + 2][column + 2] |= 0xfc;
        }
    }
}

RVA(0x00011d80, 0xd0)
MapCoord GetBestWorldTravelStep(i16 direction) {
    i16 bestRow = 0;
    i16 bestColumn = 0;
    u8 bestScore = 0;
    i16 row;
    i16 column;
    MapCoord step;
    for (row = -2; row <= 2; row++) {
        u8* scores = s_travelScores[row + 2] + 2;
        for (column = -2; column <= 2; column++) {
            if (scores[column] > bestScore) {
                bestRow = row;
                bestColumn = column;
                bestScore = scores[column];
            }
        }
    }
    step.x = step.y = 0;
    switch (direction) {
        case VIEW_NORTH:
            step.x = bestColumn;
            step.y = -bestRow;
            break;
        case VIEW_EAST:
            step.x = bestRow;
            step.y = bestColumn;
            break;
        case VIEW_SOUTH:
            step.x = -bestColumn;
            step.y = bestRow;
            break;
        case VIEW_WEST:
            step.x = -bestRow;
            step.y = -bestColumn;
            break;
    }
    return step;
}

RVA(0x00011e50, 0x7c)
void ExcludeWorldTravelStep(i16 x, i16 y, i16 direction) {
    i16 row = 0;
    i16 column = 0;
    GetWorldTravelGridOffset(x, y, direction, &row, &column);
    if (row >= -2 && row <= 2 && column >= -2 && column <= 2) {
        s_travelScores[row + 2][column + 2] = 0;
    }
}
