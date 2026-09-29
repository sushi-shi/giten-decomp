// @identity-TODO: the original TU name is unproven. One object: its .bss
// interleaves the fusion, screen-effect, fusion-screen and fusion-menu
// statics, and its code is the contiguous fusion span.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Alignment.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/FieldSight.h>
#include <Game/Fusion.h>
#include <Game/FusionCompare.h>
#include <Game/FusionMenu.h>
#include <Game/FusionScreen.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Game/ScreenEffect.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusScreen.h>
#include <Gfx/Motion.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/ScriptVars.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Ui/Panel.h>
#include <Util/Level.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x00064670)
static const i16 s_fusionFlagLevelBonuses[4] = {0, 5, 7, 10};

DATA(0x00068f88)
static i16 s_fusionInfoPlane = -1;

DATA(0x00068f8c)
static i16 s_fusionPageRows = 1;

DATA(0x00068f90)
static i16 s_fusionColumnCount = 1;

DATA(0x00068f98)
// D / N / L alignment labels.
static char* s_fusionAlignmentALabels[3] = {"\202\143", "\202\155", "\202\153"};

DATA(0x00068fa8)
// C / N / L alignment labels.
static char* s_fusionAlignmentBLabels[3] = {"\202\142", "\202\155", "\202\153"};

DATA(0x00068fb4)
static i16 s_selectedFusionIndex = -1;

DATA(0x00068fb8)
i16 g_fusionFirstSlot = -1;

DATA(0x00068fbc)
i16 g_fusionSecondSlot = -1;

DATA(0x00068fc0)
i16 g_fusionThirdSlot = -1;

// @identity-TODO: the individual contents of these auxiliary planes are unproven.
DATA(0x00068fc4)
static i16 s_firstFusionDetailPlane = -1;

DATA(0x00068fc8)
static i16 s_secondFusionDetailPlane = -1;

DATA(0x00068fcc)
static i16 s_thirdFusionDetailPlane = -1;

DATA(0x00068fd0)
static i16 s_pendingFusionResultId = -1;

DATA(0x00068fd4)
static i16 s_fusionPreviewPlane = -1;

DATA(0x00068fd8)
i16 g_fusionResult = -1;

DATA(0x00068fe0)
static i16 s_fusionSummaryIcons[14] = {14, 13, 3, 4, 5, 6, 7, 8, 9, 11, 12, 0, 1, 2};

DATA(0x00080110)
static i32 s_animationImageSize = 0;

DATA(0x00080118)
char g_fusionNameBuffer[128] = {0};

DATA(0x00080198)
static i16 s_fusionPageAction = 0;

DATA(0x000801a0)
static FusionSummary s_fusionPairSummaries[32 * 32] = {0};

DATA(0x000809a0)
static FusionSummary s_fusionSummary = {0};

DATA(0x000809a4)
static struct BmpFile* s_animationImage = 0;

DATA(0x000809a8)
static i16 s_fusionSlots[32] = {0};

DATA(0x000809e8)
static u32 s_fusionSelectionImage = 0;

DATA(0x000809ec)
static MenuBox* s_fusionMenu = 0;

DATA(0x000809f0)
static TextPlaneHook s_previousFusionTextHook = 0;

DATA(0x000809f4)
static i16 s_fusionColumnOffset = 0;

DATA(0x000809f8)
static Panel* s_fusionPager = 0;

DATA(0x000809fc)
static FusionSummary s_cachedFusionSummary = {0};

DATA(0x00080a00)
static PaletteState* s_fusionSelectionPaletteState = 0;

DATA(0x00080a04)
static i16 s_fusionResultId = 0;

DATA(0x00080a08)
static i16 s_fusionCandidateCount = 0;

DATA(0x00080a0c)
static Character* s_savedFusionCharacter = 0;

DATA(0x00080a10)
u8 g_fusionPreviewSave[16] = {0};

DATA(0x00080a20)
PaletteState* g_fusionPaletteState = 0;

DATA(0x00080a24)
static FusionSummary* s_fusionSummaryTable = 0;

DATA(0x00080a28)
static u8* s_animationScript = 0;

DATA(0x00080a2c)
static i16 s_animationResource = 0;

DATA(0x00080a30)
static i16 s_animationX = 0;

DATA(0x00080a34)
static i16 s_animationY = 0;

DATA(0x00080a38)
static b16 s_randomFusion = false;

DATA(0x00080a3c)
static i16 s_fusionLevelAllowance = 0;

DATA(0x00080a40)
static i16 s_fusionResultKind = 0;

DATA(0x00080a44)
static i16 s_initialFusionStep = 0;

DATA(0x00080a48)
static i16 s_fusionResultVariable = 0;

DATA(0x00080a4c)
static b16 s_restoreFusionRenderMode = false;

DATA(0x00080a50)
static i32 s_fusionRaceMatrix = 0;

DATA(0x00080a54)
static i32 s_fusionSameRaceChanges = 0;

DATA(0x00080a58)
static i32 s_fusionDemonMatrix = 0;

DATA(0x00080a5c)
static i32 s_fusionSpecialRaceChanges = 0;

// @identity-TODO: a two-byte configuration record loaded but never read.
DATA(0x00080a60)
static u16 s_fusionConfigValue = 0;

// @identity-TODO: the particular race (here) and class (s_fusionRestrictedClass)
// selected by the loaded fusion configuration remain unnamed.
DATA(0x00080a64)
static i32 s_fusionSpecialRace = 0;

DATA(0x00080a68)
static i32 s_fusionFallbackHandle = 0;

DATA(0x00080a6c)
static i32 s_fusionRestrictedClass = 0;

