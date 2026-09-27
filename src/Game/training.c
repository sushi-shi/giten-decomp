// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Character.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/SkillList.h>
#include <Mem/Handle.h>
#include <Script/EventFlags.h>
#include <Util/Range.h>

#include <string.h>

DATA(0x00064618)
const i16 g_affiliationGrowthStats[4][2] = {{5, 7}, {8, 0}, {2, 1}, {3, 9}};

DATA(0x0007d618)
static i32 s_learnableSkillTable;

DATA(0x0007d61c)
static i32 s_learnableSkillRequirements;

RVA(0x0001c250, 0x51)
void LoadLearnableSkillTables(void) {
    FILE* fp = OpenDataFile(48, 12, 0);
    s_learnableSkillTable = ReadRawHandle(fp);
    CloseDataFile(fp);
    fp = OpenDataFile(49, 12, 0);
    s_learnableSkillRequirements = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x0001c2b0, 0x7f)
i16* GetLearnableSkillList(i16 id, i16 source) {
    i16 key;
    i16 i;
    LearnableSkillTable* table;
    if (id == 0) {
        key = GetCharacterAffiliation(GetRosterCharacter(0), source);
        key = -1 - key;
    } else {
        key = FindCharacter(id);
        if (key < 0) {
            return NULL;
        }
    }
    table = HandleReadPtr(s_learnableSkillTable);
    for (i = 0; i < table->count; i++) {
        if (table->entries[i].character == key) {
            return OffsetBy(table, table->entries[i].offset);
        }
    }
    return NULL;
}

RVA(0x0001c330, 0x127)
i16 TakeLearnableSkill(Character* character, i16* skills) {
    i16 id = character->id;
    i16 i;
    i16 skill;
    i16 j;
    for (i = 0; skills[i] != -1; i++) {
        if (ContainsWord(GetCharacterSkills(character), skills[i])) {
            continue;
        }
        if (id == 0) {
            i16 found = -1;
            LearnableSkillRequirement* requirements = HandleReadPtr(s_learnableSkillRequirements);
            for (j = 0; requirements[j].skill != -1; j++) {
                if (requirements[j].skill == skills[i]) {
                    found = j;
                    break;
                }
            }
            if (found != -1 && !MatchFlagWord(&requirements[found].condition)) {
                continue;
            }
        }
        if (RollSkillLearning(character, skills[i])) {
            break;
        }
    }
    skill = skills[i];
    if (skill == -1) {
        skills[0] = -1;
        return -1;
    }
    i++;
    for (j = 0; skills[i + j] != -1; j++) {
        skills[j] = skills[i + j];
    }
    skills[j] = -1;
    return skill;
}

RVA(0x0001c460, 0x173)
i16 PickGrowthStats(Character* character, i16* picks, i16 turn) {
    memset(picks, -1, 3 * sizeof(i16));
    if (GetCharacterAffiliation(character, 2) >= 0) {
        picks[0] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), RandomAverage(0, 1, 0));
        picks[1] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), RandomAverage(0, 1, 0));
        picks[2] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 2), RandomAverage(0, 1, 0));
        return 3;
    }
    if (GetCharacterAffiliation(character, 1) < 0) {
        if (GetCharacterAffiliation(character, 0) >= 0) {
            picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 0);
            picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 1);
            return 1;
        }
        return 0;
    }
    if (!(turn & 1)) {
        picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 0);
        picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), 1);
        picks[2] =
            GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), RandomAverage(0, 1, 0));
        return 2;
    }
    picks[0] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), 0);
    picks[1] = GetAffiliationGrowthStat(GetCharacterAffiliation(character, 1), 1);
    picks[2] =
        GetAffiliationGrowthStat(GetCharacterAffiliation(character, 0), RandomAverage(0, 1, 0));
    return 2;
}

RVA(0x0001c5e0, 0x61)
void DropTopStatPicks(Character* character, i16* picks) {
    i16 i;
    for (i = 0; i < 3; i++) {
        if (picks[i] >= 0) {
            i16 j;
            for (j = 0; j < 10; j++) {
                if (GetBaseStat(character, j) > GetBaseStat(character, picks[i])) {
                    break;
                }
            }
            if (j >= 10 && RandomAverage(0, 3, 0) == 0) {
                picks[i] = -1;
            }
        }
    }
}

