// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The field objects: sixteen
// slots of enemies placed on the field map, spawned from map cells and
// records, checked and drawn every frame.

#include <rva.h>

#include <Game/CombatantId.h>

#include <File/DataFile.h>
#include <Game/AreaMap.h>
#include <Game/DoorRegion.h>
#include <Game/BattleEffect.h>
#include <Game/Condition.h>
#include <Game/FieldActor.h>
#include <Game/FieldLayer.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/ModeFlags.h>
#include <Game/PartyAction.h>
#include <Game/Skill.h>
#include <Game/TargetFlags.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Math/Vec3.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <string.h>

// Game/StateStack.h, Ui/Hotspot.h and Game/Field.h are not included: their declaration counts
// perturb this TU (RelativeFacing, RollEncounterSlot). These callees are
// declared by hand instead.
i16 GetGameState(void);
void ClearSelectedHotspot(void);
i16 GetFieldMarker(void);

// @identity-TODO: approach offsets by step (0..-4): x scaled by the caller,
// then y.
DATA(0x000685e0)
static i16 s_approachX[8] = {-40, -26, -16, -11, -8};

DATA(0x000685f0)
static i16 s_approachY[8] = {224, 120, 72, 48, 32};

// @identity-TODO: the sprite for each facing relative to the view.
DATA(0x00068600)
static i16 s_facingSprite[4] = {0, 1, 2, -1};

// Image codes by facing relative to the party; negative mirrors the side image.
DATA(0x00068608)
static i16 s_facingImageCodes[4] = {0, 1, 2, -1};

// @identity-TODO: the floor cell of the n-th of up to ten objects drawn
// together, far rows and the near row.
DATA(0x00064328)
static const i16 s_farCells[10][9] = {
    {4, 4, 4, 4, 4, 4, 4, 4, 4},
    {4, 3, 5, 1, 2, 0, 7, 8, 6},
    {3, 5, 4, 1, 2, 0, 7, 8, 6},
    {0, 2, 4, 3, 5, 1, 7, 8, 6},
    {0, 2, 6, 8, 4, 3, 5, 1, 7},
    {0, 2, 4, 6, 8, 3, 5, 1, 7},
    {0, 2, 3, 5, 6, 8, 4, 1, 7},
    {0, 2, 3, 5, 4, 6, 8, 1, 7},
    {0, 2, 1, 3, 5, 4, 6, 8, 7},
    {0, 2, 1, 3, 5, 4, 6, 8, 7},
};

DATA(0x000643e0)
static const i16 s_nearCells[10][9] = {
    {4, 4, 4, 4, 4, 4, 4, 4, 4},
    {4, 3, 5, 1, 2, 0, 0, 1, 2},
    {3, 5, 4, 1, 2, 0, 0, 1, 2},
    {0, 2, 4, 3, 5, 1, 0, 1, 2},
    {0, 2, 4, 5, 4, 3, 5, 1, 0},
    {0, 2, 4, 1, 0, 3, 5, 1, 2},
    {0, 2, 3, 5, 0, 2, 4, 1, 1},
    {0, 2, 3, 5, 4, 0, 2, 1, 1},
    {0, 2, 1, 3, 5, 4, 0, 2, 1},
    {0, 2, 1, 3, 5, 4, 0, 2, 1},
};

// @identity-TODO: the screen rise per view depth (0..-4).
DATA(0x00064498)
static const i16 s_depthRise[8] = {0, 20, 12, 4, 0};

DATA(0x000789d0)
static FieldObject s_objects[16];

// While set, GetLiveObject accepts any index.
DATA(0x0007b0b0)
static i16 s_objectCheckBypass;

// While set, a hidden object is not removed (DeferObjectRemoval instead).
DATA(0x0007b0b4)
static i16 s_objectRemovalDeferred;

// While set, the objects are not marked on the automap.
DATA(0x0007b0b8)
static i16 s_objectsFrozen;

RVA(0x0000d790, 0x52)
i16 InitFieldObjects(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        s_objects[i].layer = -1;
        s_objects[i].redraw = 0;
        s_objects[i].anim = 0;
        s_objects[i].script = NULL;
        InitWordList(&s_objects[i].list, 0);
    }
    ModifyEventFlag(8, 0, 1);
    return 0;
}

// Frees slot `index`. When `announce` is set, a live object also clears its
// event flag and queues its event; the level is marked when the last object
// with an event goes.
RVA(0x0000d7f0, 0x145)
void RemoveFieldObject(i16 index, i16 announce) {
    i16 queued = 0;
    i16 i;
    if (announce != 0 && s_objects[index].layer != -1) {
        if (!(s_objects[index].flagBank == 0 && s_objects[index].flagIndex == 0)
            && !(s_objects[index].flagBank == 0xff && s_objects[index].flagIndex == 0xff)) {
            ModifyEventFlag(
                s_objects[index].flagBank,
                s_objects[index].flagIndex,
                (u8)~s_objects[index].flagBank >> 7
            );
        }
        if (s_objects[index].event >= 0) {
            QueueObjectEvent(s_objects[index].event);
            queued = 1;
        }
    }
    s_objects[index].layer = -1;
    s_objects[index].redraw = 0;
    s_objects[index].anim = 0;
    s_objects[index].script = FreeScriptBlock(s_objects[index].script);
    s_objects[index].hidden = 0;
    ClearCondition(GetFieldObjectConditions(&s_objects[index]), CONDITION_ZOMBIE);
    ResetWordList(&s_objects[index].list, 0);
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1) {
            return;
        }
    }
    ModifyEventFlag(8, 0, 1);
    if (queued != 0 && !HasQueuedObjectEvents()) {
        MarkLevelEvent(g_field.pos.level);
    }
}

RVA(0x0000d940, 0x31)
i16 ResetFieldObjects(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        RemoveFieldObject(i, 0);
    }
    ModifyEventFlag(8, 0, 1);
    s_objectsFrozen = 0;
    return 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0000d980, 0x30)
i16 IsFieldActor(const void* actor) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (&s_objects[i].kind == actor) {
            return 1;
        }
    }
    return 0;
}

RVA(0x0000d9b0, 0x12)
i16 ExchangeObjectsFrozen(i16 frozen) {
    i16 old = s_objectsFrozen;
    s_objectsFrozen = frozen;
    return old;
}

// The first slot whose layer is `layer` (-1 finds a free slot), or -1.
RVA(0x0000d9d0, 0x2a)
i16 FindObjectOnLayer(i16 layer) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer == layer) {
            return i;
        }
    }
    return -1;
}