DATA(0x00080a70)
static i32 s_fusionClassMatrix = 0;

DATA(0x00080a74)
static i32 s_fusionRaceRows = 0;

DATA(0x00080a78)
static i32 s_fusionPairs = 0;

DATA(0x00080a7c)
static i32 s_fusionPrimaryComplements = 0;

DATA(0x00080a80)
static i32 s_fusionFlagRestrictions = 0;

DATA(0x00080a84)
static b16 s_fusionPageActionPending = false;

DATA(0x00080a88)
char g_fusionMissingName[4] = {0};

DATA(0x00080a8c)
char g_fusionMissingRace[4] = {0};

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
    GZ_ENUM_LOCAL(CharacterStat, i16) stat;
    for (stat = STAT_INTUITION; stat <= STAT_FORTUNE; stat++) {
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
void InheritFusionStat(
    Character* source,
    Character* result,
    GZ_ENUM_PARAM(CharacterStat, i16) stat
) {
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

RVA(0x000286a0, 0x6)
struct BmpFile* GetScreenEffectImage(void) {
    return s_animationImage;
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this accessor.
RVA(0x000286b0, 0x6)
i32 GetScreenEffectImageSize(void) {
    return s_animationImageSize;
}

RVA(0x000286c0, 0x2f)
void PushScriptAnimation(i16 animation, i16 x, i16 y) {
    s_animationResource = animation * 16;
    s_animationX = x;
    s_animationY = y;
    PushGameState(37);
}

RVA(0x000286f0, 0x9e)
b16 RunScriptAnimationState(void) {
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            LoadScriptAnimation(s_animationResource);
            LoadScriptAnimationImage(s_animationResource);
            StartEffectScript(s_animationScript, 0);
            break;
        case 1:
            if (!StepScreenEffectScript()) {
                NextGamePhase();
            }
            break;
        case 2:
            s_animationImage = FreeImageFile(s_animationImage);
            ClearEffectLayer(1);
            s_animationScript = FreeBlock(s_animationScript);
            ReturnFromGameState();
            break;
    }
    return false;
}

RVA(0x00028790, 0x32)
void LoadScriptAnimation(i16 resource) {
    FILE* file = OpenDataFile(resource + 0x3000, 2, 0);
    s_animationScript = ReadRawAlloc(file);
    CloseDataFile(file);
}

RVA(0x000287d0, 0x3b)
void LoadScriptAnimationImage(i16 resource) {
    ImageRequest request;
    request.file = resource + 0x3000;
    request.variant = 0;
    request.flags = 2;
    s_animationImage = LoadImageVariant(&request, &s_animationImageSize);
}

RVA(0x00028810, 0x2a)
void GetScriptAnimationPosition(i16 x, i16 y, i16* screenX, i16* screenY) {
    *screenX = (x + s_animationX) * 8;
    *screenY = y + s_animationY;
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this helper.
RVA(0x00028840, 0x16)
void AcquireFusionSelectionMode(void) {
    s_fusionSelectionPaletteState = SavePaletteState(s_fusionSelectionPaletteState, 1);
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this helper.
RVA(0x00028860, 0x2a)
void ReleaseFusionSelectionResources(void) {
    s_fusionSelectionImage = FreeImageHandle(s_fusionSelectionImage);
    s_fusionSelectionPaletteState = RestorePaletteState(s_fusionSelectionPaletteState, 1);
}

RVA(0x00028890, 0xa)
void ResetThirdFusionSlot(void) {
    g_fusionThirdSlot = -1;
}

RVA(0x000288a0, 0x7)
i16 GetFirstFusionSlot(void) {
    return g_fusionFirstSlot;
}

RVA(0x000288b0, 0x7)
i16 GetSecondFusionSlot(void) {
    return g_fusionSecondSlot;
}

RVA(0x000288c0, 0x7)
i16 GetThirdFusionSlot(void) {
    return g_fusionThirdSlot;
}

RVA(0x000288d0, 0x75)
i16 GetFusionResultKind(void) {
    if (s_fusionSummary.fields.overLevel) {
        return -3;
    }
    switch (s_fusionSummary.fields.kind) {
        case 0:
        case 3:
        case 4:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
            if (RosterContainsId(s_fusionResultId)) {
                return -2;
            }
            break;
        case 1:
        case 2:
            return s_fusionSummary.fields.kind;
        case 5:
        case 6:
        case 7:
            return s_fusionSummary.fields.kind;
    }
    return s_fusionSummary.fields.kind;
}

// Codegen constraint: the explicit default-bound cases retain the retail
// switch dispatch; grouping or omitting them changes its lowering.
RVA(0x00028950, 0x28f)
Character* CreatePairFusionCharacter(i16 first, i16 second, i16 rankChanges) {
    Character* firstCharacter = GetRosterEntry(first);
    Character* secondCharacter = GetRosterEntry(second);
    Character* result;
    i16 pending;
    if (firstCharacter == NULL || secondCharacter == NULL) {
        return NULL;
    }
    pending = s_pendingFusionResultId;
    s_pendingFusionResultId = -1;
    if (pending > 1) {
        s_fusionResultId = pending;
        s_fusionSummary = s_cachedFusionSummary;
    } else {
        s_fusionResultId = ResolvePairFusion(first, second, rankChanges);
        s_fusionSummary = GetPairFusionSummary(first, second);
        if (s_fusionResultId < 1) {
            return NULL;
        }
    }
    if (s_fusionSummary.fields.kind < 0) {
        return NULL;
    }
    switch (s_fusionSummary.fields.kind) {
        case 1:
        case 2:
            MoveSpecialFusionCharacters(&firstCharacter, &secondCharacter, &first, &second);
            result = CopyCharacter(firstCharacter, NULL);
            GainLevels(result, s_fusionSummary.fields.kind);
            result->level = ClampLevel(result->level + GetFusionGrowthBonus(first, second));
            RaiseExperienceToLevel(result);
            FullyRestoreCharacter(result);
            return result;
        case 5:
        case 6:
        case 7:
            MoveSpecialFusionCharacters(&firstCharacter, &secondCharacter, &first, &second);
            result = CopyCharacter(firstCharacter, NULL);
            if (s_fusionSummary.fields.kind <= 6) {
                result->pools.hp.cur += secondCharacter->pools.hp.cur;
                result->pools.mp.cur += secondCharacter->pools.mp.cur;
                if (s_fusionSummary.fields.kind == 6) {
                    ClearAllConditions(GetCharacterConditions(result));
                }
            }
            break;
        case 4:
            goto createCharacter;
        case 9:
            goto createCharacter;
        case 11:
            goto createCharacter;
        default:
        createCharacter:
            result = LoadCharacterCore(s_fusionResultId, NULL);
            InheritFusionStats(firstCharacter, secondCharacter, result);
            result->level = ClampLevel(result->level + GetFusionGrowthBonus(first, second));
            RaiseExperienceToLevel(result);
            FullyRestoreCharacter(result);
            if (s_fusionSummary.fields.kind == 4) {
                GainLevels(result, 1);
            }
            break;
    }
    return result;
}

RVA(0x00028be0, 0x290)
i16 RunFirstFusionPicker(i16 step, i16 triple) {
    i16 count;
    i16 result;
    i16 oldOffset;
    switch (step) {
        case 0:
            g_fusionFirstSlot = -1;
            g_fusionSecondSlot = -1;
            CountRosterEntries(1);
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            if (!triple) {
                g_fusionThirdSlot = -1;
                count = BuildPairFusionCandidates(0);
            } else {
                count = BuildTripleFusionSummaries(g_fusionThirdSlot);
            }
            s_fusionPageRows = -1;
            CreateFusionList(3, count);
            s_fusionColumnOffset = 0;
            s_fusionColumnCount = count;
            PaintMenuBox(s_fusionMenu);
            if (s_fusionPageRows < count) {
                s_fusionPager = CreateKindPanel(s_fusionPager, 284, 2, 1);
                PaintPanel(s_fusionPager, s_fusionMenu->plane);
            }
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionFirstSlot = g_selectedObjectId;
                    s_selectedFusionIndex = s_fusionMenu->cursor + g_hoveredObjectId;
                } else {
                    g_fusionFirstSlot = -1;
                }
                ++step;
            } else {
                result = RunPanelInput(s_fusionPager);
                if (s_fusionPageActionPending == 1) {
                    result = s_fusionPageAction;
                    s_fusionPageActionPending = false;
                    s_fusionPageAction = -1;
                    oldOffset = s_fusionColumnOffset;
                    if (result == 0) {
                        s_fusionColumnOffset -= s_fusionPageRows;
                        if (s_fusionColumnOffset < 0) {
                            s_fusionColumnOffset = 0;
                        }
                    } else if (result == 1) {
                        if (s_fusionColumnOffset + s_fusionPageRows < s_fusionColumnCount) {
                            s_fusionColumnOffset += s_fusionPageRows;
                        }
                    }
                    if (s_fusionColumnOffset != oldOffset) {
                        DrawFusionSummaryGrid();
                    }
                    ClearPanelChecksAgain(s_fusionPager);
                    PaintPanel(s_fusionPager, s_fusionMenu->plane);
                } else {
                    if (result >= 0) {
                        s_fusionPageActionPending = true;
                        s_fusionPageAction = result;
                    }
                    if (result == -1) {
                        s_fusionPreviewPlane = OpenFusionPreviewOnClick();
                        if (s_fusionPreviewPlane >= 0) {
                            step = 3;
                        }
                    }
                }
            }
            break;
        case 2:
            s_fusionPager = ReleasePanel(s_fusionPager, 1);
            step = CloseFusionPicker(g_fusionFirstSlot);
            break;
        case 3:
            s_fusionPreviewPlane = CloseFusionPreviewOnClick(s_fusionPreviewPlane);
            if (s_fusionPreviewPlane < 0) {
                step = 1;
            }
            break;
    }
    return step;
}

RVA(0x00028e70, 0x60)
i16 CreateFusionInfoPlane(i16 unused) {
    i16 plane = CreateTextPlane(1, 0);
    ResetTextPlaneLineStep(plane, 3);
    ClearTextPlane(plane);
    RepaintTextPlane(plane, -2);
    DrawPlaneImage(plane, 25, 1, 0);
    DrawPlaneImage(plane, 43, 1, 1);
    DrawPlaneImage(plane, 61, 1, 8);
    return plane;
}

RVA(0x00028ed0, 0x220)
void DrawFusionSummaryGrid(void) {
    i16 column;
    i16 row;
    i16 x;
    i16 y;
    i16 kind;
    i16 frame;
    i16 growth;
    i16 overLevel;
    FusionSummary* summary;
    ClearTextPlaneRight(s_fusionMenu->plane);
    for (column = s_fusionColumnOffset; column < s_fusionColumnOffset + s_fusionPageRows;
         ++column) {
        if (column < s_fusionColumnCount) {
            sprintf(g_scratchBuffer, "%2d", column + 1);
        } else {
            sprintf(g_scratchBuffer, "  ");
        }
        DrawPlaneText(
            s_fusionMenu->plane,
            272 + (column - s_fusionColumnOffset) * 24,
            7,
            g_scratchBuffer,
            0x1400
        );
        for (row = s_fusionMenu->cursor; row < s_fusionMenu->cursor + s_fusionMenu->pageRows;
             ++row) {
            x = (column - s_fusionColumnOffset) * 3 + 34;
            y = (row - s_fusionMenu->cursor) * 16 + 24;
            if (column < s_fusionColumnCount && row < s_fusionMenu->itemCount) {
                summary = GetFusionPairSummaryCell(s_fusionSlots[row], s_fusionSlots[column]);
                kind = summary->fields.kind;
                overLevel = summary->fields.overLevel;
                growth = summary->fields.lowFlag;
                frame = 1 - summary->fields.highFlag;
                growth = growth < 0 ? 1 : (growth > 0 ? 0 : -1);
                ++kind;
                if (kind >= 0) {
                    DrawPlaneIcon(s_fusionMenu->plane, x, y, s_fusionSummaryIcons[kind], frame);
                    if (overLevel) {
                        DrawPlaneIconKeyed(s_fusionMenu->plane, x, y, 10, frame);
                    }
                    if (growth >= 0) {
                        DrawPlaneIconKeyed(s_fusionMenu->plane, x, y, growth + 15, frame);
                    }
                }
            }
        }
    }
}

RVA(0x000290f0, 0x2b)
FusionSummary* GetFusionPairSummaryCell(i16 first, i16 second) {
    if (s_fusionSummaryTable == NULL) {
        s_fusionSummaryTable = s_fusionPairSummaries;
    }
    first = first * 32 + second;
    return &s_fusionPairSummaries[first];
}

RVA(0x00029120, 0x8d)
i16 CreateFusionList(i16 window, i16 count) {
    SetPanelImage(0x11c);
    s_fusionMenu = CreateMenuBox(s_fusionMenu, window, 2);
    SetMenuItems(s_fusionMenu, 15, NULL, count, FusionListMenuHandler);
    SetTextPlaneFirstSelectableRow(s_fusionMenu->plane, 1, 0);
    s_previousFusionTextHook = SetTextPlaneHook(FusionSelectionTextHook);
    if (s_fusionPageRows == -1) {
        s_fusionPageRows = 15;
    }
    BuildMenuPage(s_fusionMenu);
    return 15;
}

RVA(0x000291b0, 0x198)
void FusionListMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    Character* character;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->itemCount = 0;
            break;
        case MENU_EVENT_BEGIN_PAGE:
            SetTextPlaneMenuOrigin(menu->plane, 0, 1);
            break;
        case MENU_EVENT_ADD_ROW:
            g_scratchBuffer[0] = 0;
            character = GetRosterCharacter(s_fusionSlots[index]);
            if (character != NULL) {
                FormatFullName(g_fusionNameBuffer, character);
                sprintf(
                    g_scratchBuffer,
                    "%3d  %-10.10s %-16.16s",
                    index + 1,
                    GetDemonRaceName(character->id),
                    g_fusionNameBuffer
                );
            } else {
                sprintf(
                    g_scratchBuffer,
                    "%3d  %-9.9s %-16.16s",
                    index + 1,
                    g_fusionMissingRace,
                    g_fusionMissingName
                );
            }
            if (character != NULL
                && (g_fusionFirstSlot == -1 || g_fusionFirstSlot != s_fusionSlots[index])
                && (g_fusionThirdSlot == -1 || g_fusionThirdSlot != s_fusionSlots[index])
                && !IsFusionDemonRestricted(character->id)
                && (s_fusionPageRows != 1
                    || GetFusionPairSummaryCell(s_fusionSlots[index], g_fusionFirstSlot)
                               ->fields.kind
                           != -1)) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, s_fusionSlots[index], 0);
            } else {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    0x2500,
                    s_fusionSlots[index],
                    MENU_LINE_DISABLED
                );
            }
            break;
        case MENU_EVENT_BEFORE_PANEL:
            DrawFusionSummaryGrid();
            if (s_fusionPager != NULL) {
                PaintPanel(s_fusionPager, s_fusionMenu->plane);
            }
            break;
    }
}

