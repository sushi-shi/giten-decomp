// @identity-TODO: the owning TU is unproven. One retail object: clock's .bss
// statics lie inside areamap's .bss run, panel's initialized tables inside
// areamap's .data run, and the three units' code is contiguous in .text: the
// game clock, the area map and cell events, and the flag-word and menu-panel
// helpers.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <Game/AreaLevel.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/Automap.h>
#include <Game/AutomapData.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/Field.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/MapArea.h>
#include <Game/ObjectRecord.h>
#include <Game/Scene.h>
#include <Game/SkillUse.h>
#include <Game/SpecialItems.h>
#include <Game/WorldMap.h>
#include <Gfx/ImageHandle.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/EventFlags.h>
#include <Sound/Sound.h>
#include <Ui/Panel.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// The kind of each special cell code.
DATA(0x00068e50)
static CellKind s_cellKinds[] = {
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
    {0x7c, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {0x8b, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {0x8c, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {CELL_DARK, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {CELL_COMMAND_BLOCKED, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {0x8f, CELL_EVENT_FLOOR_PROPERTY, 0, 0},
    {CELL_CHUTE, CELL_EVENT_CHUTE, 0, 0},
    {0x88, CELL_EVENT_FADE_SCENE, 0, 0},
    {0x89, CELL_EVENT_FADE_SCENE, 0, 0},
    {0x65, 13, 0, 0},
    {0xff, CELL_EVENT_NONE, 0, 0},
};

DATA(0x00068ec0)
static i16 s_kindFirstRow[] = {-1, 18, -1, 14, 16, 6,  -1, -1, -1, 4,  16, -1, -1, -1,
                               -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, 8,  12, -1,
                               2,  -1, 0,  20, -1, 10, -1, -1, -1, 0,  0,  0};

DATA(0x00068f10)
static i16 s_imageFirstRow[] = {30, 63, -1, 32, 50, -1, 35, 47, 80, -1, -1, -1, -1, -1, 39, 43};

DATA(0x00068f30)
static i16 s_panelImage = -1;

// The level SelectAreaLevel last selected.
DATA(0x00068f34)
static i16 s_currentLevel = -1;

// The encrypted area-map record as read from the data file (decoded into
// g_areaMap).
DATA(0x0007d640)
static u8 s_areaRecord[0x2800] = {0};

// One-shot preservation of event flags across the next area or level load.
// SetAreaFlagPreservation accepts -1 to keep either mode unchanged.
DATA(0x0007fe40)
static i16 s_preserveAreaFlags = 0;

DATA(0x0007fe44)
static i16 s_preserveLevelFlags = 0;

// Set on the tick that ends a 24-tick round (HasTurnElapsed), and the round's
// tick count.
DATA(0x0007fe48)
static b16 s_turnElapsed = false;

DATA(0x0007fe4c)
static i16 s_roundTicks = 0;

// The (bank, index) pairs of the event flags cleared at the full moon, ended
// by 0xff (data file 0x19).
DATA(0x0007fe50)
static i32 s_moonFlags = 0;

DATA(0x0007fe54)
AreaMap* g_areaMap = NULL;

// The level of the area map the party is on (NULL without a map).
DATA(0x0007fe58)
AreaLevel* g_areaLevel = NULL;

// The name returned without an area map.
DATA(0x0007fe5c)
static char s_noAreaName[4] = "";

DATA(0x00091544)
i16 g_tickElapsed;

DATA(0x00091560)
GameClock g_clock;

// Resets the clock (day 0, 0:00, new moon; 5 frames a tick, 2 of 60 minute
// steps) and loads the full-moon flag list once.
RVA(0x00020b00, 0x79)
void InitClock(void) {
    g_clock.frames = 1;
    g_clock.framesPerTick = 5;
    g_clock.minuteStep = 2;
    g_clock.minuteAcc = 0;
    g_clock.minuteLimit = 60;
    g_clock.days = 0;
    g_clock.moonTicks = 0;
    ResetClockPhaseAndTime(&g_clock);
    g_tickElapsed = 0;
    if (!s_moonFlags) {
        FILE* fp = OpenDataFile(0x19, DATA_FILE_TABLE, 0);
        s_moonFlags = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
}

// Advances the clock by `minutes` and runs what the change brings (the moon
// flags, the countdown, special items, party timers); returns the change
// bits of TickClock.
RVA(0x00020b80, 0x6c)
GZ_ENUM_RETURN(ClockUpdate, i16) AdvanceClock(u16 minutes) {
    GZ_ENUM_STORAGE(ClockUpdate, i16) changed = TickClock(minutes);
    ModifyEventFlag(0, 0x23, g_clock.moonPhase != MOON_PHASE_FULL);
    ModifyEventFlag(0, 0x25, g_clock.moonPhase != MOON_PHASE_NEW);
    ApplyClockChanges(changed);
    DrawDownCountdown(minutes);
    ExpireSpecialItems();
    TickPartyTimers(minutes);
    return changed;
}

// Adds `minutes`: 3, | 4 when an hour passed, | 8 a day, | 0x10 a moon phase.
RVA(0x00020bf0, 0xfe)
GZ_ENUM_RETURN(ClockUpdate, i16) TickClock(u16 minutes) {
    GZ_ENUM_STORAGE(ClockUpdate, i16) changed = CLOCK_UPDATE_TICK | CLOCK_UPDATE_MINUTE;
    u16 total = minutes + g_clock.minute;
    u16 carry = total / 60;
    g_clock.minute = total % 60;
    if (carry) {
        changed = CLOCK_UPDATE_TICK | CLOCK_UPDATE_MINUTE | CLOCK_UPDATE_HOUR;
    }
    total = carry + g_clock.hour;
    carry = total / 24;
    g_clock.hour = total % 24;
    if (carry) {
        changed |= CLOCK_UPDATE_DAY;
    }
    g_clock.days += carry;
    total = g_clock.moonTicks;
    total = total + minutes;
    carry = total / MOON_PHASE_TICKS;
    g_clock.moonTicks = total % MOON_PHASE_TICKS;
    if (carry) {
        changed |= CLOCK_UPDATE_MOON;
    }
    total = carry + g_clock.moonPhase;
    g_clock.moonPhase = total % MOON_PHASE_COUNT;
    return changed;
}

// On a new moon phase: clears flag 7/0xfd and the leader's flag 0x22, sets
// flag 10 of every live object, applies the phase to the party and the
// objects, and handles the full and new moons and the phase after the full moon.
RVA(0x00020cf0, 0x149)
void ApplyClockChanges(GZ_ENUM_PARAM(ClockUpdate, i16) changed) {
    i16 i;
    u8* flags;
    Character* character;
    if (!(changed & CLOCK_UPDATE_MOON)) {
        return;
    }
    ModifyEventFlag(7, 0xfd, BIT_CHANGE_CLEAR);
    flags = GetCharacterFlags(GetRosterCharacter(ROSTER_LEADER));
    ClearBit(flags, 0x22);
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0) {
            flags = GetCharacterFlags(GetCombatant(object));
            SetBit(flags, 10);
        }
    }
    for (i = 0; i < PARTY_SIZE; i++) {
        character = GetPartyCharacter(i);
        if (ApplyMoonPhase(character, g_clock.moonPhase)) {
            RecalcCharacterStats(character);
        }
    }
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0) {
            character = GetCombatant(object);
            if (ApplyMoonPhase(character, g_clock.moonPhase)) {
                RecalcCharacterStats(character);
            }
        }
    }
    if (g_clock.moonPhase == MOON_PHASE_FULL) {
        ModifyEventFlag(0, 0x24, BIT_CHANGE_CLEAR);
    }
    if (g_clock.moonPhase == MOON_PHASE_NEW) {
        ModifyEventFlag(0, 0x26, BIT_CHANGE_CLEAR);
    }
    if (g_clock.moonPhase == MOON_PHASE_FULL) {
        ModifyEventFlag(7, 0xff, BIT_CHANGE_CLEAR);
        ModifyEventFlag(7, 0xfe, BIT_CHANGE_CLEAR);
    }
    if (g_clock.moonPhase == MOON_PHASE_FULL + 1) {
        ClearMoonFlags();
    }
}

// Clears the full-moon flag list's event flags.
// @early-stop: retail addresses the list as [index + base]; the spellings
// tried give [base + index].
RVA(0x00020e40, 0x48)
void ClearMoonFlags(void) {
    u8* list;
    i16 i;
    if (!s_moonFlags) {
        return;
    }
    list = HandleReadPtr(s_moonFlags);
    for (i = 0; list[i] != 0xff; i += 2) {
        ModifyEventFlag(list[i], list[i + 1], BIT_CHANGE_CLEAR);
    }
}

// One frame of the game clock: every framesPerTick frames a tick passes (24
// ticks end a round, HasTurnElapsed); unless `paused`, the ticks add
// minuteStep to the minute accumulator and each minuteLimit of it advances
// the clock a minute. Returns 1 on a tick, else 0 (or AdvanceClock's bits).
RVA(0x00020e90, 0x90)
GZ_ENUM_RETURN(ClockUpdate, i16) TickGameClock(i16 paused) {
    s_turnElapsed = false;
    if (--g_clock.frames != 0) {
        return CLOCK_UPDATE_NONE;
    }
    g_clock.frames = g_clock.framesPerTick;
    if (++s_roundTicks >= 24) {
        s_roundTicks = 0;
        s_turnElapsed = true;
    }
    if (paused) {
        return CLOCK_UPDATE_TICK;
    }
    g_clock.minuteAcc += g_clock.minuteStep;
    if (g_clock.minuteAcc < g_clock.minuteLimit) {
        return CLOCK_UPDATE_TICK;
    }
    g_clock.minuteAcc -= g_clock.minuteLimit;
    return AdvanceClock(1);
}

RVA(0x00020f20, 0x7)
i16 HasTurnElapsed(void) {
    return s_turnElapsed;
}

RVA(0x00020f30, 0x21)
i16 SaveClock(FILE* fp) {
    return 1 - fwrite(&g_clock, sizeof(g_clock), 1, fp);
}

// Reads a saved clock (the time and moon only).
RVA(0x00020f60, 0x59)
i16 LoadClock(FILE* fp) {
    GameClock clock;
    i16 errors = 1 - fread(&clock, sizeof(clock), 1, fp);
    g_clock.days = clock.days;
    g_clock.moonTicks = clock.moonTicks;
    g_clock.moonPhase = clock.moonPhase;
    g_clock.hour = clock.hour;
    g_clock.minute = clock.minute;
    return errors;
}

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
    if (TestLevelEvent(g_party.field.pos.level) == true) {
        return;
    }
    if (!g_areaLevel) {
        return;
    }
    if (g_party.field.pos.area == MAP_AREA_HARAJUKU && g_party.field.pos.level == 6) {
        special = 1;
    } else if (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.level == 4) {
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
    for (cell = g_areaLevel->objects; !IsCellListEnd(&cell->head); cell++) {
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
            SetFlagBank(EVENT_FLAG_BANK_LEVEL);
            ClearFlagBank(EVENT_FLAG_BANK_LEVEL_SCRATCH);
        }
        s_preserveLevelFlags = 0;
        ResetFieldObjects();
        ResetFieldScene();
        if (g_areaLevel->wallSet != 0xff) {
            LoadWallTextures(g_areaLevel->wallSet, g_areaLevel->wallVariant);
        }
        RespawnAreaActors();
    }
    g_party.field.pos.x = WrapMapCoord(g_party.field.pos.x, g_areaLevel->width);
    g_party.field.pos.y = WrapMapCoord(g_party.field.pos.y, g_areaLevel->height);
    PlayLevelMusic();
    s_currentLevel = level;
    AllocAutomapLevels();
    StoreAutomapLevel();
    LoadAutomapLevel(g_areaMap->area, level);
    RebuildViewScene();
}

// Loads area `area` (when it is not the current one: allocating the map,
// resetting the area flag banks (the first unless preserved), and decoding the data
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
            SetFlagBank(EVENT_FLAG_BANK_AREA);
        }
        s_preserveAreaFlags = 0;
        SetFlagBank(EVENT_FLAG_BANK_SCRATCH);
        UnloadAreaMap();
        fp = OpenDataFile(area, DATA_FILE_MAP, 0);
        ReadCryptRecord(fp, s_areaRecord);
        CloseDataFile(fp);
        DecodeAreaMap(g_areaMap, s_areaRecord);
        g_areaMap->area = area;
        if (area == MAP_AREA_SHINJUKU_TOCHO) {
            AreaLevelAt(g_areaMap, 1)->floor = -1;
        }
        ResetLevelEvents();
        force = true;
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
            PlayMusic(g_areaLevel->music.choices[i].music, true);
            return;
        }
    }
    PlayMusic(g_areaLevel->defaultMusic, true);
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
b16 IsCellAt(i16 x, i16 y, const CellHead* cell) {
    if (x == cell->x && y == cell->y) {
        return true;
    }
    return false;
}

