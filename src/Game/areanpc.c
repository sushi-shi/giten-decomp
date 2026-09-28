// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AreaLevel.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/DoorRegion.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/PartyCommand.h>
#include <Game/SkillUse.h>
#include <Gfx/Background.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>

DATA(0x00068b8c)
NpcTexture g_npcTextures[6] = {
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
    {0, 0xffff},
};

// The room-region grids (a byte per cell of a 64x64 map in a memory handle):
// the current regions and the copy RoomRegionsChanged compares against.
DATA(0x0007d62c)
static i32 s_roomRegions;

DATA(0x0007d630)
static i32 s_prevRegions;

static __inline void EnsureGridByteStorage(i32* grid) {
    if (!*grid) {
        *grid = AllocHandle(0x1000);
    }
}

static __inline i16 GridByteIndex(i16 x, i16 y) {
    return y * 64 + x;
}

// Sets region `value` on every cell of the rectangle x0..x1, y0..y1.
RVA(0x0001ea90, 0x3f)
void FillRegionRect(i16 x0, i16 y0, i16 x1, i16 y1, u8 value) {
    i16 x;
    i16 y;
    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            SetRoomRegion(x, y, value);
        }
    }
}

RVA(0x0001ead0, 0x1d)
void SetRoomRegion(i16 x, i16 y, u8 value) {
    SetGridByte(&s_roomRegions, x, y, value);
}

// Sets cell x/y of a 64x64 byte grid (allocated on first use).
RVA(0x0001eaf0, 0x3d)
void SetGridByte(i32* grid, i16 x, i16 y, u8 value) {
    u8* bytes;
    EnsureGridByteStorage(grid);
    bytes = HandleWritePtr(*grid);
    bytes[GridByteIndex(x, y)] = value;
}

// Gives region `value` to every cell of the w x h map still without one.
RVA(0x0001eb30, 0x49)
void FillEmptyRegions(i16 width, i16 height, u8 value) {
    i16 x;
    i16 y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (GetRoomRegion(x, y) == 0xff) {
                SetRoomRegion(x, y, value);
            }
        }
    }
}

RVA(0x0001eb80, 0x18)
u8 GetRoomRegion(i16 x, i16 y) {
    return GetGridByte(&s_roomRegions, x, y);
}

RVA(0x0001eba0, 0x39)
u8 GetGridByte(i32* grid, i16 x, i16 y) {
    u8* bytes;
    EnsureGridByteStorage(grid);
    bytes = HandleReadPtr(*grid);
    return bytes[GridByteIndex(x, y)];
}

// Whether the flag pair at `offset` of an entry is "on": its flag set, or
// clear for a pair whose bank byte (at offset + 2) is nonzero.
// @identity-TODO: the meaning of the nonzero byte is unrecovered.
RVA(0x0001ebe0, 0x2e)
b16 IsRegionFlagOn(u8* list, i16 offset) {
    b16 invert = list[offset + 2] != 0;
    return (IsCellFlagSet((CellHead*)list, offset) != 0) ^ invert;
}

// Marks the regions of a level's room list: entries of `stride` bytes (a
// rectangle x0, y0, x1, y1 then a flag pair) fill their rectangle, marker
// entries (0xff, then a flag pair; stride - 3 bytes) fill the cells left; an
// entry whose flag is on is skipped. Regions are numbered from `code`; the
// list ends with 0xff 0xff.
RVA(0x0001ec10, 0xad)
void MarkRegionList(u8* list, i16 stride, u8 code, i16 width, i16 height) {
    i16 offset = 0;
    for (;; code++) {
        u8* entry = &list[offset];
        if (list[offset] == 0xff && entry[1] == 0xff) {
            return;
        }
        if (list[offset] != 0xff) {
            if (!IsRegionFlagOn(list, offset + 4)) {
                FillRegionRect(entry[0], entry[1], entry[2], entry[3], code);
            }
            offset += stride;
        } else {
            if (!IsRegionFlagOn(list, offset + 1)) {
                FillEmptyRegions(width, height, code);
            }
            offset += stride - 3;
        }
    }
}

// The data of entry `index` of a room list (after its rectangle or marker),
// NULL past the end.
// @early-stop: the returned address is formed as [offset + list] in retail
// and [list + offset] here.
RVA(0x0001ecc0, 0x60)
u8* FindRegionData(u8* list, i16 stride, i16 index) {
    i16 offset = 0;
    i16 i = 0;
    for (;;) {
        if (list[offset] == 0xff && list[offset + 1] == 0xff) {
            return NULL;
        }
        if (list[offset] != 0xff) {
            if (i == index) {
                return offset + list + 4;
            }
            offset += stride;
        } else {
            if (i == index) {
                return offset + list + 1;
            }
            offset += stride - 3;
        }
        i++;
    }
}