RVA(0x00029350, 0x94)
void FusionSelectionTextHook(i16 plane, i16 event, i16 value) {
    Character* character;
    i16 index;
    if (plane == -1) {
        return;
    }
    switch (event) {
        case TEXT_EVENT_CANCEL:
            break;
        case TEXT_EVENT_UNHIGHLIGHT:
            ClearTextPlane(s_fusionInfoPlane);
            RepaintTextPlane(s_fusionInfoPlane, -2);
            break;
        case TEXT_EVENT_HIGHLIGHT:
            g_scratchBuffer[0] = 0;
            index = s_fusionMenu->cursor + value - 1;
            character = GetRosterCharacter(s_fusionSlots[index]);
            if (character != NULL) {
                DrawFusionCharacterDetails(s_fusionInfoPlane, character);
                RepaintTextPlane(s_fusionInfoPlane, -2);
            }
            break;
    }
}

RVA(0x000293f0, 0x192)
void DrawFusionCharacterDetails(i16 plane, Character* character) {
    ClearTextPlane(plane);
    sprintf(g_scratchBuffer, "%4d  %4d", character->pools.hp.cur, character->pools.hp.max);
    DrawPlaneText(plane, 56, 8, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d  %3d", character->pools.mp.cur, character->pools.mp.max);
    DrawPlaneText(plane, 56, 32, g_scratchBuffer, 0x1400);
    sprintf(
        g_scratchBuffer,
        " %s   %s",
        s_fusionAlignmentALabels[GetAlignmentClassA(character) + 1],
        s_fusionAlignmentBLabels[GetAlignmentClassB(character) + 1]
    );
    DrawPlaneText(plane, 56, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->levelBonus);
    DrawPlaneText(plane, 168, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->level);
    DrawPlaneText(plane, 168, 56, g_scratchBuffer, 0x1400);
    DrawFusionStatGroup(plane, 240, GetBattleStatGroup(character, 0));
    DrawFusionStatGroup(plane, 384, GetBattleStatGroup(character, 1));
    DrawFusionStatGroup(plane, 528, GetBattleStatGroup(character, 2));
}

RVA(0x00029590, 0xca)
void DrawFusionStatGroup(i16 plane, i16 x, i16* stats) {
    i16 column = x;
    sprintf(g_scratchBuffer, "%3d", stats[3]);
    DrawPlaneText(plane, column, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[5]);
    DrawPlaneText(plane, column, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[2]);
    column += 72;
    DrawPlaneText(plane, column, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", stats[4]);
    DrawPlaneText(plane, column, 56, g_scratchBuffer, 0x1400);
}

RVA(0x00029660, 0x85)
i32 CloseFusionPicker(i16 selection) {
    if (s_secondFusionDetailPlane >= 0) {
        s_secondFusionDetailPlane = CloseTextWindow(s_secondFusionDetailPlane);
    }
    if (s_firstFusionDetailPlane >= 0) {
        s_firstFusionDetailPlane = CloseTextWindow(s_firstFusionDetailPlane);
    }
    SetTextPlaneHook(s_previousFusionTextHook);
    s_previousFusionTextHook = NULL;
    s_fusionMenu = DestroyMenuBox(s_fusionMenu);
    s_fusionInfoPlane = CloseTextWindow(s_fusionInfoPlane);
    return selection == -1 ? -2 : -1;
}

RVA(0x000296f0, 0x134)
i16 BuildPairFusionCandidates(i16 skipCalculation) {
    i16 first;
    i16 second;
    i16 demon;
    FusionSummary summary;
    s_fusionCandidateCount = 0;
    for (first = 0; first < 32; first++) {
        if (GetRosterId(first) >= 32) {
            s_fusionSlots[s_fusionCandidateCount++] = first;
        }
        for (second = 0; second < 32; second++) {
            s_pendingFusionResultId = -1;
            summary.value = -256;
            StoreFusionPairSummary(first, second, &summary);
            if (first == second) {
                summary.fields.kind = -1;
                StoreFusionPairSummary(first, second, &summary);
            } else if (GetRosterId(second) < 32) {
                summary.fields.kind = -1;
                StoreFusionPairSummary(first, second, &summary);
            } else if (!skipCalculation && CalculatePairFusion(first, second)) {
                summary = GetPairFusionSummary(first, second);
                StoreFusionPairSummary(first, second, &summary);
            }
        }
    }
    summary.value = -128;
    for (first = 0; first < 32; first++) {
        demon = GetRosterId(first);
        if (demon >= 32 && IsFusionDemonRestricted(demon)) {
            for (second = 0; second < 32; second++) {
                StoreFusionPairSummary(first, second, &summary);
                StoreFusionPairSummary(second, first, &summary);
            }
        }
    }
    return s_fusionCandidateCount;
}

RVA(0x00029830, 0x33)
void StoreFusionPairSummary(i16 first, i16 second, const FusionSummary* summary) {
    if (s_fusionSummaryTable == NULL) {
        s_fusionSummaryTable = s_fusionPairSummaries;
    }
    first = first * 32 + second;
    s_fusionPairSummaries[first] = *summary;
}

RVA(0x00029870, 0x12d)
i16 BuildTripleFusionSummaries(i16 third) {
    i16 first;
    i16 second;
    i16 demon;
    FusionSummary summary;
    summary.value = -256;
    for (first = 0; first < 32; first++) {
        for (second = 0; second < 32; second++) {
            StoreFusionPairSummary(first, second, &summary);
        }
    }
    for (first = 0; first < 32; first++) {
        if (GetRosterId(first) >= 32) {
            for (second = 0; second < 32; second++) {
                s_pendingFusionResultId = -1;
                if (GetRosterId(second) < 32) {
                    summary.fields.kind = -1;
                    StoreFusionPairSummary(first, second, &summary);
                } else if (first != second && first != g_fusionThirdSlot
                           && second != g_fusionThirdSlot
                           && CalculateTripleFusion(third, first, second) > 0) {
                    summary = GetTripleFusionSummary(third, first, second);
                    StoreFusionPairSummary(first, second, &summary);
                } else {
                    summary.value = -128;
                    StoreFusionPairSummary(first, second, &summary);
                }
            }
        }
    }
    summary.value = -128;
    for (first = 0; first < 32; first++) {
        demon = GetRosterId(first);
        if (demon >= 32 && IsFusionDemonRestricted(demon)) {
            for (second = 0; second < 32; second++) {
                StoreFusionPairSummary(first, second, &summary);
                StoreFusionPairSummary(second, first, &summary);
            }
        }
    }
    return s_fusionCandidateCount;
}

RVA(0x000299a0, 0x64)
i16 CloseFusionPreviewOnClick(i16 plane) {
    if (!g_mouseLeftClick) {
        return plane;
    }
    if (s_thirdFusionDetailPlane != -1) {
        s_thirdFusionDetailPlane = CloseTextWindow(s_thirdFusionDetailPlane);
    }
    s_secondFusionDetailPlane = CloseTextWindow(s_secondFusionDetailPlane);
    s_firstFusionDetailPlane = CloseTextWindow(s_firstFusionDetailPlane);
    return CloseTextWindow(plane);
}

RVA(0x00029a10, 0x1e0)
i16 OpenFusionPreviewOnClick(void) {
    i16 column;
    i16 row;
    i16 first;
    i16 second;
    i16 plane;
    Character* character;
    if (!g_mouseLeftClick) {
        return -1;
    }
    g_mouseLeftClick = 0;
    column = (g_mousePosition.x - 272) / 24;
    row = (g_mousePosition.y - 54) / 16;
    if (column < 0 || column >= s_fusionPageRows || row < 0 || row >= s_fusionMenu->pageRows) {
        return -1;
    }
    column += s_fusionColumnOffset;
    row += s_fusionMenu->cursor;
    if (column >= 32 || row >= 32) {
        return -1;
    }
    first = s_fusionSlots[column];
    second = s_fusionSlots[row];
    if (g_fusionThirdSlot == -1) {
        character = CreatePairFusionCharacter(first, second, 0);
        if (!character) {
            return -1;
        }
        plane = CreateTextPlane(11, 0);
        ResetTextPlaneLineStep(plane, 3);
        DrawFusionPreviewCard(plane, character);
        s_firstFusionDetailPlane = CreateFusionPreviewCard(7, second);
        s_secondFusionDetailPlane = CreateFusionPreviewCard(8, first);
        RepaintTextPlane(plane, -2);
        FreeWordList(GetCharacterSkills(character));
        FreeBlock(character);
        return plane;
    } else {
        if (!CalculateTripleFusion(g_fusionThirdSlot, first, second)) {
            return -1;
        }
        character = GetCharacter(13);
        plane = CreateTextPlane(12, 0);
        ResetTextPlaneLineStep(plane, 3);
        DrawFusionPreviewCard(plane, character);
        s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionThirdSlot);
        s_secondFusionDetailPlane = CreateFusionPreviewCard(8, second);
        s_thirdFusionDetailPlane = CreateFusionPreviewCard(11, first);
        RepaintTextPlane(plane, -2);
        return plane;
    }
}

RVA(0x00029bf0, 0x19f)
void DrawFusionPreviewCard(i16 plane, Character* character) {
    ClearTextPlane(plane);
    FormatFullName(g_fusionNameBuffer, character);
    sprintf(
        g_scratchBuffer,
        "%-10.10s %-16.16s",
        GetDemonRaceName(character->id),
        g_fusionNameBuffer
    );
    DrawPlaneText(plane, 16, 8, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%4d  %4d", character->pools.hp.cur, character->pools.hp.max);
    DrawPlaneText(plane, 56, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d  %3d", character->pools.mp.cur, character->pools.mp.max);
    DrawPlaneText(plane, 56, 56, g_scratchBuffer, 0x1400);
    sprintf(
        g_scratchBuffer,
        " %s   %s",
        s_fusionAlignmentALabels[GetAlignmentClassA(character) + 1],
        s_fusionAlignmentBLabels[GetAlignmentClassB(character) + 1]
    );
    DrawPlaneText(plane, 184, 56, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->levelBonus);
    DrawPlaneText(plane, 256, 32, g_scratchBuffer, 0x1400);
    sprintf(g_scratchBuffer, "%3d", character->level);
    DrawPlaneText(plane, 184, 32, g_scratchBuffer, 0x1400);
}

RVA(0x00029d90, 0x44)
i16 CreateFusionPreviewCard(i16 window, i16 slot) {
    i16 plane = CreateTextPlane(window, 0);
    ResetTextPlaneLineStep(plane, 3);
    DrawFusionPreviewCard(plane, GetRosterCharacter(slot));
    RepaintTextPlane(plane, -2);
    return plane;
}

RVA(0x00029de0, 0x11f)
i16 RunSecondFusionPicker(i16 step) {
    i16 count;
    i16 result;
    switch (step) {
        case 0:
            count = s_fusionCandidateCount;
            g_fusionSecondSlot = -1;
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            s_fusionPageRows = 1;
            CreateFusionList(4, count);
            s_fusionColumnCount = s_selectedFusionIndex + 1;
            s_fusionColumnOffset = s_selectedFusionIndex;
            PaintMenuBox(s_fusionMenu);
            if (g_fusionThirdSlot == -1) {
                s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionFirstSlot);
            } else {
                s_firstFusionDetailPlane = CreateFusionPreviewCard(7, g_fusionThirdSlot);
                s_secondFusionDetailPlane = CreateFusionPreviewCard(8, g_fusionFirstSlot);
            }
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionSecondSlot = g_selectedObjectId;
                } else {
                    g_fusionSecondSlot = -1;
                }
                ++step;
            }
            break;
        case 2:
            step = CloseFusionPicker(g_fusionSecondSlot);
            break;
    }
    return step;
}

