// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Character.h>
#include <Game/DemonTable.h>
#include <Game/FieldObject.h>
#include <Game/ItemRecord.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/Stats.h>
#include <Game/StatUpdate.h>
#include <Mem/Handle.h>
#include <Util/Range.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <string.h>

// Record and race-class tables, followed by the four name tables.
DATA(0x0007b0d0)
static i32 s_demonRecords;

DATA(0x0007b0d4)
static i32 s_raceClasses;

DATA(0x0007b0d8)
static i32 s_raceNames;

DATA(0x0007b0dc)
static i32 s_pantheonNames;

DATA(0x0007b0e0)
static i32 s_humanTitles;

DATA(0x0007b0e4)
static i32 s_classNames;

// The object record buffer every record read goes through.
DATA(0x0007afd8)
static ObjectRecord s_record;

static __inline const DemonTable* ReadDemonTable(void) {
    return HandleReadPtr(s_demonRecords);
}

RVA(0x0000ff30, 0x70)
void LoadDemonTables(void) {
    FILE* fp = OpenDataFile(0, 12, 0);
    s_demonRecords = ReadCryptHandle(fp);
    s_raceClasses = ReadCryptHandle(fp);
    s_raceNames = ReadCryptHandle(fp);
    s_pantheonNames = ReadCryptHandle(fp);
    s_humanTitles = ReadCryptHandle(fp);
    s_classNames = ReadCryptHandle(fp);
    CloseDataFile(fp);
}

RVA(0x0000ffa0, 0x1a)
i16 GetDemonRace(i16 id) {
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
i16 GetRaceClass(i16 race) {
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
i16 FindStrongestOfRace(i16 maxLevel, i16 race) {
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
i16 FindDemonOfRace(i16 maxLevel, i16 race) {
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
    i16 wait;
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
    wait = 0xff - RandomAverage(0, 100, 0);
    GetFieldObjectActionWait(object)->remaining = wait;
    memset(object->battleTally, 0, sizeof(object->battleTally));
    SetItemSlotItem(&GetFieldObjectEquipment(object)[0], record->items[0]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[1], record->items[1]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[2], record->items[2]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[3], record->items[3]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[4], record->items[4]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[5], record->items[5]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[6], record->items[6]);
    SetItemSlotItem(&GetFieldObjectEquipment(object)[7], record->items[7]);
    if (GetFieldObjectEquipment(object)[6].item < 1) {
        GetFieldObjectEquipment(object)[7].item = -1;
        SetItemSlotItem(&GetFieldObjectEquipment(object)[6], -1);
        GetFieldObjectEquipment(object)[7].attachment = -1;
        GetFieldObjectEquipment(object)[6].quantity = 0;
        GetFieldObjectEquipment(object)[7].quantity = 0;
    } else if (GetFieldObjectEquipment(object)[7].item < 1) {
        EmptyItemSlot(&GetFieldObjectEquipment(object)[7]);
        GetFieldObjectEquipment(object)[7].attachment = -1;
    } else {
        GetFieldObjectEquipment(object)[7].quantity =
            GetGunMagazineSize(GetLoadedRecord(GetFieldObjectEquipment(object)[6].item));
    }
    NormalizeEquipSlots((Character*)&object->kind);
    memset(GetFieldObjectConditions(object)->bits, 0, sizeof(object->conditions.bits));
    memset(GetFieldObjectFlags(object), 0, sizeof(object->personalFlags));
    object->byte096 = 1;
    object->acting = 0;
    object->word098 = 0x11;
    object->attitude = 4;
    object->triggerRange = (record->bits68 >> 2) & 7;
    object->mode = 0;
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
    object->memberClass = object->byte083 = leader->byte069;
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
    FILE* fp = OpenDataFile(kind + 0x2000, 10, 0);
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
    FILE* fp = OpenDataFile(kind + 0x2000, 10, 0);
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
