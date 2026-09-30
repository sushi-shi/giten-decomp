#ifndef GITEN_GAME_DEMONTABLE_H
#define GITEN_GAME_DEMONTABLE_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/DemonClass.h>
#include <Game/DemonPantheon.h>
#include <Game/DemonRace.h>
#include <Game/HumanTitle.h>
#include <Ints.h>

// @identity-TODO: the game meaning of the two fusion value bits remains open.
GZ_ENUM_FLAGS_BEGIN(DemonTableFlags, u8)
    DEMON_FLAG_LOW_VALUE = 0x01,
    DEMON_FLAG_LOW_UNAVAILABLE = 0x04,
    DEMON_FLAG_HIGH_VALUE = 0x10,
    DEMON_FLAG_HIGH_UNAVAILABLE = 0x40
GZ_ENUM_FLAGS_END(DemonTableFlags)

// Each fusion flag reports whether its bit is unavailable, clear or set.
// The low and high bits have distinct game meanings still to recover.
GZ_ENUM_BEGIN_SPLIT(FusionFlagValue, i16)
    FUSION_FLAG_UNAVAILABLE = -1,
    FUSION_FLAG_CLEAR = 0,
    FUSION_FLAG_SET = 1
GZ_ENUM_END_SPLIT(FusionFlagValue)

// The demon (record) table loaded from data file 0 (kind 12): a count, then a
// 4-byte record per id (from id 32 on, the demons); plus five sections of
// strings and bytes indexed through those records.
typedef struct DemonTableEntry {
    GZ_ENUM_STORAGE(DemonRace, u8) race;
    GZ_ENUM_STORAGE(DemonPantheon, u8) pantheon;
    u8 level;
    GZ_ENUM_STORAGE(DemonTableFlags, u8) flags;
} DemonTableEntry;

typedef struct DemonTable {
    i16 count;
    DemonTableEntry entries[];
} DemonTable;

// Name blocks start with a count and byte offsets from the block start.
typedef struct DemonNameTable {
    u16 count;
    u16 offsets[];
} DemonNameTable;

void LoadDemonTables(void);
GZ_ENUM_RETURN(DemonRace, i16) GetDemonRace(i16 id);
GZ_ENUM_RETURN(DemonPantheon, i16) GetDemonPantheon(i16 id);
i16 GetDemonLevel(i16 id);
GZ_ENUM_RETURN(FusionFlagValue, i16) GetDemonFlagLow(i16 id);
GZ_ENUM_RETURN(FusionFlagValue, i16) GetDemonFlagHigh(i16 id);
i16 GetDemonCount(void);
GZ_ENUM_RETURN(DemonClass, i16) GetRaceClass(GZ_ENUM_PARAM(DemonRace, i16) race);
GZ_ENUM_RETURN(DemonClass, i16) GetDemonClass(i16 id);

// Names selected by the demon record; human titles are indexed directly.
char* GetDemonRaceName(i16 id);
char* GetDemonClassName(i16 id);
char* GetDemonPantheonName(i16 id);
char* GetHumanTitleName(GZ_ENUM_PARAM(HumanTitle, i16) index);
char* CopyObjectRecordName(i16 id, char* destination);

i16 FindStrongestOfRace(i16 maxLevel, GZ_ENUM_PARAM(DemonRace, i16) race);
i16 FindDemonOfRace(i16 maxLevel, GZ_ENUM_PARAM(DemonRace, i16) race);
i16 FindStrongestOfClass(i16 maxLevel, GZ_ENUM_PARAM(DemonClass, i16) cls);
i16 FindNextOfRace(i16 id, i16 wrap);
i16 ScaleLevelGap(i16 a, i16 b);

#endif // GITEN_GAME_DEMONTABLE_H
