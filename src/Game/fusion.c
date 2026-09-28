// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// fusion calculation span until link-order evidence names its owner.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Clock.h>
#include <Game/DemonTable.h>
#include <Game/Fusion.h>
#include <Game/FusionCompare.h>
#include <Game/FusionScreen.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Game/Stats.h>
#include <Mem/Handle.h>
#include <Script/EventFlags.h>
#include <Util/Level.h>

#include <stdlib.h>
#include <string.h>

DATA(0x00064670)
static const i16 s_fusionFlagLevelBonuses[4] = {0, 5, 7, 10};

DATA(0x00080a38)
static b16 s_randomFusion;
DATA(0x00080a3c)
static i16 s_fusionLevelAllowance;

DATA(0x00080a40)
static i16 s_fusionResultKind;

// @identity-TODO: a two-byte configuration record loaded but never read.
DATA(0x00080a60)
static u16 s_fusionConfigValue;

// @identity-TODO: the particular race/class selected by the loaded fusion
// configuration remains unnamed.
DATA(0x00080a64)
static i32 s_fusionSpecialRace;
DATA(0x00080a6c)
static i32 s_fusionRestrictedClass;

DATA(0x00080a50)
static i32 s_fusionRaceMatrix;
DATA(0x00080a54)
static i32 s_fusionSameRaceChanges;
DATA(0x00080a58)
static i32 s_fusionDemonMatrix;

DATA(0x00080a5c)
static i32 s_fusionSpecialRaceChanges;

DATA(0x00080a70)
static i32 s_fusionClassMatrix;
DATA(0x00080a74)
static i32 s_fusionRaceRows;

DATA(0x00080a78)
static i32 s_fusionPairs;
DATA(0x00080a7c)
static i32 s_fusionPrimaryComplements;

DATA(0x00080a80)
static i32 s_fusionFlagRestrictions;

DATA(0x00080a68)
static i32 s_fusionFallbackHandle;

RVA(0x00026550, 0x27)
i16 SetFusionResult(i16 demon, i16 kind) {
    g_fusionResult = demon;
    if (demon == -1) {
        kind = 0;
    }
    s_fusionResultKind = kind;
    return g_fusionResult;
}

RVA(0x00026580, 0xfa)
i16 CalculatePairFusion(i16 first, i16 second) {
    i16 result;
    if (!GetRosterCharacter(first)) {
        return 0;
    }
    if (!GetRosterCharacter(second)) {
        return 0;
    }
    result = CheckFusionRestrictedPair(first, second);
    if (result) {
        return result;
    }
    result = ResolveFusionDemonPair(first, second);
    if (!result) {
        result = ResolveSpecialRaceFusion(first, second);
        if (!result) {
            result = ResolveSameRaceFusion(first, second);
            if (!result) {
                result = ResolveFusionFallbackPair(first, second);
                if (!result) {
                    result = ResolveFusionRankPair(first, second);
                    if (!result) {
                        result = ResolveMixedRankFusion(first, second);
                        if (!result) {
                            result = ResolveSameClassFusion(first, second);
                            if (!result) {
                                result = ResolveFusionRacePair(first, second);
                            }
                        }
                    }
                }
            }
        }
    }
    if (g_fusionResult >= 32 && IsFusionDemonRestricted(g_fusionResult)) {
        return SetFusionResult(-1, 0);
    }
    return result;
}

RVA(0x00026680, 0x53)
i16 CheckFusionRestrictedPair(i16 first, i16 second) {
    i16 firstClass = GetRosterFusionRestrictedClass(first);
    i16 secondClass = GetRosterFusionRestrictedClass(second);
    if (firstClass && secondClass) {
        return SetFusionResult(-1, 0);
    }
    if (firstClass || secondClass) {
        SetFusionResult(0, 0);
        return 1;
    }
    return 0;
}

RVA(0x000266e0, 0x81)
i16 ResolveFusionDemonPair(i16 first, i16 second) {
    i16 result = GetFusionDemonPair(first, second);
    i16 demon;
    i16 level;
    if (!result) {
        return 0;
    }
    if (result >= 1000) {
        result -= 1000;
        return SetFusionResult(result, 8);
    }
    level = GetFusionLevel(first, second);
    demon = FindStrongestOfRace(level, result);
    if (demon >= 1) {
        return SetFusionResult(demon, 0);
    }
    demon = FindFusionFallback(first, second);
    return SetFusionResult(demon, 9);
}

RVA(0x00026770, 0x4b)
i16 GetFusionLevel(i16 first, i16 second) {
    i16 level = GetFusionSlotLevel(first);
    u8 result;
    level += GetFusionSlotLevel(second);
    result = ClampLevel(level / 2 + 5);
    return result;
}