RVA(0x0000da00, 0x33)
void SetObjectEventFlag(i16 index, u8 bank, u8 flag) {
    if (index >= 0 && index < 16) {
        s_objects[index].flagBank = bank;
        s_objects[index].flagIndex = flag;
    }
}

// Places record `kind` on the map at x/y in a free slot; returns the slot or
// -1. `alternate` picks the second script file; `fresh` resets the record's
// counters.
RVA(0x0000da40, 0x1a8)
i16 SpawnFieldObject(
    i16 layer,
    i16 x,
    i16 y,
    i16 direction,
    i16 kind,
    i16 alternate,
    i8 event,
    i16 fresh
) {
    i16 slot = FindObjectOnLayer(-1);
    FILE* file;
    if (slot == -1) {
        return slot;
    }
    LoadObjectRecord(kind, &s_objects[slot]);
    s_objects[slot].slot = slot;
    s_objects[slot].event = event;
    s_objects[slot].redraw = 0;
    s_objects[slot].anim = 0;
    s_objects[slot].layer = layer;
    s_objects[slot].byte21a = 0;
    s_objects[slot].byte21b = 0;
    s_objects[slot].pos.x = x;
    s_objects[slot].pos.y = y;
    s_objects[slot].direction = (u8)direction;
    s_objects[slot].word21d = 0;
    s_objects[slot].word221 = 0;
    s_objects[slot].word21f = 0;
    s_objects[slot].hidden = 0;
    if (alternate != 0) {
        file = OpenDataFile(0x6802, 9, 0);
    } else if (TestModeFlags(MODE_WORLD_MAP)) {
        file = OpenDataFile(0x6801, 9, 0);
    } else {
        file = OpenDataFile(0x6800, 9, 0);
    }
    s_objects[slot].script = ReadScriptBlock(s_objects[slot].script, file);
    s_objects[slot].script->id = 0xff;
    CloseDataFile(file);
    SetBit(GetFieldObjectFlags(&s_objects[slot]), 10);
    if (alternate != 0) {
        SetBit(GetFieldObjectFlags(&s_objects[slot]), 0x20);
    }
    if (fresh != 0) {
        s_objects[slot].macca = 0;
        s_objects[slot].magnetite = 0;
        s_objects[slot].pickItem = 0;
        s_objects[slot].dropChance = 0;
        s_objects[slot].pools.hp.cur = s_objects[slot].rank;
        s_objects[slot].experience = s_objects[slot].rank;
    }
    SetObjectEventFlag(slot, 0xff, 0xff);
    ModifyEventFlag(8, 0, 0);
    return slot;
}

// Spawns the object the map cell at x/y holds for `layer`; -1 when none.
RVA(0x0000dbf0, 0x7f)
i16 SpawnMapObject(i16 layer, i16 x, i16 y, i16 direction, i8 event) {
    i16 code = GetMapCellCode(x, y);
    DoorRegionData* table;
    i16 kind;
    if (code == 0xff || code == -1) {
        return -1;
    }
    if (!IsObjectCell(code)) {
        return -1;
    }
    table = GetCellObjectTable(code);
    if (table == NULL) {
        return -1;
    }
    kind = LookupCellObject(table, layer);
    if (kind < 0x20 || kind >= 0x2020) {
        return -1;
    }
    return SpawnFieldObject(layer, x, y, direction, kind, 0, event, 0);
}

// `index` when it names a live, visible object (or checks are bypassed), else -1.
RVA(0x0000dc70, 0x41)
i16 GetLiveObject(i16 index) {
    if (s_objectCheckBypass == 0) {
        if (index < 0 || index >= 16 || s_objects[index].layer == -1
            || s_objects[index].hidden != 0) {
            return -1;
        }
    }
    return index;
}

// Spawns a copy of object `index` at its place; returns the new slot or -1.
RVA(0x0000dcc0, 0x99)
i16 RespawnFieldObject(i16 index, i16 alternate, i8 event, i16 fresh) {
    i16 live = GetLiveObject(index);
    i16 layer;
    i16 slot;
    if (live < 0) {
        return -1;
    }
    layer = FindLayerOfKind(s_objects[live].kind);
    if (layer < 0) {
        return -1;
    }
    slot = SpawnFieldObject(
        layer,
        s_objects[live].pos.x,
        s_objects[live].pos.y,
        s_objects[live].direction,
        s_objects[live].kind,
        alternate,
        event,
        fresh
    );
    if (slot >= 0) {
        RequestObjectRedraw(live, -1, -1);
    }
    return slot;
}

RVA(0x0000dd60, 0x15)
void ResetObjectAnims(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        ResetObjectAnim(i);
    }
}

RVA(0x0000dd80, 0x20)
void ResetObjectAnim(i16 index) {
    if (index >= 0) {
        s_objects[index].anim = -1;
    }
}

RVA(0x0000dda0, 0x15)
FieldObject* GetFieldObject(i16 index) {
    return &s_objects[index];
}

RVA(0x0000ddc0, 0x15)
// The character record that starts at `kind` (conditions land at +0x170).
Character* GetFieldActor(i16 index) {
    return (Character*)&s_objects[index].kind;
}

RVA(0x0000dde0, 0x43)
void MarkObjectsOnMap(void) {
    i16 i;
    if (s_objectsFrozen == 0) {
        for (i = 0; i < 16; i++) {
            if (s_objects[i].layer != -1) {
                MarkMapCell(5, s_objects[i].pos.x, s_objects[i].pos.y);
            }
        }
    }
}

RVA(0x0000de30, 0x2d)
MapCoord GetObjectCoord(i16 index) {
    MapCoord coord;
    coord.x = s_objects[index].pos.x;
    coord.y = s_objects[index].pos.y;
    return coord;
}

RVA(0x0000de60, 0x16)
i16 GetObjectDirection(i16 index) {
    return s_objects[index].direction;
}

RVA(0x0000de80, 0x12)
i16 ExchangeObjectCheckBypass(i16 bypass) {
    i16 old = s_objectCheckBypass;
    s_objectCheckBypass = bypass;
    return old;
}

RVA(0x0000dea0, 0x3d)
i16 FlushObjectRedraws(void) {
    i16 drawn = 0;
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1 && s_objects[i].redraw != 0) {
            s_objects[i].redraw = 0;
            RedrawFieldView();
            drawn |= 1;
        }
    }
    return drawn;
}

// 0 for no object (or one whose layer is gone), 2 when it has a fatal
// condition, else 1.
RVA(0x0000dee0, 0x4a)
i16 GetObjectLifeState(FieldObject* object) {
    if (object == NULL) {
        return 0;
    }
    if (object->layer == -1) {
        return 0;
    }
    if (FindLayerOfKind(object->kind) < 0) {
        return 0;
    }
    return (GetFatalCondition(GetFieldObjectConditions(object)) != 0) + 1;
}

