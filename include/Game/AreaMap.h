#ifndef GITEN_GAME_AREAMAP_H
#define GITEN_GAME_AREAMAP_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/CellCode.h>
#include <Game/ViewDirection.h>
#include <Ints.h>

// The loaded area map (LoadAreaMap reads it into a 0x2c00-byte buffer) and the
// per-level cell lists CheckCellEvent walks. Every cell record starts with its
// map cell and its kind code; a list ends at x == 0xff. The records' other
// bytes are addressed by offset: an event-flag pair (bank, index) that
// disables the cell, and the destination bytes LatchCellDestination copies.

typedef struct CellHead {
    u8 x;
    u8 y;
    u8 code;
} CellHead;

// A cell list ends at an x of CELL_LIST_X_END.
#define CELL_LIST_X_END 0xff
#define IsCellListEnd(cell) ((cell)->x == CELL_LIST_X_END)

GZ_ENUM_BEGIN(CellEventKind)
    CELL_EVENT_NONE = 0,
    CELL_EVENT_WARP = 1,
    CELL_EVENT_BATTLE = 2,
    CELL_EVENT_SCRIPT = 3,
    CELL_EVENT_OBJECT = 4,
    CELL_EVENT_WORLD_EXIT = 5,
    CELL_EVENT_FROZEN_SCENE = 6,
    CELL_EVENT_CHUTE = 7,
    CELL_EVENT_STAIRS = 8,
    CELL_EVENT_FORCED_MOVE = 9,
    CELL_EVENT_FLOOR_PROPERTY = 10,
    CELL_EVENT_TRAP = 11,
    CELL_EVENT_FADE_SCENE = 12,
    CELL_EVENT_INERT = 13,
    CELL_EVENT_MARKED_WARP = 14
GZ_ENUM_END(CellEventKind)

GZ_ENUM_FLAGS_BEGIN(CellKindFlags, u8)
    CELL_KIND_CHECK_FACING = 1
GZ_ENUM_FLAGS_END(CellKindFlags)

// A code's entry in the cell-kind table: `kind` is what CheckCellEvent returns
// (RunCellEvent's case).
// @identity-TODO: CELL_INERT's authored role is unrecovered; its mapped event
// kind takes no action in either version's event dispatcher.
typedef struct CellKind {
    GZ_ENUM_STORAGE(CellCode, u8) code;
    GZ_ENUM_STORAGE(CellEventKind, u8) kind;
    GZ_ENUM_STORAGE(CellKindFlags, u8) flags;
    u8 pad03;
} CellKind;

// Destination x/y at +3/+4, level +5, area +8; disabled by the flag at +6.
typedef struct WarpCell {
    CellHead head;
    u8 dest[3];
    u8 disableFlag[2];
    u8 destArea;
} WarpCell;

// Returns kind 2 and hands its second flag to SetFieldPair.
// @identity-TODO: that these start the field encounters (SetFieldPair's pair
// being the flag set on winning) is inferred.
typedef struct BattleCell {
    CellHead head;
    u8 disableFlag[2];
    u8 dest[2];
    u8 battleFlag[2];
    u8 pad09[2];
} BattleCell;

// A link: active only while facing a direction in `facings` (for kinds with
// CELL_KIND_CHECK_FACING); a world-map link (kind 5) reads the same byte as the map layer
// and travels to spot (`spotX`, `spotY`) on it. `script` is the offset in the
// area map of the script it runs.
typedef struct LinkCell {
    CellHead head;
    u8 disableFlag[2];
    u8 facings;
    i16 spotX;
    u8 spotY;
    u8 pad09[3];
    u16 script;
} LinkCell;

// A level object (SpawnLevelObjects places them; codes 0x48..0x4e are skipped
// here); only kind 4 is an event.
typedef struct ObjectCell {
    CellHead head;
    u8 disableFlag[2];
    u8 dest[3];
} ObjectCell;

// An exit: kind 7 moves within the current area (destination level at +5).
// @identity-TODO: what the exits without a kind entry (event 11) are is
// unrecovered.
typedef struct ExitCell {
    CellHead head;
    u8 dest[3];
    u8 disableFlag[2];
    union {
        u8 damagePercent;
        u8 alignmentMask;
    } trap;
    u8 secondaryDamagePercent;
} ExitCell;