RVA(0x000267c0, 0x72)
i16 FindFusionFallback(i16 first, i16 second) {
    i16 result = -1;
    i16 bestLevel = -1;
    i16 limit = GetFusionLevel(first, second);
    i16 index;
    i16 demon;
    i16 level;
    limit -= 5;
    limit /= 4;
    if (limit < 2) {
        limit = 2;
    }
    for (index = 0; index < 7; index++) {
        demon = GetFusionFallbackEntry(index);
        level = GetDemonLevel(demon);
        if (level <= limit && bestLevel < level) {
            bestLevel = level;
            result = demon;
        }
    }
    return result;
}

RVA(0x00026840, 0xc1)
i16 ResolveSpecialRaceFusion(i16 first, i16 second) {
    i16 race = -1;
    i16 demon = -1;
    i16 side = -1;
    i16 result;
    if (GetFusionSpecialRace(first)) {
        side = 0;
        demon = GetRosterId(first);
        race = GetFusionSlotRace(second);
    } else if (GetFusionSpecialRace(second)) {
        side = 1;
        demon = GetRosterId(second);
        race = GetFusionSlotRace(first);
    }
    if (side < 0) {
        return 0;
    }
    demon = GetFusionSpecialRaceChange(demon, race);
    if (!demon) {
        return 0;
    }
    if (side == 0) {
        result = GetRosterId(second);
    } else {
        result = GetRosterId(first);
    }
    result = ApplyFusionRankChange(result, demon);
    return SetFusionResult(result, demon);
}

RVA(0x00026910, 0x24)
i16 ApplyFusionRankChange(i16 demon, i16 kind) {
    switch (kind) {
        case 3:
        case 4:
            demon = FindNextOfRace(demon, 1);
            break;
    }
    return demon;
}

RVA(0x00026940, 0xbb)
i16 ResolveSameRaceFusion(i16 first, i16 second) {
    i16 race = GetFusionSlotRace(first);
    i16 otherRace = GetFusionSlotRace(second);
    i16 level;
    i16 demon;
    if (race != otherRace) {
        return 0;
    }
    if (GetDemonFlagLow(GetRosterId(first)) != -1) {
        demon = GetFusionRacePair(first, second);
        return SetFusionResult(demon, 8);
    }
    race = GetSameRaceFusionRace(first);
    level = GetFusionLevel(first, second);
    demon = FindStrongestOfRace(level, race);
    if (demon >= 1) {
        return SetFusionResult(demon, 0);
    }
    demon = FindFusionFallback(first, second);
    return SetFusionResult(demon, 9);
}

RVA(0x00026a00, 0x5b)
i16 ResolveFusionFallbackPair(i16 first, i16 second) {
    if (FindFusionFallbackIndex(GetRosterId(first)) >= 0) {
        return SetFusionResult(GetRosterId(second), 5);
    }
    if (FindFusionFallbackIndex(GetRosterId(second)) >= 0) {
        return SetFusionResult(GetRosterId(first), 7);
    }
    return 0;
}

RVA(0x00026a60, 0xb1)
i16 ResolveFusionRankPair(i16 first, i16 second) {
    Character* members[2];
    i16 selected;
    if (GetDemonFlagLow(GetRosterId(first)) != -1) {
        return 0;
    }
    if (GetDemonFlagLow(GetRosterId(second)) != -1) {
        return 0;
    }
    members[0] = GetRosterCharacter(first);
    members[1] = GetRosterCharacter(second);
    selected = CompareFusionCharacters(members[0], members[1]);
    if (selected != -1) {
        return SetFusionResult(FindNextOfRace(members[selected]->id, 0), 0);
    }
    return SetFusionResult(-1, 0);
}

RVA(0x00026b20, 0x17f)
i16 ResolveMixedRankFusion(i16 first, i16 second) {
    Character* members[2];
    Character* ranked;
    Character* other;
    i16 side = -1;
    i16 level;
    i16 demon;
    if (GetDemonFlagLow(GetRosterId(first)) != -1) {
        side = 0;
    }
    if (GetDemonFlagLow(GetRosterId(second)) != -1) {
        if (side != -1) {
            return 0;
        }
        side = 1;
    } else if (side != 0) {
        return 0;
    }
    members[0] = GetRosterCharacter(first);
    members[1] = GetRosterCharacter(second);
    level = members[0]->level + members[1]->level;
    other = members[side ^ 1];
    ranked = members[side];
    if (ranked->level >= other->level) {
        if (level % 7 == 0) {
            demon = FindNextOfRace(ranked->id, 0);
            return SetFusionResult(demon, 0);
        }
        if (level % 5 == 0) {
            demon = FindNextOfRace(ranked->id, 0);
            return SetFusionResult(demon, 3);
        }
        if (level % 3 == 0) {
            return SetFusionResult(ranked->id, 0);
        }
    }
    if (!(level & 1)) {
        demon = FindNextOfRace(other->id, 0);
        return SetFusionResult(demon, 0);
    }
    demon = FindFusionFallback(first, second);
    return SetFusionResult(demon, 9);
}

