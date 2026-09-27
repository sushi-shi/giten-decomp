// @identity-TODO: the owning TU is unproven; this unit holds the area map's
// cell-event span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/AreaLevel.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/Automap.h>
#include <Game/AutomapData.h>
#include <Game/Field.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/ObjectRecord.h>
#include <Game/Scene.h>
#include <Game/WorldMap.h>
#include <Mem/Alloc.h>
#include <Script/EventFlags.h>
#include <Sound/Sound.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <string.h>

DATA(0x000712a4)
static i16 s_cellX;

DATA(0x000712a8)
static i16 s_cellY;

DATA(0x000712ac)
u8 g_cellCode;

DATA(0x000712b0)
static u8 s_cellDestDirection;

DATA(0x000712b4)
i16 g_cellDestX;

DATA(0x000712b8)
i16 g_cellDestY;

DATA(0x000712bc)
i16 g_cellDestLevel;

DATA(0x000712c0)
i16 g_cellDestArea;

// The kind of each special cell code.
DATA(0x00068e50)
static const CellKind s_cellKinds[] = {
    {CELL_STAIRS_UP, CELL_EVENT_STAIRS, 0, 0},
    {CELL_STAIRS_DOWN, CELL_EVENT_STAIRS, 0, 0},
    {0x90, CELL_EVENT_STAIRS, 0, 0},
    {0x91, CELL_EVENT_STAIRS, 0, 0},
    {0x70, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x71, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x72, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x73, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x74, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x75, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x76, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x64, CELL_EVENT_FORCED_MOVE, 0, 0},
    {0x77, CELL_EVENT_MARKED_WARP, 0, 0},
    {CELL_EXIT, CELL_EVENT_WORLD_EXIT, 0, 0},
    {0x7f, CELL_EVENT_FROZEN_SCENE, CELL_KIND_CHECK_FACING, 0},
    {0x79, CELL_EVENT_SCRIPT, CELL_KIND_CHECK_FACING, 0},
    {0x7c, 10, 0, 0},
    {0x8b, 10, 0, 0},
    {0x8c, 10, 0, 0},
    {0x8d, 10, 0, 0},
    {0x8e, 10, 0, 0},
    {0x8f, 10, 0, 0},
    {CELL_CHUTE, CELL_EVENT_CHUTE, 0, 0},
    {0x88, 12, 0, 0},
    {0x89, 12, 0, 0},
    {0x65, 13, 0, 0},
    {0xff, CELL_EVENT_NONE, 0, 0},
};

DATA(0x0007fe54)
AreaMap* g_areaMap;

// The encrypted area-map record as read from the data file (decoded into
// g_areaMap).
DATA(0x0007d640)
static u8 s_areaRecord[0x2800];

// One-shot preservation of event flags across the next area or level load.
// SetAreaFlagPreservation accepts -1 to keep either mode unchanged.
DATA(0x0007fe40)
static i16 s_preserveAreaFlags;

DATA(0x0007fe44)
static i16 s_preserveLevelFlags;

// The level SelectAreaLevel last selected.
DATA(0x00068f34)
static i16 s_currentLevel = -1;

RVA(0x00020fc0, 0x23)
void SetAreaFlagPreservation(i16 area, i16 level) {
    if (area != -1) {
        s_preserveAreaFlags = area;
    }
    if (level != -1) {
        s_preserveLevelFlags = level;
    }
}

// Stores the automap level and drops the current area and level.
RVA(0x00020ff0, 0x1e)
void UnloadAreaMap(void) {
    StoreAutomapLevel();
    if (g_areaMap) {
        g_areaMap->area = -1;
    }
    g_areaLevel = NULL;
}

// The level of the area map the party is on (NULL without a map).
DATA(0x0007fe58)
AreaLevel* g_areaLevel;

// The name returned without an area map.
DATA(0x0007fe5c)
static char s_noAreaName[4];