RVA(0x00029f00, 0xd4)
i16 RunThirdFusionPicker(i16 step) {
    i16 count;
    i16 result;
    switch (step) {
        case 0:
            g_fusionThirdSlot = -1;
            g_fusionFirstSlot = -1;
            g_fusionSecondSlot = -1;
            CountRosterEntries(1);
            s_fusionInfoPlane = CreateFusionInfoPlane(6);
            count = BuildPairFusionCandidates(1);
            s_fusionPageRows = 0;
            CreateFusionList(10, count);
            s_fusionColumnOffset = 0;
            PaintMenuBox(s_fusionMenu);
            ++step;
            break;
        case 1:
            result = RunMenu(s_fusionMenu);
            if (result != 0) {
                if (result > 0) {
                    g_fusionThirdSlot = g_selectedObjectId;
                } else {
                    g_fusionThirdSlot = -1;
                }
                ++step;
            }
            break;
        case 2:
            step = CloseFusionPicker(g_fusionThirdSlot);
            break;
    }
    return step;
}

RVA(0x00029fe0, 0x7e)
b16 PreviewFusionCharacter(Character* character) {
    Character* previous = SetRosterEntry(0, character);
    RaiseExperienceToLevel(character);
    AllocScreenSave(g_fusionPreviewSave);
    CaptureScreenSaveWithState(g_fusionPreviewSave);
    g_fusionPaletteState = SavePaletteState(g_fusionPaletteState, 2);
    EnterStatusScreen(1);
    DrawStatusScreen(0);
    SetRosterEntry(0, previous);
    return false;
}

