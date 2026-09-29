// @identity-TODO: the owning TU is unproven. One retail object: the .bss
// statics of fieldobj, demontable, familiarity and worldencounter interleave
// in one run, each read only by its own unit's code, and the code is
// contiguous in .text in that order. It holds the field objects (sixteen
// slots of enemies placed on the field map, spawned from map cells and
// records, checked and drawn every frame), the demon tables, the saved
// per-id counts that become a demon's familiarity with the per-id analyze
// bits, and the world-map encounters.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/ActionMark.h>
#include <Game/Actor.h>
#include <Game/Alignment.h>
#include <Game/AnalyzeData.h>
#include <Game/AreaMap.h>
#include <Game/BattleEffect.h>
#include <Game/CharInfo.h>
#include <Game/Character.h>
#include <Game/Clock.h>
#include <Game/CombatantId.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/DoorRegion.h>
#include <Game/EquipSlotIndex.h>
#include <Game/Familiarity.h>
#include <Game/Field.h>
#include <Game/FieldActor.h>
#include <Game/FieldLayer.h>
#include <Game/FieldMain.h>
#include <Game/FieldMap.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/ItemRecord.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyAction.h>
#include <Game/PartyCommand.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Game/StatUpdate.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/TargetFlags.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Math/FieldSubcell.h>
#include <Math/Vec3.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Ui/Hotspot.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

// @identity-TODO: the floor cell of the n-th of up to ten objects drawn
// together, far rows and the near row.
DATA(0x00064328)
static const GZ_ENUM_STORAGE(FieldSubcell, i16) s_farCells[10][9] = {
    {FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER},
    {FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW2_COL1,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW2_COL0},
    {FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW2_COL1,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW2_COL0},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW2_COL1,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW2_COL0},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW2_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW2_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW2_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW2_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW2_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW2_COL0,
     FIELD_SUBCELL_ROW2_COL2,
     FIELD_SUBCELL_ROW2_COL1},
};

DATA(0x000643e0)
static const GZ_ENUM_STORAGE(FieldSubcell, i16) s_nearCells[10][9] = {
    {FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_CENTER},
    {FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2},
    {FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL0},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL2},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW0_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1},
    {FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1,
     FIELD_SUBCELL_ROW1_COL0,
     FIELD_SUBCELL_ROW1_COL2,
     FIELD_SUBCELL_CENTER,
     FIELD_SUBCELL_ROW0_COL0,
     FIELD_SUBCELL_ROW0_COL2,
     FIELD_SUBCELL_ROW0_COL1},
};

// @identity-TODO: the screen rise per view depth (0..-4).
DATA(0x00064498)
static const i16 s_depthRise[8] = {0, 20, 12, 4, 0};

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

DATA(0x00068610)
static i16 s_encounterSpread[4] = {0, 0, 1, 2};

// One bit per id: whether it has been analyzed.
DATA(0x00078878)
static u8 s_analyzed[0x40] = {0};

DATA(0x000788b8)
static i16 s_encounterGroups[2] = {0};

DATA(0x000788c0)
u8 g_worldEncounterGroupSlots[16] = {0};

// @identity-TODO: nine cumulative weights per encounter row (data file 5).
DATA(0x000788d0)
static u8 s_encounterBuffer[0x100] = {0};

DATA(0x000789d0)
static FieldObject s_objects[FIELD_OBJECT_COUNT] = {0};

// The 7x7 sight grid around the object being stepped (1: seen).
DATA(0x0007ada0)
static u8 s_sight[7][7] = {0};

// @identity-TODO: a count per id (0..255) whose eighth is the familiarity.
DATA(0x0007add8)
static u8 s_familiarityCounts[0x200] = {0};

// The object record buffer every record read goes through.
DATA(0x0007afd8)
static ObjectRecord s_record = {0};

DATA(0x0007b058)
static u8 s_scriptSetBuffer[0x50] = {0};

DATA(0x0007b0a8)
static i16 s_encounterCount = 0;

DATA(0x0007b0ac)
static i16 s_fieldTableIndex = 0;

// While set, GetLiveObject accepts any index.
DATA(0x0007b0b0)
static i16 s_objectCheckBypass = 0;

// While set, a hidden object is not removed (DeferObjectRemoval instead).
DATA(0x0007b0b4)
static i16 s_objectRemovalDeferred = 0;

// While set, the objects are not marked on the automap.
DATA(0x0007b0b8)
static i16 s_objectsFrozen = 0;

DATA(0x0007b0bc)
static i16 s_encounterChanceBonus = 0;

DATA(0x0007b0c0)
static i32 s_encounterChoices = 0;

DATA(0x0007b0c4)
static i32 s_encounterWeights = 0;

DATA(0x0007b0c8)
static i32 s_encounterBlock = 0;

DATA(0x0007b0cc)
static i32 s_fieldTable = 0;

// Record and race-class tables, followed by the four name tables.
DATA(0x0007b0d0)
static i32 s_demonRecords = 0;

DATA(0x0007b0d4)
static i32 s_raceClasses = 0;

DATA(0x0007b0d8)
static i32 s_raceNames = 0;

DATA(0x0007b0dc)
static i32 s_pantheonNames = 0;

DATA(0x0007b0e0)
static i32 s_humanTitles = 0;

DATA(0x0007b0e4)
static i32 s_classNames = 0;

DATA(0x0007b0e8)
static u32 s_lastEncounterMinute = 0;

DATA(0x0007b0ec)
static u8* s_fieldEncounterWeights = NULL;

// The countdown (in ticks) to the next random spawn and the spawn interval
// in seconds.
DATA(0x0007b0f0)
static i32 s_spawnTimer = 0;

DATA(0x0007b0f4)
static u16 s_spawnInterval = 0;

DATA(0x0007b0f8)
static FieldLayer s_layers[2] = {0};

DATA(0x0007b2f8)
static FieldLayer s_savedLayers[2] = {0};

// The object script block (data file 0x6800), loaded on first use.
DATA(0x0007b4f4)
static ScriptBlock* s_objectScripts = NULL;

// The script-set table: three data-file numbers per set (0xff: none).
DATA(0x0007b4f8)
static u8* s_scriptSets = NULL;

RVA(0x0000d790, 0x52)
b16 InitFieldObjects(void) {
    i16 i;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        s_objects[i].layer = FIELD_LAYER_NONE;
        s_objects[i].redraw = false;
        s_objects[i].anim = 0;
        s_objects[i].script = NULL;
        InitWordList(&s_objects[i].list, 0);
    }
    ModifyEventFlag(8, 0, BIT_CHANGE_SET);
    return false;
}

