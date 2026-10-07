#ifndef GITEN_GAME_OBJECTRECORD_H
#define GITEN_GAME_OBJECTRECORD_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/FieldObject.h>
#include <Game/GameState.h>
#include <Game/ObjectRecordId.h>
#include <Ints.h>

#include <stdio.h>

typedef struct ObjectRecordFlags {
    // @identity-TODO: two serialized bits copied to FieldObject.byte083.
    u8 unknownObjectBits : 2;
    u8 triggerRange : 3;
    u8 pickFlagA : 1;
    u8 pickFlagB : 1;
} ObjectRecordFlags;

// A field object's 0x7a-byte record (data file kind 10, id 0x2000 + kind),
// read into the shared buffer 0x47afd8 and turned into a FieldObject by
// InitObjectFromRecord.
// @identity-TODO: fields are named from where the loader copies them
// (experience/macca/magnetite, the HP/MP pools, the eight skills and item
// slots, the id and the 17-byte name, the level, the stat bytes, ...);
// `alignB`/`alignA` are the pairs ScaleLevelGap turns into the object's
// alignment levels; flags contains the fields copied to byte083, triggerRange
// and pickFlags. The picture uses
// imageIndex/imageVariant; scriptSet selects three extra script files.
typedef struct ObjectRecord {
    i32 experience;
    i32 macca;
    i32 magnetite;
    i16 hp;
    i16 mp;
    i16 skills[OBJECT_SKILL_COUNT];
    i16 equipGroup;
    i16 items[8];
    i16 pickItem;
    GZ_ENUM_STORAGE(ObjectRecordId, i16) id;
    char name[17];
    u8 level;
    u8 alignB[2];
    u8 alignA[2];
    u8 stats[11];
    u8 levelBonus;
    i8 moonRow;
    u8 resistance[10];
    i8 affiliation[AFFILIATION_COUNT];
    u8 actionSpeed;
    u8 dropChance;
    ObjectRecordFlags flags;
    u8 pad69;
    u8 encounterRow;
    i16 imageIndex;
    u8 imageVariant;
    u8 pad6e[5];
    i16 scriptSet;
    u8 pad75[5];
} ObjectRecord;

typedef struct ObjectPicture {
    i16 index;
    i16 variant;
} ObjectPicture;

void LoadObjectRecord(i16 kind, FieldObject* object);
void LoadLayerRecord(i16 kind, ObjectRecord* out);
void InitObjectFromRecord(FieldObject* object, const ObjectRecord* record);
void CopyLeaderIntoObject(FieldObject* object);
void ReadObjectRecord(i16 kind);
i16 GetObjectRecordScriptSet(i16 kind);
char* GetObjectRecordName(i16 kind);
ObjectPicture GetObjectRecordPicture(i16 kind);
void ReadRecordIfWanted(FILE* fp, void* out);
i32 ReadObjectRecordField(i16 kind, i16 offset, i16 size);

#endif // GITEN_GAME_OBJECTRECORD_H
