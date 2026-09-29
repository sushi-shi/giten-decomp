#ifndef GITEN_GAME_WORLDMAP_H
#define GITEN_GAME_WORLDMAP_H

#include <rva.h>

#include <Game/FieldView.h>
#include <Ints.h>

// A world-map transition request consumed and cleared by its state machine:
// negative leaves the map; positive marks the saved spot on entry.
extern i16 g_worldMapRequest;

// Copies the current world-map location name into the shared text buffer.
char* FormatWorldMapLocation(void);

// The party's position on the world map.
extern i16 g_worldMapX;
extern i16 g_worldMapY;

// Set when the field screen must be redrawn; RedrawScreen latches refresh requests.
extern i16 g_fieldRedrawRequest;

// The shared information text plane created during game startup.
extern i16 g_infoPlane;

#include <Game/GameState.h>

typedef struct WorldEncounterChoices {
    i16 groups[6];
} WorldEncounterChoices;

typedef struct WorldEncounterWeights {
    u8 weights[6];
} WorldEncounterWeights;

typedef struct WorldEncounterVariant {
    u16 condition;
    u8 chance;
    u8 maximum;
    u8 weights;
    u8 choices;
} WorldEncounterVariant;

typedef struct WorldEncounterCell {
    u8 fieldTable;
    WorldEncounterVariant variants[2];
} WorldEncounterCell;

static __inline WorldEncounterVariant*
GetWorldEncounterVariant(WorldEncounterCell* cell, i16 index) {
    return &cell->variants[index];
}

typedef struct EncounterFieldImage {
    u8 image;
    u8 variant;
    u8 option; // @identity-TODO: copied to the field image's unused option word.
} EncounterFieldImage;

void FreeEncounterTables(void);
void LoadEncounterTables(void);
void LoadFieldTable(void);
void PrepareFieldRandom(void);
i16 LoadWorldEncounterBlock(i16 x, i16 y);
i16 PickWorldEncounterGroup(i16 weights, i16 choices);
i16 CheckWorldEncounterInterval(void);
b16 TestWorldEncounterChance(i16 chance);
i16 PrepareWorldEncounter(i16 weights, i16 choices, i16 maximum);
i16 GetWorldEncounterMaximum(i16 maximum);
i16 GetPartyEncounterSizeBonus(void);
i16 AssignWorldEncounterGroups(i16 count);
// Group selection for each member of an encounter, capped at sixteen.
extern u8 g_worldEncounterGroupSlots[16];

// @identity-TODO: What leader personal flag 0x22 is (it suppresses encounters) is unrecovered.
i16 RollWorldMapEncounter(i16 x, i16 y);

// @identity-TODO: What word 0x47be68 (passed as layer; bit 0 picks surface 0x48f5e4 vs 0x48d714
// in 0x581b0) distinguishes is unrecovered.
b16 PickWorldMapDestination(i16 layer);
i16 GetWorldTravelDirection(i16 x, i16 y);
i16 GetWorldTravelLateralDelta(i16 x, i16 y, i16 direction);

i16 StepWorldMapTravel(i16 layer, i16 speed);
MapCoord ComputeWorldTravelStep(i16 layer, i16 x, i16 y, i16 destX, i16 destY, i16 speed);
MapCoord FindWorldTravelStep(i16 layer, i16 x, i16 y, i16 destX, i16 destY);
void LoadWorldTravelCandidates(i16 layer, i16 x, i16 y, i16 direction);
void MarkReachableWorldTravelCells(i16 row, i16 column);
void WeightWorldTravelCandidates(i16 lateral);
// Invalid directions leave the caller's row and column unchanged.
static __inline void GetWorldTravelGridOffset(i16 x, i16 y, i16 direction, i16* row, i16* column) {
    switch (direction) {
        case VIEW_NORTH:
            *row = -y;
            *column = x;
            break;
        case VIEW_EAST:
            *row = x;
            *column = y;
            break;
        case VIEW_SOUTH:
            *row = y;
            *column = -x;
            break;
        case VIEW_WEST:
            *row = -x;
            *column = -y;
            break;
    }
}

void PreferWorldTravelDestination(i16 x, i16 y, i16 direction);
// Initial reachability flags indexed by the four-bit map terrain code.
extern const u8 g_worldTravelTerrainFlags[16];

