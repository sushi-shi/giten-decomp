#ifndef GITEN_GAME_FUSION_H
#define GITEN_GAME_FUSION_H

#include <rva.h>

#include <Game/Character.h>
#include <Ints.h>
#include <Mem/Handle.h>

typedef union FusionSummary {
    i16 value;
    struct {
        i16 overLevel : 1;
        i16 highFlag : 3;
        i16 lowFlag : 3;
        i16 kind : 9;
    } fields;
} FusionSummary;

static __inline void
SetFusionSummaryKind(FusionSummary* summary, i16 kind, i16 resultLevel, i16 sourceLevel) {
    if (kind) {
        summary->fields.kind = kind;
    } else if (resultLevel < sourceLevel) {
        summary->fields.kind = 11;
    } else if (resultLevel > sourceLevel) {
        summary->fields.kind = 10;
    } else {
        summary->fields.kind = 12;
    }
}

void InheritFusionStats(Character* first, Character* second, Character* result);
void InheritFusionStat(Character* source, Character* result, i16 stat);

i16 ResolveThreeSpecialRaceFusion(i16 first, i16 second, i16 third);
i16 ResolveSpecialRaceTripleFusion(i16 first, i16 second, i16 third);
i16 ResolveSameClassTripleFusion(i16 first, i16 second, i16 third);
i16 ResolveGeneralTripleFusion(i16 first, i16 second, i16 third);
i16 CalculateTripleFusion(i16 first, i16 second, i16 third);
i16 ResolveTwoSpecialRaceFusion(i16 first, i16 second, i16 third);
i16 ResolveOneSpecialRaceFusion(i16 first, i16 second, i16 third);
i16 ResolveThreeUnrankedFusion(i16 first, i16 second, i16 third);
i16 ResolveTwoUnrankedFusion(i16 first, i16 second, i16 third);
i16 ResolveUnrankedTripleFusion(i16 first, i16 second, i16 third);
i16 CountSpecialRaceFusionSlots(i16 first, i16 second, i16 third);
i16 CountUnrankedFusionSlots(i16 first, i16 second, i16 third);
i16 CountFusionLowFlagOne(i16 first, i16 second, i16 third);
i16 CountFusionHighFlagOne(i16 first, i16 second, i16 third);
i16 GetTripleFusionLevelBonus(i16 first, i16 second, i16 third);
i16 GetTripleFusionLevel(i16 first, i16 second, i16 third);

FusionSummary GetPairFusionSummary(i16 first, i16 second);
FusionSummary GetTripleFusionSummary(i16 first, i16 second, i16 third);

typedef struct FusionFlagRestriction {
    i16 demon;
    u16 condition;
} FusionFlagRestriction;

i16 IsFusionDemonRestricted(i16 demon);

typedef struct FusionDemonPair {
    i16 first;
    i16 second;
} FusionDemonPair;

i16 GetFusionPairSide(i16 slot);
i16 MatchFusionPair(i16 first, i16 second);
i16 FindSecondaryFusionComplement(i16 first, i16 second, i16 third);
i16 FindPrimaryFusionComplement(i16 first, i16 second, i16 third);

typedef struct FusionByteMatrix {
    i16 count;
    i8 entries[1];
} FusionByteMatrix;

#define ReturnFusionBytePair(matrixHandle, firstValue, secondValue)                                \
    do {                                                                                           \
        i16 firstIndex;                                                                            \
        i16 secondIndex;                                                                           \
        FusionByteMatrix* table;                                                                   \
        i16 count;                                                                                 \
        i16 index;                                                                                 \
        firstIndex = -1;                                                                           \
        secondIndex = -1;                                                                          \
        table = HandleReadPtr((matrixHandle));                                                     \
        count = table->count;                                                                      \
        for (index = 0; index < count; index++) {                                                  \
            if (table->entries[index] == (firstValue)) {                                           \
                firstIndex = index;                                                                \
            }                                                                                      \
            if (table->entries[index] == (secondValue)) {                                          \
                secondIndex = index;                                                               \
            }                                                                                      \
        }                                                                                          \
        if (firstIndex >= 0 && secondIndex >= 0) {                                                 \
            if (firstIndex < secondIndex) {                                                        \
                firstIndex = firstIndex * count + secondIndex + table->count;                      \
                return table->entries[firstIndex];                                                 \
            }                                                                                      \
            firstIndex = secondIndex * count + firstIndex + table->count;                          \
            return table->entries[firstIndex];                                                     \
        }                                                                                          \
        return 0;                                                                                  \
    } while (0)