RVA(0x0000df30, 0x11)
i16 RelativeFacing(i16 from, i16 to) {
    return (to - (from + 2)) & 3;
}

RVA(0x0000df50, 0x51)
MapCoord GetApproachOffset(i16 scale, i16 step) {
    MapCoord offset;
    offset.x = -80;
    offset.y = -80;
    if (step <= 0 && step >= -4) {
        offset.x = -(s_approachX[-step] * scale);
        offset.y = s_approachY[-step];
    }
    return offset;
}

// Draws one object at slot `drawn` of `total` in the view; 0 when it is not
// in play.
RVA(0x0000dfb0, 0x1a9)
i16 DrawFieldObject(FieldObject* object, u32 image, i16 index, i16 total, i16 drawn) {
    Vec3 cell;
    ScreenPoint point;
    i16 facing;
    i16 sprite;
    u32 frame;
    i16 redraw;
    if (!GetObjectLifeState(object)) {
        return 0;
    }
    if (total > 9) {
        total = 9;
    }
    if (drawn > 8) {
        drawn = 8;
    }
    facing = RelativeFacing(g_viewFacing, object->direction);
    point = GetApproachOffset(g_viewLateral, g_viewDepth);
    sprite = s_facingSprite[facing];
    CellToField(g_viewLateral, g_viewDepth, 4, &cell);
    frame = GetLayerFrame(image, 0, cell.z);
    redraw = object->redraw != 0;
    if (g_viewDepth == 0) {
        CellToField(g_viewLateral, 0, s_nearCells[total][drawn], &cell);
    } else {
        CellToField(g_viewLateral, g_viewDepth, s_farCells[total][drawn], &cell);
    }
    cell.y = point.y;
    point = ProjectFloorPoint(cell.x, cell.z);
    point.y += s_depthRise[-g_viewDepth] - 0x72;
    point.x = (i16)(point.x - 320) / 8;
    if (g_viewDepth == 0) {
        point.y = cell.y;
    }
    RefreshObjectDraw(sprite, point.x, point.y, g_viewDepth, redraw, object, frame, index);
    object->redraw = 0;
    return 1;
}

RVA(0x0000e160, 0x7a)
void DrawFieldObjects(void) {
    i16 total = 0;
    i16 drawn;
    i16 i;
    for (i = 0; i < 16; i++) {
        if (CheckObjectState(i) >= 1) {
            total += GetObjectLifeState(&s_objects[i]);
        }
    }
    drawn = 0;
    for (i = 0; i < 16; i++) {
        if (CheckObjectState(i) >= 1) {
            drawn +=
                DrawFieldObject(&s_objects[i], GetLayerImage(s_objects[i].layer), i, total, drawn);
        }
    }
}

// -1 for a free slot, 1 for an object still in play; a hidden object is
// removed (0) unless removal is deferred.
RVA(0x0000e1e0, 0x65)
i16 CheckObjectState(i16 index) {
    if (s_objects[index].layer == -1) {
        return -1;
    }
    if (s_objects[index].hidden == 0) {
        return 1;
    }
    if (s_objectRemovalDeferred) {
        DeferObjectRemoval();
        return 1;
    }
    RemoveFieldObject(index, 1);
    RequestObjectRedraw(index, -1, -1);
    return 0;
}

RVA(0x0000e250, 0x60)
void RunFieldIdle(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (CheckObjectState(i) >= 1) {
            BuildSightGrid(s_objects[i].pos.x, s_objects[i].pos.y, s_objects[i].direction);
            if (RunObjectStep(&s_objects[i], i)) {
                return;
            }
        }
    }
}

// The first live object from `start` at x/y; `mode` 1 wants record `kind`,
// mode 2 any other record. -1 when none.
RVA(0x0000e2b0, 0xc8)
i16 FindObjectAt(i16 x, i16 y, i16 start, i16 mode, i16 kind) {
    i16 i;
    for (i = start; i < 16; i++) {
        if (s_objects[i].layer == -1 || s_objects[i].hidden != 0) {
            continue;
        }
        if (FindLayerOfKind(s_objects[i].kind) < 0) {
            continue;
        }
        if (GetFatalCondition(GetFieldObjectConditions(&s_objects[i]))) {
            continue;
        }
        if (x != s_objects[i].pos.x || y != s_objects[i].pos.y) {
            continue;
        }
        if (mode == 1 && kind != s_objects[i].kind) {
            continue;
        }
        if (mode == 2 && kind == s_objects[i].kind) {
            continue;
        }
        return i;
    }
    return -1;
}

RVA(0x0000e380, 0x4a)
i16 CountObjectsAt(i16 x, i16 y, i16 mode, i16 kind) {
    i16 count = 0;
    i16 i = FindObjectAt(x, y, count, mode, kind);
    while (i != -1) {
        count++;
        i = FindObjectAt(x, y, i + 1, mode, kind);
    }
    return count;
}

RVA(0x0000e3d0, 0x24)
i16 FindObjectAtParty(void) {
    MapCoord coord;
    coord = GetMapCoord();
    return FindObjectAt(coord.x, coord.y, 0, 0, 0);
}

RVA(0x0000e400, 0x48)
void UpdateFieldObjects(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1 && s_objects[i].hidden == 0 && s_objects[i].pos.y >= 4) {
            ResetObjectAnim(i);
            s_objects[i].hidden = 1;
        }
    }
}

RVA(0x0000e450, 0x12)
i16 ExchangeObjectRemovalDeferred(i16 deferred) {
    i16 old = s_objectRemovalDeferred;
    s_objectRemovalDeferred = deferred;
    return old;
}

// Clears conditions 0, 1, 2 and 8 of every hidden object; `cleared[i]` says
// which were.
RVA(0x0000e470, 0x6f)
void ClearObjectStuns(i16* cleared) {
    i16 i;
    for (i = 0; i < 16; i++) {
        cleared[i] = 0;
        if (s_objects[i].layer != -1 && s_objects[i].hidden != 0) {
            cleared[i] = 1;
            ClearCondition(GetFieldObjectConditions(&s_objects[i]), CONDITION_ASH);
            ClearCondition(GetFieldObjectConditions(&s_objects[i]), CONDITION_DEAD);
            ClearCondition(GetFieldObjectConditions(&s_objects[i]), CONDITION_DYING);
            ClearCondition(GetFieldObjectConditions(&s_objects[i]), CONDITION_ZOMBIE);
        }
    }
}