// The 128-cell travel history buffer.
extern MapCoord g_worldTravelHistory[];
MapCoord GetBestWorldTravelStep(i16 direction);
void ExcludeWorldTravelStep(i16 x, i16 y, i16 direction);

// The world-map route queue: points walked one per travel step.
void GrowRoute(i16 more);
void FreeRoute(void);
void PushRoutePoint(MapCoord point);
MapCoord PopRoutePoint(void);
i16 IsRouteActive(void);

// The place-id grid for a map block, divided into 32-by-40-pixel cells.
typedef struct WorldMapPlaceGrid {
    u8 places[5][9];
} WorldMapPlaceGrid;

// A count followed by byte offsets to place grids or null-terminated names.
typedef struct WorldMapPlaceOffsets {
    u16 count;
    u16 offsets[1];
} WorldMapPlaceOffsets;

char* FormatWorldMapPlaceAt(i16 x, i16 y);
char* FormatWorldMapPlace(i16 place);
void DrawWorldMapPlaceName(i16 place);

void LoadWorldMapPlaces(void);

void FreeWorldMapPlaces(void);

void ShowWorldMapPlaceName(i16 x, i16 y, i16 force);

// @identity-TODO: the dword is only cleared and swapped in this build.
typedef struct WorldMapBlock {
    u32 reserved;
    i16 index;
} WorldMapBlock;

// Overlay presence for the 88 map blocks and the alternate block image.
extern i16 g_worldMapOverlayFlags[89];

void LoadWorldMapBlockImage(i16 block, i16 slot);
void ClearWorldMapBlock(i16 slot);
void SwapWorldMapBlocks(i16 first, i16 second);
void DrawWorldMapBlock(i16 slot, i16 x, i16 y);
void RotateWorldMapBlocks(i16 x, i16 y);
void LoadWorldMapBlocks(i16 block);

// @identity-TODO: What image 0x47b804 is (never loaded in this build) is unrecovered.
void ResetWorldMapBlocks(i16 freeOverlay);

void ScrollWorldMapView(i16 x, i16 y);

static __inline void ClampWorldMapViewOrigin(MapCoord* origin) {
    if (origin->x < 0) {
        origin->x = 0;
    }
    if (origin->x > 1664) {
        origin->x = 1664;
    }
    if (origin->y < 0) {
        origin->y = 0;
    }
    if (origin->y > 1872) {
        origin->y = 1872;
    }
    origin->x &= ~7;
    origin->y &= ~7;
}

MapCoord GetWorldMapViewOrigin(i16 x, i16 y);
MapCoord GetCenteredWorldMapViewOrigin(i16 x, i16 y);
MapCoord GetWorldViewBlockDelta(i16 x, i16 y);
u8 GetWorldMapCellCode(i16 layer, i16 x, i16 y);

// @identity-TODO: What area handle 0x47b810 saves is unrecovered (no writer in this build;
// 0x167e0 restores it through 0x3900 before freeing).
void FreeWorldMapScreenSave(void);
void RestoreWorldMapCursor(void);

// @identity-TODO: Its role beside FreeWorldMapScreenSave (it is only a jmp to it, also reached
// from 0x16800 after the restoring 0x167e0) is unproven.
void DiscardWorldMapScreenSave(void);

// @identity-TODO: this restore-and-discard entry has no callers in this build.
void RestoreWorldMapScreenSave(void);

i16 TickStepCounter(void);

// A world-map event spot, in coordinates local to its map block.
typedef struct WorldMapEvent {
    i16 x;
    i16 y;
    u8 pad04;
    u8 flagBank;
    u8 flagIndex;
    u8 script;
    u8 entry;
    // @identity-TODO: the remaining scene parameters are not fully decoded.
    u8 params[7];
} WorldMapEvent;

// One offset for each map block and layer; each selects an event list
// terminated by x == -1.
typedef struct WorldMapEventTable {
    u16 count;
    u16 offsets[1];
} WorldMapEventTable;

void FreeWorldMapEvents(void);
void LoadWorldMapEvents(void);

void MarkWorldMapEventSpot(i16 x, i16 y);

b16 CheckWorldMapEvent(i16 x, i16 y);

b16 RunWorldMap(void);

// Sets the world-map layer and the spot (offset by the layer's origin
// 0x40cc60) the world map opens at.
void SetWorldMapSpot(i16 layer, i16 x, i16 y);

#endif // GITEN_GAME_WORLDMAP_H