RVA(0x00026ca0, 0x77)
i16 ResolveSameClassFusion(i16 first, i16 second) {
    i16 cls;
    i16 level;
    i16 demon;
    if (CompareRosterFusionClasses(first, second)) {
        return 0;
    }
    cls = GetFusionSlotClass(first);
    level = GetFusionLevel(first, second);
    demon = FindStrongestOfClass(level, cls);
    if (demon >= 1) {
        return SetFusionResult(demon, 0);
    }
    demon = FindFusionFallback(first, second);
    return SetFusionResult(demon, 9);
}

RVA(0x00026d20, 0x7c)
i16 ResolveFusionRacePair(i16 first, i16 second) {
    i16 race = GetFusionRacePair(first, second);
    i16 demon;
    i16 level;
    if (race < 1) {
        if (race == -1) {
            return SetFusionResult(-1, 0);
        }
        return 0;
    }
    level = GetFusionLevel(first, second);
    demon = FindStrongestOfRace(level, race);
    if (demon >= 1) {
        return SetFusionResult(demon, 0);
    }
    demon = FindFusionFallback(first, second);
    return SetFusionResult(demon, 9);
}

RVA(0x00026da0, 0x6a)
i16 GetFusionGrowthBonus(i16 first, i16 second) {
    i16 growth = GetRosterCharacter(first)->level;
    u8 result;
    growth += GetRosterCharacter(second)->level;
    growth -= GetFusionSlotLevel(first);
    growth -= GetFusionSlotLevel(second);
    result = ClampLevel(growth / 2);
    return result;
}

static __inline void InitFusionSummary(FusionSummary* summary) {
    memset(summary, 0, sizeof(*summary));
}

RVA(0x00026e10, 0x119)
FusionSummary GetPairFusionSummary(i16 first, i16 second) {
    i16 sourceLevel = GetRosterCharacter(first)->level;
    FusionSummary summary;
    FusionSummary* result = &summary;
    i16 resultLevel;
    u8 clampedLevel;
    InitFusionSummary(&summary);
    if (!g_fusionResult) {
        return summary;
    }
    if (g_fusionResult == -1) {
        summary.fields.kind = -1;
        return summary;
    }
    resultLevel = GetDemonLevel(g_fusionResult);
    resultLevel += GetFusionGrowthBonus(first, second);
    clampedLevel = ClampLevel(resultLevel);
    resultLevel = clampedLevel;
    result->fields.highFlag = GetDemonFlagHigh(g_fusionResult);
    result->fields.lowFlag = GetDemonFlagLow(g_fusionResult);
    SetFusionSummaryKind(&summary, s_fusionResultKind, resultLevel, sourceLevel);
    if (GetRosterCharacter(0)->level + 3 <= resultLevel) {
        summary.fields.overLevel = 1;
    }
    return summary;
}

RVA(0x00026f30, 0x59)
i16 ResolveRandomFusion(void) {
    i16 phaseDistance = abs(14 - g_clock.moonPhase) + 1;
    i16 limit = 10 / phaseDistance;
    i16 demon;
    limit += GetRosterCharacter(0)->level;
    do {
        demon = SelectRandomFusionDemon();
    } while (limit < GetDemonLevel(demon));
    return SetFusionResult(demon, 0);
}

RVA(0x00026f90, 0x7c)
i16 SelectRandomFusionDemon(void) {
    i16 count = GetDemonCount();
    i32 sum;
    i16 index;
    i16 demon;
    for (;;) {
        sum = 0;
        for (index = 0; index < 3; index++) {
            sum += rand();
        }
        demon = (sum / 3) * (count - 32) / 32768 + 32;
        if (GetFusionRestrictedClass(demon)) {
            continue;
        }
        if (GetDemonClass(demon) == 14) {
            continue;
        }
        if (IsFusionDemonRestricted(demon)) {
            continue;
        }
        break;
    }
    MarkRandomFusion();
    return demon;
}