// Clears the region grid, then marks the level's rooms (0x1f-byte entries,
// regions 0..) and doors (13-byte entries, regions 0x80..).
RVA(0x0001ed20, 0x4b)
void MarkRoomRegions(u8* rooms, u8* doors, i16 width, i16 height) {
    FillRegionRect(0, 0, 0x3f, 0x3f, 0xff);
    MarkRegionList(rooms, 0x1f, 0, width, height);
    MarkRegionList(doors, 0xd, 0x80, width, height);
}

static __inline void SaveRoomRegions(i16 width, i16 height) {
    i16 x;
    i16 y;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            SetPrevRegion(x, y, GetRoomRegion(x, y));
        }
    }
}

static __inline i16 RestoreRoomRegions(i16 width, i16 height) {
    i16 x;
    i16 y;
    i16 changed = 0;
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            i16 now = GetRoomRegion(x, y);
            changed |= now - GetPrevRegion(x, y);
            SetRoomRegion(x, y, GetPrevRegion(x, y));
        }
    }
    return changed;
}

// Whether re-marking the rooms would change any cell's region (the grid is
// left as it was).
// @early-stop register allocation: width and height swap ebx/ebp.
RVA(0x0001ed70, 0xb9)
i16 RoomRegionsChanged(u8* rooms, u8* doors, i16 width, i16 height) {
    SaveRoomRegions(width, height);
    MarkRoomRegions(rooms, doors, width, height);
    return RestoreRoomRegions(width, height);
}

RVA(0x0001ee30, 0x1d)
void SetPrevRegion(i16 x, i16 y, u8 value) {
    SetGridByte(&s_prevRegions, x, y, value);
}

RVA(0x0001ee50, 0x18)
u8 GetPrevRegion(i16 x, i16 y) {
    return GetGridByte(&s_prevRegions, x, y);
}

// The region (room or door) of map cell x/y, wrapped into the map.
RVA(0x0001ee70, 0x4f)
i16 GetMapCellCode(i16 x, i16 y) {
    i16 width;
    i16 height;
    GetMapSize(&width, &height);
    x = WrapMapCoord(x, width);
    return GetRoomRegion(x, WrapMapCoord(y, height));
}

RVA(0x0001eec0, 0x18)
i16 GetPartyCellCode(void) {
    return GetMapCellCode(g_field.pos.x, g_field.pos.y);
}

// Whether a region code is a door (0x80..).
RVA(0x0001eee0, 0xa)
i16 IsObjectCell(i16 code) {
    return code & 0x80;
}

RVA(0x0001eef0, 0x1a)
i16 GetCellSpawnRate(i16 code) {
    DoorRegionData* table = GetCellObjectTable(code);
    if (!table) {
        return 0;
    }
    return table->spawnInterval;
}

RVA(0x0001ef10, 0x19)
i16 CellCodeDiffers(i16 code, i16 x, i16 y) {
    return GetMapCellCode(x, y) - code;
}

RVA(0x0001ef30, 0x24)
void ResetFieldScene(void) {
    ReleaseNpcTextures();
    ResetFieldLayer(1);
    ResetFieldLayer(0);
    SetCurrentRoomCode(-1);
}

// The object of layer 0 or 1 a door's data names (0x20 for none).
RVA(0x0001ef60, 0x26)
i16 LookupCellObject(DoorRegionData* table, i16 layer) {
    if (table && layer >= 0 && layer <= 1) {
        return table->objects[layer];
    }
    return 0x20;
}

RVA(0x0001ef90, 0x1e)
u8* GetRoomData(i16 code) {
    return FindRegionData(GetLevelList(0), 0x1f, code & 0x7f);
}

RVA(0x0001efb0, 0x1e)
DoorRegionData* GetCellObjectTable(i16 code) {
    void* data = FindRegionData(GetLevelList(1), 0xd, code & 0x7f);
    return data;
}

