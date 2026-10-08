#ifndef GITEN_GAME_LEVELUP_H
#define GITEN_GAME_LEVELUP_H

void LevelUpNop(void);

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/CharacterStat.h>
#include <Ints.h>
#include <Ui/Message.h>

// The experience a battle awards, cleared when the level-up screen closes.
extern i32 g_rewardExperience;

// The magnetite a battle awards (GrantBattleRewards pays it to the leader),
// cleared when the field is entered.
extern i32 g_rewardMagnetite;

// Battle rewards and the level-up messages after a field battle.

struct CharacterCore;

// The experience curve: where a level begins, how far past it an amount is,
// and adding or sharing experience.
u32 ExperienceForLevel(i16 level, i16 id);
i32 ExperienceToLevel(i16 level, u32 experience, i16 id);
i32 AddExperience(struct CharacterCore* character, i32 amount);
i32 ShareExperience(i32 amount);
void RaiseExperienceToLevel(struct CharacterCore* character);

// Pending levels (of one roster slot, of the party) and the first party member
// with one.
i16 CountPendingLevels(i16 slot);
i16 CountPartyPendingLevels(void);
i16 FindLevelUpSlot(void);

// The level's stat growth and the stat-point picks.
b16 ApplyLevelStatGrowth(struct CharacterCore* character);
i16 IsStatCapped(const struct CharacterCore* character, GZ_ENUM_PARAM(CharacterStat, i16) stat);
i16 CountRaisableStats(const struct CharacterCore* character);
i16 ResolveRaisableStat(const struct CharacterCore* character, i16 stat);

// The battle rewards: the reward screen's click, paying them out, and
// marking them pending.
// @identity-TODO: the three input roles are unproven; Windows ignores
// them and leaves the two output words untouched.
GZ_ENUM_BEGIN_SPLIT(RewardClickResult, i16)
    REWARD_CLICK_NONE = -1,
    REWARD_CLICK_CONFIRM = 0,
    REWARD_CLICK_RIGHT_BUTTON = 2
GZ_ENUM_END_SPLIT(RewardClickResult)

GZ_ENUM_RETURN(RewardClickResult, i16)
PollRewardClick(i16 inputA, i16 inputB, i16 inputC, i16* x, i16* y);
i16 GrantBattleRewards(void);
void MarkRewardsPending(void);

RVA_DECL(0x00019760)
char* FormatLevelUpMessage(char* buf, i16 slot);

#define ShowPendingLevelUpMessage(buffer)                                                          \
    do {                                                                                           \
        MarkRewardsPending();                                                                      \
        FormatLevelUpMessage((buffer), FindLevelUpSlot());                                         \
        ShowMessage((buffer), 0x3c);                                                               \
    } while (0)

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

GZ_ENUM_BEGIN(LevelUpCloseStep)
    LEVEL_UP_CLOSE_FADE = 0,
    LEVEL_UP_CLOSE_RESTORE = 1
GZ_ENUM_END(LevelUpCloseStep)

GZ_ENUM_BEGIN(LevelUpHumanStep)
    LEVEL_UP_HUMAN_COUNT_LEVELS = 0,
    LEVEL_UP_HUMAN_PICK_GROWTH = 1,
    LEVEL_UP_HUMAN_APPLY_GROWTH = 2,
    LEVEL_UP_HUMAN_OPEN_POINT_PICKER = 3,
    LEVEL_UP_HUMAN_DISTRIBUTE_POINTS = 4,
    LEVEL_UP_HUMAN_FINISH_LEVELS = 5,
    LEVEL_UP_HUMAN_LEARN_SKILL = 6
GZ_ENUM_END(LevelUpHumanStep)

GZ_ENUM_BEGIN(LevelUpDemonStep)
    LEVEL_UP_DEMON_CHECK_PENDING = 0,
    LEVEL_UP_DEMON_APPLY_GROWTH = 1,
    LEVEL_UP_DEMON_FINISH_LEVEL = 2
GZ_ENUM_END(LevelUpDemonStep)

GZ_ENUM_BEGIN(LevelUpRedrawStep)
    LEVEL_UP_REDRAW_RAISED = 0,
    LEVEL_UP_REDRAW_NORMAL = 1,
    LEVEL_UP_REDRAW_RETURN = 2
GZ_ENUM_END(LevelUpRedrawStep)

b16 RunLevelUp(void);

#endif // GITEN_GAME_LEVELUP_H