// Whether the event flag at `offset` in the cell is set; a zero pair is
// never set.
RVA(0x00021810, 0x31)
b16 IsCellFlagSet(const CellHead* cell, i16 offset) {
    const u8* bytes = &cell->x;

    if (bytes[offset] == 0 && bytes[offset + 1] == 0) {
        return false;
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
GZ_ENUM_RETURN(CellEventKind, i16) CheckCellEvent(i16 x, i16 y, i16 level) {
    WarpCell* warp;
    BattleCell* battle;
    LinkCell* link;
    ObjectCell* object;
    ExitCell* exit;
    TreasureBox* box;
    ScriptCell* script;
    const CellKind* kind;
    i16 layer;

    for (warp = AreaLevelAt(g_areaMap, level)->warps; !IsCellListEnd(&warp->head); warp++) {
        if (IsCellAt(x, y, &warp->head) && !IsCellFlagSet(&warp->head, 6)) {
            LatchCellDestination(&warp->head, 3, 4, CELL_FIELD_NONE, 5, 8);
            SetSceneCell(&warp->head);
            kind = FindCellKind(&warp->head);
            if (kind == NULL) {
                return CELL_EVENT_WARP;
            }
            return kind->kind;
        }
    }

    for (battle = AreaLevelAt(g_areaMap, level)->battles; !IsCellListEnd(&battle->head); battle++) {
        if (IsCellAt(x, y, &battle->head) && !IsCellFlagSet(&battle->head, 3)
            && !IsCellFlagSet(&battle->head, 7)) {
            SetFieldPair(battle->battleFlag[0], battle->battleFlag[1]);
            LatchCellDestination(
                &battle->head,
                5,
                6,
                CELL_FIELD_NONE,
                CELL_FIELD_NONE,
                CELL_FIELD_NONE
            );
            SetSceneCell(&battle->head);
            return CELL_EVENT_BATTLE;
        }
    }

    for (link = AreaLevelAt(g_areaMap, level)->links; !IsCellListEnd(&link->head); link++) {
        if (!IsCellAt(x, y, &link->head) || IsCellFlagSet(&link->head, 3)) {
            continue;
        }
        LatchCellDestination(&link->head, 5, 6, 7, CELL_FIELD_NONE, CELL_FIELD_NONE);
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
        if (kind->code == CELL_FROZEN_SCENE) {
            SetSceneScriptByIndex(7, 8);
        }
        return kind->kind;
    }

    for (object = AreaLevelAt(g_areaMap, level)->objects; !IsCellListEnd(&object->head); object++) {
        if (IsReservedObjectCell(&object->head)) {
            continue;
        }
        if (!IsCellAt(x, y, &object->head) || IsCellFlagSet(&object->head, 3)) {
            continue;
        }
        LatchCellDestination(&object->head, 5, 6, 7, CELL_FIELD_NONE, CELL_FIELD_NONE);
        SetSceneCell(&object->head);
        kind = FindCellKind(&object->head);
        if (kind == NULL) {
            return CELL_EVENT_OBJECT;
        }
        if (kind->kind == CELL_EVENT_OBJECT) {
            return kind->kind;
        }
    }

    for (exit = AreaLevelAt(g_areaMap, level)->exits; !IsCellListEnd(&exit->head); exit++) {
        if (!IsCellAt(x, y, &exit->head)) {
            continue;
        }
        LatchCellDestination(&exit->head, 3, 4, 5, CELL_FIELD_NONE, CELL_FIELD_NONE);
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
        LatchCellDestination(&exit->head, 3, 4, CELL_FIELD_NONE, 5, CELL_FIELD_NONE);
        g_cellDestArea = g_areaMap->area;
        return kind->kind;
    }

    for (box = AreaLevelAt(g_areaMap, level)->boxes; !IsCellListEnd(&box->head); box++) {
        if (!IsCellAt(x, y, &box->head)) {
            continue;
        }
        kind = FindCellKind(&box->head);
        if (kind == NULL) {
            continue;
        }
        LatchCellDestination(&box->head, 5, 6, 7, CELL_FIELD_NONE, CELL_FIELD_NONE);
        SetSceneCell(&box->head);
        SetCellScript(NULL);
        return kind->kind;
    }

    for (script = AreaLevelAt(g_areaMap, level)->scripts; !IsCellListEnd(&script->head); script++) {
        if (!IsCellAt(x, y, &script->head) || IsCellFlagSet(&script->head, 3)) {
            continue;
        }
        LatchCellDestination(&script->head, 5, 6, 7, CELL_FIELD_NONE, CELL_FIELD_NONE);
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
    g_cellX = cell->x;
    g_cellY = cell->y;
    g_cellCode = cell->code;
    g_cellDestX = bytes[x];
    g_cellDestY = bytes[y];
    if (direction != CELL_FIELD_NONE) {
        g_cellDestDirection = bytes[direction];
    }
    if (level != CELL_FIELD_NONE) {
        g_cellDestLevel = bytes[level];
    }
    if (area != CELL_FIELD_NONE) {
        g_cellDestArea = bytes[area];
    }
}

// 1 when x/y holds an enabled object of code 0x8d (and while no level is
// loaded).
RVA(0x00021dc0, 0x70)
b16 IsDarkCell(i16 x, i16 y) {
    ObjectCell* cell;
    if (g_areaLevel == NULL) {
        return true;
    }
    for (cell = g_areaLevel->objects; !IsCellListEnd(&cell->head); cell++) {
        if (IsCellAt(x, y, &cell->head) && !IsCellFlagSet(&cell->head, 3)
            && cell->head.code == CELL_DARK) {
            return true;
        }
    }
    return false;
}

// 1 when x/y holds an enabled object of code 0x8e (and while no level is
// loaded).
RVA(0x00021e30, 0x70)
b16 IsCellCommandBlocked(i16 x, i16 y) {
    ObjectCell* cell;
    if (g_areaLevel == NULL) {
        return true;
    }
    for (cell = g_areaLevel->objects; !IsCellListEnd(&cell->head); cell++) {
        if (IsCellAt(x, y, &cell->head) && !IsCellFlagSet(&cell->head, 3)
            && cell->head.code == CELL_COMMAND_BLOCKED) {
            return true;
        }
    }
    return false;
}

// The bit of x/y in the level's room bitmap.
RVA(0x00021ea0, 0x2b)
b16 IsRoomCell(i16 x, i16 y) {
    if (g_areaLevel == NULL) {
        return false;
    }
    return TestBit(g_areaLevel->roomBits, g_areaLevel->width * y + x);
}

// With `mode` set, whether x/y is blocked on `level`: a cell of any list
// there, else its wall or room bit. With `mode` 0, draws each enabled cell's
// automap icon instead and returns 0.
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
    for (warp = AreaLevelAt(g_areaMap, level)->warps; !IsCellListEnd(&warp->head); warp++) {
        if ((hit = CheckBlockingCell(&warp->head, mode, 6, x, y)) > 0) {
            return hit;
        }
    }
    for (battle = AreaLevelAt(g_areaMap, level)->battles; !IsCellListEnd(&battle->head); battle++) {
        if ((hit = CheckBlockingCell(&battle->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (link = AreaLevelAt(g_areaMap, level)->links; !IsCellListEnd(&link->head); link++) {
        if ((hit = CheckBlockingCell(&link->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (script = AreaLevelAt(g_areaMap, level)->scripts; !IsCellListEnd(&script->head); script++) {
        if ((hit = CheckBlockingCell(&script->head, mode, 3, x, y)) > 0) {
            return hit;
        }
    }
    for (exit = AreaLevelAt(g_areaMap, level)->exits; !IsCellListEnd(&exit->head); exit++) {
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
    for (box = AreaLevelAt(g_areaMap, level)->boxes; !IsCellListEnd(&box->head); box++) {
        if ((hit = CheckBlockingCell(&box->head, mode, 0xb, x, y)) > 0) {
            return hit;
        }
    }
    if (mode == 0) {
        return 0;
    }
    y = AreaLevelAt(g_areaMap, level)->width * y + x;
    if (TestBit(AreaLevelAt(g_areaMap, level)->blockBits, y)) {
        return 1;
    }
    return TestBit(AreaLevelAt(g_areaMap, level)->roomBits, y);
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
    for (warp = g_areaLevel->warps; !IsCellListEnd(&warp->head); warp++) {
        if (IsCellAt(x, y, &warp->head) && !IsCellFlagSet(&warp->head, 6)) {
            return warp->head.code;
        }
    }
    for (link = g_areaLevel->links; !IsCellListEnd(&link->head); link++) {
        if (IsCellAt(x, y, &link->head) && !IsCellFlagSet(&link->head, 3)) {
            return link->head.code;
        }
    }
    for (script = g_areaLevel->scripts; !IsCellListEnd(&script->head); script++) {
        if (IsCellAt(x, y, &script->head) && !IsCellFlagSet(&script->head, 3)) {
            return script->head.code;
        }
    }
    return 0;
}

RVA(0x000221f0, 0x99)
i16 IsStepBarred(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 turn) {
    i16 facing;
    DoorCell* door;
    if (g_areaLevel == NULL) {
        return 0;
    }
    facing = TurnDirection(direction, turn);
    for (door = g_areaLevel->doors; !IsCellListEnd(&door->head); door++) {
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
        for (warp = (firstWarp); !IsCellListEnd(&warp->head); warp++) {                            \
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
    dx += g_party.field.pos.x;
    dy += g_party.field.pos.y;
    ReturnWarpCodeAt(g_areaLevel->warps, dx, dy, 0);
    return 0;
}

// The warp code at (dx, dy) from the party: a warp under the party itself
// answers with bit 3 set, and one fixed spot (area 9, level 6, at 2/5 looking
// one cell west) answers 0x42.
// @early-stop register allocation: retail keeps dy in esi and dx in memory,
// loads the party x as a dword and y as a word, forms the offset x in edx and
// spills the offset y into dy's slot; the in-place dx/dy update (as in
// GetWarpCodeAtOffset) keeps dx/dy in ebp/ebx, and x/y locals updated in
// place or x/dx and dy/y mixes each lose more.
RVA(0x00022460, 0xd4)
i16 GetCellAtOffset(i16 dx, i16 dy) {
    if (g_areaLevel == NULL) {
        return 0;
    }
    if (g_party.field.pos.x == 2 && g_party.field.pos.y == 5
        && g_party.field.pos.area == MAP_AREA_SHINJUKU_TOCHO && g_party.field.pos.level == 6
        && dx == -1 && dy == 0) {
        return CELL_STAIRS_UP;
    }
    ReturnWarpCodeAt(g_areaLevel->warps, g_party.field.pos.x, g_party.field.pos.y, 8);
    dx += g_party.field.pos.x;
    dy += g_party.field.pos.y;
    ReturnWarpCodeAt(g_areaLevel->warps, dx, dy, 0);
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
    for (box = g_areaLevel->boxes; !IsCellListEnd(&box->head); box++) {
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
b16 IsLevelMapRevealed(void) {
    if (g_areaLevel == NULL) {
        return false;
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
    for (exit = g_areaLevel->exits; !IsCellListEnd(&exit->head); exit++) {
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

RVA(0x000226c0, 0xc)
void SetPanelImage(i16 image) {
    s_panelImage = image;
}

static __inline Panel* InitPanelImage(Panel* panel, i16 image, i16 count) {
    if (panel == NULL) {
        panel = AllocCleared(1, offsetof(Panel, rows) + count * sizeof(PanelRow));
        panel->image = 0;
    }
    panel->count = count;
    if (panel->image) {
        panel->image = FreeImageHandle(panel->image);
    }
    panel->image = LoadMenuImage(image);
    panel->flags = 0;
    SetPanelPosition(panel, 0, 0);
    return panel;
}

RVA(0x000226d0, 0xc0)
Panel* CreateImagePanel(Panel* panel, i16 image, i16 count, i16 unused) {
    i16 first;
    i16 i;
    panel = InitPanelImage(panel, image, count);
    if (image == 0x12) {
        image = 0;
    } else if (image == 0x110 && count == 13) {
        image = 1;
    }
    first = s_imageFirstRow[image & 15];
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, first + i, PanelRowHandlerDefault);
    }
    return panel;
}

RVA(0x00022790, 0x26)
i16 PanelRowHandlerDefault(PanelRow* row, i16 value, i16 op) {
    ApplyRowCheck(row, value, op);
    IsPanelActive(value);
    return value;
}

RVA(0x000227c0, 0xa6)
Panel* CreateKindPanel(Panel* panel, i16 image, i16 count, i16 kind) {
    i16 first;
    i16 i;
    panel = InitPanelImage(panel, image, count);
    first = s_kindFirstRow[kind];
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, first + i, PanelRowHandlerDefault);
    }
    return panel;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x00022870, 0x99)
Panel* CreateSequentialPanel(Panel* panel, i16 image, i16 count) {
    i16 i;
    panel = InitPanelImage(panel, image, count);
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        InitPanelRow(panel, i, image + i, PanelRowHandlerDefault);
    }
    return panel;
}

RVA(0x00022910, 0x70)
Panel* CreatePositionedPanel(Panel* panel, i16 x, i16 y, i16 count, i16 kind) {
    if (s_panelImage == -1) {
        s_panelImage = 0x112;
    }
    panel = CreateKindPanel(panel, s_panelImage, count, kind);
    SetPanelPosition(panel, x == -1 ? 0 : x, y == -1 ? 0 : y);
    s_panelImage = -1;
    return panel;
}

RVA(0x00022980, 0x3c)
Panel* ReleasePanel(Panel* panel, i16 freePanel) {
    if (panel == NULL) {
        return NULL;
    }
    ClearPanelChecks(panel);
    panel->image = FreeImageHandle(panel->image);
    if (freePanel) {
        panel = FreeBlock(panel);
    }
    return panel;
}

RVA(0x000229c0, 0x22)
void SetPanelRowState(Panel* panel, i16 index, GZ_ENUM_PARAM(PanelFlags, u16) flags) {
    if (index < GetPanelRowCount(panel)) {
        AssignPanelRowState(panel, index, flags);
    }
}

RVA(0x000229f0, 0x25)
void PaintPanel(Panel* panel, i16 mode) {
    i16 token = SaveDrawState();
    DrawPanel(panel, NULL, mode);
    RestoreDrawState(token);
}

RVA(0x00022a20, 0x6d)
i16 RunPanelInput(Panel* panel) {
    i16 result;
    if (panel == NULL) {
        return PANEL_INPUT_NONE;
    }
    ExchangeActivePanel(panel);
    if (!(panel->flags & PANEL_ALLOW_RIGHT_CLICK) && TakeMouseCancelSound()) {
        g_hoveredObjectId = g_selectedObjectId = -1;
        ExchangeActivePanel(NULL);
        return PANEL_INPUT_CANCELLED;
    }
    result = PollPanel(panel);
    // Codegen constraint: preserve the explicit no-selection result assignment.
    if (result == PANEL_INPUT_NONE) {
        result = PANEL_INPUT_NONE;
    }
    ExchangeActivePanel(NULL);
    return result;
}

// @identity-TODO: a second entry of ClearPanelChecks (its callers are the
// menu code).
RVA(0x00022a90, 0xe)
void ClearPanelChecksAgain(Panel* panel) {
    ClearPanelChecks(panel);
}

RVA(0x00022aa0, 0xd)
void SetFlagBits(GZ_ENUM_STORAGE(PanelFlags, u16) * flags, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    *flags |= mask;
}

RVA(0x00022ab0, 0xe)
void ClearFlagBits(GZ_ENUM_STORAGE(PanelFlags, u16) * flags, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    *flags &= ~mask;
}

RVA(0x00022ac0, 0x11)
i16 ToggleFlagBits(GZ_ENUM_STORAGE(PanelFlags, u16) * flags, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    *flags ^= mask;
    mask &= *flags;
    return mask;
}

RVA(0x00022ae0, 0x15)
b32 TestFlagBits(u16* flags, u16 mask) {
    return (*flags & mask) != 0;
}

RVA(0x00022b00, 0x21)
b32 TestPanelRowFlags(Panel* panel, i16 row, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    return TestFlagBits(&GetPanelRow(panel, row)->flags, mask);
}

RVA(0x00022b30, 0x4a)
void SetPanelRowFlags(Panel* panel, i16 row, GZ_ENUM_PARAM(PanelFlags, u16) mask, i16 on) {
    if (on == false) {
        ClearFlagBits(&GetPanelRow(panel, row)->flags, mask);
        return;
    }
    SetFlagBits(&GetPanelRow(panel, row)->flags, mask);
}

RVA(0x00022b80, 0x15)
b32 IsPanelRowChecked(Panel* panel, i16 row) {
    return TestPanelRowFlags(panel, row, PANEL_ROW_CHECKED);
}

RVA(0x00022ba0, 0x2f)
void ClearPanelChecks(Panel* panel) {
    i16 i;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        ClearPanelRowCheck(GetPanelRow(panel, i));
    }
}

// Copies each row's check (bit 0) to bit 4 and bit 15 to bit 7, then clears
// the checks.
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00022bd0, 0x84)
void SavePanelChecks(Panel* panel) {
    i16 i;
    PanelRow* row;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        row = GetPanelRow(panel, i);
        ClearFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK);
        if (TestFlagBits(&row->flags, PANEL_ROW_CHECKED) == true) {
            SetFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK);
        }
        ClearFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED);
        if (TestFlagBits(&row->flags, PANEL_INPUT_DISABLED) == true) {
            SetFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED);
        }
    }
    ClearPanelChecks(panel);
}

// Brings the saved bits back (bit 4 to the check, bit 7 to bit 15).
// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00022c60, 0x7a)
void RestorePanelChecks(Panel* panel) {
    i16 i;
    PanelRow* row;
    for (i = 0; i < GetPanelRowCount(panel); i++) {
        row = GetPanelRow(panel, i);
        ClearPanelRowCheck(row);
        if (TestFlagBits(&row->flags, PANEL_ROW_SAVED_CHECK) == true) {
            SetFlagBits(&row->flags, PANEL_ROW_CHECKED);
        }
        ClearFlagBits(&row->flags, PANEL_INPUT_DISABLED);
        if (TestFlagBits(&row->flags, PANEL_ROW_SAVED_INPUT_DISABLED) == true) {
            SetFlagBits(&row->flags, PANEL_INPUT_DISABLED);
        }
    }
}

RVA(0x00022ce0, 0xd)
void SetPanelFlags(Panel* panel, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    panel->flags |= mask;
}

RVA(0x00022cf0, 0xe)
void ClearPanelFlags(Panel* panel, GZ_ENUM_PARAM(PanelFlags, u16) mask) {
    panel->flags &= ~mask;
}