// Enters region `code`: a room loads its NPC images, a door its two enemy
// groups (object ids 0x20..0x201f).
RVA(0x0001efd0, 0x95)
void EnterRoom(i16 code) {
    DoorRegionData* table;
    i16 object;
    SetCurrentRoomCode(code);
    if (code == 0xff || code == -1) {
        return;
    }
    if (!IsObjectCell(code)) {
        u8* data = GetRoomData(code);
        if (data) {
            LoadAreaNpcImages(data);
        }
        return;
    }
    table = GetCellObjectTable(code);
    if (!table) {
        return;
    }
    object = LookupCellObject(table, 0);
    if (object >= 0x20 && object <= 0x201f) {
        LoadEnemyGroupSlot(0, object);
    }
    object = LookupCellObject(table, 1);
    if (object >= 0x20 && object <= 0x201f) {
        LoadEnemyGroupSlot(1, object);
    }
}

// Re-enters the party's region when it changed (resetting the field and
// respawning), else re-picks the two special pictures of area 0x85 level 3.
RVA(0x0001f070, 0xb6)
void UpdateCurrentRoom(void) {
    i16 code;
    if (!CellCodeDiffers(GetCurrentRoomCode(), g_field.pos.x, g_field.pos.y)) {
        if (g_field.pos.area == 0x85 && g_field.pos.level == 3 && g_field.pos.x == 3) {
            if (g_field.pos.y == 4) {
                LoadNpcTexture(0, 0x53, 0);
            } else if (g_field.pos.y == 5) {
                LoadNpcTexture(0, 0x4c, 0);
            }
        }
        return;
    }
    RequestFieldRefresh();
    ResetFieldObjects();
    ResetFieldScene();
    code = GetMapCellCode(g_field.pos.x, g_field.pos.y);
    EnterRoom(code);
    SpawnMapObjects(code);
}

// After a step through a door: when the cell behind the party is in another
// region, resets the field and enters that region.
RVA(0x0001f130, 0x99)
void FinishDoorStep(void) {
    i16 x = g_field.pos.x;
    i16 y = g_field.pos.y;
    i16 direction = TurnDirection(g_field.pos.direction, g_field.moveCommand);
    StepMapCoordBy(&x, &y, direction, 0, -1);
    if (CellCodeDiffers(GetCurrentRoomCode(), x, y)) {
        i16 code;
        ResetFieldObjects();
        ResetFieldScene();
        code = GetMapCellCode(x, y);
        EnterRoom(code);
        SpawnMapObjects(code);
    }
}

// The current area's NPCs and how many are placed.
DATA(0x0007d300)
static AreaNpc s_npcs[16];

DATA(0x0007d5dc)
static i16 s_npcCount;

// Which of the two objects of the cell at x/y is object `id`: 0 or 1, else -1
// (also when the cell holds no objects).
RVA(0x0001f1d0, 0x75)
i16 FindCellObject(i16 id, i16 x, i16 y) {
    i16 code = GetMapCellCode(x, y);
    DoorRegionData* table;
    if (!IsObjectCell(code)) {
        return -1;
    }
    table = GetCellObjectTable(code);
    if (!table) {
        return -1;
    }
    if (LookupCellObject(table, 0) == id) {
        return 0;
    }
    return LookupCellObject(table, 1) != id ? -1 : 1;
}

// The slot the next NPC takes, -1 when all 16 are placed.
RVA(0x0001f250, 0x11)
i16 NextNpcSlot(void) {
    if (s_npcCount >= 16) {
        return -1;
    }
    return s_npcCount;
}

RVA(0x0001f270, 0x12)
void CountPlacedNpc(void) {
    if (s_npcCount < 16) {
        s_npcCount++;
    }
}

// The picture of NPC code `code`.
RVA(0x0001f290, 0xe)
i16 GetNpcImageOfCode(i16 code) {
    // Retail indexes the physical recovery list from its fifth entry.
    return g_physicalRecoveryConditions[code + 4];
}

RVA(0x0001f2a0, 0xa)
void ClearAreaNpcs(void) {
    s_npcCount = 0;
}

// Places an NPC from its map record: cell x/y, picture code, event flag
// (bank, index), scene script (file, entry) and texture slot.
RVA(0x0001f2b0, 0x93)
void AddAreaNpc(const u8* record) {
    i16 slot = NextNpcSlot();
    if (slot < 0) {
        return;
    }
    s_npcs[slot].x = *record++;
    s_npcs[slot].y = *record++;
    s_npcs[slot].image = GetNpcImageOfCode(*record++);
    s_npcs[slot].flagBank = *record++;
    s_npcs[slot].flagIndex = *record++;
    s_npcs[slot].script = *record++;
    s_npcs[slot].entry = *record;
    s_npcs[slot].textureSlot = record[1];
    CountPlacedNpc();
}