RVA(0x00027010, 0x73)
i16 ResolvePairFusion(i16 first, i16 second, i16 rankChanges) {
    Character* firstEntry = GetRosterEntry(first);
    Character* secondEntry = GetRosterEntry(second);
    i16 result;
    i16 index;
    if (!firstEntry || !secondEntry) {
        return -1;
    }
    result = CalculatePairFusion(first, second);
    if (result < 0) {
        return -1;
    }
    if (result == 1) {
        result = ResolveRandomFusion();
    }
    if (result > 0) {
        for (index = 0; index < rankChanges; index++) {
            result = FindNextOfRace(result, 0);
        }
    }
    g_fusionResult = result;
    return result;
}

RVA(0x00027090, 0x1c)
void GetFusionResult(i16* demon, i16* kind) {
    *demon = g_fusionResult;
    *kind = s_fusionResultKind;
}

RVA(0x000270b0, 0x72)
void InheritFusionStats(Character* first, Character* second, Character* result) {
    i16 stat;
    for (stat = 0; stat <= 10; stat++) {
        if (first->stats.base[stat] < second->stats.base[stat]) {
            InheritFusionStat(first, result, stat);
            InheritFusionStat(second, result, stat);
        } else {
            InheritFusionStat(second, result, stat);
            InheritFusionStat(first, result, stat);
        }
    }
}

RVA(0x00027130, 0x44)
void InheritFusionStat(Character* source, Character* result, i16 stat) {
    i16 difference = GetBaseStat(source, stat) - GetBaseStat(result, stat);
    if (difference >= 0) {
        result->stats.base[stat] = ClampTo100(GetBaseStat(result, stat) + difference / 4);
    }
}

static __inline void ClassifyFusionSlot(
    i16 slot,
    i16* primary,
    i16* primaryCount,
    i16* secondary,
    i16* secondaryCount,
    i16* other,
    i16* otherCount
) {
    i16 side = GetFusionPairSide(slot);
    if (side == 0) {
        secondary[(*secondaryCount)++] = slot;
    } else if (side == 1) {
        primary[(*primaryCount)++] = slot;
    } else {
        other[(*otherCount)++] = slot;
    }
}

#define StageTripleFusionCharacter(result, first, second, third, pairMode, thirdMode)              \
    do {                                                                                           \
        (result) = StagePairFusionCharacter((first), (second), (pairMode));                        \
        if ((result) >= 32) {                                                                      \
            (result) = StagePairFusionCharacter(0, (third), (thirdMode));                          \
        }                                                                                          \
    } while (0)

RVA(0x00027180, 0x1c0)
i16 ResolveThreeSpecialRaceFusion(i16 first, i16 second, i16 third) {
    i16 primary[3];
    i16 secondary[3];
    i16 other[3];
    i16 primaryCount;
    i16 secondaryCount;
    i16 otherCount;
    i16 result = FindPrimaryFusionComplement(first, second, third);
    if (result <= 1) {
        result = FindSecondaryFusionComplement(first, second, third);
    }
    if (result > 1) {
        StageFusionCharacter(result);
        RestoreFusionCharacter();
        return SetFusionResult(result, 0);
    }
    primaryCount = secondaryCount = otherCount = 0;
    ClassifyFusionSlot(
        first,
        primary,
        &primaryCount,
        secondary,
        &secondaryCount,
        other,
        &otherCount
    );
    ClassifyFusionSlot(
        second,
        primary,
        &primaryCount,
        secondary,
        &secondaryCount,
        other,
        &otherCount
    );
    ClassifyFusionSlot(
        third,
        primary,
        &primaryCount,
        secondary,
        &secondaryCount,
        other,
        &otherCount
    );
    if (secondaryCount == 2) {
        result = StagePairFusionCharacter(secondary[0], secondary[1], 0);
        if (result >= 32) {
            if (primaryCount) {
                result = StagePairFusionCharacter(0, primary[0], 1);
            } else {
                result = StagePairFusionCharacter(0, other[0], 1);
            }
        }
    } else if (primaryCount == 2) {
        result = StagePairFusionCharacter(primary[0], primary[1], 1);
        if (result >= 32) {
            if (secondaryCount) {
                result = StagePairFusionCharacter(0, secondary[0], 1);
            } else {
                result = StagePairFusionCharacter(0, other[0], 1);
            }
        }
    } else {
        StageTripleFusionCharacter(result, first, second, third, 1, 1);
    }
    RestoreFusionCharacter();
    return result;
}

RVA(0x00027340, 0xa8)
i16 ResolveTwoSpecialRaceFusion(i16 first, i16 second, i16 third) {
    i16 result;
    MoveSpecialRaceFusionSlot(&first, &second);
    MoveSpecialRaceFusionSlot(&second, &third);
    MoveSpecialRaceFusionSlot(&first, &second);
    if (MatchFusionPair(second, third) == 1) {
        StageTripleFusionCharacter(result, second, third, first, 0, 1);
    } else {
        StageTripleFusionCharacter(result, second, third, first, 1, 0);
    }
    RestoreFusionCharacter();
    return result;
}