// A treasure box of the level: its cell and kind (head.code; 0x8a boxes use
// the lower half of the box texture), the destination bytes CheckCellEvent
// latches, and the event flag set once it is opened (which also takes it out
// of IsCellBlocked); the list ends at x 0xff.
// @identity-TODO: bytes +3..+4, +8..+0xa and +0xd..+0xf are unrecovered.
typedef struct TreasureBox {
    CellHead head;
    u8 pad03[2];
    u8 dest[3];
    u8 pad08[3];
    u8 flagBank;
    u8 flagIndex;
    u8 pad0d[3];
} TreasureBox;

// A cell that runs the script at `script` (an offset in the area map) and
// swaps the scene record's two parameter triples.
typedef struct ScriptCell {
    CellHead head;
    u8 disableFlag[2];
    u8 dest[3];
    u8 pad08[7];
    u16 script;
} ScriptCell;

// A level's music choice: played while event flag bank/index is clear.
typedef struct LevelMusic {
    u8 flagBank;
    u8 flagIndex;
    u8 music;
} LevelMusic;

// The five choices (copied whole when a level is decoded).
typedef struct LevelMusicSet {
    LevelMusic choices[5];
} LevelMusicSet;

// A door cell: facing (high nibble) and wall kind (low nibble) in the code
// byte, disabled by the flag at +3. Kind 11 does not bar a step.
typedef struct DoorCell {
    CellHead head;
    u8 disableFlag[2];
    u8 pad05[11];
} DoorCell;

// A four-byte map-object spawn record; xLayer == 0xff terminates the list.
typedef struct MapSpawn {
    u8 xLayer; // Low seven bits are x; the top bit selects the object layer.
    u8 y;
    u8 flagBank;
    u8 flagIndex;
} MapSpawn;

#define GetMapSpawnX(spawn) ((spawn)->xLayer & 0x7f)

#define GetMapSpawnLayer(spawn) ((spawn)->xLayer >> 7)

// `walls` is the wall word of each cell (GetWallMap), `width`/`height` the
// map size (GetMapSize, GetAreaSize), `music` the five choices and
// `defaultMusic` the fallback PlayLevelMusic picks from.
// `spawns` is the map-object spawn list (0x421010), `roomWalls`/`roomDoors`
// the room-region inputs (BuildRoomMap), `wallSet`/`wallVariant` the wall
// textures (0xff: keep).
// `floor` is the displayed floor: negative for basements, zero for no label.
// Clearing the reveal flag shows unexplored cells on the automap.
typedef struct AreaLevel {
    u8* blockBits;
    u16* walls;
    WarpCell* warps;
    BattleCell* battles;
    LinkCell* links;
    ObjectCell* objects;
    MapSpawn* spawns;
    ScriptCell* scripts;
    DoorCell* doors;
    ExitCell* exits;
    TreasureBox* boxes;
    u8* roomDoors;
    u8* roomWalls;
    i16 width;
    i16 height;
    u8 revealFlagBank;
    u8 revealFlagIndex;
    i16 floor;
    u8 wallSet;
    u8 wallVariant;
    u8 byte3e;
    u8 byte3f;
    LevelMusicSet music;
    u8 defaultMusic;
    u8* roomBits;
} AreaLevel;

// `name` is the area's name (GetAreaName; the save header and the automap
// copy it).
typedef struct AreaMap {
    i16 area;
    char* name;
    i16 levelCount;
    AreaLevel* levels[];
} AreaMap;

static __inline AreaLevel* AreaLevelAt(AreaMap* map, i16 level) {
    return map->levels[level];
}

// An area-map record as stored (s_areaRecord): the level headers hold 16-bit
// record offsets where the decoded AreaLevel holds pointers, so each decoded
// level sits 28 bytes further on per level before it (plus the grown header).
typedef struct AreaLevelRecord {
    u16 blockBitsOffset;
    u16 wallsOffset;
    u16 warpsOffset;
    u16 battlesOffset;
    u16 linksOffset;
    u16 objectsOffset;
    u16 spawnsOffset;
    u16 scriptsOffset;
    u16 doorsOffset;
    u16 exitsOffset;
    u16 boxesOffset;
    u16 roomDoorsOffset;
    u16 roomWallsOffset;
    i16 width;
    i16 height;
    u8 revealFlagBank;
    u8 revealFlagIndex;
    i16 floor;
    u8 wallSet;
    u8 wallVariant;
    u8 byte24;
    u8 byte25;
    LevelMusicSet music;
    u8 defaultMusic;
    u16 roomBitsOffset;
} AreaLevelRecord;

typedef struct AreaRecord {
    i16 area;
    u16 nameOffset;
    i16 levelCount;
    u16 levelOffsets[1];
} AreaRecord;