RVA(0x0000e4e0, 0x40)
void ApplyObjectConditions(i16* marked) {
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1 && marked[i] != 0) {
            AddCondition(GetFieldObjectConditions(&s_objects[i]), CONDITION_DEAD);
        }
    }
}

RVA(0x0000e520, 0x7)
i16 GetObjectsFrozen(void) {
    return s_objectsFrozen;
}

RVA(0x0000e530, 0x1e)
i16 GetObjectLifeStateAt(i16 index) {
    return GetObjectLifeState(&s_objects[index]);
}

RVA(0x0000e550, 0x15)
MapCoord* GetObjectCoordPtr(i16 index) {
    return &s_objects[index].pos;
}

RVA(0x0000e570, 0x17)
i16 GetObjectSlot(i16 index) {
    return s_objects[index].slot;
}

RVA(0x0000e590, 0x16)
i16 GetObjectAnim(i16 index) {
    return s_objects[index].anim;
}

RVA(0x0000e5b0, 0x4f)
i16 GetObjectImageCode(i16 index) {
    FieldObject* object = &s_objects[index];
    i16 imageCode = 0;
    if (object->redraw != 0) {
        imageCode = 4;
    } else if (object->anim != 0) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->hidden != 0) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->acting) {
        imageCode = 3;
    }
    return imageCode;
}

RVA(0x0000e600, 0x77)
i16 GetObjectFacingImageCode(i16 index) {
    FieldObject* object = &s_objects[index];
    i16 imageCode = s_facingImageCodes[RelativeFacing(g_field.pos.direction, object->direction)];
    if (object->redraw != 0) {
        imageCode = 4;
    } else if (object->anim != 0) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->hidden != 0) {
        imageCode = 4;
    } else if (object->acting) {
        imageCode = 3;
    }
    return imageCode;
}

RVA(0x0000e680, 0x2b)
i16 CountActiveObjects(void) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1 && s_objects[i].hidden == 0) {
            count++;
        }
    }
    return count;
}

// Steps each running object animation (-1 restarts it); past frame 8 the
// object is hidden. Returns how many ran.
RVA(0x0000e6b0, 0x65)
i16 AdvanceObjectAnims(void) {
    i32 count = 0;
    i16 i;
    for (i = 0; i < 16; i++) {
        if (s_objects[i].layer != -1 && s_objects[i].anim != 0) {
            if (s_objects[i].anim == -1) {
                s_objects[i].anim = 1;
            } else {
                s_objects[i].anim++;
            }
            if (s_objects[i].anim > 8) {
                s_objects[i].hidden = 1;
            }
            RedrawFieldView();
            count++;
        }
    }
    return count;
}

RVA(0x0000e720, 0x15)
void CheckAllObjects(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        CheckObjectState(i);
    }
}

RVA(0x0000e740, 0x16)
i16 GetObjectLayer(i16 index) {
    return s_objects[index].layer;
}

DATA(0x0007b058)
static u8 s_scriptSetBuffer[0x50];

DATA(0x0007b0f8)
static FieldLayer s_layers[2];

DATA(0x0007b2f8)
static FieldLayer s_savedLayers[2];

// The script-set table: three data-file numbers per set (0xff: none).
DATA(0x0007b4f8)
static u8* s_scriptSets;

// The object script block (data file 0x6800), loaded on first use.
DATA(0x0007b4f4)
static ScriptBlock* s_objectScripts;

static __inline u8 GetScriptSetFile(i16 set, i16 index) {
    return s_scriptSets[set * 3 + index];
}

RVA(0x0000e760, 0x3f)
void SaveFieldLayer(i16 layer) {
    i16 i;
    s_savedLayers[layer] = s_layers[layer];
    s_layers[layer].image = 0;
    for (i = 0; i < 32; i++) {
        s_layers[layer].scripts[i] = NULL;
    }
}

RVA(0x0000e7a0, 0x3f)
void RestoreFieldLayer(i16 layer) {
    i16 i;
    s_layers[layer] = s_savedLayers[layer];
    s_savedLayers[layer].image = 0;
    for (i = 0; i < 32; i++) {
        s_savedLayers[layer].scripts[i] = NULL;
    }
}

RVA(0x0000e7e0, 0x3c)
void ResetFieldLayer(i16 layer) {
    s_layers[layer].image = DropLayerImage(s_layers[layer].image);
    FreeLayerScripts(&s_layers[layer]);
    s_layers[layer].record.id = -1;
}

// Loads record `kind` and its picture into layer `layer` (-1 empties it).
RVA(0x0000e820, 0xcb)
void LoadEnemyGroupSlot(i16 layer, i16 kind) {
    ImageRequest request;
    i32 size;
    struct BmpFile* data;
    i16 base;
    s_layers[layer].record.id = -1;
    if (kind < 0) {
        return;
    }
    LoadLayerRecord(kind, &s_layers[layer].record);
    base = s_layers[layer].record.imageIndex << 4;
    s_layers[layer].image = AllocLayerImage();
    request.file = base + 0x2002;
    request.variant = s_layers[layer].record.imageVariant;
    request.flags = 0;
    data = LoadImageFile(&request, &size);
    if (base == 0xb80) {
        DecodeLayerImageAlt(data, layer, size);
    } else {
        DecodeLayerImage(data, layer, size);
    }
    FreeImageFile(data);
}

RVA(0x0000e8f0, 0x15)
i16 GetLayerKind(i16 layer) {
    return s_layers[layer].record.id;
}

RVA(0x0000e910, 0x13b)
ScriptEntry FindLayerScriptEntry(i16 layerSlot, i16 file, i16 entry) {
    i16 layer;
    i16 i;
    FILE* fp;
    ScriptEntry none;
    ClearScriptEntry(&none);
    if (layerSlot != 0) {
        layer = layerSlot - 1;
        if (file == 0xff) {
            if (s_objectScripts == NULL) {
                fp = OpenDataFile(0x6800, 9, 0);
                s_objectScripts = ReadScriptBlock(s_objectScripts, fp);
                CloseDataFile(fp);
            }
            return MakeScriptEntry(s_objectScripts, entry);
        }
        if (GetLayerScript(&s_layers[layer], 0) == NULL) {
            LoadLayerScriptSet(&s_layers[layer], s_layers[layer].record.scriptSet);
        }
        for (i = 0; i < 32; i++) {
            if (GetScriptBlockId(GetLayerScript(&s_layers[layer], i)) == file) {
                return MakeScriptEntry(GetLayerScript(&s_layers[layer], i), entry);
            }
        }
    }
    return none;
}

