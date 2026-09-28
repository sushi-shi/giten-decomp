#ifndef GITEN_GAME_FIELDOBJECT_H
#define GITEN_GAME_FIELDOBJECT_H

#include <rva.h>

#include <Enums.h>
#include <Game/Character.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Ints.h>

// Negative image codes request horizontal mirroring. Nonnegative codes
// can include the lit flag for the animated specular-lighting path.
// clang-format off
GZ_ENUM_BEGIN(FieldObjectImageCode)
    FIELD_OBJECT_IMAGE_INDEX_MASK = 0x0f,
    FIELD_OBJECT_IMAGE_LIT = 0x10
GZ_ENUM_END(FieldObjectImageCode);
// clang-format on

#define IsFieldObjectImageLit(code) ((code) >= FIELD_OBJECT_IMAGE_LIT)

// One of the sixteen actors placed on the field map. Layer -1 is a free slot;
// hidden actors are removed on the next check.
// @identity-TODO: the shared character prefix has not been factored into its
// own type. CopyCharacterCore copies from kind through moonRow; pos begins
// the distinct field tail. A full Character would overlap the script pointer.
// rank corresponds to Character.level and list to Character.skills. Slots
// 6/7 are the gun/ammunition pair; the ammunition extra field holds the
// magazine size.
typedef struct FieldObject {
    u8 pad000[0x14];
    i16 layer;
    i16 redraw;
    i16 anim;
    i16 kind;
    char namePrefix[17];
    u8 pad02d[0x25];
    u8 resistance[10];
    i8 affiliation[3];
    i16 equipGroup;
    GZ_ENUM_STORAGE(PickFlags, u8) pickFlags;
    u32 trainingPoints[4];
    // @identity-TODO: three bytes between training points and drop chance.
    u8 unknownAfterTraining[3];
    u8 dropChance;
    i16 pickItem;
    i16 actionSpeed;
    u16 conditionActionTicks;
    u8 encounterRow;
    i16 shield;
    u8 pad07f[3];
    u8 pantheon;
    u8 byte083;
    u8 memberClass;
    u8 rank;
    u8 title;
    u8 triggerRange;
    u32 experience;
    i32 macca;
    i32 magnetite;
    i8 alignmentLevelB;
    i8 alignmentLevelA;
    u8 byte096;
    u8 acting;
    i16 word098;
    CharacterPools pools;
    StatBlock stats;
    i16 battleStats[24];
    i16 battleStatsShown[24];
    ConditionSet conditions;
    ActionWait actionWait;
    i8 pickRole;
    i16 pickTarget : 15;
    i16 pickTargetHigh : 1;
    i16 pickObject : 14;
    i16 pickCostPaid : 1;
    i16 pickNoEffect : 1;
    i8 result : 7;
    u8 resultFlag : 1;
    i32 lastChange;
    i32 selfChange;
    // @identity-TODO: fifteen bytes the record loader clears as one block.
    u8 battleTally[15];
    u16 levelBonus;
    u16 hundredths;
    ItemSlot slots[8];
    u8 levelGap;
    u8 familiarity;
    u8 attitude;
    u8 fieldState;
    u8 mode;
    u8 pad1e1;
    u8 personalFlags[32];
    u32 clearedOnLoad[2];
    u8 hpRollBonus;
    WordList list;
    u8 pad211;
    i8 moonRow;
    MapCoord pos;
    i16 direction;
    u8 pad219;
    u8 byte21a;
    u8 byte21b;
    u8 pad21c;
    i16 word21d;
    i16 word21f;
    i16 word221;
    i16 hidden;
    u8 flagBank;
    u8 flagIndex;
    // @identity-TODO: skill words 1..8 a field actor rolls from (index 0 overlaps
    // flagBank/flagIndex).
    i16 skills[8];
    ScriptBlock* script;
    i8 slot;
    i8 event;
} FieldObject;

#define SetObjectDirection(object, facing, changed)                                                \
    do {                                                                                           \
        if ((facing) != (object)->direction) {                                                     \
            (changed) = 1;                                                                         \
            (object)->direction = (facing);                                                        \
        }                                                                                          \
    } while (0)

#define GetFieldObjectEquipment(object) ((object)->slots)

static __inline ActionWait* GetFieldObjectActionWait(FieldObject* actor) {
    return &actor->actionWait;
}

static __inline ConditionSet* GetFieldObjectConditions(FieldObject* object) {
    return &object->conditions;
}

static __inline u8* GetFieldObjectFlags(FieldObject* object) {
    return object->personalFlags;
}

static __inline b32 TestFieldObjectFlag(FieldObject* object, i16 index) {
    return TestBit(GetFieldObjectFlags(object), index);
}

b16 InitFieldObjects(void);
void RemoveFieldObject(i16 index, i16 announce);
b16 ResetFieldObjects(void);
i16 ExchangeObjectsFrozen(i16 frozen);
i16 FindObjectOnLayer(i16 layer);
void SetObjectEventFlag(i16 index, u8 bank, u8 flag);
i16 SpawnFieldObject(
    i16 layer,
    i16 x,
    i16 y,
    i16 direction,
    i16 kind,
    i16 alternate,
    i8 event,
    i16 fresh
);
i16 SpawnMapObject(i16 layer, i16 x, i16 y, i16 direction, i8 event);
i16 GetLiveObject(i16 index);
i16 RespawnFieldObject(i16 index, i16 alternate, i8 event, i16 fresh);
void ResetObjectAnims(void);
void ResetObjectAnim(i16 index);
FieldObject* GetFieldObject(i16 index);
Character* GetFieldActor(i16 index);
b16 IsFieldActor(const void* actor);
void MarkObjectsOnMap(void);
MapCoord GetObjectCoord(i16 index);

#define GetFieldTargetCoord(id) ((id) < 0 ? GetMapCoord() : GetObjectCoord(id))
i16 GetObjectDirection(i16 index);
i16 ExchangeObjectCheckBypass(i16 bypass);
i16 FlushObjectRedraws(void);
i16 GetObjectLifeState(FieldObject* object);
i16 RelativeFacing(i16 from, i16 to);

b16 DrawFieldObject(FieldObject* object, u32 image, i16 index, i16 total, i16 drawn);
MapCoord GetApproachOffset(i16 scale, i16 step);

void DrawFieldObjects(void);
i16 CheckObjectState(i16 index);
void RunFieldIdle(void);
i16 FindObjectAt(i16 x, i16 y, i16 start, i16 mode, i16 kind);
i16 CountObjectsAt(i16 x, i16 y, i16 mode, i16 kind);
i16 FindObjectAtParty(void);
void UpdateFieldObjects(void);
i16 ExchangeObjectRemovalDeferred(i16 deferred);
void ClearObjectStuns(i16* cleared);
void ApplyObjectConditions(i16* marked);
i16 GetObjectsFrozen(void);
i16 GetObjectLifeStateAt(i16 index);
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

void SaveFieldLayer(i16 layer);
void RestoreFieldLayer(i16 layer);
void ResetFieldLayer(i16 layer);
i16 GetLayerKind(i16 layer);
u32 GetLayerImage(i16 layer);
i16 FindLayerOfKind(i16 kind);

// Where `entry` of script file `file` starts in a field layer's scripts (file
// 0xff: the object script block). `layerSlot` is the layer index plus one;
// 0 gives a zero entry.
// @identity-TODO: label-only until the field-object TU claims it.
RVA_DECL(0x0000e910)
ScriptEntry FindLayerScriptEntry(i16 layerSlot, i16 file, i16 entry);

void LoadEncounterWeights(void);

#endif // GITEN_GAME_FIELDOBJECT_H
