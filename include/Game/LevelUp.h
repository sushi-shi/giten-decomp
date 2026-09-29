#ifndef GITEN_GAME_LEVELUP_H
#define GITEN_GAME_LEVELUP_H

void LevelUpNop(void);

#include <rva.h>

#include <Ints.h>
#include <Enums.h>

// The experience a battle awards, cleared when the level-up screen closes.
extern i32 g_rewardExperience;

// The magnetite a battle awards (GrantBattleRewards pays it to the leader),
// cleared when the field is entered.
extern i32 g_rewardMagnetite;

// Battle rewards and the level-up messages after a field battle.

struct Character;

// The experience curve: where a level begins, how far past it an amount is,
// and adding or sharing experience.
u32 ExperienceForLevel(i16 level, i16 id);
i32 ExperienceToLevel(i16 level, u32 experience, i16 id);
i32 AddExperience(struct Character* character, i32 amount);
i32 ShareExperience(i32 amount);
void RaiseExperienceToLevel(struct Character* character);

// Pending levels (of one roster slot, of the party) and the first party member
// with one.
i16 CountPendingLevels(i16 slot);
i16 CountPartyPendingLevels(void);
i16 FindLevelUpSlot(void);

// The level's stat growth and the stat-point picks.
b16 ApplyLevelStatGrowth(struct Character* character);
i16 IsStatCapped(struct Character* character, i16 stat);
i16 CountRaisableStats(struct Character* character);
i16 ResolveRaisableStat(struct Character* character, i16 stat);

// The battle rewards: the reward screen's click, paying them out, and
// marking them pending.
// @identity-TODO: the three input roles are unproven; Windows ignores
// them and leaves the two output words untouched.
i16 PollRewardClick(i16 inputA, i16 inputB, i16 inputC, i16* x, i16* y);
i16 GrantBattleRewards(void);
void MarkRewardsPending(void);

RVA_DECL(0x00019760)
char* FormatLevelUpMessage(char* buf, i16 slot);

// The phases of the level-up screen (RunLevelUp): open, close, pick the member,
// skip, hand out levels and points (with a choice, or without for demons),
// and redraw the raised stat.
GZ_ENUM_BEGIN(LevelUpPhase)
    LEVEL_UP_PHASE_OPEN = 0,
    LEVEL_UP_PHASE_CLOSE = 1,
    LEVEL_UP_PHASE_PICK_MEMBER = 2,
    LEVEL_UP_PHASE_SKIP = 3,
    LEVEL_UP_PHASE_DISTRIBUTE = 4,
    LEVEL_UP_PHASE_DISTRIBUTE_DEMON = 5,
    LEVEL_UP_PHASE_REDRAW_STAT = 6
GZ_ENUM_END(LevelUpPhase)

b16 RunLevelUp(void);

#endif // GITEN_GAME_LEVELUP_H