RVA(0x0001f350, 0x18)
b16 IsReservedObjectCell(const CellHead* cell) {
    if (cell->code >= 0x48 && cell->code <= 0x4e) {
        return true;
    }
    return false;
}

// Draws the NPCs still present on the view cell being drawn (with the
// approach offset of the view position).
RVA(0x0001f370, 0xad)
void DrawAreaNpcs(void) {
    i16 i;
    MapCoord offset;
    for (i = 0; i < s_npcCount; i++) {
        if (IsEventFlagSet(s_npcs[i].flagBank, s_npcs[i].flagIndex)) {
            continue;
        }
        if (s_npcs[i].x != g_viewCellX || s_npcs[i].y != g_viewCellY) {
            continue;
        }
        offset = GetApproachOffset(g_viewLateral, g_viewDepth);
        DrawNpcAt(offset.x, offset.y, g_viewDepth, &s_npcs[i], i);
    }
}

RVA(0x0001f420, 0x7)
i16 GetAreaNpcCount(void) {
    return s_npcCount;
}

RVA(0x0001f430, 0x2a)
b32 IsAreaNpcGone(i16 npc) {
    return IsEventFlagSet(s_npcs[npc].flagBank, s_npcs[npc].flagIndex);
}

RVA(0x0001f460, 0x17)
i16* GetAreaNpcCell(i16 npc) {
    return &s_npcs[npc].x;
}

RVA(0x0001f480, 0x18)
i16 GetAreaNpcTextureSlot(i16 npc) {
    return s_npcs[npc].textureSlot;
}

RVA(0x0001f4a0, 0x17)
AreaNpc* GetAreaNpc(i16 npc) {
    return &s_npcs[npc];
}

// Marks every NPC still present on the automap (kind 4).
RVA(0x0001f4c0, 0x5f)
void MarkAreaNpcs(void) {
    i16 i;
    for (i = 0; i < s_npcCount; i++) {
        if (!IsEventFlagSet(s_npcs[i].flagBank, s_npcs[i].flagIndex)) {
            MarkMapCell(4, s_npcs[i].x, s_npcs[i].y);
        }
    }
}

RVA(0x0001f520, 0x1)
void NotifyEncounterStart(void) {}

RVA(0x0001f530, 0x1)
void NotifyEncounterEnd(void) {}

RVA(0x0001f540, 0x5)
void ReleaseNpcTextures(void) {
    ReleaseObjectTextures();
}

// Loads NPC picture `code` (image 0x4000 + code; `mode` bit 7 picks the
// variant) into object texture slot `slot`. Two spots swap the picture: code
// 0x2b at area 0x82 level 8 cell 11/7 or 12/6 shows 0x24, and code 0x53 at
// area 0x85 level 3 south of row 4 shows 0x4c.
// @identity-TODO: why those spots swap pictures is unrecovered.
RVA(0x0001f550, 0xd0)
void LoadNpcTexture(i16 slot, i16 code, i16 mode) {
    ImageRequest request;
    void* image;
    if (code == 0x2b && mode == 0 && g_field.pos.area == 0x82 && g_field.pos.level == 8) {
        if ((g_field.pos.x == 0xb && g_field.pos.y == 7)
            || (g_field.pos.x == 0xc && g_field.pos.y == 6)) {
            code = 0x24;
        }
    } else if (code == 0x53 && mode == 0 && g_field.pos.area == 0x85 && g_field.pos.level == 3
               && g_field.pos.y > 4) {
        code = 0x4c;
    }
    request.file = code + 0x4000;
    request.variant = 0;
    request.flags = ((u32)(mode & 0x80) >> 7) + 3;
    image = LoadImageRequest(&request, mode);
    LoadObjectTexture(image, slot);
    FreeImageFile(image);
}

// Sets palette colours 8..13 from six GRB words; returns the data after them.
RVA(0x0001f620, 0x32)
u16* LoadNpcPalette(u16* colors) {
    i16 i;
    for (i = 0; i < 6; i++) {
        SetPaletteColor(i + 8, GrbToRgb(*colors));
        colors++;
    }
    return colors;
}