RVA(0x0002a060, 0x49)
b16 CloseFusionPreview(void) {
    LeaveStatusScreen(1);
    RestoreDrawState(SaveDrawState());
    g_fusionPaletteState = RestorePaletteState(g_fusionPaletteState, 1);
    RestoreScreenSave(g_fusionPreviewSave);
    FreeScreenSave(g_fusionPreviewSave);
    return false;
}

RVA(0x0002a0b0, 0x16)
Character* LoadFusionResultCharacter(Character* destination) {
    return LoadCharacterCore(s_fusionResultId, destination);
}

RVA(0x0002a0d0, 0x78)
void RunPairFusion(void) {
    Character* result;
    LoadFusionTables();
    s_pendingFusionResultId = -1;
    result = CreatePairFusionCharacter(g_fusionFirstSlot, g_fusionSecondSlot, 0);
    if (result != NULL) {
        s_pendingFusionResultId = result->id;
        s_cachedFusionSummary = GetPairFusionSummary(g_fusionFirstSlot, g_fusionSecondSlot);
        PreviewFusionCharacter(result);
        FreeWordList(GetCharacterSkills(result));
        FreeBlock(result);
    }
}

RVA(0x0002a150, 0x81)
void RunTripleFusion(void) {
    Character* result;
    i16 id;
    LoadFusionTables();
    s_pendingFusionResultId = -1;
    CalculateTripleFusion(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    s_cachedFusionSummary =
        GetTripleFusionSummary(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    s_fusionSummary = s_cachedFusionSummary;
    result = GetCharacter(13);
    PreviewFusionCharacter(result);
    id = result->id;
    s_pendingFusionResultId = id;
    s_fusionResultId = id;
}

RVA(0x0002a1e0, 0xa)
void EndFusion(void) {
    CloseFusionPreview();
    FreeFusionTables();
}

RVA(0x0002a1f0, 0x52)
i16 CommitPairFusion(void) {
    Character* result = CreatePairFusionCharacter(g_fusionFirstSlot, g_fusionSecondSlot, 0);
    if (result == NULL) {
        return 0;
    }
    RemoveFromRoster(g_fusionFirstSlot);
    RemoveFromRoster(g_fusionSecondSlot);
    AddToRoster(result);
    return result->id;
}

RVA(0x0002a250, 0xd6)
i16 CommitTripleFusion(void) {
    Character* result;
    Character* temporary;
    i16 id = s_pendingFusionResultId;
    s_pendingFusionResultId = -1;
    if (id > 1) {
        s_fusionResultId = id;
        s_fusionSummary = s_cachedFusionSummary;
    } else {
        CalculateTripleFusion(g_fusionThirdSlot, g_fusionFirstSlot, g_fusionSecondSlot);
    }
    RemoveFromRoster(g_fusionThirdSlot);
    RemoveFromRoster(g_fusionFirstSlot);
    RemoveFromRoster(g_fusionSecondSlot);
    result = AllocCleared(1, sizeof(Character));
    memcpy(result, GetCharacter(13), sizeof(Character));
    id = result->id;
    s_fusionResultId = id;
    AddToRoster(result);
    temporary = GetCharacter(13);
    InitWordList(GetCharacterSkills(temporary), 0);
    return id;
}

RVA(0x0002a330, 0x50)
b16 StageFusionCharacter(i16 id) {
    Character* character = GetCharacter(13);
    FreeWordList(GetCharacterSkills(character));
    if (id >= 32) {
        character = LoadCharacterCore(id, character);
    }
    if (s_savedFusionCharacter == NULL) {
        s_savedFusionCharacter = SetRosterEntry(0, character);
    }
    return false;
}

RVA(0x0002a380, 0xb3)
i16 StagePairFusionCharacter(i16 first, i16 second, i16 rankChanges) {
    i16 result = ResolvePairFusion(first, second, rankChanges);
    Character* temporary = GetCharacter(13);
    Character* fusion;
    FreeWordList(GetCharacterSkills(temporary));
    fusion = CreatePairFusionCharacter(first, second, rankChanges);
    if (fusion != NULL) {
        memcpy(temporary, fusion, sizeof(Character));
        InitWordList(GetCharacterSkills(fusion), 0);
        FreeBlock(fusion);
    }
    if (s_savedFusionCharacter == NULL) {
        s_savedFusionCharacter = SetRosterEntry(0, temporary);
        if (s_savedFusionCharacter != GetCharacter(0)) {
            return -1;
        }
    }
    return result;
}

RVA(0x0002a440, 0x35)
b32 RestoreFusionCharacter(void) {
    if (s_savedFusionCharacter != NULL) {
        if (s_savedFusionCharacter != GetCharacter(0)) {
            return false;
        }
        SetRosterEntry(0, s_savedFusionCharacter);
    }
    s_savedFusionCharacter = NULL;
    return false;
}

RVA(0x0002a480, 0x130)
i16 CompareFusionCharacters(Character* first, Character* second) {
    i16 result;
    i16 firstStat;
    i16 secondStat;
    i16 index;
    result = CompareFusionValues(first->level, second->level);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(GetDemonLevel(first->id), GetDemonLevel(second->id));
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.hp.max, second->pools.hp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.hp.max, second->pools.hp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.mp.max, second->pools.mp.max);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->pools.mp.max, second->pools.mp.max);
    if (result != -1) {
        return result;
    }
    for (index = 0; index < 11; index++) {
        firstStat = GetBaseStat(first, index);
        secondStat = GetBaseStat(second, index);
    }
    result = CompareFusionValues(firstStat, secondStat);
    if (result != -1) {
        return result;
    }
    result = CompareFusionValues(first->id, second->id);
    if (result != -1) {
        return 1 - result;
    }
    return 0;
}

