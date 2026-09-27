// @identity-TODO: the owning TU is unproven; this unit holds the world-map
// place-name span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/FieldView.h>
#include <Game/InfoBar.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/VramAccess.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <string.h>

DATA(0x00068628)
static i16 s_shownPlace = -1;
DATA(0x0006862c)
static i16 s_place = -1;
DATA(0x0007b724)
static i32 s_placeGrid;
DATA(0x0007b728)
static i32 s_placeNames;

RVA(0x00012020, 0x41)
void LoadWorldMapPlaces(void) {
    FILE* fp = OpenDataFile(13, 12, 0);
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
        DrawLayerText(SCREEN_LAYER_LOCATION, 8, 8, g_scratchBuffer, 0x3400);
        RestoreDrawState(saved);
    }
}

RVA(0x00012240, 0x18)
char* FormatWorldMapLocation(void) {
    return FormatWorldMapPlaceAt(g_worldMapX, g_worldMapY);
}