// Loads the area's NPC palette and pictures from its record (the six
// (code, mode) pairs after the palette; mode 0xff leaves a slot empty).
RVA(0x0001f660, 0x41)
void LoadAreaNpcImages(u8* record) {
    u8* p;
    i16 i;
    if (!record) {
        return;
    }
    p = (u8*)LoadNpcPalette((u16*)(record + 3));
    for (i = 0; i < 6; i++) {
        if (p[1] != 0xff) {
            LoadNpcTexture(i, p[0], p[1]);
        }
        p += 2;
    }
}

// The NPC texture in `slot` (0..5), else 0.
RVA(0x0001f6b0, 0x1e)
u32 GetNpcTexture(i16 slot) {
    if (slot >= 0 && slot < 6) {
        return g_npcTextures[slot].texture;
    }
    return 0;
}

// The texture an NPC is drawn with at depth `depth` (only 0 and -1 draw it).
// @identity-TODO: at other depths nothing is returned (retail leaves eax as
// it was); x, y and `index` are unread.
RVA(0x0001f6d0, 0x22)
u32 DrawNpcAt(i16 x, i16 y, i16 depth, AreaNpc* npc, i16 index) {
    if (depth == 0 || depth == -1) {
        return GetNpcTexture(npc->textureSlot);
    }
}

// Runs the field effect of a skill or item: 1 knocks the target back, 3
// shields it, 0x11 and 0x14 set flags, 0x15/0x16 return to the leader's
// recorded point or mark, 0x17 knocks the actor back, 0x19/0x1a spawn a second
// group, 0x1b seals a demon, 0x20..0x22 set stat flags, 0x23 does nothing but
// succeed. Returns the handler's result (1 done, 0 no effect, -1 failed).
// @identity-TODO: the handlers are named from their bodies only.
RVA(0x0001f700, 0xcc)
i16 RunFieldEffect(i16 effect) {
    switch (effect) {
        case 0:
            break;
        case 1:
            return KnockBack(g_targetId);
        case 3:
            return ShieldTarget();
        case 0x11:
            return SetTargetFlag21();
        case 0x14:
            return ScatterObjects();
        case 0x15:
            return ReturnToLeaderWarp();
        case 0x16:
            return ReturnToLeaderMark();
        case 0x17:
            return KnockBackActor();
        case 0x19:
        case 0x1a:
            return SpawnActorGroup();
        case 0x1b:
            return SealTarget();
        case 0x20:
            return RaiseTargetFlag23();
        case 0x21:
            return RaiseTargetFlag25();
        case 0x22:
            return RaiseTargetFlag26();
        case 0x23:
            return 1;
    }
    return 0;
}

// Pushes `who` (a field object, or the party for a negative id) one cell
// back (the party: behind itself; an object: away from the party), unless a
// wall, a map-cell change or a blocked cell stops it; -1 then.
RVA(0x0001f7d0, 0x152)
i16 KnockBack(i16 who) {
    i16* at;
    i16 direction;
    if (who < 0) {
        direction = g_field.pos.direction;
        at = &g_field.pos.x;
    } else {
        i16 object;
        i16 code;
        i16 x;
        i16 y;
        Character* actor;
        direction = OppositeDirection(g_field.pos.direction);
        object = GetLiveObject(who);
        if (object < 0) {
            return -1;
        }
        actor = GetFieldActor(object);
        at = &((FieldActor*)GetFieldActor(object))->pos.x;
        if (TestCharacterFlag(actor, 0x20)) {
            return -1;
        }
        code = GetMapCellCode(at[0], at[1]);
        x = at[0];
        y = at[1];
        StepMapCoord(&x, &y, direction, 2);
        WrapMapPosition(&x, &y);
        if (CellCodeDiffers(code, x, y)) {
            return -1;
        }
        if (IsCellBlocked(g_field.pos.level, 1, x, y)) {
            return -1;
        }
    }
    if (WallStopsToward(at[0], at[1], direction, 2)) {
        return -1;
    }
    StepMapCoord(&at[0], &at[1], direction, 2);
    RefreshFieldScene();
    return 1;
}

// Gives the target a shield of a tenth of its maximum HP (a field object:
// respawns it instead).
RVA(0x0001f930, 0x56)
i16 ShieldTarget(void) {
    Character* target;
    if (g_targetId < 0) {
        target = GetCombatant(g_targetId);
        if (!target) {
            return -1;
        }
        target->shield = target->pools.hp.max / 10;
        return 1;
    }
    return RespawnFieldObject(g_targetId, 0, -1, 1);
}