// Spawns the level's map objects standing on cells of code `cellCode` (the
// spawn interval is the code's rate): each 4-byte spawn entry (x | layer
// bit 7, y, flag bank, flag index) whose flag allows it raises its object
// event and spawns facing a random direction. Unless the level's event bit
// is set. Two spots are special-cased (area 0x10 level 6 skips entry x 4 on
// row 0; area 0x13 level 4 moves an entry at x 9 from row 2 to row 1).
// @identity-TODO: why those two spots are special-cased is unrecovered.
RVA(0x00021010, 0x147)
void SpawnMapObjects(i16 cellCode) {
    u8 special = 0;
    i16 index = 0;
    MapSpawn* entry;
    i16 layer;
    i16 object;
    SetSpawnInterval(GetCellSpawnRate(cellCode));
    if (TestLevelEvent(g_field.pos.level) == 1) {
        return;
    }
    if (!g_areaLevel) {
        return;
    }
    if (g_field.pos.area == 0x10 && g_field.pos.level == 6) {
        special = 1;
    } else if (g_field.pos.area == 0x13 && g_field.pos.level == 4) {
        special = 2;
    }
    for (entry = g_areaLevel->spawns; entry->xLayer != 0xff; entry++, index++) {
        if (special == 1 && GetMapSpawnX(entry) == 4 && entry->y == 0) {
            continue;
        }
        if (special == 2 && GetMapSpawnX(entry) == 9 && entry->y == special) {
            entry->y = 1;
        }
        if (!IsSpawnEnabled(entry)) {
            RaiseObjectEvent(index, 0);
            continue;
        }
        if (!RaiseObjectEvent(index, 1)) {
            continue;
        }
        if (GetMapCellCode(GetMapSpawnX(entry), entry->y) != cellCode) {
            continue;
        }
        layer = GetMapSpawnLayer(entry);
        object =
            SpawnMapObject(layer, GetMapSpawnX(entry), entry->y, RandomAverage(0, 3, 0), index);
        SetObjectEventFlag(object, entry->flagBank, entry->flagIndex);
    }
}

// Whether a spawn entry's flag allows it: bit 7 of the bank byte asks for the
// flag set, else clear; bank and index 0xff always allow it.
// @early-stop: retail xors into the flag byte (xor al,bl) and returns it
// zero-extended; here the bank register takes the result.
RVA(0x00021160, 0x43)
i16 IsSpawnEnabled(MapSpawn* entry) {
    u8 bank = entry->flagBank;
    u8 index = entry->flagIndex;
    u8 result;
    if (bank == 0xff && index == bank) {
        return 1;
    }
    result = IsEventFlagSet(bank, index) ? 0x80 : 0;
    result = ~result;
    result ^= bank;
    result >>= 7;
    return result;
}

// Marks the level's room regions (with `detectChanges`, returns whether the
// regions changed instead).
RVA(0x000211b0, 0x38)
i16 BuildRoomMap(i16 detectChanges) {
    if (!g_areaLevel) {
        return 0;
    }
    if (detectChanges) {
        return RoomRegionsChanged(
            g_areaLevel->roomWalls,
            g_areaLevel->roomDoors,
            g_areaLevel->width,
            g_areaLevel->height
        );
    }
    MarkRoomRegions(
        g_areaLevel->roomWalls,
        g_areaLevel->roomDoors,
        g_areaLevel->width,
        g_areaLevel->height
    );
    return 0;
}

// Places the level's NPCs: every object entry whose flag is clear, on a
// reserved object cell, where the map shows no object.
RVA(0x000211f0, 0x70)
void SpawnLevelObjects(void) {
    ObjectCell* cell;
    if (!g_areaLevel) {
        return;
    }
    ClearAreaNpcs();
    for (cell = g_areaLevel->objects; cell->head.x != 0xff; cell++) {
        if (!IsCellFlagSet(&cell->head, 3) && IsReservedObjectCell(&cell->head)
            && !IsObjectCell(GetMapCellCode(cell->head.x, cell->head.y))) {
            AddAreaNpc((u8*)cell);
        }
    }
}

// Resets the field objects and scene and re-places the level's rooms, map
// objects and NPCs.
RVA(0x00021260, 0x37)
void RespawnAreaActors(void) {
    ResetFieldObjects();
    ResetFieldScene();
    BuildRoomMap(0);
    SpawnMapObjects(GetPartyCellCode());
    SpawnLevelObjects();
    UpdateCurrentRoom();
    SetRebuildRoom(0);
}