// @identity-TODO: after an object is drawn: refresh the screen when forced
// (the object was marked for redraw) or when it is out of play; the other
// draw arguments are unused here.
RVA(0x0000ea50, 0x21)
void RefreshObjectDraw(
    i16 sprite,
    i16 x,
    i16 y,
    i16 depth,
    i16 force,
    FieldObject* object,
    u32 frame,
    i16 index
) {
    if (force != 0) {
        RequestRefresh();
        return;
    }
    if (object->hidden != 0) {
        RequestRefresh();
    }
}

RVA(0x0000ea80, 0x14)
u32 GetLayerImage(i16 layer) {
    return s_layers[layer].image;
}

RVA(0x0000eaa0, 0x29)
i16 FindLayerOfKind(i16 kind) {
    i16 i;
    for (i = 0; i < 2; i++) {
        if (s_layers[i].record.id == kind) {
            return i;
        }
    }
    return -1;
}

RVA(0x0000ead0, 0x2b)
void FreeLayerScripts(FieldLayer* layer) {
    i16 i;
    if (layer != NULL) {
        for (i = 31; i >= 0; i--) {
            layer->scripts[i] = FreeScriptBlock(GetLayerScript(layer, i));
        }
    }
}

// Reads script blocks first..end-1 of data file `file` into the layer; each
// block starts in state (index - 32).
RVA(0x0000eb00, 0x6a)
void LoadLayerScripts(FieldLayer* layer, i16 file, i16 kind, i16 first, i16 end) {
    FILE* fp = OpenDataFile(file, kind, 0);
    i16 i;
    if (fp != NULL) {
        for (i = first; i < end; i++) {
            layer->scripts[i] = ReadScriptBlock(GetLayerScript(layer, i), fp);
            GetLayerScript(layer, i)->id = (u8)(i - 0x20);
        }
    }
    CloseDataFile(fp);
}

// Loads the common scripts, the up-to-three files of script set `set` and the
// layer's own record scripts.
RVA(0x0000eb70, 0xea)
void LoadLayerScriptSet(FieldLayer* layer, i16 set) {
    FILE* fp;
    if (s_scriptSets == NULL) {
        fp = OpenDataFile(7, 12, 0);
        ReadRawBlock(fp, s_scriptSetBuffer);
        s_scriptSets = s_scriptSetBuffer;
        CloseDataFile(fp);
    }
    LoadLayerScripts(layer, 0x6000, 9, 0, 0x10);
    if (GetScriptSetFile(set, 0) != 0xff) {
        LoadLayerScripts(layer, GetScriptSetFile(set, 0) + 0x6000, 9, 0, 0x10);
    }
    if (GetScriptSetFile(set, 1) != 0xff) {
        LoadLayerScripts(layer, GetScriptSetFile(set, 1) + 0x6000, 9, 0, 0x10);
    }
    if (GetScriptSetFile(set, 2) != 0xff) {
        LoadLayerScripts(layer, GetScriptSetFile(set, 2) + 0x6100, 9, 0, 0x10);
    }
    LoadLayerScripts(layer, layer->record.id, 14, 0, 0x10);
}

// The spawn interval in seconds and the countdown (in ticks) to the next
// random spawn.
DATA(0x0007b0f4)
static u16 s_spawnInterval;

DATA(0x0007b0f0)
static i32 s_spawnTimer;

// The 7x7 sight grid around the object being stepped (1: seen).
DATA(0x0007ada0)
static u8 s_sight[7][7];

// @identity-TODO: nine cumulative weights per encounter row (data file 5).
DATA(0x000788d0)
static u8 s_encounterBuffer[0x100];

DATA(0x0007b0ec)
static u8* s_encounterWeights;

MapCoord RandomNearOffset(void);

// Sets the spawn interval (0 picks 40..60 seconds) and restarts the timer.
RVA(0x0000ec60, 0x31)
void SetSpawnInterval(i16 seconds) {
    if (seconds == 0) {
        seconds = RandomAverage(0x28, 0x3c, 0);
    }
    s_spawnInterval = seconds;
    s_spawnTimer = s_spawnInterval * 60;
}

// Spawns a random enemy of either loaded layer on a free cell of the party's
// cell type near the party (ten tries); -1 when none.
RVA(0x0000eca0, 0x142)
i16 SpawnRandomEnemy(void) {
    MapCoord offset;
    i16 width;
    i16 height;
    i16 code;
    i16 layer;
    i16 tries;
    i16 x;
    i16 y;
    GetMapSize(&width, &height);
    code = GetPartyCellCode();
    if (!IsObjectCell(code)) {
        return -1;
    }
    layer = RandomAverage(0, 9, 0) & 1;
    if (GetLayerKind(layer) < 0) {
        layer ^= 1;
        if (GetLayerKind(layer) < 0) {
            return -1;
        }
    }
    if (FindObjectOnLayer(-1) < 0) {
        return -1;
    }
    for (tries = 0; tries < 10; tries++) {
        offset = RandomNearOffset();
        x = offset.x + g_field.pos.x;
        y = offset.y + g_field.pos.y;
        x = WrapMapCoord(x, width);
        y = WrapMapCoord(y, height);
        if (!IsCellBlocked(g_field.pos.level, 1, x, y) && GetMapCellCode(x, y) == code) {
            break;
        }
    }
    if (tries >= 10) {
        return -1;
    }
    RequestFieldRefresh();
    return SpawnMapObject(layer, x, y, RandomAverage(0, 3, 0), -1);
}

RVA(0x0000edf0, 0x2d)
MapCoord RandomNearOffset(void) {
    MapCoord offset;
    offset.x = RandomAverage(-3, 3, 0);
    offset.y = RandomAverage(-3, 3, 0);
    return offset;
}

// Counts the spawn timer down; when it runs out it restarts and, unless
// objects are barred (event flag 8/0), spawns a random enemy.
RVA(0x0000ee20, 0x45)
i16 TickEnemySpawnTimer(void) {
    i32 timer = s_spawnTimer;
    if (timer != 0) {
        timer--;
        s_spawnTimer = timer;
    }
    if (timer == 0) {
        s_spawnTimer = s_spawnInterval * 60;
        if (!IsEventFlagSet(8, 0)) {
            return SpawnRandomEnemy();
        }
    }
    return -1;
}