RVA(0x0001f990, 0x2d)
i16 SetTargetFlag21(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    SetCharacterFlag(target, 0x21);
    return 1;
}

// Sets the leader's flag 0x22 and clears flag 10 of every live object out of
// reach.
RVA(0x0001f9c0, 0x67)
b16 ScatterObjects(void) {
    i16 i;
    u8* flags = GetCharacterFlags(GetRosterCharacter(0));
    SetBit(flags, 0x22);
    for (i = 0; i < 16; i++) {
        i16 object = GetLiveObject(i);
        if (object >= 0 && !HasObjectInReach(1, -1, object)) {
            flags = GetCharacterFlags(GetCombatant(object));
            ClearBit(flags, 10);
        }
    }
    return true;
}

// Sends the party to the return point recorded in the roster leader.
RVA(0x0001fa30, 0x35)
b16 ReturnToLeaderWarp(void) {
    Character* leader = GetRosterCharacter(0);
    SetReturnPoint(
        leader->returnPosition.area,
        leader->returnPosition.level,
        leader->returnPosition.x,
        leader->returnPosition.y,
        leader->returnPosition.direction
    );
    return true;
}

// Sends the party to the cell in front of the leader's marked position.
RVA(0x0001fa70, 0x6d)
b16 ReturnToLeaderMark(void) {
    Character* leader = GetRosterCharacter(0);
    i16 area = leader->markPosition.area;
    i16 level = leader->markPosition.level;
    i16 x = leader->markPosition.x;
    i16 y = leader->markPosition.y;
    i16 direction = OppositeDirection(leader->markPosition.direction);
    OffsetMapCoordFacing(&x, &y, direction, 0, -1);
    SetReturnPoint(area, level, x, y, direction);
    return true;
}

// Knocks the acting object back; when nothing is left within reach, raises
// the pending abort.
RVA(0x0001fae0, 0x44)
i16 KnockBackActor(void) {
    i16 result;
    if (g_actorId >= 0) {
        return -1;
    }
    result = KnockBack(g_actorId);
    if (result < 0) {
        return result;
    }
    if (HasObjectInReach(0, -1, 0)) {
        return 0;
    }
    ExchangeAbortPending(1);
    return 1;
}

// Spawns a second enemy group at the acting object's cell.
RVA(0x0001fb30, 0x42)
i16 SpawnActorGroup(void) {
    i16 object = GetLiveObject(g_actorId);
    MapCoord* pos;
    if (object < 0) {
        return -1;
    }
    pos = &((FieldActor*)GetFieldActor(object))->pos;
    SpawnSecondGroupActor(pos->x, pos->y, -1);
}

// Seals the target (flag 0x3f, result 3) unless it is human or of races
// 0x1a..0x22; while the field marker is set against an object the action
// fails with result 6.
RVA(0x0001fb80, 0xe4)
i16 SealTarget(void) {
    Character* actor = GetCombatant(g_actorId);
    Character* target;
    i16 race;
    if (g_targetId >= 0 && GetFieldMarker()) {
        SetActionResult(actor, 6);
        return 0;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (IsHumanCharacter(target)) {
        return 0;
    }
    race = GetDemonRace(target->id);
    if (race == 0x1a || race == 0x1b || race == 0x1c || race == 0x1d || race == 0x1e || race == 0x1f
        || race == 0x20 || race == 0x21 || race == 0x22) {
        return 0;
    }
    SetActionResult(actor, 3);
    SetCharacterFlag(target, 0x3f);
    return 1;
}

// Sets the target's flag 0x23 (not with 0x23 or 0x24 already set) and
// recalculates its stats.
RVA(0x0001fc70, 0x6d)
i16 RaiseTargetFlag23(void) {
    Character* target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x23) == 1) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x24) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x23);
    RecalcCharacterStats(target);
    return 1;
}

// The same with flag 0x25, only while the moon is not new.
RVA(0x0001fce0, 0x67)
i16 RaiseTargetFlag25(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return -1;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x25) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x25);
    RecalcCharacterStats(target);
    return 1;
}

RVA(0x0001fd50, 0x67)
i16 RaiseTargetFlag26(void) {
    Character* target;
    if (!GetMoonPhase()) {
        return -1;
    }
    target = GetCombatant(g_targetId);
    if (!target) {
        return -1;
    }
    if (TestCharacterFlag(target, 0x26) == 1) {
        return -1;
    }
    SetCharacterFlag(target, 0x26);
    RecalcCharacterStats(target);
    return 1;
}