// Selects level `level` of the area map (0 when out of range): when it
// changes (or with `force`), resets the field memory, the flag banks (unless
// their preservation is requested), the field and wall textures and respawns the actors;
// then wraps the party into the map, plays the level music and switches the
// automap level.
RVA(0x000212a0, 0x100)
void SelectAreaLevel(i16 level, i16 force) {
    if (level >= g_areaMap->levelCount) {
        level = 0;
    }
    if (force || level != s_currentLevel) {
        ResetFieldMemory();
        g_areaLevel = AreaLevelAt(g_areaMap, level);
        if (!s_preserveLevelFlags) {
            SetFlagBank(8);
            ClearFlagBank(0xd);
        }
        s_preserveLevelFlags = 0;
        ResetFieldObjects();
        ResetFieldScene();
        if (g_areaLevel->wallSet != 0xff) {
            LoadWallTextures(g_areaLevel->wallSet, g_areaLevel->wallVariant);
        }
        RespawnAreaActors();
    }
    g_field.pos.x = WrapMapCoord(g_field.pos.x, g_areaLevel->width);
    g_field.pos.y = WrapMapCoord(g_field.pos.y, g_areaLevel->height);
    PlayLevelMusic();
    s_currentLevel = level;
    AllocAutomapLevels();
    StoreAutomapLevel();
    LoadAutomapLevel(g_areaMap->area, level);
    RebuildViewScene();
}

// Loads area `area` (when it is not the current one: allocating the map,
// resetting flag banks 9 (unless preserved) and 12, and decoding the data
// file) and selects level `level`, forced after a load. Area 9's second
// level is displayed as basement floor 1.
RVA(0x000213a0, 0xd0)
void LoadAreaMap(i16 area, i16 level) {
    i16 force = 0;
    if (!g_areaMap || g_areaMap->area != area) {
        FILE* fp;
        if (!g_areaMap) {
            g_areaMap = AllocCleared(1, 0x2c00);
        }
        if (!s_preserveAreaFlags) {
            SetFlagBank(9);
        }
        s_preserveAreaFlags = 0;
        SetFlagBank(0xc);
        UnloadAreaMap();
        fp = OpenDataFile(area, 3, 0);
        ReadCryptRecord(fp, s_areaRecord);
        CloseDataFile(fp);
        DecodeAreaMap(g_areaMap, s_areaRecord);
        g_areaMap->area = area;
        if (area == 9) {
            AreaLevelAt(g_areaMap, 1)->floor = -1;
        }
        ResetLevelEvents();
        force = 1;
    }
    SelectAreaLevel(level, force);
}

// Decodes the area-map record into `map`: the name, then each level's header
// (its list offsets rebased to pointers) and its data.
// @early-stop register allocation: retail keeps the level index in bp and the
// record in edx, spilling `map` (so the two data copies stay unmerged); every
// ordering and local tried (offset/src/base locals, declaration order, shift
// source, a next-index local) keeps `map` in ebx and spills the index.
RVA(0x00021470, 0x206)
void DecodeAreaMap(AreaMap* map, u8* record) {
    AreaRecord* head = (AreaRecord*)record;
    u16 shift;
    i16 i;
    map->name = (char*)record + head->nameOffset;
    map->levelCount = head->levelCount;
    shift = map->levelCount * 2 + 2;
    for (i = 0; i < map->levelCount; i++) {
        AreaLevelRecord* src = (AreaLevelRecord*)(record + GetAreaLevelOffset(head, i));
        AreaLevel* level = (AreaLevel*)((u8*)map + i * 28 + shift + GetAreaLevelOffset(head, i));
        u8* base;
        map->levels[i] = level;
        base = (u8*)map + (i + 1) * 28 + shift;
        level->blockBits = base + src->blockBitsOffset;
        level->walls = (u16*)(base + src->wallsOffset);
        level->warps = (WarpCell*)(base + src->warpsOffset);
        level->battles = (BattleCell*)(base + src->battlesOffset);
        level->links = (LinkCell*)(base + src->linksOffset);
        level->objects = (ObjectCell*)(base + src->objectsOffset);
        level->spawns = (MapSpawn*)(base + src->spawnsOffset);
        level->scripts = (ScriptCell*)(base + src->scriptsOffset);
        level->doors = base + src->doorsOffset;
        level->exits = (ExitCell*)(base + src->exitsOffset);
        level->boxes = (TreasureBox*)(base + src->boxesOffset);
        level->roomDoors = base + src->roomDoorsOffset;
        level->roomWalls = base + src->roomWallsOffset;
        level->width = src->width;
        level->height = src->height;
        level->revealFlagBank = src->revealFlagBank;
        level->revealFlagIndex = src->revealFlagIndex;
        level->floor = src->floor;
        level->wallSet = src->wallSet;
        level->wallVariant = src->wallVariant;
        level->byte3e = src->byte24;
        level->byte3f = src->byte25;
        level->music = src->music;
        level->defaultMusic = src->defaultMusic;
        level->roomBits = base + src->roomBitsOffset;
        if (i < map->levelCount - 1) {
            memcpy(
                level + 1,
                src + 1,
                GetAreaLevelOffset(head, i + 1) - GetAreaLevelOffset(head, i)
            );
        } else {
            memcpy(level + 1, src + 1, 0x2800 - GetAreaLevelOffset(head, i));
        }
    }
}

