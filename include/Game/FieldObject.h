#ifndef GITEN_GAME_FIELDOBJECT_H
#define GITEN_GAME_FIELDOBJECT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/Character.h>
#include <Game/DemonPantheon.h>
#include <Game/FieldActor.h>
#include <Game/FieldLayerIndex.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/HumanTitle.h>
#include <Game/ObjectRecordId.h>
#include <Game/ViewDirection.h>
#include <Ints.h>

// Negative image codes request horizontal mirroring. Nonnegative codes
// can include the lit flag for the animated specular-lighting path.
// clang-format off
GZ_ENUM_BEGIN(FieldObjectImageCode)
    FIELD_OBJECT_IMAGE_SIDE_MIRRORED = -1,
    FIELD_OBJECT_IMAGE_FRONT = 0,
    FIELD_OBJECT_IMAGE_SIDE = 1,
    FIELD_OBJECT_IMAGE_BACK = 2,
    FIELD_OBJECT_IMAGE_ACTING = 3,
    FIELD_OBJECT_IMAGE_REACTION = 4,
    FIELD_OBJECT_IMAGE_INDEX_MASK = 0x0f,
    FIELD_OBJECT_IMAGE_LIT = 0x10
GZ_ENUM_END(FieldObjectImageCode);
// clang-format on

#define IsFieldObjectImageLit(code) ((code) >= FIELD_OBJECT_IMAGE_LIT)

// One of the sixteen actors placed on the field map. Layer -1 is a free slot;
// hidden actors are removed on the next check.
// The actor carries the shared state and map position. Event flags, record
// skill choices and the script remain in the containing field object. Slots
// 6/7 are the gun/ammunition pair; the ammunition extra field holds the
// magazine size.
// The field object table, and the layer of an unused object slot.
#define FIELD_OBJECT_COUNT 16
// No field-object index was selected from a target hotspot.
#define FIELD_OBJECT_INDEX_NONE (-1)
// Field-object record kinds run from HUMAN_ID_LIMIT below OBJECT_KIND_END.
#define OBJECT_KIND_END 0x2020
// The skills an object record lists (a field object rolls them by slot).
#define OBJECT_SKILL_COUNT 8
#define FIELD_LAYER_NONE (-1)
// An object with no event to queue, and the bank and index of an object
// with no event flag.
#define FIELD_OBJECT_NO_EVENT (-1)
#define FIELD_OBJECT_NO_FLAG 0xff

// Which records FindObjectAt and CountObjectsAt accept: any, only `kind`, or
// any but `kind`.
GZ_ENUM_BEGIN_SPLIT(ObjectKindMatch, i16)
    OBJECT_MATCH_ANY = 0,
    OBJECT_MATCH_KIND = 1,
    OBJECT_MATCH_OTHER_KIND = 2
GZ_ENUM_END_SPLIT(ObjectKindMatch)

// CheckObjectState's result for a slot.
GZ_ENUM_BEGIN_SPLIT(ObjectSlotState, i16)
    OBJECT_SLOT_FREE = -1,
    OBJECT_SLOT_REMOVED = 0,
    OBJECT_SLOT_LIVE = 1
GZ_ENUM_END_SPLIT(ObjectSlotState)

// Whether an object is absent, active, or has a fatal condition.
GZ_ENUM_BEGIN_SPLIT(ObjectLifeState, i16)
    OBJECT_LIFE_ABSENT = 0,
    OBJECT_LIFE_ACTIVE = 1,
    OBJECT_LIFE_FALLEN = 2
GZ_ENUM_END_SPLIT(ObjectLifeState)

// Whether a field object's movement stops when it reaches the party's cell.
GZ_ENUM_BEGIN_SPLIT(ObjectPartyCellStop, i16)
    OBJECT_PARTY_CELL_CONTINUE = 0,
    OBJECT_PARTY_CELL_STOP = 1
GZ_ENUM_END_SPLIT(ObjectPartyCellStop)

typedef struct FieldObject {
    u8 pad000[0x14];
    i16 layer;
    i16 redraw;
    i16 anim;
    FieldActor actor;
    u8 flagBank;
    u8 flagIndex;
    // @identity-TODO: skill words 1..8 a field actor rolls from (index 0 overlaps
    // flagBank/flagIndex).
    i16 skills[OBJECT_SKILL_COUNT];
    ScriptBlock* script;
    i8 slot;
    i8 event;
} FieldObject;

static __inline void SetFieldObjectPickTarget(FieldObject* actor, i16 target) {
    actor->actor.core.pickTarget = target;
    actor->actor.core.pickTargetHigh = 0;
}

#define SetObjectDirection(object, facing, changed)                                                \
    do {                                                                                           \
        if ((facing) != (object)->actor.direction) {                                               \
            (changed) = true;                                                                      \
            (object)->actor.direction = (facing);                                                  \
        }                                                                                          \
    } while (0)

#define SetFieldObjectPickRole(object, role) ((object)->actor.core.pickRole = (role))

#define GetFieldObjectEquipment(object) ((object)->actor.core.slots)

#define GetFieldObjectHpPool(object) (&(object)->actor.core.pools.hp)

#define GetFieldObjectMpPool(object) (&(object)->actor.core.pools.mp)

#define IsFieldObjectActive(object)                                                                \
    ((object)->layer != FIELD_LAYER_NONE && (object)->actor.hidden == false)

