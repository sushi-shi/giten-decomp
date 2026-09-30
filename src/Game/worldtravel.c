// @identity-TODO: the owning TU is unproven. One retail object: the world-map
// travel code, the route queue and the world-map place names. Their .bss
// statics form one run ahead of fieldmain's, and their initialized data forms
// one .data run out of .text order (the place-name words, then the
// travel-history limit) closed by the place-name code's literal, before
// fieldmain's .data.

#include <rva.h>

#include <EnumDomain.h>
#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/FieldScreen.h>
#include <Game/FieldView.h>
#include <Game/InfoBar.h>
#include <Game/WorldMap.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Text/TextAttr.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stdlib.h>
#include <string.h>

DATA(0x00068628)
static i16 s_shownPlace = -1;

DATA(0x0006862c)
static i16 s_place = -1;

DATA(0x00068630)
static i16 s_travelHistoryLimit = 64;

DATA(0x000912fc)
i16 g_destinationY;

DATA(0x000912fe)
i16 g_destinationX;

// The travel grid covers WORLD_TRAVEL_RADIUS cells each way around the party;
// a cell's score holds TRAVEL_PASSABLE and TRAVEL_REACHABLE, with its weight in
// the bits above them.
#define WORLD_TRAVEL_RADIUS 2
#define WORLD_TRAVEL_SPAN (2 * WORLD_TRAVEL_RADIUS + 1)
GZ_ENUM_FLAGS_BEGIN(WorldTravelCellFlag, u8)
    TRAVEL_PASSABLE = 1,
    TRAVEL_REACHABLE = 2
GZ_ENUM_FLAGS_END(WorldTravelCellFlag)
#define TRAVEL_CELL_STATE (TRAVEL_PASSABLE | TRAVEL_REACHABLE)

DATA(0x0007b500)
static u8 s_travelScores[WORLD_TRAVEL_SPAN][WORLD_TRAVEL_SPAN] = {0};

DATA(0x0007b520)
MapCoord g_worldTravelHistory[128] = {0};

DATA(0x0007b720)
static i16 s_travelHistoryCount = 0;

DATA(0x0007b724)
static i32 s_placeGrid = 0;
DATA(0x0007b728)
static i32 s_placeNames = 0;

// The world-map route queue (MapCoord points in a memory handle): its
// capacity, read and write positions, and whether a route is being walked.
DATA(0x0007b72c)
static i32 s_route = 0;

DATA(0x0007b730)
static i16 s_routeCapacity = 0;

DATA(0x0007b734)
static i16 s_routeRead = 0;

DATA(0x0007b738)
static i16 s_routeCount = 0;

DATA(0x0007b73c)
static b16 s_routeActive = false;

static __inline void SetWorldTravelDestination(MapCoord destination) {
    g_destinationX = destination.x;
    g_destinationY = destination.y;
    s_travelHistoryCount = 0;
}