typedef struct FusionRaceRow {
    i8 values[27];
} FusionRaceRow;

typedef struct FusionRaceRows {
    i16 count;
    FusionRaceRow rows[1];
} FusionRaceRows;

i16 GetFusionClassPair(i16 first, i16 second);
i16 GetFusionRaceEntry(i16 index, i16 slot);

typedef struct FusionRaceChange {
    i16 source;
    i16 target;
} FusionRaceChange;

i16 GetFusionSpecialRaceChange(i16 demon, i16 race);
i16 ResolveSpecialRaceFusion(i16 first, i16 second);
i16 GetSameRaceFusionRace(i16 slot);
i16 ResolveSameRaceFusion(i16 first, i16 second);
void LoadFusionTables(void);
void FreeFusionTables(void);
void MarkRandomFusion(void);
i16 SelectRandomFusionDemon(void);
i16 ResolveRandomFusion(void);
i16 ResolvePairFusion(i16 first, i16 second, i16 rankChanges);
i16 CalculatePairFusion(i16 first, i16 second);
i16 ResolveMixedRankFusion(i16 first, i16 second);
i16 ResolveFusionRankPair(i16 first, i16 second);
i16 ResolveSameClassFusion(i16 first, i16 second);

// Count labels followed by a square matrix with the same row width.
typedef struct FusionWordMatrix {
    i16 count;
    i16 entries[1];
} FusionWordMatrix;

#define ReturnFusionWordPair(handle, first, second)                                                \
    do {                                                                                           \
        i16 firstIndex = -1;                                                                       \
        i16 secondIndex = -1;                                                                      \
        FusionWordMatrix* table = HandleReadPtr(handle);                                           \
        i16 count = table->count;                                                                  \
        i16 index;                                                                                 \
        i16 temporary;                                                                             \
        for (index = 0; index < count; index++) {                                                  \
            if (table->entries[index] == (first)) {                                                \
                firstIndex = index;                                                                \
            }                                                                                      \
            if (table->entries[index] == (second)) {                                               \
                secondIndex = index;                                                               \
            }                                                                                      \
        }                                                                                          \
        if (firstIndex < 0 || secondIndex < 0) {                                                   \
            return 0;                                                                              \
        }                                                                                          \
        if (firstIndex < secondIndex) {                                                            \
            temporary = firstIndex;                                                                \
            firstIndex = secondIndex;                                                              \
            secondIndex = temporary;                                                               \
        }                                                                                          \
        secondIndex = (secondIndex + 1) * count + firstIndex;                                      \
        return table->entries[secondIndex];                                                        \
    } while (0)

i16 GetFusionRacePair(i16 first, i16 second);
i16 GetFusionDemonPair(i16 first, i16 second);
i16 GetFusionGrowthBonus(i16 first, i16 second);
i16 ApplyFusionRankChange(i16 demon, i16 kind);
i16 ResolveFusionDemonPair(i16 first, i16 second);
i16 ResolveFusionRacePair(i16 first, i16 second);
i16 GetFusionSpecialRace(i16 slot);
i16 GetFusionRestrictedClass(i16 demon);
i16 GetRosterFusionRestrictedClass(i16 slot);
i16 CheckFusionRestrictedPair(i16 first, i16 second);
i16 SetFusionResult(i16 demon, i16 kind);
void GetFusionResult(i16* demon, i16* kind);
i16 ResolveFusionFallbackPair(i16 first, i16 second);
i16 GetFusionLevel(i16 first, i16 second);
i16 FindFusionFallback(i16 first, i16 second);
i16 GetFusionFallbackEntry(i16 index);
i16 FindFusionFallbackIndex(i16 demon);

#endif // GITEN_GAME_FUSION_H