RVA(0x000273f0, 0x70)
i16 ResolveOneSpecialRaceFusion(i16 first, i16 second, i16 third) {
    i16 result;
    MoveSpecialRaceFusionSlot(&first, &second);
    MoveSpecialRaceFusionSlot(&second, &third);
    MoveSpecialRaceFusionSlot(&first, &second);
    StageTripleFusionCharacter(result, first, second, third, 1, 0);
    RestoreFusionCharacter();
    return result;
}

RVA(0x00027460, 0x49)
i16 CountSpecialRaceFusionSlots(i16 first, i16 second, i16 third) {
    i16 count = GetFusionSpecialRace(first) != 0;
    count += GetFusionSpecialRace(second) != 0;
    count += GetFusionSpecialRace(third) != 0;
    return count;
}

RVA(0x000274b0, 0x60)
i16 ResolveSpecialRaceTripleFusion(i16 first, i16 second, i16 third) {
    i16 count = CountSpecialRaceFusionSlots(first, second, third);
    if (count == 3) {
        return ResolveThreeSpecialRaceFusion(first, second, third);
    }
    if (count == 2) {
        return ResolveTwoSpecialRaceFusion(first, second, third);
    }
    if (count == 1) {
        return ResolveOneSpecialRaceFusion(first, second, third);
    }
    return 0;
}

static __inline void SortThreeFusionSlots(i16* first, i16* second, i16* third) {
    SortFusionSlots(first, second);
    SortFusionSlots(second, third);
    SortFusionSlots(first, second);
}

RVA(0x00027510, 0x79)
i16 ResolveThreeUnrankedFusion(i16 first, i16 second, i16 third) {
    i16 result;
    SortThreeFusionSlots(&first, &second, &third);
    StageTripleFusionCharacter(result, first, second, third, 0, 0);
    s_fusionLevelAllowance = 10;
    RestoreFusionCharacter();
    return result;
}

RVA(0x00027590, 0x70)
i16 ResolveTwoUnrankedFusion(i16 first, i16 second, i16 third) {
    i16 result;
    MoveUnrankedFusionSlot(&first, &second);
    MoveUnrankedFusionSlot(&second, &third);
    MoveUnrankedFusionSlot(&first, &second);
    StageTripleFusionCharacter(result, second, third, first, 1, 0);
    RestoreFusionCharacter();
    return result;
}

RVA(0x00027600, 0x69)
i16 CountUnrankedFusionSlots(i16 first, i16 second, i16 third) {
    i16 count = GetDemonFlagLow(GetRosterId(first)) == -1;
    count += GetDemonFlagLow(GetRosterId(second)) == -1;
    count += GetDemonFlagLow(GetRosterId(third)) == -1;
    return count;
}

RVA(0x00027670, 0x4b)
i16 ResolveUnrankedTripleFusion(i16 first, i16 second, i16 third) {
    i16 count = CountUnrankedFusionSlots(first, second, third);
    if (count == 3) {
        return ResolveThreeUnrankedFusion(first, second, third);
    }
    if (count == 2) {
        return ResolveTwoUnrankedFusion(first, second, third);
    }
    return 0;
}

RVA(0x000276c0, 0xdb)
i16 ResolveSameClassTripleFusion(i16 first, i16 second, i16 third) {
    i16 difference;
    i16 result;
    if (CountUnrankedFusionSlots(first, second, third)) {
        return 0;
    }
    difference = CompareRosterFusionClasses(first, second);
    difference |= CompareRosterFusionClasses(second, third);
    if (difference) {
        return 0;
    }
    if (GetRosterFusionRestrictedClass(first)) {
        return -1;
    }
    SortThreeFusionSlots(&first, &second, &third);
    StageTripleFusionCharacter(result, first, second, third, 0, 1);
    RestoreFusionCharacter();
    return result;
}

RVA(0x000277a0, 0x67)
i16 CountFusionLowFlagOne(i16 first, i16 second, i16 third) {
    i16 count = GetDemonFlagLow(GetRosterId(first)) == 1;
    count += GetDemonFlagLow(GetRosterId(second)) == 1;
    count += GetDemonFlagLow(GetRosterId(third)) == 1;
    return count;
}

RVA(0x00027810, 0x67)
i16 CountFusionHighFlagOne(i16 first, i16 second, i16 third) {
    i16 count = GetDemonFlagHigh(GetRosterId(first)) == 1;
    count += GetDemonFlagHigh(GetRosterId(second)) == 1;
    count += GetDemonFlagHigh(GetRosterId(third)) == 1;
    return count;
}