RVA(0x00011660, 0xe2)
b16 PickWorldMapDestination(i16 layer) {
    MapCoord destination = PopRoutePoint();
    i16 hit;
    if (destination.x != MAP_COORD_NONE && destination.y != MAP_COORD_NONE) {
        SetWorldTravelDestination(destination);
        return true;
    }
    if (!TakeMouseLeftClick()) {
        return false;
    }
    destination = GetMouseWorldCell();
    if (destination.x == MAP_COORD_NONE && destination.y == MAP_COORD_NONE) {
        return false;
    }
    hit = HitTestWorldMap(g_mousePosition.x, g_mousePosition.y, layer);
    if (!hit) {
        return false;
    }
    if (hit == WORLD_MAP_HIT_MARKER_COLOR) {
        SetWorldTravelDestination(destination);
        return true;
    }
    destination = GetMouseTravelCell();
    SetWorldTravelDestination(destination);
    return true;
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
        g_destinationX,
        g_destinationY,
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
    MarkReachableWorldTravelCells(WORLD_TRAVEL_RADIUS, WORLD_TRAVEL_RADIUS);
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

DATA(0x000644a8)
static const u8 s_travelWeights[3][WORLD_TRAVEL_SPAN][WORLD_TRAVEL_SPAN] = {
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

RVA(0x00011a10, 0x120)
void LoadWorldTravelCandidates(i16 layer, i16 x, i16 y, i16 direction) {
    i16 row;
    i16 column;
    u8 code = 0;
    for (row = -WORLD_TRAVEL_RADIUS; row <= WORLD_TRAVEL_RADIUS; row++) {
        for (column = -WORLD_TRAVEL_RADIUS; column <= WORLD_TRAVEL_RADIUS; column++) {
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
            s_travelScores[row + WORLD_TRAVEL_RADIUS][column + WORLD_TRAVEL_RADIUS] = code;
        }
    }
}

RVA(0x00011b30, 0x10e)
void MarkReachableWorldTravelCells(i16 row, i16 column) {
    i16 scan;
    if (s_travelScores[row][column] & TRAVEL_REACHABLE) {
        return;
    }
    if (!(s_travelScores[row][column] & TRAVEL_PASSABLE)) {
        return;
    }
    for (scan = column; scan >= 0; scan--) {
        if ((s_travelScores[row][scan] & TRAVEL_CELL_STATE) != TRAVEL_PASSABLE) {
            break;
        }
        s_travelScores[row][scan] |= TRAVEL_REACHABLE;
        if (row < WORLD_TRAVEL_SPAN - 1
            && (s_travelScores[row + 1][scan] & TRAVEL_CELL_STATE) == TRAVEL_PASSABLE) {
            MarkReachableWorldTravelCells(row + 1, scan);
        }
        if (row > 0 && (s_travelScores[row - 1][scan] & TRAVEL_CELL_STATE) == TRAVEL_PASSABLE) {
            MarkReachableWorldTravelCells(row - 1, scan);
        }
    }
    for (scan = column + 1; scan <= WORLD_TRAVEL_SPAN - 1; scan++) {
        if ((s_travelScores[row][scan] & TRAVEL_CELL_STATE) != TRAVEL_PASSABLE) {
            break;
        }
        s_travelScores[row][scan] |= TRAVEL_REACHABLE;
        if (row < WORLD_TRAVEL_SPAN - 1
            && (s_travelScores[row + 1][scan] & TRAVEL_CELL_STATE) == TRAVEL_PASSABLE) {
            MarkReachableWorldTravelCells(row + 1, scan);
        }
        if (row > 0 && (s_travelScores[row - 1][scan] & TRAVEL_CELL_STATE) == TRAVEL_PASSABLE) {
            MarkReachableWorldTravelCells(row - 1, scan);
        }
    }
}

RVA(0x00011c40, 0xad)
void WeightWorldTravelCandidates(i16 lateral) {
    i16 magnitude;
    i16 row;
    i16 column;
    if (lateral < -WORLD_TRAVEL_RADIUS) {
        lateral = -WORLD_TRAVEL_RADIUS;
    } else if (lateral > WORLD_TRAVEL_RADIUS) {
        lateral = WORLD_TRAVEL_RADIUS;
    }
    magnitude = abs(lateral);
    lateral = lateral >= 0 ? -1 : 1;
    for (row = -WORLD_TRAVEL_RADIUS; row <= WORLD_TRAVEL_RADIUS; row++) {
        u8* scores = s_travelScores[row + WORLD_TRAVEL_RADIUS] + WORLD_TRAVEL_RADIUS;
        for (column = -WORLD_TRAVEL_RADIUS; column <= WORLD_TRAVEL_RADIUS; column++) {
            if (scores[column] & TRAVEL_REACHABLE) {
                scores[column] |= s_travelWeights[magnitude][row + WORLD_TRAVEL_RADIUS]
                                                 [column * lateral + WORLD_TRAVEL_RADIUS];
            }
        }
    }
}

RVA(0x00011cf0, 0x84)
void PreferWorldTravelDestination(i16 x, i16 y, i16 direction) {
    i16 row = 0;
    i16 column = 0;
    GetWorldTravelGridOffset(x, y, direction, &row, &column);
    if (row >= -WORLD_TRAVEL_RADIUS && row <= WORLD_TRAVEL_RADIUS && column >= -WORLD_TRAVEL_RADIUS
        && column <= WORLD_TRAVEL_RADIUS) {
        if (s_travelScores[row + WORLD_TRAVEL_RADIUS][column + WORLD_TRAVEL_RADIUS]
            & TRAVEL_REACHABLE) {
            s_travelScores[row + WORLD_TRAVEL_RADIUS][column + WORLD_TRAVEL_RADIUS] |= 0xfc;
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
    for (row = -WORLD_TRAVEL_RADIUS; row <= WORLD_TRAVEL_RADIUS; row++) {
        u8* scores = s_travelScores[row + WORLD_TRAVEL_RADIUS] + WORLD_TRAVEL_RADIUS;
        for (column = -WORLD_TRAVEL_RADIUS; column <= WORLD_TRAVEL_RADIUS; column++) {
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
    if (row >= -WORLD_TRAVEL_RADIUS && row <= WORLD_TRAVEL_RADIUS && column >= -WORLD_TRAVEL_RADIUS
        && column <= WORLD_TRAVEL_RADIUS) {
        s_travelScores[row + WORLD_TRAVEL_RADIUS][column + WORLD_TRAVEL_RADIUS] = 0;
    }
}

// Makes room for `more` points in the world-map route queue (starting it
// active when it was empty).
RVA(0x00011ed0, 0x48)
void GrowRoute(i16 more) {
    if (!s_route) {
        s_routeRead = 0;
        s_routeCount = 0;
        s_routeActive = true;
    }
    s_routeCapacity += more;
    s_route = ResizeHandle(s_route, s_routeCapacity * 4);
}

RVA(0x00011f20, 0x28)
void FreeRoute(void) {
    s_route = FreeHandle(s_route);
    s_routeCapacity = 0;
    s_routeRead = 0;
    s_routeCount = 0;
}

RVA(0x00011f50, 0x3e)
void PushRoutePoint(MapCoord point) {
    if (s_routeCount >= s_routeCapacity) {
        GrowRoute(1);
    }
    ((MapCoord*)HandleWritePtr(s_route))[s_routeCount] = point;
    s_routeCount++;
}

// The next route point (MAP_COORD_NONE twice, and inactive, when the route is
// done).
RVA(0x00011f90, 0x7d)
MapCoord PopRoutePoint(void) {
    MapCoord point;
    point.x = MAP_COORD_NONE;
    point.y = MAP_COORD_NONE;
    if (!s_route) {
        s_routeActive = false;
        return point;
    }
    if (s_routeRead >= s_routeCount) {
        FreeRoute();
        s_routeActive = false;
        return point;
    }
    point = ((MapCoord*)HandleReadPtr(s_route))[s_routeRead++];
    if (s_routeRead >= s_routeCount) {
        FreeRoute();
    }
    return point;
}

RVA(0x00012010, 0x7)
i16 IsRouteActive(void) {
    return s_routeActive;
}

RVA(0x00012020, 0x41)
void LoadWorldMapPlaces(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_WORLD_PLACES, DATA_FILE_TABLE, 0);
    s_placeGrid = ReadCryptHandle(fp);
    s_placeNames = ReadCryptHandle(fp);
    CloseDataFile(fp);
    s_shownPlace = -1;
}

RVA(0x00012070, 0x31)
void FreeWorldMapPlaces(void) {
    s_placeGrid = FreeHandle(s_placeGrid);
    s_placeNames = FreeHandle(s_placeNames);
    s_shownPlace = -1;
}

static __inline void* GetWorldMapPlaceEntry(i32 handle, i16 index) {
    WorldMapPlaceOffsets* table = HandleReadPtr(handle);
    return OffsetBy(table, table->offsets[index]);
}

RVA(0x000120b0, 0x8f)
char* FormatWorldMapPlaceAt(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    MapCoord offset = GetWorldBlockOffset(x, y);
    WorldMapPlaceGrid* grid;
    offset.x /= 32;
    offset.y /= 40;
    block *= 2;
    grid = GetWorldMapPlaceEntry(s_placeGrid, block);
    return FormatWorldMapPlace(grid->places[offset.y][offset.x]);
}

RVA(0x00012140, 0x58)
char* FormatWorldMapPlace(i16 place) {
    const char* name;
    s_place = place;
    name = GetWorldMapPlaceEntry(s_placeNames, place);
    strcpy(g_scratchBuffer, name);
    return g_scratchBuffer;
}

RVA(0x000121a0, 0x34)
void ShowWorldMapPlaceName(i16 x, i16 y, i16 force) {
    FormatWorldMapPlaceAt(x, y);
    if (force) {
        s_shownPlace = -1;
    }
    DrawWorldMapPlaceName(s_place);
}

RVA(0x000121e0, 0x55)
void DrawWorldMapPlaceName(i16 place) {
    i16 saved;
    if (place != s_shownPlace) {
        s_shownPlace = place;
        saved = SaveDrawState();
        ClearLocationCaption();
        DrawLayerText(
            SCREEN_LAYER_LOCATION,
            8,
            8,
            g_scratchBuffer,
            TEXT_ATTR_OPAQUE | TEXT_ATTR_FLAG1
                | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK)
        );
        RestoreDrawState(saved);
    }
}

RVA(0x00012240, 0x18)
char* FormatWorldMapLocation(void) {
    return FormatWorldMapPlaceAt(g_worldMapX, g_worldMapY);
}