RVA(0x0002a5b0, 0x20)
i16 CompareFusionValues(i16 first, i16 second) {
    if (first == second) {
        return -1;
    }
    return first <= second;
}

RVA(0x0002a5d0, 0x44)
i16 SortFusionSlots(i16* first, i16* second) {
    Character* firstCharacter = GetRosterCharacter(*first);
    Character* secondCharacter = GetRosterCharacter(*second);
    i16 result = CompareFusionCharacters(firstCharacter, secondCharacter);
    if (result == 0) {
        SwapFusionSlotValues(first, second);
    }
    return result;
}

RVA(0x0002a620, 0x4b)
b16 MoveSpecialFusionSlot(i16* first, i16* second) {
    if (GetFusionSpecialRace(*first) || FindFusionFallbackIndex(GetRosterId(*first)) >= 0) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a670, 0x2a)
void MoveSpecialFusionCharacters(
    Character** firstCharacter,
    Character** secondCharacter,
    i16* first,
    i16* second
) {
    if (MoveSpecialFusionSlot(first, second)) {
        Character* temporary = *firstCharacter;
        *firstCharacter = *secondCharacter;
        *secondCharacter = temporary;
    }
}

RVA(0x0002a6a0, 0x31)
b16 MoveSpecialRaceFusionSlot(i16* first, i16* second) {
    if (GetFusionSpecialRace(*first)) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a6e0, 0x3b)