static __inline ActionWait* GetFieldObjectActionWait(FieldObject* actor) {
    return &actor->actor.core.actionWait;
}

static __inline ConditionSet* GetFieldObjectConditions(FieldObject* object) {
    return &object->actor.core.conditions;
}

static __inline u8* GetFieldObjectFlags(FieldObject* object) {
    return object->actor.core.personalFlags;
}

static __inline b32 TestFieldObjectFlag(FieldObject* object, i16 index) {
    return TestBit(GetFieldObjectFlags(object), index);
}

b16 InitFieldObjects(void);
void RemoveFieldObject(i16 index, b16 announce);
b16 ResetFieldObjects(void);
b16 ExchangeObjectsFrozen(b16 frozen);
i16 FindObjectOnLayer(i16 layer);
void SetObjectEventFlag(i16 index, u8 bank, u8 flag);
i16 SpawnFieldObject(
    i16 layer,
    i16 x,
    i16 y,
    i16 direction,
    i16 kind,
    b16 alternate,
    i8 event,
    b16 fresh
);
i16 SpawnMapObject(i16 layer, i16 x, i16 y, i16 direction, i8 event);
i16 GetLiveObject(i16 index);
i16 RespawnFieldObject(i16 index, b16 alternate, i8 event, b16 fresh);
void ResetObjectAnims(void);
void ResetObjectAnim(i16 index);
FieldObject* GetFieldObject(i16 index);
FieldActor* GetFieldActor(i16 index);
b16 IsFieldActor(const void* actor);
void MarkObjectsOnMap(void);
MapCoord GetObjectCoord(i16 index);

#define GetFieldTargetCoord(id) ((id) < 0 ? GetMapCoord() : GetObjectCoord(id))
GZ_ENUM_RETURN(ViewDirection, i16) GetObjectDirection(i16 index);
i16 ExchangeObjectCheckBypass(i16 bypass);
i16 FlushObjectRedraws(void);
GZ_ENUM_RETURN(ObjectLifeState, i16) GetObjectLifeState(FieldObject* object);
i16 RelativeFacing(i16 from, i16 to);

b16 DrawFieldObject(FieldObject* object, u32 image, i16 index, i16 total, i16 drawn);
MapCoord GetApproachOffset(i16 scale, i16 step);

void DrawFieldObjects(void);
GZ_ENUM_RETURN(ObjectSlotState, i16) CheckObjectState(i16 index);
void RunFieldIdle(void);
i16 FindObjectAt(i16 x, i16 y, i16 start, GZ_ENUM_PARAM(ObjectKindMatch, i16) mode, i16 kind);
i16 CountObjectsAt(i16 x, i16 y, GZ_ENUM_PARAM(ObjectKindMatch, i16) mode, i16 kind);
i16 FindObjectAtParty(void);
void UpdateFieldObjects(void);
i16 ExchangeObjectRemovalDeferred(i16 deferred);
void ClearObjectStuns(i16* cleared);
void ApplyObjectConditions(i16* marked);
i16 GetObjectsFrozen(void);
GZ_ENUM_RETURN(ObjectLifeState, i16) GetObjectLifeStateAt(i16 index);
MapCoord* GetObjectCoordPtr(i16 index);
i16 GetObjectSlot(i16 index);
i16 GetObjectAnim(i16 index);
i16 GetObjectImageCode(i16 index);
i16 GetObjectFacingImageCode(i16 index);
i16 CountActiveObjects(void);
i16 AdvanceObjectAnims(void);
void CheckAllObjects(void);
i16 GetObjectLayer(i16 index);

// @identity-TODO: per-object steps of the idle loop.
void BuildSightGrid(i16 x, i16 y, i16 direction);

typedef struct FieldSkillCandidate {
    i16 index;
    i16 value;
} FieldSkillCandidate;

#define SetFieldSkillCandidate(candidate, selectedIndex, candidateValue)                           \
    do {                                                                                           \
        (candidate)->index = (selectedIndex);                                                      \
        (candidate)->value = (candidateValue);                                                     \
    } while (0)

#define InitFieldSkillCandidate(candidate) SetFieldSkillCandidate(candidate, -1, 0x7fff)

b16 RunObjectStep(FieldObject* object, i16 index);

void SaveFieldLayer(GZ_ENUM_PARAM(FieldLayerIndex, i16) layer);
void RestoreFieldLayer(GZ_ENUM_PARAM(FieldLayerIndex, i16) layer);
void ResetFieldLayer(GZ_ENUM_PARAM(FieldLayerIndex, i16) layer);
GZ_ENUM_RETURN(ObjectRecordId, i16) GetLayerKind(i16 layer);
u32 GetLayerImage(i16 layer);
i16 FindLayerOfKind(GZ_ENUM_PARAM(ObjectRecordId, i16) kind);

// Where `entry` of script file `file` starts in a field layer's scripts (file
// 0xff: the object script block). `layerSlot` is the layer index plus one;
// 0 gives a zero entry.
// @identity-TODO: label-only until the field-object TU claims it.
RVA_DECL(0x0000e910)
ScriptEntry FindLayerScriptEntry(i16 layerSlot, i16 file, i16 entry);

void LoadEncounterWeights(void);

#endif // GITEN_GAME_FIELDOBJECT_H