// Plays the music of the first of the level's five choices whose event flag
// is clear, else its default music.
RVA(0x00021680, 0x73)
void PlayLevelMusic(void) {
    i16 i;
    if (!g_areaLevel) {
        return;
    }
    for (i = 0; i < 5; i++) {
        if (!IsEventFlagSet(
                g_areaLevel->music.choices[i].flagBank,
                g_areaLevel->music.choices[i].flagIndex
            )) {
            PlayMusic(g_areaLevel->music.choices[i].music, 1);
            return;
        }
    }
    PlayMusic(g_areaLevel->defaultMusic, 1);
}

RVA(0x00021700, 0x9)
i16 GetCurrentArea(void) {
    return g_areaMap->area;
}

RVA(0x00021710, 0xa)
i16 GetAreaLevelCount(void) {
    return g_areaMap->levelCount;
}

RVA(0x00021720, 0x26)
MapCoord GetAreaSize(i16 level) {
    MapCoord size;
    size.x = AreaLevelAt(g_areaMap, level)->width;
    size.y = AreaLevelAt(g_areaMap, level)->height;
    return size;
}

// The current area's name ("" without a map).
RVA(0x00021750, 0x13)
char* GetAreaName(void) {
    if (!g_areaMap) {
        return s_noAreaName;
    }
    return g_areaMap->name;
}

// The displayed floor number, or zero without a selected level.
RVA(0x00021770, 0x12)
i16 GetLevelFloor(void) {
    if (!g_areaLevel) {
        return 0;
    }
    return g_areaLevel->floor;
}

RVA(0x00021790, 0x39)
void GetMapSize(i16* width, i16* height) {
    if (!g_areaLevel) {
        *height = 0;
        *width = 0;
        return;
    }
    *width = g_areaLevel->width;
    *height = g_areaLevel->height;
}

RVA(0x000217d0, 0xe)
u16* GetWallMap(void) {
    if (!g_areaLevel) {
        return NULL;
    }
    return g_areaLevel->walls;
}

RVA(0x000217e0, 0x24)
i16 IsCellAt(i16 x, i16 y, const CellHead* cell) {
    if (x == cell->x && y == cell->y) {
        return 1;
    }
    return 0;
}

// Whether the event flag at `offset` in the cell is set; a zero pair is
// never set.
RVA(0x00021810, 0x31)
i16 IsCellFlagSet(const CellHead* cell, i16 offset) {
    const u8* bytes = &cell->x;

    if (bytes[offset] == 0 && bytes[offset + 1] == 0) {
        return 0;
    }
    return IsEventFlagSet(bytes[offset], bytes[offset + 1]);
}

RVA(0x00021850, 0x29)
const CellKind* FindCellKind(const CellHead* cell) {
    const CellKind* kind;

    for (kind = s_cellKinds; kind->code != 0xff; kind++) {
        if (kind->code == cell->code) {
            return kind;
        }
    }
    return NULL;
}