b16 MoveUnrankedFusionSlot(i16* first, i16* second) {
    if (GetDemonFlagLow(GetRosterId(*first)) == -1) {
        SwapFusionSlotValues(first, second);
        return true;
    }
    return false;
}

RVA(0x0002a720, 0x36)
i16 CompareRosterFusionClasses(i16 first, i16 second) {
    i16 cls = GetFusionSlotClass(first);
    cls -= GetFusionSlotClass(second);
    return cls;
}

RVA(0x0002a760, 0x21)
void PushFusionMenu(i16 kind, i16 resultVariable) {
    s_initialFusionStep = kind;
    s_fusionResultVariable = resultVariable;
    PushGameState(31);
}

RVA(0x0002a790, 0x23d)
b16 RunFusionMenuState(void) {
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                case 1:
                    NextGamePhase();
                    NextGamePhase();
                    g_fusionPaletteState = SavePaletteState(g_fusionPaletteState, 2);
                    LoadFusionTables();
                    SetGameStep(s_initialFusionStep);
                    break;
            }
            break;
        case 1:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                case 1:
                    ReturnFromGameState();
                    FreeFusionTables();
                    g_fusionPaletteState = RestorePaletteState(g_fusionPaletteState, 1);
                    ErasePictureSurface(54);
                    if (s_restoreFusionRenderMode) {
                        SetSceneRenderMode();
                        SetBlankStep(0);
                        s_restoreFusionRenderMode = false;
                    }
                    break;
            }
            break;
        case 2:
            switch (GetGameStep()) {
                case 0:
                    ClearStatusPicture();
                    ResetThirdFusionSlot();
                    result = RunFirstFusionPicker(GetGameSub(), 0);
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetFirstFusionSlot());
                    }
                    break;
                case 3:
                    result = CommitPairFusion();
                    if (s_fusionResultVariable >= 0) {
                        SetScriptLongVar(s_fusionResultVariable, result);
                    }
                    PrevGamePhase();
                    break;
                case 16:
                    ClearStatusPicture();
                    result = RunThirdFusionPicker(GetGameSub());
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetThirdFusionSlot());
                    }
                    break;
                case 17:
                    result = RunFirstFusionPicker(GetGameSub(), 1);
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetFirstFusionSlot());
                    }
                    break;
                case 1:
                case 18:
                    result = RunSecondFusionPicker(GetGameSub());
                    SetGameSub(result);
                    if (result < 0) {
                        FinishFusionMenuSelection(result, GetSecondFusionSlot());
                    }
                    break;
                case 20:
                    result = CommitTripleFusion();
                    if (s_fusionResultVariable >= 0) {
                        SetScriptLongVar(s_fusionResultVariable, result);
                    }
                    PrevGamePhase();
                    break;
            }
            break;
    }
    return false;
}

RVA(0x0002a9d0, 0x44)
void FinishFusionMenuSelection(i16 status, i16 selection) {
    if (status < 0) {
        if (s_fusionResultVariable >= 0) {
            SetScriptLongVar(s_fusionResultVariable, selection);
        }
        if (selection < 0 && s_fusionResultVariable == 1) {
            s_restoreFusionRenderMode = true;
        }
        PrevGamePhase();
    }
}