// Traces the sight lines from x/y along `direction` row by row (up to three
// steps back) until a cell blocks it.
// @early-stop register residue: retail keeps direction in esi and the step
// in edi; this build swaps them. The call, branch and relocation shapes match,
// and a 32-island compiler-state search stayed in one state.
RVA(0x0000ee70, 0x80)
void TraceSight(i16 x, i16 y, i16 direction) {
    i16 step;
    i16 right;
    i16 left;
    i16 cellX;
    i16 cellY;
    left = -3;
    right = 3;
    for (step = 0; step >= -3; step--) {
        ScanSightRow(x, y, step, direction, &left, &right);
        cellX = x;
        cellY = y;
        OffsetMapCoord(&cellX, &cellY, direction, 0, step);
        if (GetMapWallKind(cellX, cellY, direction)) {
            break;
        }
    }
}

// Marks the cells of sight row `step` seen from x/y facing `direction`, out
// to the left and right bounds; a blocking cell narrows its bound.
RVA(0x0000eef0, 0x179)
void ScanSightRow(i16 x, i16 y, i16 step, i16 direction, i16* left, i16* right) {
    i16 side;
    i16 i;
    i16 cellX;
    i16 cellY;
    i16 blocked;
    for (i = 0; i >= *left; i--) {
        side = (direction - 1) & 3;
        cellX = x;
        cellY = y;
        OffsetMapCoord(&cellX, &cellY, direction, i, step);
        blocked = GetMapWallKind(cellX, cellY, side);
        cellX += 3 - x;
        cellY += 3 - y;
        if (cellY >= 0 && cellX >= 0) {
            s_sight[cellY][cellX] = 1;
        }
        if (blocked) {
            *left = i;
            break;
        }
    }
    for (i = 0; i <= *right; i++) {
        side = (direction + 1) & 3;
        cellX = x;
        cellY = y;
        OffsetMapCoord(&cellX, &cellY, direction, i, step);
        blocked = GetMapWallKind(cellX, cellY, side);
        cellX += 3 - x;
        cellY += 3 - y;
        if (cellY >= 0 && cellX >= 0) {
            s_sight[cellY][cellX] = 1;
        }
        if (blocked) {
            *right = i;
            break;
        }
    }
}

// Rebuilds the sight grid for an object at x/y facing `facing`.
RVA(0x0000f070, 0x46)
void BuildSightGrid(i16 x, i16 y, i16 facing) {
    i16 direction;
    memset(s_sight, 0, sizeof(s_sight));
    for (direction = facing; direction < facing + 4; direction++) {
        TraceSight(x, y, direction & 3);
    }
}

// The sight-grid cell for x/y around an object at x0/y0 (0 outside).
RVA(0x0000f0c0, 0x53)
i16 GetSightCell(i16 x0, i16 y0, i16 x, i16 y) {
    x += 3 - x0;
    y += 3 - y0;
    if (x >= 0 && x < 7 && y >= 0 && y < 7) {
        return s_sight[y][x];
    }
    return 0;
}

RVA(0x0000f120, 0x28)
i16 IsPartyInSight(i16 x0, i16 y0) {
    MapCoord coord;
    coord = GetMapCoord();
    return GetSightCell(x0, y0, coord.x, coord.y);
}

// The direction from x/y towards the party (the party's facing turned
// around when they share the cell).
RVA(0x0000f150, 0x4f)
i16 DirectionToParty(i16 x, i16 y) {
    MapCoord coord;
    i16 direction;
    coord = GetMapCoord();
    direction = RelativeDirection(x, y, coord.x, coord.y, 0);
    if (x == coord.x && y == coord.y) {
        direction = TurnDirection(g_field.pos.direction, 2);
    }
    return direction;
}

RVA(0x0000f1a0, 0x34)
void LoadEncounterWeights(void) {
    FILE* fp = OpenDataFile(5, 12, 0);
    ReadRawBlock(fp, s_encounterBuffer);
    s_encounterWeights = s_encounterBuffer;
    CloseDataFile(fp);
}

// Rolls one of the nine slots of encounter row `row` by its weights.
RVA(0x0000f1e0, 0x35)
i16 RollEncounterSlot(i16 row) {
    u8 roll = RandomUpTo(0xff);
    i16 i;
    for (i = 0; i < 9; i++) {
        if (roll <= s_encounterWeights[row * 9 + i]) {
            break;
        }
    }
    return i;
}

RVA(0x0000f220, 0x1e)
i16 RefreshIfTurned(i16 visible, i16 turned) {
    if (visible != 0 && turned != 0) {
        RequestFieldRefresh();
        return 1;
    }
    return 0;
}

// The side (1 or 3) the party is on seen from x/y facing `direction`; a
// random side when straight ahead or behind.
RVA(0x0000f240, 0x48)
i16 GetPartySide(i16 x, i16 y, i16 direction) {
    MapCoord offset;
    i16 side;
    offset = RelativeOffset(x, y, direction, g_field.pos.x, g_field.pos.y);
    if (offset.x < 0) {
        side = 3;
    } else if (offset.x > 0) {
        side = 1;
    } else {
        side = RandomUpTo(1) * 2 + 1;
    }
    return side;
}

// Turns an object towards the party (plus `turn` quarter turns) and steps it
// one cell forward when the way is free; when blocked and not already turned
// aside, retries once towards the party's side. Returns whether a visible
// change was refreshed. `mode` 1 stops next to the party.
// @early-stop tail merge: the two visible/turned refresh exits coalesce here;
// retail keeps them separate. Shared loop breaks retain that merge, while
// routing the successful move through the same exit merges all three sites.
RVA(0x0000f290, 0x24d)
i16 StepObjectTowardParty(FieldObject* object, i16 turn, i16 mode) {
    i16 x;
    i16 y;
    i16 visible;
    i16 retried;
    i16 code;
    i16 turned;
    i16 direction;
    retried = 0;
    if (TestFieldObjectFlag(object, 0x20)) {
        return 0;
    }
    code = GetMapCellCode(object->pos.x, object->pos.y);
    visible = GetPartyView(object->pos.x, object->pos.y);
    visible |= IsCellInView(object->pos.x, object->pos.y);
    for (;;) {
        x = object->pos.x;
        y = object->pos.y;
        turned = 0;
        direction = DirectionToParty(x, y);
        direction = TurnDirection(direction, turn);
        SetObjectDirection(object, direction, turned);
        if (!DistanceFromParty(x, y) && mode == 1) {
            return RefreshIfTurned(visible, turned);
        }
        if (!WallStops(GetMapWallKind(x, y, direction), WALL_STOP_MOVEMENT)) {
            StepMapCoord(&x, &y, object->direction, 0);
            WrapMapPosition(&x, &y);
            if (!CellCodeDiffers(code, x, y) && !IsCellBlocked(g_field.pos.level, 1, x, y)) {
                visible |= GetPartyView(x, y);
                object->pos.y = y;
                object->pos.x = x;
                object->word21d = 1;
                if (!DistanceFromParty(x, y)) {
                    InvalidateSelectedHotspot();
                }
                if (GetGameState() == 0x22) {
                    ClearSelectedHotspot();
                }
                return RefreshIfTurned(visible, 1);
            }
        }
        if (turn != 0 || retried != 0) {
            return RefreshIfTurned(visible, turned);
        }
        retried = 1;
        turn = GetPartySide(object->pos.x, object->pos.y, direction);
    }
}