RVA(0x00027880, 0x4b)
i16 GetTripleFusionLevelBonus(i16 first, i16 second, i16 third) {
    i16 count = CountFusionLowFlagOne(first, second, third);
    if (count) {
        return s_fusionFlagLevelBonuses[count];
    }
    return CountFusionHighFlagOne(first, second, third) == 3 ? 2 : 0;
}

RVA(0x000278d0, 0x6c)
i16 GetTripleFusionLevel(i16 first, i16 second, i16 third) {
    i16 level = GetFusionSlotLevel(first);
    u8 result;
    level += GetFusionSlotLevel(second);
    level += GetFusionSlotLevel(third);
    level /= 3;
    level += 5;
    result = ClampLevel(level);
    return result;
}

RVA(0x00027940, 0x1ec)
i16 ResolveGeneralTripleFusion(i16 first, i16 second, i16 third) {
    i16 result;
    i16 index;
    i16 race;
    i16 level;
    SortThreeFusionSlots(&first, &second, &third);
    GetFusionSlotClass(first);
    GetFusionSlotClass(second);
    GetFusionSlotClass(third);
    if (CompareRosterFusionClasses(first, second) == 0) {
        StageTripleFusionCharacter(result, first, second, third, 1, 1);
        RestoreFusionCharacter();
        if (GetDemonFlagLow(GetRosterId(third)) == -1) {
            s_fusionLevelAllowance = 8;
        }
        return result;
    }
    index = GetFusionClassPair(first, second);
    if (index == 30) {
        StageFusionCharacter(FindNextOfRace(GetRosterId(second), 0));
        result = StagePairFusionCharacter(0, third, 0);
        RestoreFusionCharacter();
        return result;
    }
    index = min(26, index);
    race = GetFusionRaceEntry(index, third);
    if (race >= 100) {
        race = 5;
    }
    level = GetTripleFusionLevel(first, second, third);
    level += GetTripleFusionLevelBonus(first, second, third);
    result = FindDemonOfRace(level, race);
    StageFusionCharacter(result);
    RestoreFusionCharacter();
    if (GetDemonFlagLow(GetRosterId(third)) == -1) {
        s_fusionLevelAllowance = 8;
    }
    return SetFusionResult(result, 0);
}

RVA(0x00027b30, 0xe9)
i16 CalculateTripleFusion(i16 first, i16 second, i16 third) {
    i16 result;
    if (!GetRosterCharacter(first)) {
        return 0;
    }
    if (!GetRosterCharacter(second)) {
        return 0;
    }
    if (!GetRosterCharacter(third)) {
        return 0;
    }
    s_randomFusion = false;
    s_fusionLevelAllowance = 5;
    result = ResolveSpecialRaceTripleFusion(first, second, third);
    if (!result) {
        result = ResolveUnrankedTripleFusion(first, second, third);
        if (!result) {
            result = ResolveSameClassTripleFusion(first, second, third);
            if (!result) {
                result = ResolveGeneralTripleFusion(first, second, third);
            }
        }
    }
    if (result < 0) {
        SetFusionResult(-1, 0);
        return 0;
    }
    if (result > 0 && IsFusionDemonRestricted(result)) {
        SetFusionResult(-1, 0);
        return 0;
    }
    return result;
}

RVA(0x00027c20, 0xa)
void MarkRandomFusion(void) {
    s_randomFusion = true;
}

RVA(0x00027c30, 0x129)
FusionSummary GetTripleFusionSummary(i16 first, i16 second, i16 third) {
    i16 demon;
    i16 kind;
    i16 sourceLevel;
    i16 resultLevel;
    u8 clampedLevel;
    FusionSummary summary;
    FusionSummary* result = &summary;
    GetFusionResult(&demon, &kind);
    sourceLevel = GetRosterCharacter(first)->level;
    InitFusionSummary(&summary);
    if (s_randomFusion || !demon) {
        return summary;
    }
    if (demon == -1) {
        summary.fields.kind = -1;
        return summary;
    }
    clampedLevel = ClampLevel(GetDemonLevel(demon));
    resultLevel = clampedLevel;
    result->fields.highFlag = GetDemonFlagHigh(demon);
    result->fields.lowFlag = GetDemonFlagLow(demon);
    SetFusionSummaryKind(&summary, kind, resultLevel, sourceLevel);
    if (GetCharacter(0)->level + s_fusionLevelAllowance < resultLevel) {
        summary.fields.overLevel = 1;
    }
    return summary;
}