// Finds the event of cell x/y on `level`, latches its destination and scene
// record, and returns its kind (0 for none).
RVA(0x00021880, 0x49d)
i16 CheckCellEvent(i16 x, i16 y, i16 level) {
    WarpCell* warp;
    BattleCell* battle;
    LinkCell* link;
    ObjectCell* object;
    ExitCell* exit;
    TreasureBox* box;
    ScriptCell* script;
    const CellKind* kind;
    i16 layer;

    for (warp = AreaLevelAt(g_areaMap, level)->warps; warp->head.x != 0xff; warp++) {
        if (IsCellAt(x, y, &warp->head) && !IsCellFlagSet(&warp->head, 6)) {
            LatchCellDestination(&warp->head, 3, 4, -1, 5, 8);
            SetSceneCell(&warp->head);
            kind = FindCellKind(&warp->head);
            if (kind == NULL) {
                return CELL_EVENT_WARP;
            }
            return kind->kind;
        }
    }

    for (battle = AreaLevelAt(g_areaMap, level)->battles; battle->head.x != 0xff; battle++) {
        if (IsCellAt(x, y, &battle->head) && !IsCellFlagSet(&battle->head, 3)
            && !IsCellFlagSet(&battle->head, 7)) {
            SetFieldPair(battle->battleFlag[0], battle->battleFlag[1]);
            LatchCellDestination(&battle->head, 5, 6, -1, -1, -1);
            SetSceneCell(&battle->head);
            return CELL_EVENT_BATTLE;
        }
    }

    for (link = AreaLevelAt(g_areaMap, level)->links; link->head.x != 0xff; link++) {
        if (!IsCellAt(x, y, &link->head) || IsCellFlagSet(&link->head, 3)) {
            continue;
        }
        LatchCellDestination(&link->head, 5, 6, 7, -1, -1);
        SetSceneCell(&link->head);
        SetCellScript(OffsetByWord((u8*)g_areaMap, &link->script));
        kind = FindCellKind(&link->head);
        if (kind == NULL) {
            return CELL_EVENT_SCRIPT;
        }
        if (kind->kind == CELL_EVENT_WORLD_EXIT) {
            layer = link->facings;
            if (layer == 0xff) {
                return CELL_EVENT_SCRIPT;
            }
            SetWorldMapSpot(layer, link->spotX, link->spotY);
            g_worldMapRequest = 1;
            return kind->kind;
        }
        if ((kind->flags & CELL_KIND_CHECK_FACING) && link->facings != 0
            && !(link->facings & GetFacingBit())) {
            continue;
        }
        if (kind->code == 0x7f) {
            SetSceneScriptByIndex(7, 8);
        }
        return kind->kind;
    }

    for (object = AreaLevelAt(g_areaMap, level)->objects; object->head.x != 0xff; object++) {
        if (IsReservedObjectCell(&object->head)) {
            continue;
        }
        if (!IsCellAt(x, y, &object->head) || IsCellFlagSet(&object->head, 3)) {
            continue;
        }
        LatchCellDestination(&object->head, 5, 6, 7, -1, -1);
        SetSceneCell(&object->head);
        kind = FindCellKind(&object->head);
        if (kind == NULL) {
            return CELL_EVENT_OBJECT;
        }
        if (kind->kind == CELL_EVENT_OBJECT) {
            return kind->kind;
        }
    }

    for (exit = AreaLevelAt(g_areaMap, level)->exits; exit->head.x != 0xff; exit++) {
        if (!IsCellAt(x, y, &exit->head)) {
            continue;
        }
        LatchCellDestination(&exit->head, 3, 4, 5, -1, -1);
        SetSceneCell(&exit->head);
        kind = FindCellKind(&exit->head);
        if (kind == NULL) {
            if (IsCellFlagSet(&exit->head, 3)) {
                continue;
            }
            return CELL_EVENT_TRAP;
        }
        if (kind->kind != CELL_EVENT_CHUTE) {
            return kind->kind;
        }
        if (IsCellFlagSet(&exit->head, 6)) {
            continue;
        }
        LatchCellDestination(&exit->head, 3, 4, -1, 5, -1);
        g_cellDestArea = g_areaMap->area;
        return kind->kind;
    }

    for (box = AreaLevelAt(g_areaMap, level)->boxes; box->head.x != 0xff; box++) {
        if (!IsCellAt(x, y, &box->head)) {
            continue;
        }
        kind = FindCellKind(&box->head);
        if (kind == NULL) {
            continue;
        }
        LatchCellDestination(&box->head, 5, 6, 7, -1, -1);
        SetSceneCell(&box->head);
        SetCellScript(NULL);
        return kind->kind;
    }

    for (script = AreaLevelAt(g_areaMap, level)->scripts; script->head.x != 0xff; script++) {
        if (!IsCellAt(x, y, &script->head) || IsCellFlagSet(&script->head, 3)) {
            continue;
        }
        LatchCellDestination(&script->head, 5, 6, 7, -1, -1);
        SetSceneCell(&script->head);
        SetCellScript(OffsetByWord((u8*)g_areaMap, &script->script));
        SwapSceneCellParams();
        kind = FindCellKind(&script->head);
        if (kind == NULL) {
            return CELL_EVENT_SCRIPT;
        }
        return kind->kind;
    }
    return CELL_EVENT_NONE;
}

