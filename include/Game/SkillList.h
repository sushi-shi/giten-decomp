#ifndef GITEN_GAME_SKILLLIST_H
#define GITEN_GAME_SKILLLIST_H

#include <rva.h>

#include <Game/Character.h>
#include <Util/WordList.h>

// The sentinel-terminated list of skills available for `id` and `source`.
// For id zero, source selects one of the roster leader's affiliations.
i16* GetLearnableSkillList(i16 id, i16 source);

typedef struct SkillRank {
    i16 skill;
    i16 level;
} SkillRank;

typedef struct SkillRankList {
    i16 count;
    SkillRank entries[1];
} SkillRankList;

static __inline SkillRank* GetSkillRank(SkillRankList* list, i16 index) {
    return &list->entries[index];
}

i32 BuildLearnableSkillRanks(Character* character);
int CompareSkillRanks(const void* left, const void* right);

i16 RollSkillLearning(Character* character, i16 skill);

i16 AddSkill(WordList* list, i16 skill);

static __inline i32 IsSkillListFull(WordList* list) {
    return GetWordCount(list) >= 30;
}

i16 LearnLevelSkill(Character* character);

// Remove and return the lowest-level affiliated skill; -1 if none.
RVA_DECL(0x0002e110)
i16 RemoveLowestAffiliatedSkill(Character* character);

// Index of the matching character affiliation, or -1.
RVA_DECL(0x0002e1a0)
i16 FindSkillAffiliation(Character* character, i16 skill);

// Highest level among a character's skills matching an affiliation; -1 if none.
RVA_DECL(0x0002e1e0)
i16 HighestAffiliatedSkillLevel(Character* character);

#endif // GITEN_GAME_SKILLLIST_H