// The training points a level needs: 3 (level - 1)^2 + 7, levels clamped to
// 0..99 (level 0 counting as 1).
RVA(0x0001c650, 0x33)
u32 TrainingThreshold(i16 level) {
    i32 n;
    if (level < 0) {
        n = 0;
    } else if (level > 99) {
        n = 99;
    } else {
        n = level - 1;
    }
    return n * n * 3 + 7;
}

// Adds `amount` to training counter `kind`, capped at level 99's threshold;
// returns the new count.
RVA(0x0001c690, 0x2c)
u32 AddTrainingPointsRaw(Character* character, i16 kind, u32 amount) {
    u32* points = &character->trainingPoints[kind];
    u32 limit;
    amount += *points;
    limit = TrainingThreshold(99);
    if (limit < amount) {
        amount = limit;
    }
    *points = amount;
    return amount;
}

RVA(0x0001c6c0, 0x24)
u32 AddTrainingPoints(Character* character, i16 kind, i16 amount) {
    if (kind >= 0 && kind < 4) {
        return AddTrainingPointsRaw(character, kind, amount);
    }
}

#define RaiseTrainedLevel(level, points, raised)                                                   \
    do {                                                                                           \
        while (TrainingThreshold((level) + 1) <= (points)) {                                       \
            (level)++;                                                                             \
            (raised)++;                                                                            \
        }                                                                                          \
    } while (0)

// Raises training level `kind` (the battle-stat words 0, 6, 12 and 18) while
// its counter covers the next level's threshold; returns the levels gained.
// @identity-TODO: what the four training kinds measure is unrecovered.
RVA(0x0001c6f0, 0x140)
i16 ApplyTraining(Character* character, i16 kind) {
    i16 raised = 0;
    switch (kind) {
        case 0:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 0),
                GetTrainingPoints(character, 0),
                raised
            );
            break;
        case 1:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 6),
                GetTrainingPoints(character, 1),
                raised
            );
            break;
        case 2:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 12),
                GetTrainingPoints(character, 2),
                raised
            );
            break;
        case 3:
            RaiseTrainedLevel(
                GetBattleStatBase(character, 18),
                GetTrainingPoints(character, 3),
                raised
            );
            break;
    }
    return raised;
}

// Clamps the three affiliations to 0..3 (-1 otherwise), drops repeats and
// packs the remaining ones to the front.
// @early-stop: retail addresses the affiliation bytes as [character + index];
// the spellings tried give [index + character].
RVA(0x0001c830, 0x9b)
void NormalizeAffiliations(Character* character) {
    i16 i;
    i16 j;
    for (i = 0; i < 3; i++) {
        if (character->affiliation[i] > 3 || character->affiliation[i] < 0) {
            SetCharacterAffiliation(character, i, -1);
        }
    }
    for (i = 2; i > 0; i--) {
        if (character->affiliation[i] != -1) {
            for (j = i - 1; j >= 0; j--) {
                if (character->affiliation[i] == character->affiliation[j]) {
                    SetCharacterAffiliation(character, i, -1);
                }
            }
        }
    }
    for (i = 0; i < 2; i++) {
        if (character->affiliation[i] < 0) {
            for (j = 0; i + j + 1 < 3; j++) {
                SetCharacterAffiliation(character, i + j, character->affiliation[i + j + 1]);
            }
            SetCharacterAffiliation(character, i + j, -1);
        }
    }
}

// Applies the training of each of the character's affiliations.
RVA(0x0001c8d0, 0x46)
void RaiseAffiliationLevels(Character* character) {
    if (GetCharacterAffiliation(character, 0) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 0));
    }
    if (GetCharacterAffiliation(character, 1) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 1));
    }
    if (GetCharacterAffiliation(character, 2) >= 0) {
        ApplyTraining(character, GetCharacterAffiliation(character, 2));
    }
}