static __inline u16 GetAreaLevelOffset(const AreaRecord* record, i16 level) {
    return record->levelOffsets[level];
}

extern AreaMap* g_areaMap;
extern AreaLevel* g_areaLevel;

// The cell event's latch (LatchCellDestination): the cell's position and
// code, and the destination's x, y, direction, level and area RunCellEvent
// travels to (the position and direction are latched but never read).
// @identity-TODO: what the codes 0x64, 0x67 and
// 0x70..0x76 of a kind-9 cell are is unrecovered.
extern i16 g_cellX;
extern i16 g_cellY;
extern GZ_ENUM_STORAGE(CellCode, u8) g_cellCode;
extern u8 g_cellDestDirection;
extern i16 g_cellDestX;
extern i16 g_cellDestY;
extern i16 g_cellDestLevel;
extern i16 g_cellDestArea;

b16 IsCellAt(i16 x, i16 y, const CellHead* cell);
b16 IsCellFlagSet(const CellHead* cell, i16 offset);
const CellKind* FindCellKind(const CellHead* cell);

// Latches the cell and the destination bytes at the given offsets (x, y, then
// direction, level and area where the offset is not -1) for RunCellEvent.
// LatchCellDestination's field arguments are byte offsets into the cell record;
// CELL_FIELD_NONE marks a field the cell does not have.
#define CELL_FIELD_NONE (-1)

void LatchCellDestination(const CellHead* cell, i16 x, i16 y, i16 direction, i16 level, i16 area);

// Codes 0x48..0x4e select the legacy NPC direction mask.
b16 IsReservedObjectCell(const CellHead* cell);

// Decodes an area-map record into `map` (header, levels and their lists).
void DecodeAreaMap(AreaMap* map, u8* record);

// The area name and current floor, also stored in the save header.
char* GetAreaName(void);
i16 GetLevelFloor(void);

// The level-cell queries: dark (0x8d) and command-blocking (0x8e) objects,
// the room-bitmap bit, blocking cells on a level (or their automap icons), the
// event cell code, locked doors, the wall word, warp codes near the party, the
// room lists, the level's own flag and the enabled exit at x/y.
b16 IsDarkCell(i16 x, i16 y);
b16 IsCellCommandBlocked(i16 x, i16 y);
b16 IsRoomCell(i16 x, i16 y);
// What IsCellBlocked and CheckBlockingCell do with a level's cells: draw
// their automap icons, or test whether one blocks x/y.
GZ_ENUM_BEGIN_SPLIT(CellScanMode, i16)
    CELL_SCAN_DRAW_ICONS = 0,
    CELL_SCAN_TEST = 1
GZ_ENUM_END_SPLIT(CellScanMode)

i16 IsCellBlocked(i16 level, GZ_ENUM_PARAM(CellScanMode, i16) mode, i16 x, i16 y);
i16 CheckBlockingCell(
    const CellHead* cell,
    GZ_ENUM_PARAM(CellScanMode, i16) mode,
    i16 flagOffset,
    i16 x,
    i16 y
);
GZ_ENUM_RETURN(CellCode, i16) GetEventCellCode(i16 x, i16 y);
// Returns the barring door's code (0: none) for a step from x/y facing
// `direction`, move `turn`.
i16 IsStepBarred(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction, i16 turn);
i16 WrapMapCoord(i16 value, i16 size);
i16 ClampMapCoord(i16 value, i16 size);
i16 RevealAreaMapAt(i16 x, i16 y);
void WrapMapPosition(i16* x, i16* y);
void ClampMapPosition(i16* x, i16* y);
i16 GetWarpCodeAtOffset(i16 dx, i16 dy);
i16 GetCellAtOffset(i16 dx, i16 dy);
// GetLevelList's lists: the level's rooms or its doors.
GZ_ENUM_BEGIN_SPLIT(LevelListKind, i16)
    LEVEL_LIST_ROOMS = 0,
    LEVEL_LIST_DOORS = 1
GZ_ENUM_END_SPLIT(LevelListKind)
u8* GetLevelList(GZ_ENUM_PARAM(LevelListKind, i16) which);
b16 IsLevelMapRevealed(void);
ExitCell* CopyExitAt(i16 x, i16 y, ExitCell* out);

// The level's treasure boxes (NULL without a level) and the box at x/y (with
// `select`, made the scene cell).
TreasureBox* GetMapTreasureBoxes(void);
TreasureBox* FindTreasureBoxAt(i16 x, i16 y, i16 select);

#endif // GITEN_GAME_AREAMAP_H