// Frees slot `index`. When `announce` is set, a live object also clears its
// event flag and queues its event; the level is marked when the last object
// with an event goes.
RVA(0x0000d7f0, 0x145)
void RemoveFieldObject(i16 index, i16 announce) {
    b16 queued = false;
    i16 i;
    if (announce != 0 && s_objects[index].layer != FIELD_LAYER_NONE) {
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
            queued = true;
        }
    }
    s_objects[index].layer = FIELD_LAYER_NONE;
    s_objects[index].redraw = false;
    s_objects[index].anim = 0;
    s_objects[index].script = FreeScriptBlock(s_objects[index].script);
    s_objects[index].hidden = false;
    ClearCondition(GetFieldObjectConditions(&s_objects[index]), CONDITION_ZOMBIE);
    ResetWordList(&s_objects[index].list, 0);
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE) {
            return;
        }
    }
    ModifyEventFlag(8, 0, BIT_CHANGE_SET);
    if (queued != false && !HasQueuedObjectEvents()) {
        MarkLevelEvent(g_party.field.pos.level);
    }
}

RVA(0x0000d940, 0x31)
b16 ResetFieldObjects(void) {
    i16 i;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        RemoveFieldObject(i, 0);
    }
    ModifyEventFlag(8, 0, BIT_CHANGE_SET);
    s_objectsFrozen = 0;
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0000d980, 0x30)
b16 IsFieldActor(const void* actor) {
    i16 i;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (&s_objects[i].kind == actor) {
            return true;
        }
    }
    return false;
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer == layer) {
            return i;
        }
    }
    return -1;
}