// Destination offsets address bytes in the cell's variable-format record.
RVA(0x00021d20, 0x91)
void LatchCellDestination(const CellHead* cell, i16 x, i16 y, i16 direction, i16 level, i16 area) {
    const u8* bytes = &cell->x;
    s_cellX = cell->x;
    s_cellY = cell->y;
    g_cellCode = cell->code;
    g_cellDestX = bytes[x];
    g_cellDestY = bytes[y];
    if (direction != -1) {
        s_cellDestDirection = bytes[direction];
    }
    if (level != -1) {
        g_cellDestLevel = bytes[level];
    }
    if (area != -1) {
        g_cellDestArea = bytes[area];
    }
}

// 1 when x/y holds an enabled object of code 0x8d (and while no level is
// loaded).
RVA(0x00021dc0, 0x70)
i16 IsDarkCell(i16 x, i16 y) {
    ObjectCell* cell;
    if (g_areaLevel == NULL) {
        return 1;
    }
    for (cell = g_areaLevel->objects; cell->head.x != 0xff; cell++) {
        if (IsCellAt(x, y, &cell->head) && !IsCellFlagSet(&cell->head, 3)
            && cell->head.code == 0x8d) {
            return 1;
        }
    }
    return 0;
}

// 1 when x/y holds an enabled object of code 0x8e (and while no level is
// loaded).
RVA(0x00021e30, 0x70)
i16 IsCellCommandBlocked(i16 x, i16 y) {
    ObjectCell* cell;
    if (g_areaLevel == NULL) {
        return 1;
    }
    for (cell = g_areaLevel->objects; cell->head.x != 0xff; cell++) {
        if (IsCellAt(x, y, &cell->head) && !IsCellFlagSet(&cell->head, 3)
            && cell->head.code == 0x8e) {
            return 1;
        }
    }
    return 0;
}

// The bit of x/y in the level's room bitmap.
RVA(0x00021ea0, 0x2b)
i16 IsRoomCell(i16 x, i16 y) {
    if (g_areaLevel == NULL) {
        return 0;
    }
    return TestBit(g_areaLevel->roomBits, g_areaLevel->width * y + x);
}