// Faces the party plus `turn` quarter turns; refreshes when turned in view.
RVA(0x0000f4e0, 0x66)
void FaceObjectToParty(FieldObject* object, i16 turn) {
    i16 x = object->pos.x;
    i16 y = object->pos.y;
    i16 turned = 0;
    i16 view = GetPartyView(x, y);
    i16 direction = DirectionToParty(x, y);
    direction = TurnDirection(direction, turn);
    SetObjectDirection(object, direction, turned);
    RefreshIfTurned(view, turned);
}

// The grid distance from a field actor to the party.
RVA(0x0000f550, 0x32)
i16 DistanceToParty(FieldActor* actor) {
    MapCoord coord;
    coord = GetMapCoord();
    return GridDistance(actor->pos.x, actor->pos.y, coord.x, coord.y);
}

// Whether the two script objects are between the near (high nibble) and far
// (low nibble) distances of `range`.
RVA(0x0000f590, 0x8c)
i16 IsWithinRange(i16 range) {
    i16 low = (range >> 4) & 15;
    MapCoord a;
    MapCoord b;
    i16 distance;
    range &= 0xf;
    a = GetFieldTargetCoord(g_actorId);
    b = GetFieldTargetCoord(g_targetId);
    distance = GridDistance(a.x, a.y, b.x, b.y);
    if (distance < low) {
        return 0;
    }
    return distance <= range;
}

// Callees of the object action flow (0x40f620, 0x40f890), declared here rather
// than through their headers (FieldSight.h, and before RunObjectStep
// Actor.h, ConditionAge.h, Script.h, FieldMap.h): included at the top of this
// file they perturb RelativeFacing/RollEncounterSlot (TU state).
i32 IsSkillIdBlocked(Character* character, i16 id);

// @identity-TODO: the enemy action flow's helpers: the action wait (0x43f510,
// on the actor's field mark), the action pick (0x405cd0) and its adjustment
// (0x406180), and the scene start of a talking actor (0x43b340).
RVA_DECL(0x0003f510)
i16 TickActionWait(ActionWait* wait, i16 speed);

// An object's use of skill `skill` in the field. A kind-2 skill first picks
// its target among the objects in sight: with byte +0xa set, the ones while
// the user is at three quarters of its HP or less, else the ones whose
// conditions the skill's kind applies to (lowest value wins). Returns -1 when
// the skill cannot be used, is out of range or fails, else whether a target
// was picked.
RVA(0x0000f620, 0x26c)
i16 UseObjectSkill(FieldObject* object, i16 skill) {
    FieldSkillCandidate best, candidates[16];
    Character* actor = (Character*)&object->kind;
    FieldObject* target;
    MapCoord coord;
    i16 picked;
    i16 count;
    i16 i;
    picked = 0;
    if (CanUseSkill(skill, actor) <= 0) {
        return -1;
    }
    if ((i16)GetSkillKind(skill) == 2) {
        count = 0;
        for (i = 0; i < 16; i++) {
            InitFieldSkillCandidate(&candidates[i]);
        }
        if (GetCachedSkill(skill)->parameters.valueB) {
            for (i = 0; i < 16; i++) {
                if (GetLiveObject(i) >= 0) {
                    target = GetFieldObject(i);
                    coord = GetObjectCoord(i);
                    if (GetSightCell(object->pos.x, object->pos.y, coord.x, coord.y)
                        && object->pools.hp.cur <= (u16)((object->pools.hp.max >> 2) * 3)) {
                        SetFieldSkillCandidate(&candidates[count], i, object->pools.hp.cur);
                        count++;
                    }
                }
            }
        } else {
            for (i = 0; i < 16; i++) {
                if (GetLiveObject(i) >= 0) {
                    target = GetFieldObject(i);
                    coord = GetObjectCoord(i);
                    if (GetSightCell(object->pos.x, object->pos.y, coord.x, coord.y)
                        && ConditionKindApplies(
                            GetSkillEffectCode(GetCachedSkill(skill)),
                            GetFieldObjectConditions(target)
                        )) {
                        SetFieldSkillCandidate(&candidates[count], i, object->pools.hp.cur);
                        count++;
                    }
                }
            }
        }
        InitFieldSkillCandidate(&best);
        for (i = 0; i < count; i++) {
            if (candidates[i].value < best.value) {
                SetFieldSkillCandidate(&best, candidates[i].index, candidates[i].value);
            }
        }
        if (best.index < 0) {
            return -1;
        }
        object->pickObject = best.index;
        picked = 1;
        g_targetId = object->pickObject;
    }
    if (!GetFieldMarker() && !IsWithinRange(GetSkillAttackRange(skill))) {
        return -1;
    }
    if (IsSkillIdBlocked(actor, skill) == 1) {
        return -1;
    }
    return picked != 0;
}

// Declared here, after the functions RelativeFacing and RollEncounterSlot sit
// among: in FieldObject.h or FieldMap.h they perturb those (TU state).
i16 HasTurnElapsed(void);
i16 AgeConditions(ConditionSet* conditions, i16 amount);
i16 RecoverConditions(Character* character);
void AlertActor(Character* actor, i16 state);
struct ScriptContext* GetCurrentScript(void);
void StartScriptInCode(u32 code, i16 arg, i16 entry, struct ScriptContext* script);
struct ScriptContext* NewScriptContext(i16 mode, Character* actor);
void FreeScriptContext(struct ScriptContext* script);
i16 RunScriptStep(i16 window);
u16 RetakeDeferredChar(i16 window, i16 result, const char* caller);
i16 ChooseObjectTarget(FieldObject* object);