RVA(0x0000da00, 0x33)
void SetObjectEventFlag(i16 index, u8 bank, u8 flag) {
    if (index >= 0 && index < FIELD_OBJECT_COUNT) {
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
    s_objects[slot].redraw = false;
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
    s_objects[slot].hidden = false;
    if (alternate != 0) {
        file = OpenDataFile(0x6802, DATA_FILE_SCRIPT, 0);
    } else if (TestModeFlags(MODE_WORLD_MAP)) {
        file = OpenDataFile(0x6801, DATA_FILE_SCRIPT, 0);
    } else {
        file = OpenDataFile(0x6800, DATA_FILE_SCRIPT, 0);
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
    ModifyEventFlag(8, 0, BIT_CHANGE_CLEAR);
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
        if (index < 0 || index >= FIELD_OBJECT_COUNT || s_objects[index].layer == FIELD_LAYER_NONE
            || s_objects[index].hidden != false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
        for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
            if (s_objects[i].layer != FIELD_LAYER_NONE) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE && s_objects[i].redraw != false) {
            s_objects[i].redraw = false;
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
    if (object->layer == FIELD_LAYER_NONE) {
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
b16 DrawFieldObject(FieldObject* object, u32 image, i16 index, i16 total, i16 drawn) {
    Vec3 cell;
    ScreenPoint point;
    i16 facing;
    i16 sprite;
    u32 frame;
    i16 redraw;
    if (!GetObjectLifeState(object)) {
        return false;
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
    CellToField(g_viewLateral, g_viewDepth, FIELD_SUBCELL_CENTER, &cell);
    frame = GetLayerFrame(image, 0, cell.z);
    redraw = object->redraw != false;
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
    object->redraw = false;
    return true;
}

RVA(0x0000e160, 0x7a)
void DrawFieldObjects(void) {
    i16 total = 0;
    i16 drawn;
    i16 i;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (CheckObjectState(i) >= 1) {
            total += GetObjectLifeState(&s_objects[i]);
        }
    }
    drawn = 0;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
    if (s_objects[index].layer == FIELD_LAYER_NONE) {
        return -1;
    }
    if (s_objects[index].hidden == false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
    for (i = start; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer == FIELD_LAYER_NONE || s_objects[i].hidden != false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE && s_objects[i].hidden == false
            && s_objects[i].pos.y >= 4) {
            ResetObjectAnim(i);
            s_objects[i].hidden = true;
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        cleared[i] = 0;
        if (s_objects[i].layer != FIELD_LAYER_NONE && s_objects[i].hidden != false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE && marked[i] != 0) {
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
    if (object->redraw != false) {
        imageCode = 4;
    } else if (object->anim != 0) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->hidden != false) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->acting) {
        imageCode = 3;
    }
    return imageCode;
}

RVA(0x0000e600, 0x77)
i16 GetObjectFacingImageCode(i16 index) {
    FieldObject* object = &s_objects[index];
    i16 imageCode =
        s_facingImageCodes[RelativeFacing(g_party.field.pos.direction, object->direction)];
    if (object->redraw != false) {
        imageCode = 4;
    } else if (object->anim != 0) {
        imageCode = FIELD_OBJECT_IMAGE_LIT | 4;
    } else if (object->hidden != false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE && s_objects[i].hidden == false) {
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        if (s_objects[i].layer != FIELD_LAYER_NONE && s_objects[i].anim != 0) {
            if (s_objects[i].anim == -1) {
                s_objects[i].anim = 1;
            } else {
                s_objects[i].anim++;
            }
            if (s_objects[i].anim > 8) {
                s_objects[i].hidden = true;
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
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        CheckObjectState(i);
    }
}

RVA(0x0000e740, 0x16)
i16 GetObjectLayer(i16 index) {
    return s_objects[index].layer;
}

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
    if (layerSlot == 0) {
        return none;
    }
    layer = layerSlot - 1;
    if (file == 0xff) {
        if (s_objectScripts == NULL) {
            fp = OpenDataFile(0x6800, DATA_FILE_SCRIPT, 0);
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
    if (force != false) {
        RequestRefresh();
        return;
    }
    if (object->hidden != false) {
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
        fp = OpenDataFile(DATA_TABLE_OBJECT_SCRIPT_SETS, DATA_FILE_TABLE, 0);
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

MapCoord RandomNearOffset(void);

static __inline void RestartEnemySpawnTimer(void) {
    s_spawnTimer = s_spawnInterval * 60;
}

// Sets the spawn interval (0 picks 40..60 seconds) and restarts the timer.
RVA(0x0000ec60, 0x31)
void SetSpawnInterval(i16 seconds) {
    if (seconds == 0) {
        seconds = RandomAverage(0x28, 0x3c, 0);
    }
    s_spawnInterval = seconds;
    RestartEnemySpawnTimer();
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
        x = offset.x + g_party.field.pos.x;
        y = offset.y + g_party.field.pos.y;
        x = WrapMapCoord(x, width);
        y = WrapMapCoord(y, height);
        if (!IsCellBlocked(g_party.field.pos.level, 1, x, y) && GetMapCellCode(x, y) == code) {
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
        RestartEnemySpawnTimer();
        if (!IsEventFlagSet(8, 0)) {
            return SpawnRandomEnemy();
        }
    }
    return -1;
}

// Traces the sight lines from x/y along `direction` row by row (up to three
// steps back) until a cell blocks it.
RVA(0x0000ee70, 0x80)
void TraceSight(i16 x, i16 y, GZ_ENUM_PARAM(ViewDirection, i16) direction) {
    i16 step;
    i16 right;
    i16 left;
    i16 cellX;
    i16 cellY;
    i16 blocked;
    left = -3;
    right = 3;
    for (step = 0; step >= -3; step--) {
        ScanSightRow(x, y, step, direction, &left, &right);
        cellX = x;
        cellY = y;
        OffsetMapCoord(&cellX, &cellY, direction, 0, step);
        blocked = GetMapWallKind(cellX, cellY, direction);
        if (blocked) {
            break;
        }
    }
}

#define MarkSightCell(x0, y0, x, y)                                                                \
    do {                                                                                           \
        (x) += 3 - (x0);                                                                           \
        (y) += 3 - (y0);                                                                           \
        if ((y) >= 0 && (x) >= 0) {                                                                \
            s_sight[(y)][(x)] = 1;                                                                 \
        }                                                                                          \
    } while (0)

// Marks the cells of sight row `step` seen from x/y facing `direction`, out
// to the left and right bounds; a blocking cell narrows its bound.
RVA(0x0000eef0, 0x179)
void ScanSightRow(
    i16 x,
    i16 y,
    i16 step,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16* left,
    i16* right
) {
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
        MarkSightCell(x, y, cellX, cellY);
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
        MarkSightCell(x, y, cellX, cellY);
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
    direction = RelativeDirection(x, y, coord.x, coord.y, VIEW_NORTH);
    if (x == coord.x && y == coord.y) {
        direction = TurnDirection(g_party.field.pos.direction, 2);
    }
    return direction;
}

RVA(0x0000f1a0, 0x34)
void LoadEncounterWeights(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_FIELD_ENCOUNTER_WEIGHTS, DATA_FILE_TABLE, 0);
    ReadRawBlock(fp, s_encounterBuffer);
    s_fieldEncounterWeights = s_encounterBuffer;
    CloseDataFile(fp);
}

// Rolls one of the nine slots of encounter row `row` by its weights.
RVA(0x0000f1e0, 0x35)
i16 RollEncounterSlot(i16 row) {
    u8 roll = RandomUpTo(0xff);
    i16 i;
    for (i = 0; i < 9; i++) {
        if (roll <= s_fieldEncounterWeights[row * 9 + i]) {
            break;
        }
    }
    return i;
}

RVA(0x0000f220, 0x1e)
b16 RefreshIfTurned(i16 visible, i16 turned) {
    if (visible != 0 && turned != 0) {
        RequestFieldRefresh();
        return true;
    }
    return false;
}

// The side (1 or 3) the party is on seen from x/y facing `direction`; a
// random side when straight ahead or behind.
RVA(0x0000f240, 0x48)
i16 GetPartySide(i16 x, i16 y, i16 direction) {
    MapCoord offset;
    i16 side;
    offset = RelativeOffset(x, y, direction, g_party.field.pos.x, g_party.field.pos.y);
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
b16 StepObjectTowardParty(FieldObject* object, i16 turn, i16 mode) {
    i16 x;
    i16 y;
    i16 visible;
    i16 retried;
    i16 code;
    i16 turned;
    i16 direction;
    retried = false;
    if (TestFieldObjectFlag(object, 0x20)) {
        return false;
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
            StepMapCoord(&x, &y, object->direction, MOVE_FORWARD);
            WrapMapPosition(&x, &y);
            if (!CellCodeDiffers(code, x, y) && !IsCellBlocked(g_party.field.pos.level, 1, x, y)) {
                visible |= GetPartyView(x, y);
                object->pos.y = y;
                object->pos.x = x;
                object->word21d = 1;
                if (!DistanceFromParty(x, y)) {
                    InvalidateSelectedHotspot();
                }
                if (GetGameState() == GAME_STATE_FIELD) {
                    ClearSelectedHotspot();
                }
                return RefreshIfTurned(visible, 1);
            }
        }
        if (turn != 0 || retried != false) {
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
b16 IsWithinRange(i16 range) {
    i16 low = (range >> 4) & 15;
    MapCoord a;
    MapCoord b;
    i16 distance;
    range &= 0xf;
    a = GetFieldTargetCoord(g_actorId);
    b = GetFieldTargetCoord(g_targetId);
    distance = GridDistance(a.x, a.y, b.x, b.y);
    if (distance < low) {
        return false;
    }
    return distance <= range;
}

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
        for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
            InitFieldSkillCandidate(&candidates[i]);
        }
        if (GetSkillValueB(GetCachedSkill(skill))) {
            for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
            for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
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
    if (IsSkillIdBlocked(actor, skill) == true) {
        return -1;
    }
    return picked != 0;
}

b16 ChooseObjectTarget(FieldObject* object);

// One step of a field object's action flow: ticks its conditions and action
// wait, runs its object script, then acts by its mode (1 attack or skill,
// 2/4/5/6/7/9 move, 11 talk). Returns 0 when it did not act.
// The attack tail exits both the retry loop and the mode switch. The retry
// count cannot identify success: a final failed skill can still attack.
// Inlining that tail at each exit prevents the retail tail merge.
RVA(0x0000f890, 0x490)
b16 RunObjectStep(FieldObject* object, i16 index) {
    i16 scenes[5];
    Character* actor;
    i16 action;
    i16 tries;
    i16 slot;
    i16 result;
    i16 attitude;
    i16 scene;
    if (g_tickElapsed == 0) {
        return false;
    }
    if (HasTurnElapsed()) {
        AgeConditions(GetFieldObjectConditions(object), 1);
        RecoverConditions((Character*)&object->kind);
    }
    if (TickActionWait(GetFieldObjectActionWait(object), object->actionSpeed)) {
        return false;
    }
    ResetActionWaitDelay(GetFieldObjectActionWait(object));
    if (GetPickBlockingCondition(GetFieldObjectConditions(object))) {
        return false;
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
        if ((action & CONDITION_ACTION_MASK) == CONDITION_ACTION_NONE) {
            action = (action & CONDITION_ACTION_FLAGS_MASK) | CONDITION_ACTION_ATTACK_OPPONENT;
        }
        action = AdjustActorAction(index, action);
    }
    switch (object->mode) {
        case ACTOR_MODE_ATTACK:
            if (action == 1 || action == 2) {
                goto attack;
            }
            g_actorId = index;
            object->mode = ACTOR_MODE_IDLE;
            object->pickRole = PICK_ROLE_ATTACK;
            SetFieldObjectPickTarget(
                object,
                GetFieldObjectEquipment(object)[EQUIP_SLOT_WEAPON].item
            );
            g_actionId = 1;
            tries = 0;
            for (;;) {
                slot = RollEncounterSlot(object->encounterRow);
                if (slot >= 1 && slot <= 8) {
                    g_actionId = object->skills[slot - 1];
                    object->pickTarget = g_actionId;
                    object->pickTargetHigh = 1;
                    object->pickRole = PICK_ROLE_MAGIC;
                    result = UseObjectSkill(object, g_actionId);
                    if (result < 0) {
                        if (++tries < 4) {
                            continue;
                        }
                        g_actionId = 1;
                        object->pickRole = PICK_ROLE_ATTACK;
                        SetFieldObjectPickTarget(
                            object,
                            GetFieldObjectEquipment(object)[EQUIP_SLOT_WEAPON].item
                        );
                    } else if (result != 0) {
                        action = 2;
                        goto attack;
                    }
                } else {
                    object->mode = ACTOR_MODE_IDLE;
                    object->pickRole = PICK_ROLE_ATTACK;
                    SetFieldObjectPickTarget(
                        object,
                        GetFieldObjectEquipment(object)[EQUIP_SLOT_WEAPON].item
                    );
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
            GetFieldObjectActionWait(object)->ready = ACTION_UNMARKED;
            if (action < 2) {
                ChooseObjectTarget(object);
                break;
            }
            if (!PushPromptState(0, 0, 200, 450, 0)) {
                g_targetId = object->pickObject;
            }
            break;
        case ACTOR_MODE_FLEE:
            StepObjectTowardParty(object, 2, 0);
            break;
        case ACTOR_MODE_CHARGE:
            StepObjectTowardParty(object, 0, 0);
            break;
        case ACTOR_MODE_APPROACH:
        case ACTOR_MODE_PURSUE:
            StepObjectTowardParty(object, 0, 1);
            break;
        case ACTOR_MODE_SIDESTEP:
            StepObjectTowardParty(object, RandomUpTo(1) * 2 + 1, 0);
            break;
        case ACTOR_MODE_WANDER:
            StepObjectTowardParty(object, RandomUpTo(3), 0);
            break;
        case ACTOR_MODE_TALK:
            scenes[0] = 3;
            scenes[1] = 1;
            scenes[2] = -1;
            scenes[3] = 2;
            scenes[4] = -1;
            if (IsEventFlagSet(2, 7) && IsEventFlagSet(2, 8)) {
                object->mode = ACTOR_MODE_IDLE;
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
    return true;
}

// Picks the party member an object acts on: its party-target skill prompt,
// or a random member able to act (else any member alive); 0 when there is
// none. The pick goes to script object B and the actor's pick target.
b16 BeginPartyTargetSkill(Character* character);

RVA(0x0000fd20, 0x171)
b16 ChooseObjectTarget(FieldObject* object) {
    Character* member;
    i16 candidates[6];
    i16 count;
    i16 i;
    i16 target;
    if (object->kind == 0xce) {
        target = FindPartyPositionOfId(0x22);
        if (target != -1) {
            if (PushPromptState(0, 0, 200, 450, 0)) {
                return true;
            }
            g_targetId = PartyCombatantId(target);
            object->pickObject = g_targetId;
            return true;
        }
    }
    if (GetPickBlockingCondition(GetFieldObjectConditions(object))) {
        return false;
    }
    if (BeginPartyTargetSkill((Character*)&object->kind)) {
        return true;
    }
    count = 0;
    for (i = 0; i < PARTY_SIZE; i++) {
        member = GetPartyCharacter(i);
        if (member != NULL && !GetDisablingCondition(GetCharacterConditions(member))
            && TestCharacterFlag(member, 0x21) != true) {
            candidates[count++] = i;
        }
    }
    if (count == 0) {
        for (i = 0; i < PARTY_SIZE; i++) {
            member = GetPartyCharacter(i);
            if (member != NULL && !GetFatalCondition(GetCharacterConditions(member))) {
                candidates[count++] = i;
            }
        }
        if (count == 0) {
            return false;
        }
    }
    target = candidates[RandomUpTo(--count)];
    if (!PushPromptState(0, 0, 200, 450, 0)) {
        g_targetId = PartyCombatantId(target);
        object->pickObject = g_targetId;
    }
    return true;
}

// For a member using a party-target skill: opens the target prompt, or
// targets script object A at once. 0 for other picks.
RVA(0x0000fea0, 0x88)
b16 BeginPartyTargetSkill(Character* character) {
    i16 flags;
    if (character->pickRole != PICK_ROLE_MAGIC) {
        return false;
    }
    flags = GetSkillTargetFlags(character->pickTarget);
    if (!TargetFlagsSelectActorGroup(flags) && !(flags & (TARGET_ACTOR_SIDE | TARGET_SELF))) {
        return false;
    }
    if (!PushPromptState(0, 0, 200, 450, 0)) {
        g_targetId = g_actorId;
        character->pickObject = g_targetId;
    }
    return true;
}

static __inline const DemonTable* ReadDemonTable(void) {
    return HandleReadPtr(s_demonRecords);
}

RVA(0x0000ff30, 0x70)
void LoadDemonTables(void) {
    FILE* fp = OpenDataFile(DATA_TABLE_DEMONS, DATA_FILE_TABLE, 0);
    s_demonRecords = ReadCryptHandle(fp);
    s_raceClasses = ReadCryptHandle(fp);
    s_raceNames = ReadCryptHandle(fp);
    s_pantheonNames = ReadCryptHandle(fp);
    s_humanTitles = ReadCryptHandle(fp);
    s_classNames = ReadCryptHandle(fp);
    CloseDataFile(fp);
}

RVA(0x0000ffa0, 0x1a)
GZ_ENUM_RETURN(DemonRace, i16) GetDemonRace(i16 id) {
    return ReadDemonTable()->entries[id].race;
}

RVA(0x0000ffc0, 0x1a)
i16 GetDemonPantheon(i16 id) {
    return ReadDemonTable()->entries[id].pantheon;
}

RVA(0x0000ffe0, 0x1a)
i16 GetDemonLevel(i16 id) {
    return ReadDemonTable()->entries[id].level;
}

// -1 when flag bit 2 is set, else flag bit 0.
RVA(0x00010000, 0x27)
i16 GetDemonFlagLow(i16 id) {
    u8 flags = ReadDemonTable()->entries[id].flags;
    if (flags & 4) {
        return -1;
    }
    return (u8)(flags & 1);
}

// -1 when flag bit 6 is set, else flag bit 4.
RVA(0x00010030, 0x2a)
i16 GetDemonFlagHigh(i16 id) {
    u8 flags = ReadDemonTable()->entries[id].flags;
    if (flags & 0x40) {
        return -1;
    }
    return (u8)((flags >> 4) & 1);
}

RVA(0x00010060, 0x12)
i16 GetDemonCount(void) {
    return ReadDemonTable()->count;
}

RVA(0x00010080, 0x19)
i16 GetRaceClass(GZ_ENUM_PARAM(DemonRace, i16) race) {
    u8* classes = HandleReadPtr(s_raceClasses);
    return classes[race];
}

RVA(0x000100a0, 0x17)
i16 GetDemonClass(i16 id) {
    return GetRaceClass(GetDemonRace(id));
}

static __inline char* ReadDemonName(i32 handle, i16 index) {
    DemonNameTable* table = HandleReadPtr(handle);
    return OffsetBy(table, table->offsets[index]);
}

RVA(0x000100c0, 0x33)
char* GetDemonRaceName(i16 id) {
    i16 race = GetDemonRace(id);
    return ReadDemonName(s_raceNames, race);
}

RVA(0x00010100, 0x33)
char* GetDemonClassName(i16 id) {
    i16 cls = GetDemonClass(id);
    return ReadDemonName(s_classNames, cls);
}

RVA(0x00010140, 0x33)
char* GetDemonPantheonName(i16 id) {
    i16 index = GetDemonPantheon(id);
    return ReadDemonName(s_pantheonNames, index);
}

RVA(0x00010180, 0x23)
char* GetHumanTitleName(i16 index) {
    return ReadDemonName(s_humanTitles, index);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x000101b0, 0x35)
char* CopyObjectRecordName(i16 id, char* destination) {
    strcpy(destination, GetObjectRecordName(id));
    return destination;
}

// The highest-level demon of `race` at or below `maxLevel` (-1: none).
RVA(0x000101f0, 0x52)
i16 FindStrongestOfRace(i16 maxLevel, GZ_ENUM_PARAM(DemonRace, i16) race) {
    i16 count = GetDemonCount();
    i16 i;
    i16 bestLevel = -1;
    i16 best = -1;
    i16 level;
    for (i = 32; i < count; i++) {
        if (GetDemonRace(i) == race) {
            level = GetDemonLevel(i);
            if (level <= maxLevel && bestLevel < level) {
                bestLevel = level;
                best = i;
            }
        }
    }
    return best;
}

// The strongest of `race` at or below `maxLevel`, else its weakest.
RVA(0x00010250, 0x65)
i16 FindDemonOfRace(i16 maxLevel, GZ_ENUM_PARAM(DemonRace, i16) race) {
    i16 best = FindStrongestOfRace(maxLevel, race);
    i16 count;
    i16 i;
    i16 bestLevel;
    i16 level;
    if (best != -1) {
        return best;
    }
    count = GetDemonCount();
    best = -1;
    bestLevel = 500;
    for (i = 32; i < count; i++) {
        if (GetDemonRace(i) == race) {
            level = GetDemonLevel(i);
            if (bestLevel > level) {
                bestLevel = level;
                best = i;
            }
        }
    }
    return best;
}

// The highest-level demon of race class `cls` at or below `maxLevel` whose
// low flag is not -1.
RVA(0x000102c0, 0x69)
i16 FindStrongestOfClass(i16 maxLevel, i16 cls) {
    i16 count = GetDemonCount();
    i16 i;
    i16 bestLevel = -1;
    i16 best = -1;
    i16 level;
    for (i = 32; i < count; i++) {
        level = GetDemonLevel(i);
        if (level <= maxLevel && GetDemonFlagLow(i) != -1 && GetDemonClass(i) == cls
            && bestLevel < level) {
            bestLevel = level;
            best = i;
        }
    }
    return best;
}

// The next stronger demon of `id`'s race; when none and `wrap` is set, the
// weakest of the race (else `id`).
RVA(0x00010330, 0xef)
i16 FindNextOfRace(i16 id, i16 wrap) {
    i16 count = GetDemonCount();
    i16 race = GetDemonRace(id);
    i16 bestLevel = 0x7fff;
    i16 best = bestLevel;
    i16 level = GetDemonLevel(id);
    i16 i;
    i16 other;
    for (i = 32; i < count; i++) {
        other = GetDemonLevel(i);
        if (other > level && GetDemonRace(i) == race && bestLevel > other) {
            bestLevel = other;
            best = i;
        }
    }
    if (best < 0x7fff) {
        return best;
    }
    if (wrap == 0) {
        return id;
    }
    bestLevel = 0x7fff;
    best = 0x7fff;
    for (i = 32; i < count; i++) {
        other = GetDemonLevel(i);
        if (other > -1 && GetDemonRace(i) == race && bestLevel > other) {
            bestLevel = other;
            best = i;
        }
    }
    if (best == 0x8000) {
        best = -1;
    }
    return best;
}

// @identity-TODO: (a - b) * 42 / 20 clamped to a signed byte.
RVA(0x00010420, 0x44)
i16 ScaleLevelGap(i16 a, i16 b) {
    i16 gap = (a - b) * 42;
    gap /= 20;
    if (gap < -128) {
        gap = -128;
    } else if (gap > 127) {
        gap = 127;
    }
    return gap;
}

static __inline void RecalcObjectStats(FieldObject* object) {
    ClearStatModifiers(&object->stats);
    RecalcEquippedStatTotals(&object->stats, GetFieldObjectEquipment(object));
    RecalcDerivedStats((Character*)&object->kind);
    ResetBattleStatsToBase(object);
}

// Builds object `object` from its record: identity, pools, stats, item slots
// (an empty gun clears its ammunition, else the ammunition count is the gun's
// magazine size) and skills; kind 0x117 then mirrors the first party member.
RVA(0x00010470, 0x4b1)
void InitObjectFromRecord(FieldObject* object, ObjectRecord* record) {
    i16 i;
    memset(object, 0, sizeof(FieldObject));
    object->kind = record->id;
    strncpy(object->namePrefix, record->name, 17);
    object->macca = record->macca;
    object->magnetite = record->magnetite;
    object->rank = record->level;
    object->title = 0;
    object->experience = record->experience;
    object->byte083 = record->bits68 & 3;
    object->pantheon = GetDemonPantheon(record->id);
    object->alignmentLevelB = ScaleLevelGap(record->alignB[0], record->alignB[1]);
    object->alignmentLevelA = ScaleLevelGap(record->alignA[0], record->alignA[1]);
    object->levelBonus = record->levelBonus;
    object->hundredths = 0;
    object->stats.base[STAT_INTUITION] = record->stats[STAT_INTUITION];
    object->stats.base[STAT_MENTAL_STRENGTH] = record->stats[STAT_MENTAL_STRENGTH];
    object->stats.base[STAT_MAGIC] = record->stats[STAT_MAGIC];
    object->stats.base[STAT_INTELLIGENCE] = record->stats[STAT_INTELLIGENCE];
    object->stats.base[STAT_STRENGTH] = record->stats[STAT_STRENGTH];
    object->stats.base[STAT_VITALITY] = record->stats[STAT_VITALITY];
    object->stats.base[STAT_PROTECTION] = record->stats[STAT_PROTECTION];
    object->stats.base[STAT_AGILITY] = record->stats[STAT_AGILITY];
    object->stats.base[STAT_DEXTERITY] = record->stats[STAT_DEXTERITY];
    object->stats.base[STAT_CHARM] = record->stats[STAT_CHARM];
    object->stats.base[STAT_FORTUNE] = record->stats[STAT_FORTUNE];
    object->actionSpeed = record->actionSpeed;
    GetFieldObjectActionWait(object)->remaining = ACTION_WAIT_RESET - RandomAverage(0, 100, 0);
    for (i = 0; i < sizeof(object->battleTally); i++) {
        object->battleTally[i] = 0;
    }
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_HEAD], record->items[0]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_BODY], record->items[1]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_ARMS], record->items[2]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_LEGS], record->items[3]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_ACCESSORY], record->items[4]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_WEAPON], record->items[5]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_GUN], record->items[6]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO], record->items[7]);
    if (GetFieldObjectEquipment(object)[EQUIP_SLOT_GUN].item < 1) {
        EmptyItemSlot(&GetFieldObjectEquipment(object)[EQUIP_SLOT_GUN]);
        GetFieldObjectEquipment(object)[EQUIP_SLOT_GUN].attachment = -1;
        EmptyItemSlot(&GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO]);
        GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO].attachment = -1;
    } else if (GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO].item < 1) {
        EmptyItemSlot(&GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO]);
        GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO].attachment = -1;
    } else {
        GetFieldObjectEquipment(object)[EQUIP_SLOT_AMMO].quantity = GetGunMagazineSize(
            GetLoadedRecord(GetFieldObjectEquipment(object)[EQUIP_SLOT_GUN].item)
        );
    }
    NormalizeEquipSlots((Character*)&object->kind);
    for (i = 0; i < sizeof(object->conditions.bits); i++) {
        GetFieldObjectConditions(object)->bits[i] = 0;
    }
    for (i = 0; i < sizeof(object->personalFlags); i++) {
        GetFieldObjectFlags(object)[i] = 0;
    }
    object->byte096 = 1;
    object->acting = false;
    object->word098 = 0x11;
    object->attitude = 4;
    object->triggerRange = (record->bits68 >> 2) & 7;
    object->mode = ACTOR_MODE_NONE;
    object->fieldState = 0;
    object->encounterRow = record->encounterRow;
    object->shield = 0;
    for (i = 0; i < 10; i++) {
        object->resistance[i] = record->resistance[i];
    }
    object->moonRow = record->moonRow;
    object->equipGroup = record->equipGroup;
    object->pickFlags = (object->pickFlags & ~1) | ((record->bits68 >> 5) & 1);
    object->pickFlags = (object->pickFlags & ~6) | ((record->bits68 >> 5) & 2);
    object->dropChance = record->dropChance;
    object->pickItem = record->pickItem;
    for (i = 0; i < 3; i++) {
        SetCharacterAffiliation(object, i, record->affiliation[i]);
    }
    ResetWordList(&object->list, 8);
    for (i = 0; i < 8; i++) {
        SetWord(&object->list, i, record->skills[i]);
        object->skills[i] = record->skills[i];
    }
    object->battleStats[0] = 0;
    object->battleStats[6] = 0;
    object->battleStats[12] = 0;
    object->battleStats[18] = 0;
    RecalcObjectStats(object);
    InitCurMax(&object->pools.hp, record->hp);
    InitCurMax(&object->pools.mp, record->mp);
    if (object->kind == 0x117) {
        CopyLeaderIntoObject(object);
    }
}

// @identity-TODO: object kind 0x117 takes over the first party member's
// level, title, the bytes +0x69 (twice), stats, fieldMarkValue and full pools.
RVA(0x00010930, 0xcb)
void CopyLeaderIntoObject(FieldObject* object) {
    Character* leader = GetCharacters();
    object->rank = leader->level;
    object->title = leader->title;
    object->gender = object->byte083 = leader->byte069;
    object->stats = leader->stats;
    GetFieldObjectActionWait(object)->remaining = GetCharacterActionWait(leader)->remaining;
    RecalcObjectStats(object);
    InitCurMax(&object->pools.hp, leader->pools.hp.max);
    InitCurMax(&object->pools.mp, leader->pools.mp.max);
}

// Loads the object record `kind` into `object`.
RVA(0x00010a00, 0x20)
void LoadObjectRecord(i16 kind, FieldObject* object) {
    ReadObjectRecord(kind);
    InitObjectFromRecord(object, &s_record);
}

RVA(0x00010a20, 0x32)
void ReadObjectRecord(i16 kind) {
    FILE* fp = OpenDataFile(kind + 0x2000, DATA_FILE_OBJECT, 0);
    ReadCryptRecord(fp, &s_record);
    CloseDataFile(fp);
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00010a60, 0x14)
i16 GetObjectRecordScriptSet(i16 kind) {
    ReadObjectRecord(kind);
    return s_record.scriptSet;
}

RVA(0x00010a80, 0x13)
char* GetObjectRecordName(i16 kind) {
    ReadObjectRecord(kind);
    return s_record.name;
}

RVA(0x00010aa0, 0x2b)
ObjectPicture GetObjectRecordPicture(i16 kind) {
    ObjectPicture picture;
    ReadObjectRecord(kind);
    picture.index = s_record.imageIndex;
    picture.variant = s_record.imageVariant;
    return picture;
}

// Copies the (cached) record `kind` into `out` (0x7a bytes).
RVA(0x00010ad0, 0x28)
void LoadLayerRecord(i16 kind, ObjectRecord* out) {
    if (out != NULL) {
        ReadObjectRecord(kind);
        memcpy(out, &s_record, sizeof(s_record));
    }
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00010b00, 0x17)
void ReadRecordIfWanted(FILE* fp, void* out) {
    if (out != NULL) {
        ReadCryptRecord(fp, out);
    }
}

// Reads object record `kind` into a local copy and returns the byte (size
// below 2), word (below 4) or dword at `offset`.
RVA(0x00010b20, 0x7d)
i32 ReadObjectRecordField(i16 kind, i16 offset, i16 size) {
    u8 buffer[sizeof(ObjectRecord)];
    FILE* fp = OpenDataFile(kind + 0x2000, DATA_FILE_OBJECT, 0);
    ReadCryptRecord(fp, buffer);
    CloseDataFile(fp);
    if (size < 2) {
        return buffer[offset];
    }
    if (size < 4) {
        return *(i16*)(buffer + offset);
    }
    return *(i32*)(buffer + offset);
}

RVA(0x00010ba0, 0x20)
void SetFamiliarityCount(i16 id, i16 count) {
    s_familiarityCounts[id] = ClampShort(count, 0, 0xff);
}

RVA(0x00010bc0, 0xe)
i16 GetFamiliarityCount(i16 id) {
    return s_familiarityCounts[id];
}

RVA(0x00010bd0, 0x20)
void AddFamiliarityCount(i16 id, i16 delta) {
    delta += GetFamiliarityCount(id);
    SetFamiliarityCount(id, delta);
}

// Derives a record's familiarity and level gap once (personal flag 0).
RVA(0x00010bf0, 0x7b)
void RefreshFamiliarity(Character* character) {
    i16 value;
    i16 leaderLevel;
    if (TestCharacterFlag(character, 0)) {
        return;
    }
    value = GetFamiliarityCount(character->id) / 8;
    character->familiarity = ClampShort(value, 0, 0x3f);
    leaderLevel = GetRosterLeader()->level;
    value = leaderLevel - character->level;
    character->levelGap = ClampShort(value, 0, 0xff);
    SetCharacterFlag(character, 0);
}

RVA(0x00010c70, 0x2a)
void SetLevelGap(Character* character, i16 gap) {
    RefreshFamiliarity(character);
    character->levelGap = ClampShort(gap, 0, 0xff);
}

RVA(0x00010ca0, 0x28)
void AddLevelGap(Character* character, i16 delta) {
    RefreshFamiliarity(character);
    SetLevelGap(character, character->levelGap + delta);
}

// The familiarity, two more unless event flag 2/8 is set.
RVA(0x00010cd0, 0x2e)
i16 GetFamiliarity(Character* character) {
    i16 familiarity;
    RefreshFamiliarity(character);
    familiarity = character->familiarity;
    if (!IsEventFlagSet(2, 8)) {
        familiarity += 2;
    }
    return familiarity;
}

RVA(0x00010d00, 0x27)
void SetFamiliarity(Character* character, i16 familiarity) {
    RefreshFamiliarity(character);
    character->familiarity = ClampShort(familiarity, 0, 0x3f);
}

RVA(0x00010d30, 0x20)
void AddFamiliarity(Character* character, i16 delta) {
    delta += GetFamiliarity(character);
    SetFamiliarity(character, delta);
}

static __inline i32 RollCharacterFunds(Character* character) {
    i32 score = character->level * 10;
    score += RandomUpTo(10);
    return score;
}

// The actor's script magnetite roll: ten times its level plus 0..10.
RVA(0x00010d50, 0x22)
i32 RollCharacterMagnetite(Character* character) {
    return RollCharacterFunds(character);
}

// The actor's script macca roll uses the same level-based distribution.
RVA(0x00010d80, 0x22)
i32 RollCharacterMacca(Character* character) {
    return RollCharacterFunds(character);
}

// @early-stop register scheduling: the alignment argument loads cross the
// saved-result moves differently, and the final sum uses different scratch
// registers. Calls, branch destinations and return paths agree.
RVA(0x00010db0, 0x8e)
i16 AlignmentConflicts(Character* character) {
    i16 leaderClass;
    i16 characterClass;
    if (character == NULL) {
        return -1;
    }
    leaderClass = GetAlignmentClassA(GetRosterLeader());
    characterClass = GetAlignmentClassA(character);
    if ((characterClass < 0 && leaderClass >= 0) || (characterClass >= 0 && leaderClass < 0)) {
        return -1;
    }
    leaderClass = GetAlignmentClassB(GetRosterLeader());
    characterClass = GetAlignmentClassB(character);
    if (characterClass + leaderClass == 0 && leaderClass != 0) {
        return -1;
    }
    return 0;
}

RVA(0x00010e40, 0x14)
b16 ClearAnalyzed(void) {
    memset(s_analyzed, 0, sizeof(s_analyzed));
    return false;
}

RVA(0x00010e60, 0x2e)
void SetAnalyzed(i16 id, i16 on) {
    if (on == false) {
        ClearBit(s_analyzed, id);
        return;
    }
    SetBit(s_analyzed, id);
}

RVA(0x00010e90, 0x13)
b16 HasAnalyzeData(i16 id) {
    return TestBit(s_analyzed, id);
}

// Save and load the counts and the analyze bits; each returns how many bytes
// fell short (0 on success).
RVA(0x00010eb0, 0x24)
i16 SaveFamiliarityCounts(FILE* fp) {
    return 0x200 - fwrite(s_familiarityCounts, 1, 0x200, fp);
}

RVA(0x00010ee0, 0x24)
i16 LoadFamiliarityCounts(FILE* fp) {
    return 0x200 - fread(s_familiarityCounts, 1, 0x200, fp);
}

RVA(0x00010f10, 0x21)
i16 SaveAnalyzed(FILE* fp) {
    return 0x40 - fwrite(s_analyzed, 1, 0x40, fp);
}

RVA(0x00010f40, 0x26)
i16 LoadAnalyzed(FILE* fp) {
    ClearAnalyzed();
    return 0x40 - fread(s_analyzed, 1, 0x40, fp);
}

RVA(0x00010f70, 0x4f)
void FreeEncounterTables(void) {
    s_fieldTable = FreeHandle(s_fieldTable);
    s_encounterBlock = FreeHandle(s_encounterBlock);
    s_encounterWeights = FreeHandle(s_encounterWeights);
    s_encounterChoices = FreeHandle(s_encounterChoices);
}

RVA(0x00010fc0, 0x67)
void LoadEncounterTables(void) {
    FILE* fp;
    FreeEncounterTables();
    fp = OpenDataFile(DATA_TABLE_WORLD_ENCOUNTERS, DATA_FILE_TABLE, 0);
    s_encounterChoices = ReadRawHandle(fp);
    s_encounterWeights = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(DATA_TABLE_WORLD_FIELD_INDEX, DATA_FILE_TABLE, 0);
    s_fieldTable = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00011030, 0xfe)
i16 RollWorldMapEncounter(i16 x, i16 y) {
    i16 cell;
    i16 variant;
    WorldEncounterCell* cells;
    Character* leader = GetRosterCharacter(ROSTER_LEADER);
    if (TestCharacterFlag(leader, 0x22) == true) {
        return -1;
    }
    if (CheckWorldEncounterInterval() < 1) {
        return -1;
    }
    variant = 0;
    cell = LoadWorldEncounterBlock(x, y);
    cells = HandleReadPtr(s_encounterBlock);
    if (!CheckFlagWord(&GetWorldEncounterVariant(&cells[cell], variant)->condition)) {
        variant = 1;
        if (!CheckFlagWord(&GetWorldEncounterVariant(&cells[cell], variant)->condition)) {
            return 0;
        }
    }
    s_fieldTableIndex = cells[cell].fieldTable;
    if (!TestWorldEncounterChance(GetWorldEncounterVariant(&cells[cell], variant)->chance)) {
        return 0;
    }
    s_encounterCount = PrepareWorldEncounter(
        GetWorldEncounterVariant(&cells[cell], variant)->weights,
        GetWorldEncounterVariant(&cells[cell], variant)->choices,
        GetWorldEncounterVariant(&cells[cell], variant)->maximum
    );
    return s_encounterCount;
}

RVA(0x00011130, 0x38)
i16 CheckWorldEncounterInterval(void) {
    u32 next;
    u32 now;
    if (!(g_mousePosition.buttons & MOUSE_RIGHT_DOWN)) {
        next = s_lastEncounterMinute + 120;
    } else {
        next = GetClockMinutes() + 120;
    }
    now = GetClockMinutes();
    if (now < next) {
        return -1;
    }
    s_lastEncounterMinute = now;
    return 1;
}

RVA(0x00011170, 0xab)
i16 LoadWorldEncounterBlock(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    FILE* fp;
    s_encounterBlock = FreeHandle(s_encounterBlock);
    fp = OpenDataFile(block + 0x1000, DATA_FILE_TABLE, 0);
    s_encounterBlock = ReadRawHandle(fp);
    CloseDataFile(fp);
    if (IsOddMapLayer()) {
        return 45;
    }
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    x /= 32;
    y /= 40;
    x += y * 9;
    return x;
}

RVA(0x00011220, 0x53)
b16 TestWorldEncounterChance(i16 chance) {
    i16 totalChance = chance + s_encounterChanceBonus;
    if (totalChance < RandomAverage(1, 100, 2)) {
        s_encounterChanceBonus++;
        if (RosterContainsId(4)) {
            s_encounterChanceBonus++;
        }
        return false;
    }
    s_encounterChanceBonus = 0;
    return true;
}

RVA(0x00011280, 0x77)
i16 PrepareWorldEncounter(i16 weights, i16 choices, i16 maximum) {
    i16 count;
    i16 i;
    maximum = GetWorldEncounterMaximum(maximum);
    if (maximum == 0) {
        return 0;
    }
    count = RandomAverage(1, maximum, 1);
    s_encounterGroups[0] = s_encounterGroups[1] = PickWorldEncounterGroup(weights, choices);
    for (i = 0; i < 10 && s_encounterGroups[1] == s_encounterGroups[0]; i++) {
        s_encounterGroups[1] = PickWorldEncounterGroup(weights, choices);
    }
    AssignWorldEncounterGroups(count);
    return count;
}

RVA(0x00011300, 0x2f)
i16 GetWorldEncounterMaximum(i16 maximum) {
    if (RosterContainsId(4)) {
        maximum += 2;
    }
    maximum += GetPartyEncounterSizeBonus();
    if (maximum >= 16) {
        maximum = 16;
    }
    return maximum;
}

RVA(0x00011330, 0x6f)
i16 GetPartyEncounterSizeBonus(void) {
    i16 total = 0;
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < PARTY_SIZE; i++) {
        character = GetPartyCharacter(i);
        if (character != NULL && IsHumanCharacter(character)) {
            count++;
            total += GetStatTotal(character, STAT_FORTUNE);
        }
    }
    if (count < 1) {
        return 0;
    }
    total = 50 - total / count;
    total /= 10;
    if (total < 1) {
        total = 1;
    }
    return total;
}

RVA(0x000113a0, 0x9f)
i16 PickWorldEncounterGroup(i16 weights, i16 choices) {
    WorldEncounterWeights* table = HandleReadPtr(s_encounterWeights);
    WorldEncounterChoices* groups;
    i16 roll = RandomAverage(1, 100, 0);
    i16 total;
    i16 i;
    if (RosterContainsId(4)) {
        roll += 10;
    }
    total = 0;
    for (i = 0; i < 6; i++) {
        total += table[weights].weights[i];
        if (roll < total) {
            groups = HandleReadPtr(s_encounterChoices);
            return groups[choices].groups[i];
        }
    }
    groups = HandleReadPtr(s_encounterChoices);
    return groups[choices + 1].groups[0];
}

RVA(0x00011440, 0x37)
i16 AssignWorldEncounterGroups(i16 count) {
    i16 i;
    for (i = 0; i < count; i++) {
        if (RandomAverage(1, 100, 0) < 40) {
            g_worldEncounterGroupSlots[i] = 1;
        } else {
            g_worldEncounterGroupSlots[i] = 0;
        }
    }
    return count;
}

RVA(0x00011480, 0x89)
void LoadFieldTable(void) {
    FILE* fp = NULL;
    EncounterFieldImage* images;
    if (s_fieldTable == 0) {
        fp = OpenDataFile(DATA_TABLE_WORLD_FIELD_INDEX, DATA_FILE_TABLE, 0);
        s_fieldTable = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
    images = HandleReadPtr(s_fieldTable);
    LoadFieldImage(
        images[s_fieldTableIndex].image + 0x5000,
        images[s_fieldTableIndex].variant,
        images[s_fieldTableIndex].option
    );
    if (fp != NULL) {
        s_fieldTable = FreeHandle(s_fieldTable);
    }
}

RVA(0x00011510, 0x102)
void PrepareFieldRandom(void) {
    i16 i;
    i16 x;
    i16 y;
    i16 along;
    i16 across;
    i16 spread;
    Character* actor;
    for (i = 0; i < s_encounterCount; i++) {
        x = g_party.field.pos.x;
        y = g_party.field.pos.y;
        along = RandomAverage(-3, 0, 0);
        spread = s_encounterSpread[-along];
        across = RandomAverage(-spread, spread, 0);
        OffsetMapCoord(&x, &y, g_party.field.pos.direction, across, along);
        SpawnFieldObject(
            g_worldEncounterGroupSlots[i],
            x,
            y,
            OppositeDirection(g_party.field.pos.direction),
            s_encounterGroups[g_worldEncounterGroupSlots[i]],
            0,
            -1,
            0
        );
        actor = GetFieldActor(i);
        AlertActor(actor, 2);
    }
    LoadEnemyGroupSlot(0, s_encounterGroups[0]);
    LoadEnemyGroupSlot(1, s_encounterGroups[1]);
}

RVA(0x00011620, 0x37)
b32 AnyObjectInReach(void) {
    b32 found = false;
    i16 i;
    i16 object;
    for (i = 0; i < FIELD_OBJECT_COUNT; i++) {
        object = GetLiveObject(i);
        if (object >= 0 && HasObjectInReach(1, -1, object)) {
            found = true;
        }
    }
    return found;
}