// With `mode` set, whether x/y is blocked on `level`: a cell of any list
// there, else its wall or room bit. With `mode` 0, draws each enabled cell's
// automap icon instead and returns 0.
// @early-stop register allocation: retail holds y in esi and x in edi; every
// spelling here swaps them (declaration order, an index local or none).
RVA(0x00021ed0, 0x1d3)
i16 IsCellBlocked(i16 level, i16 mode, i16 x, i16 y) {
    WarpCell* warp;
    BattleCell* battle;
    LinkCell* link;
    ScriptCell* script;
    ExitCell* exit;
    TreasureBox* box;
    i16 hit;
    i16 index;
    for (warp = AreaLevelAt(g_areaMap, level)->warps; warp->head.x != 0xff; warp++) {
        if ((hit = CheckBlockingCell(&warp->head, mode, 6, x, y)) > 0) {
            return hit;
        }
    }
    for (battle = AreaLevelAt(g_areaMap, level)->battles; battle->head.x != 0xff; battle++) {
        if ((hit = CheckBlockingCell(&battle->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (link = AreaLevelAt(g_areaMap, level)->links; link->head.x != 0xff; link++) {
        if ((hit = CheckBlockingCell(&link->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (script = AreaLevelAt(g_areaMap, level)->scripts; script->head.x != 0xff; script++) {
        if ((hit = CheckBlockingCell(&script->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (exit = AreaLevelAt(g_areaMap, level)->exits; exit->head.x != 0xff; exit++) {
        if (exit->head.code == CELL_CHUTE) {
            if ((hit = CheckBlockingCell(&exit->head, mode, 6, x, y)) > 0) {
                return hit;
            }
        } else {
            if ((hit = CheckBlockingCell(&exit->head, mode, 3, x, y)) > 0) {
                return hit;
            }
        }
    }
    for (box = AreaLevelAt(g_areaMap, level)->boxes; box->head.x != 0xff; box++) {
        if ((hit = CheckBlockingCell(&box->head, mode, 0xb, x, y)) > 0) {
            return hit;
        }
    }
    if (mode == 0) {
        return 0;
    }
    index = AreaLevelAt(g_areaMap, level)->width * y + x;
    if (TestBit(AreaLevelAt(g_areaMap, level)->blockBits, index)) {
        return 1;
    }
    return TestBit(AreaLevelAt(g_areaMap, level)->roomBits, index);
}

// With `mode` set, 1 when the cell is at x/y. With `mode` 0, draws the cell's
// automap icon unless its flag at `flagOffset` is set, and returns -1.
// @early-stop scheduling: retail loads each cell byte just before its
// subtraction (reusing edx) and the icon arguments into eax/ecx/dl; the
// difference test (or/neg/sbb) spelled `!(a | b)`, `(a | b) == 0`, with a dy
// local or as a compare pair keeps an extra register.
RVA(0x000220b0, 0x62)
i16 CheckBlockingCell(const CellHead* cell, i16 mode, i16 flagOffset, i16 x, i16 y) {
    if (mode != 0) {
        return !((y - cell->y) | (x - cell->x));
    }
    if (!IsCellFlagSet(cell, flagOffset)) {
        DrawAutomapCellIcon(cell->code, cell->x, cell->y);
    }
    return -1;
}

// The code of the enabled warp, link or script cell at x/y (0: none).
RVA(0x00022120, 0xcf)
i16 GetEventCellCode(i16 x, i16 y) {
    WarpCell* warp;
    LinkCell* link;
    ScriptCell* script;
    if (g_areaLevel == NULL) {
        return 0;
    }
    for (warp = g_areaLevel->warps; warp->head.x != 0xff; warp++) {
        if (IsCellAt(x, y, &warp->head) && !IsCellFlagSet(&warp->head, 6)) {
            return warp->head.code;
        }
    }
    for (link = g_areaLevel->links; link->head.x != 0xff; link++) {
        if (IsCellAt(x, y, &link->head) && !IsCellFlagSet(&link->head, 3)) {
            return link->head.code;
        }
    }
    for (script = g_areaLevel->scripts; script->head.x != 0xff; script++) {
        if (IsCellAt(x, y, &script->head) && !IsCellFlagSet(&script->head, 3)) {
            return script->head.code;
        }
    }
    return 0;
}

RVA(0x000221f0, 0x99)
i16 IsStepBarred(i16 x, i16 y, i16 direction, i16 turn) {
    i16 facing;
    DoorCell* door;
    if (g_areaLevel == NULL) {
        return 0;
    }
    facing = TurnDirection(direction, turn);
    for (door = g_areaLevel->doors; door->head.x != 0xff; door++) {
        if ((door->head.code & 0xf) != 0xb && (door->head.code >> 4) == facing
            && IsCellAt(x, y, &door->head) && !IsCellFlagSet(&door->head, 3)) {
            return door->head.code;
        }
    }
    return 0;
}

// Wraps `value` into 0..size-1.
RVA(0x00022290, 0x21)
i16 WrapMapCoord(i16 value, i16 size) {
    while (value >= size) {
        value -= size;
    }
    while (value < 0) {
        value += size;
    }
    return value;
}

// Pulls `value` back from `size` (one step) and up to 0.
RVA(0x000222c0, 0x14)
i16 ClampMapCoord(i16 value, i16 size) {
    if (value >= size) {
        value--;
    }
    if (value < 0) {
        value = 0;
    }
    return value;
}

// The wall word of x/y (both wrapped into the level); 0 without a level.
RVA(0x000222e0, 0x54)
i16 RevealAreaMapAt(i16 x, i16 y) {
    i16 height;
    i16 width;
    if (g_areaLevel == NULL) {
        return 0;
    }
    height = g_areaLevel->height;
    width = g_areaLevel->width;
    x = WrapMapCoord(x, width);
    y = WrapMapCoord(y, height);
    return g_areaLevel->walls[y * width + x];
}

// Wraps x/y into the level (0/0 without a level).
RVA(0x00022340, 0x55)
void WrapMapPosition(i16* x, i16* y) {
    if (g_areaLevel == NULL) {
        *y = 0;
        *x = 0;
        return;
    }
    *x = WrapMapCoord(*x, g_areaLevel->width);
    *y = WrapMapCoord(*y, g_areaLevel->height);
}

// Clamps x/y into the level (0/0 without a level).
RVA(0x000223a0, 0x55)
void ClampMapPosition(i16* x, i16* y) {
    if (g_areaLevel == NULL) {
        *y = 0;
        *x = 0;
        return;
    }
    *x = ClampMapCoord(*x, g_areaLevel->width);
    *y = ClampMapCoord(*y, g_areaLevel->height);
}

#define ReturnWarpCodeAt(firstWarp, mapX, mapY, codeFlags)                                         \
    do {                                                                                           \
        WarpCell* warp;                                                                            \
        for (warp = (firstWarp); warp->head.x != 0xff; warp++) {                                   \
            if ((mapX) == warp->head.x && (mapY) == warp->head.y) {                                \
                return warp->head.code | (codeFlags);                                              \
            }                                                                                      \
        }                                                                                          \
    } while (0)

// The code of the warp at (dx, dy) from the party (0: none).
RVA(0x00022400, 0x59)
i16 GetWarpCodeAtOffset(i16 dx, i16 dy) {
    if (g_areaLevel == NULL) {
        return 0;
    }
    dx += g_field.pos.x;
    dy += g_field.pos.y;
    ReturnWarpCodeAt(g_areaLevel->warps, dx, dy, 0);
    return 0;
}

// The warp code at (dx, dy) from the party: a warp under the party itself
// answers with bit 3 set, and one fixed spot (area 9, level 6, at 2/5 looking
// one cell west) answers 0x42.
// @early-stop register allocation: retail keeps dy in esi and dx in memory,
// forms the offset x in edx and spills the offset y into dy's slot, and reuses
// the first loop's list pointer; x/y locals updated in place or dx/dy updated,
// and a single list local, each keep dx/dy in ebp/ebx.
RVA(0x00022460, 0xd4)
i16 GetCellAtOffset(i16 dx, i16 dy) {
    i16 x;
    i16 y;
    if (g_areaLevel == NULL) {
        return 0;
    }
    x = g_field.pos.x;
    y = g_field.pos.y;
    if (x == 2 && y == 5 && g_field.pos.area == 9 && g_field.pos.level == 6 && dx == -1
        && dy == 0) {
        return CELL_STAIRS_UP;
    }
    ReturnWarpCodeAt(g_areaLevel->warps, x, y, 8);
    x += dx;
    y += dy;
    ReturnWarpCodeAt(g_areaLevel->warps, x, y, 0);
    return 0;
}

// The current level's room list (0) or door list (1).
RVA(0x00022540, 0x1c)
u8* GetLevelList(i16 which) {
    AreaLevel* level = g_areaLevel;
    u8* list;
    if (level == NULL) {
        return NULL;
    }
    list = level->roomWalls;
    if (which) {
        list = level->roomDoors;
    }
    return list;
}

// The treasure box at x/y (NULL: none); with `select`, it becomes the scene
// cell.
RVA(0x00022560, 0x59)
TreasureBox* FindTreasureBoxAt(i16 x, i16 y, i16 select) {
    TreasureBox* box;
    if (g_areaLevel == NULL) {
        return NULL;
    }
    for (box = g_areaLevel->boxes; box->head.x != 0xff; box++) {
        if (IsCellAt(x, y, &box->head)) {
            if (select) {
                SetSceneCell(&box->head);
            }
            return box;
        }
    }
    return NULL;
}

RVA(0x000225c0, 0xe)
TreasureBox* GetMapTreasureBoxes(void) {
    if (g_areaLevel == NULL) {
        return NULL;
    }
    return g_areaLevel->boxes;
}

// Whether unexplored cells are shown on the current level's automap.
// The reveal flag is active when clear; no selected level returns false.
RVA(0x000225d0, 0x27)
i16 IsLevelMapRevealed(void) {
    if (g_areaLevel == NULL) {
        return 0;
    }
    return !IsEventFlagSet(g_areaLevel->revealFlagBank, g_areaLevel->revealFlagIndex);
}

// Copies the enabled exit at x/y into `out` (an exit without a kind entry, or
// a kind-7 in-area exit); NULL when there is none.
RVA(0x00022600, 0xbd)
ExitCell* CopyExitAt(i16 x, i16 y, ExitCell* out) {
    ExitCell* exit;
    const CellKind* kind;
    u16 i;
    if (g_areaLevel == NULL) {
        return NULL;
    }
    for (exit = g_areaLevel->exits; exit->head.x != 0xff; exit++) {
        if (IsCellAt(x, y, &exit->head)) {
            kind = FindCellKind(&exit->head);
            if (kind == NULL) {
                if (!IsCellFlagSet(&exit->head, 3)) {
                    for (i = 0; i < sizeof(ExitCell); i++) {
                        ((u8*)out)[i] = ((u8*)exit)[i];
                    }
                    return out;
                }
            } else if (kind->kind == 7 && !IsCellFlagSet(&exit->head, 6)) {
                for (i = 0; i < sizeof(ExitCell); i++) {
                    ((u8*)out)[i] = ((u8*)exit)[i];
                }
                return out;
            }
        }
    }
    return NULL;
}
