#ifndef GITEN_GAME_GROWTH_H
#define GITEN_GAME_GROWTH_H

#include <rva.h>

#include <Game/Character.h>

// Raises a character by `count` levels.
// @identity-TODO: label-only; humans (id < 0x20) and demons take separate
// paths, and the level table 0x43dec0 is read for each level.
RVA_DECL(0x000195c0)
void GainLevels(Character* character, i16 count);

typedef struct LearnableSkillIndex {
    u16 offset;
    i16 character;
} LearnableSkillIndex;

typedef struct LearnableSkillTable {
    i16 count;
    LearnableSkillIndex entries[1];
} LearnableSkillTable;

typedef struct LearnableSkillRequirement {
    i16 skill;
    u16 condition;
} LearnableSkillRequirement;

void LoadLearnableSkillTables(void);
i16 TakeLearnableSkill(Character* character, i16* skills);

// Four stat pairs; unrelated nonzero words follow this table in retail.
extern const i16 g_affiliationGrowthStats[4][2];

#define GetAffiliationGrowthStat(affiliation, choice)                                              \
    (g_affiliationGrowthStats[(affiliation) & 3][choice])

// @identity-TODO: The role of the three signed bytes +0x42..+0x44 (0..3, +0x42 is
// Character.affiliation) as growth types is inferred from this table lookup only.
i16 PickGrowthStats(Character* character, i16* picks, i16 turn);

void DropTopStatPicks(Character* character, i16* picks);

// The training counters (Character.trainingPoints) and levels (battle-stat
// words 0, 6, 12, 18) of the four affiliations (training.c).
// @identity-TODO: What the four kinds measure is unrecovered.
u32 TrainingThreshold(i16 level);
u32 AddTrainingPointsRaw(Character* character, i16 kind, u32 amount);
u32 AddTrainingPoints(Character* character, i16 kind, i16 amount);
i16 ApplyTraining(Character* character, i16 kind);
void NormalizeAffiliations(Character* character);
void RaiseAffiliationLevels(Character* character);

void ShowStatPointPrompt(i16 points);

// Rolls one of ten stats by cumulative weights from uncapped base stats.
// This is used on the level-up path without manual stat allocation.
i16 RollWeightedStat(Character* character);

// @identity-TODO: the meanings of list sources 0..2 are unrecovered; only id
// zero gets sources 1 and 2 when source is -1.
i16 CollectLearnableSkills(Character* character, i16 source);

// Teaches `character` every skill it can learn from `source`; returns how
// many it learned.
i16 LearnAllSkills(Character* character, i16 source);

#endif // GITEN_GAME_GROWTH_H