RVA(0x00027d60, 0x104)
void LoadFusionTables(void) {
    FILE* fp = OpenDataFile(12, 12, 0);
    s_fusionRaceMatrix = ReadCryptHandle(fp);
    s_fusionSameRaceChanges = ReadCryptHandle(fp);
    s_fusionDemonMatrix = ReadCryptHandle(fp);
    s_fusionSpecialRaceChanges = ReadCryptHandle(fp);
    ReadCryptRecord(fp, &s_fusionConfigValue);
    ReadCryptRecord(fp, &s_fusionSpecialRace);
    s_fusionFallbackHandle = ReadCryptHandle(fp);
    ReadCryptRecord(fp, &s_fusionRestrictedClass);
    CloseDataFile(fp);
    fp = OpenDataFile(15, 12, 0);
    s_fusionClassMatrix = ReadRawHandle(fp);
    s_fusionRaceRows = ReadRawHandle(fp);
    s_fusionPairs = ReadRawHandle(fp);
    s_fusionPrimaryComplements = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(17, 12, 0);
    s_fusionFlagRestrictions = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00027e70, 0xc5)
void FreeFusionTables(void) {
    s_fusionRaceMatrix = FreeHandle(s_fusionRaceMatrix);
    s_fusionSameRaceChanges = FreeHandle(s_fusionSameRaceChanges);
    s_fusionDemonMatrix = FreeHandle(s_fusionDemonMatrix);
    s_fusionSpecialRaceChanges = FreeHandle(s_fusionSpecialRaceChanges);
    s_fusionFallbackHandle = FreeHandle(s_fusionFallbackHandle);
    s_fusionClassMatrix = FreeHandle(s_fusionClassMatrix);
    s_fusionRaceRows = FreeHandle(s_fusionRaceRows);
    s_fusionPairs = FreeHandle(s_fusionPairs);
    s_fusionPrimaryComplements = FreeHandle(s_fusionPrimaryComplements);
    s_fusionFlagRestrictions = FreeHandle(s_fusionFlagRestrictions);
}

// @early-stop return width: retail clears ax on a failed matrix lookup;
// cl clears eax. Narrow helper and caller result forms do not recover it.
RVA(0x00027f40, 0xb3)
i16 GetFusionRacePair(i16 first, i16 second) {
    first = GetFusionSlotRace(first);
    second = GetFusionSlotRace(second);
    ReturnFusionWordPair(s_fusionRaceMatrix, first, second);
}

RVA(0x00028000, 0x51)
i16 GetSameRaceFusionRace(i16 slot) {
    i16 race = GetFusionSlotRace(slot);
    FusionRaceChange* changes = HandleReadPtr(s_fusionSameRaceChanges);
    while (changes->source) {
        if (changes->source == race) {
            return changes->target;
        }
        changes++;
    }
    return 0;
}

// @early-stop return width: retail clears ax on a failed matrix lookup;
// cl clears eax. Narrow helper and caller result forms do not recover it.
RVA(0x00028060, 0xa1)
i16 GetFusionDemonPair(i16 first, i16 second) {
    first = GetRosterId(first);
    second = GetRosterId(second);
    ReturnFusionWordPair(s_fusionDemonMatrix, first, second);
}

RVA(0x00028110, 0x87)
i16 GetFusionSpecialRaceChange(i16 demon, i16 race) {
    i16 column = -1;
    FusionWordMatrix* table = HandleReadPtr(s_fusionSpecialRaceChanges);
    i16 count = table->count;
    i16 index;
    for (index = 1; index < count; index++) {
        if (table->entries[index] == demon) {
            column = index;
        }
    }
    if (column >= 0) {
        for (index = count; table->entries[index] != -1; index += count) {
            if (table->entries[index] == race) {
                return table->entries[index + column];
            }
        }
    }
    return 0;
}

RVA(0x000281a0, 0x43)
i16 GetFusionFallbackEntry(i16 index) {
    i16* entries;
    i16 current;
    if (index >= 0) {
        entries = HandleReadPtr(s_fusionFallbackHandle);
        for (current = 0; entries[current]; current++) {
            if (current == index) {
                return entries[index];
            }
        }
    }
    return 0;
}

RVA(0x000281f0, 0x38)
i16 FindFusionFallbackIndex(i16 demon) {
    i16* entries = HandleReadPtr(s_fusionFallbackHandle);
    i16 index;
    for (index = 0; entries[index]; index++) {
        if (entries[index] == demon) {
            return index;
        }
    }
    return -1;
}

RVA(0x00028230, 0x2b)
i16 GetFusionSpecialRace(i16 slot) {
    i16 race = GetFusionSlotRace(slot);
    i16 special = s_fusionSpecialRace;
    return race != special ? 0 : s_fusionSpecialRace;
}