// One step of a field object's action flow: ticks its conditions and action
// wait, runs its object script, then acts by its mode (1 attack or skill,
// 2/4/5/6/7/9 move, 11 talk). Returns 0 when it did not act.
RVA(0x0000f890, 0x490)
i16 RunObjectStep(FieldObject* object, i16 index) {
    i16 scenes[5];
    Character* actor;
    i16 action;
    i16 tries;
    i16 slot;
    i16 result;
    i16 attitude;
    i16 scene;
    if (g_tickElapsed == 0) {
        return 0;
    }
    if (HasTurnElapsed()) {
        AgeConditions(GetFieldObjectConditions(object), 1);
        RecoverConditions((Character*)&object->kind);
    }
    if (TickActionWait(GetFieldObjectActionWait(object), object->actionSpeed)) {
        return 0;
    }
    ResetActionWaitDelay(GetFieldObjectActionWait(object));
    if (GetPickBlockingCondition(GetFieldObjectConditions(object))) {
        return 0;
    }
    SetFieldBusy(1);
    actor = (Character*)&object->kind;
    if (!DistanceToParty((FieldActor*)actor)) {
        AlertActor(actor, 2);
    }
    StartScriptInCode(object->script->code, 0xff, 0, NewScriptContext(1, actor));
    do {
        result = RunScriptStep(0);
    } while (result >= 0);
    // "敵行動フロー"
    RetakeDeferredChar(0, result, "\223G\215s\223\256\203t\203\215\201[");
    FreeScriptContext(GetCurrentScript());
    g_actorId = index;
    action = PickActorAction(actor);
    if (action > 0) {
        if ((action & 0xf) == 4) {
            action = (action & 0xf0) | 1;
        }
        action = AdjustActorAction(index, action);
    }
    switch (object->mode) {
        case 1:
            if (action == 1 || action == 2) {
                goto attack;
            }
            g_actorId = index;
            object->mode = 10;
            object->pickRole = 1;
            object->pickTarget = GetFieldObjectEquipment(object)[5].item;
            object->pickTargetHigh = 0;
            g_actionId = 1;
            tries = 0;
            for (;;) {
                slot = RollEncounterSlot(object->encounterRow);
                if (slot >= 1 && slot <= 8) {
                    g_actionId = object->skills[slot - 1];
                    object->pickTarget = g_actionId;
                    object->pickTargetHigh = 1;
                    object->pickRole = 4;
                    result = UseObjectSkill(object, g_actionId);
                    if (result < 0) {
                        if (++tries < 4) {
                            continue;
                        }
                        g_actionId = 1;
                        object->pickRole = 1;
                        object->pickTarget = GetFieldObjectEquipment(object)[5].item;
                        object->pickTargetHigh = 0;
                    } else if (result != 0) {
                        action = 2;
                        goto attack;
                    }
                } else {
                    object->mode = 10;
                    object->pickRole = 1;
                    object->pickTarget = GetFieldObjectEquipment(object)[5].item;
                    object->pickTargetHigh = 0;
                    g_actionId = 1;
                }
                if (GetFieldMarker() || IsWithinRange(GetSkillAttackRange(g_actionId))) {
                    goto attack;
                }
                if (++tries >= 4) {
                    StepObjectTowardParty(object, 0, 1);
                    break;
                }
            }
            break;
        attack:
            FaceObjectToParty(object, 0);
            GetFieldObjectActionWait(object)->ready = 0;
            if (action < 2) {
                ChooseObjectTarget(object);
                break;
            }
            if (!PushPromptState(0, 0, 200, 450, 0)) {
                g_targetId = object->pickObject;
            }
            break;
        case 2:
            StepObjectTowardParty(object, 2, 0);
            break;
        case 5:
            StepObjectTowardParty(object, 0, 0);
            break;
        case 4:
        case 6:
            StepObjectTowardParty(object, 0, 1);
            break;
        case 7:
            StepObjectTowardParty(object, RandomUpTo(1) * 2 + 1, 0);
            break;
        case 9:
            StepObjectTowardParty(object, RandomUpTo(3), 0);
            break;
        case 11:
            scenes[0] = 3;
            scenes[1] = 1;
            scenes[2] = -1;
            scenes[3] = 2;
            scenes[4] = -1;
            if (IsEventFlagSet(2, 7) && IsEventFlagSet(2, 8)) {
                object->mode = 10;
                break;
            }
            attitude = object->attitude;
            if (attitude < 0 || attitude > 4) {
                attitude = 3;
            }
            scene = scenes[attitude];
            if (scene == -1) {
                scene = 2;
            }
            StartActorScene(0xe2, scene, index, actor);
            break;
    }
    RequestObjectRedraw(object->slot, object->pos.x, object->pos.y);
    return 1;
}

// Picks the party member an object acts on: its party-target skill prompt,
// or a random member able to act (else any member alive); 0 when there is
// none. The pick goes to script object B and the actor's pick target.
i16 BeginPartyTargetSkill(Character* character);
i16 FindPartyPositionOfId(i16 id);

RVA(0x0000fd20, 0x171)
i16 ChooseObjectTarget(FieldObject* object) {
    Character* member;
    i16 candidates[6];
    i16 count;
    i16 i;
    i16 target;
    if (object->kind == 0xce) {
        target = FindPartyPositionOfId(0x22);
        if (target != -1) {
            if (PushPromptState(0, 0, 200, 450, 0)) {
                goto done;
            }
            goto picked;
        }
    }
    if (GetPickBlockingCondition(GetFieldObjectConditions(object))) {
        return 0;
    }
    if (BeginPartyTargetSkill((Character*)&object->kind)) {
        goto done;
    }
    count = 0;
    for (i = 0; i < 6; i++) {
        member = GetPartyCharacter(i);
        if (member != NULL && !GetDisablingCondition(GetCharacterConditions(member))
            && TestCharacterFlag(member, 0x21) != 1) {
            candidates[count++] = i;
        }
    }
    if (count == 0) {
        for (i = 0; i < 6; i++) {
            member = GetPartyCharacter(i);
            if (member != NULL && !GetFatalCondition(GetCharacterConditions(member))) {
                candidates[count++] = i;
            }
        }
        if (count == 0) {
            return 0;
        }
    }
    target = candidates[RandomUpTo(--count)];
    if (!PushPromptState(0, 0, 200, 450, 0)) {
    picked:
        g_targetId = PartyCombatantId(target);
        ((Character*)&object->kind)->pickObject = g_targetId;
    }
done:
    return 1;
}

// For a member using a party-target skill: opens the target prompt, or
// targets script object A at once. 0 for other picks.
RVA(0x0000fea0, 0x88)
i16 BeginPartyTargetSkill(Character* character) {
    i16 flags;
    if (character->pickRole != 4) {
        return 0;
    }
    flags = GetSkillTargetFlags(character->pickTarget);
    if (!TargetFlagsSelectActorGroup(flags) && !(flags & (TARGET_ACTOR_SIDE | TARGET_SELF))) {
        return 0;
    }
    if (!PushPromptState(0, 0, 200, 450, 0)) {
        g_targetId = g_actorId;
        character->pickObject = g_targetId;
    }
    return 1;
}