RVA(0x00028260, 0x22)
i16 GetFusionRestrictedClass(i16 demon) {
    i16 cls = GetDemonClass(demon);
    i16 restricted = s_fusionRestrictedClass;
    return cls != restricted ? 0 : s_fusionRestrictedClass;
}

RVA(0x00028290, 0x17)
i16 GetRosterFusionRestrictedClass(i16 slot) {
    return GetFusionRestrictedClass(GetRosterId(slot));
}

// @early-stop register allocation: retail multiplies the selected index
// in place; this build copies the count to a shared temporary before the
// branch. Compound and staged index updates retain that extra copy.
RVA(0x000282b0, 0xc4)
i16 GetFusionClassPair(i16 first, i16 second) {
    first = GetFusionSlotClass(first);
    second = GetFusionSlotClass(second);
    ReturnFusionBytePair(s_fusionClassMatrix, first, second);
}

RVA(0x00028380, 0x78)
i16 GetFusionRaceEntry(i16 index, i16 slot) {
    i16 race = GetFusionSlotRace(slot);
    FusionRaceRows* table = HandleReadPtr(s_fusionRaceRows);
    i16 count = table->count;
    i16 row;
    for (row = 0; row < count; row++) {
        if (table->rows[row].values[0] == race) {
            return table->rows[row].values[index];
        }
    }
    return 0;
}

RVA(0x00028400, 0x63)
i16 GetFusionPairSide(i16 slot) {
    i16 demon = GetRosterId(slot);
    FusionDemonPair* pairs = HandleReadPtr(s_fusionPairs);
    i16 index;
    for (index = 0; pairs[index].first != -1; index++) {
        if (pairs[index].first == demon) {
            return 1;
        }
        if (pairs[index].second == demon) {
            return 0;
        }
    }
    return -1;
}

RVA(0x00028470, 0x7b)
i16 MatchFusionPair(i16 first, i16 second) {
    i16 firstDemon = GetRosterId(first);
    i16 secondDemon = GetRosterId(second);
    FusionDemonPair* pairs = HandleReadPtr(s_fusionPairs);
    i16 index;
    for (index = 0; pairs[index].first != -1; index++) {
        if (pairs[index].first == firstDemon && pairs[index].second == secondDemon
            || pairs[index].first == secondDemon && pairs[index].second == firstDemon) {
            return 1;
        }
    }
    return -1;
}

RVA(0x000284f0, 0xae)
i16 FindSecondaryFusionComplement(i16 first, i16 second, i16 third) {
    i16 sides = GetFusionPairSide(first);
    i16 firstDemon;
    i16 secondDemon;
    i16 thirdDemon;
    i16 index;
    FusionDemonPair* pairs;
    sides |= GetFusionPairSide(second);
    sides |= GetFusionPairSide(third);
    if (sides) {
        return -1;
    }
    firstDemon = GetRosterId(first);
    secondDemon = GetRosterId(second);
    thirdDemon = GetRosterId(third);
    pairs = HandleReadPtr(s_fusionPairs);
    for (index = 0; pairs[index].first != -1; index++) {
        if (pairs[index].second != firstDemon && pairs[index].second != secondDemon
            && pairs[index].second != thirdDemon) {
            return pairs[index].first;
        }
    }
    return -1;
}

RVA(0x000285a0, 0xac)
i16 FindPrimaryFusionComplement(i16 first, i16 second, i16 third) {
    i16 sides = GetFusionPairSide(first);
    i16 firstDemon;
    i16 secondDemon;
    i16 thirdDemon;
    i16 index;
    FusionDemonPair* pairs;
    sides += GetFusionPairSide(second);
    sides += GetFusionPairSide(third);
    if (sides != 3) {
        return -1;
    }
    firstDemon = GetRosterId(first);
    secondDemon = GetRosterId(second);
    thirdDemon = GetRosterId(third);
    pairs = HandleReadPtr(s_fusionPrimaryComplements);
    for (index = 0; pairs[index].first != -1; index++) {
        if (pairs[index].first != firstDemon && pairs[index].first != secondDemon
            && pairs[index].first != thirdDemon) {
            return pairs[index].second;
        }
    }
    return -1;
}

RVA(0x00028650, 0x4d)
i16 IsFusionDemonRestricted(i16 demon) {
    i16 restricted = 0;
    FusionFlagRestriction* entry = HandleReadPtr(s_fusionFlagRestrictions);
    while (entry->demon >= 0) {
        if (entry->demon == demon) {
            restricted |= MatchFlagWord(&entry->condition);
        }
        entry++;
    }
    return restricted;
}
