// @identity-TODO: the owning TU is unproven; this unit holds the level-up
// screen's span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/FieldMain.h>
#include <Game/FieldScreen.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/InfoBar.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Game/Skill.h>
#include <Game/SkillList.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusScreen.h>
#include <Game/WaitState.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenSave.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Script/TextToken.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Message.h>
#include <Util/Level.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The conditions a level-up cures.
DATA(0x00064540)
static const i16 s_levelUpCures[] = {17, 26, 27, 28, 14, 18, 19, 29, 23, 16, 10, -1};

// The stat-list window of the member levelling up.
DATA(0x00068a74)
static i16 s_statWindow = -1;

// The roster slot of the member levelling up.
DATA(0x00068a78)
static i16 s_levelUpSlot = -1;

// The window asking how many stat points are left.
DATA(0x00068a7c)
static i16 s_pointPrompt = -1;

// The points of each stat raised this level.
DATA(0x0007b8a0)
static i16 s_statPicks[10];

// The skills the member can learn now.
DATA(0x0007bb78)
static i16 s_learnableSkills[64];

// Set when a battle's rewards wait to be handed out.
DATA(0x0007be90)
static b16 s_rewardsPending;

// The stat most recently raised.
DATA(0x0007be94)
static i16 s_raisedStat;

// The levels (then the stat points) still to hand out.
DATA(0x0007be98)
static i16 s_remaining;

// The music that played before the level-up screen.
DATA(0x0007be9c)
static i16 s_savedMusic;

// @identity-TODO: the screen area kept while the level-up screen is open; its
// layout is not recovered (the Windows build's save/restore bodies are empty).
DATA(0x0007beb8)
static u8 s_screenSave[16];

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
// @identity-TODO: an empty hook at the start of the level-up family.
RVA(0x00018730, 0x1)
void LevelUpNop(void) {}

// The reward screen's click: 2 for the right button, 0 for a left click in
// the OK box, -1 otherwise.
RVA(0x00018740, 0x3e)
i16 PollRewardClick(i16 inputA, i16 inputB, i16 inputC, i16* x, i16* y) {
    if (g_mousePosition.buttons & MOUSE_RIGHT_DOWN) {
        return 2;
    }
    if ((g_mousePosition.buttons & MOUSE_LEFT_DOWN) && g_mousePosition.x >= 0xea
        && g_mousePosition.x <= 0x192 && g_mousePosition.y >= 0xc0 && g_mousePosition.y <= 0xf6) {
        return 0;
    }
    return -1;
}

// The experience at which `level` begins: 4(n^3)+6 for a human (id below
// 0x20), 5(n^3+1) for a demon, n = level - 1.
RVA(0x00018780, 0x25)
u32 ExperienceForLevel(i16 level, i16 id) {
    i16 n = level - 1;
    i32 cube = n * n * n;
    if (id < 0x20) {
        return cube * 4 + 6;
    }
    return (cube + 1) * 5;
}

// How far `experience` is past the start of `level` (-1 from level 100).
RVA(0x000187b0, 0x25)
i32 ExperienceToLevel(i16 level, u32 experience, i16 id) {
    if (level >= 100) {
        return -1;
    }
    experience -= ExperienceForLevel(level, id);
    return experience;
}

// Adds experience to a living character; how far it is past the next level
// (negative: not reached), 0 for none or a disabled one, -1 at level 99.
RVA(0x000187e0, 0x54)
i32 AddExperience(Character* character, i32 amount) {
    if (character == NULL) {
        return 0;
    }
    if (GetDisablingCondition(GetCharacterConditions(character))) {
        return 0;
    }
    if (character->level >= 99) {
        return -1;
    }
    character->experience += amount;
    return ExperienceToLevel(character->level + 1, character->experience, character->id);
}

// Shares twice `amount` among the party's able members; how many reached a
// new level.
RVA(0x00018840, 0x4a)
i32 ShareExperience(i32 amount) {
    i16 count = CountPartyMembers(1);
    i32 share = amount * 2 / count;
    i32 reached = 0;
    i16 i;
    for (i = 0; i < 6; i++) {
        reached += AddExperience(GetPartyCharacter(i), share) >= 0;
    }
    return reached;
}

// The levels roster member `slot` has earned but not taken yet.
RVA(0x00018890, 0x79)
i16 CountPendingLevels(i16 slot) {
    Character* character = GetRosterCharacter(slot);
    u16 level;
    if (character == NULL) {
        return 0;
    }
    if (GetDisablingCondition(GetCharacterConditions(character))) {
        return 0;
    }
    for (level = character->level;
         ExperienceToLevel(level + 1, character->experience, character->id) >= 0;
         level++) {
    }
    return level - character->level;
}

// The pending levels of the whole party.
RVA(0x00018910, 0x27)
i16 CountPartyPendingLevels(void) {
    i16 total = 0;
    i16 i;
    for (i = 0; i < 6; i++) {
        total += CountPendingLevels(GetPartySlot(i));
    }
    return total;
}

// The roster slot of the first party member with a pending level (-1: none).
RVA(0x00018940, 0x32)
i16 FindLevelUpSlot(void) {
    i16 i;
    for (i = 0; i < 6; i++) {
        if (CountPendingLevels(GetPartySlot(i)) > 0) {
            return GetPartySlot(i);
        }
    }
    return -1;
}

// Raises fortune by one on every second level of a human and every third
// level of a demon; 1 when it grew.
RVA(0x00018980, 0x4b)
b16 ApplyLevelStatGrowth(Character* character) {
    if (IsHumanCharacter(character)) {
        if (character->level & 1) {
            return false;
        }
    } else if (character->level % 3) {
        return false;
    }
    character->stats.base[STAT_FORTUNE] = ClampTo100(GetBaseStat(character, STAT_FORTUNE) + 1);
    return true;
}

// Nonzero when one more point would take `stat` past its cap.
RVA(0x000189d0, 0x24)
i16 IsStatCapped(Character* character, i16 stat) {
    i16 raised = GetBaseStat(character, stat) + 1;
    return raised - ClampTo100(raised);
}

// How many of the ten stats can still take a point.
RVA(0x00018a00, 0x2b)
i16 CountRaisableStats(Character* character) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < 10; i++) {
        count += !IsStatCapped(character, i);
    }
    return count;
}

// `stat`, or a random stat when negative, re-rolled until one can take a
// point; -1 when none can.
RVA(0x00018a30, 0x67)
i16 ResolveRaisableStat(Character* character, i16 stat) {
    if (!CountRaisableStats(character)) {
        return -1;
    }
    if (stat < 0) {
        stat = RandomAverage(0, 9, 0);
    }
    while (IsStatCapped(character, stat)) {
        stat = RandomAverage(0, 9, 0);
    }
    return stat;
}

// Raises `experience` to at least the start of the character's level.
RVA(0x00018aa0, 0x23)
void RaiseExperienceToLevel(Character* character) {
    u32 floor = ExperienceForLevel(character->level, character->id);
    if (character->experience < floor) {
        character->experience = floor;
    }
}

// Pays a pending battle's macca and magnetite to the leader and shares its
// experience; the party's pending level count (0 with nothing pending).
RVA(0x00018ad0, 0x76)
i16 GrantBattleRewards(void) {
    Character* leader;
    if (s_rewardsPending) {
        leader = GetRosterCharacter(0);
        AddMacca(leader, g_rewardMacca);
        g_rewardMacca = 0;
        AddMagnetite(leader, g_rewardMagnetite);
        g_rewardMagnetite = 0;
        DrawInfoBar(0, 0);
        ShareExperience(g_rewardExperience);
        g_rewardExperience = 0;
        s_rewardsPending = false;
        return CountPartyPendingLevels();
    }
    return 0;
}

RVA(0x00018b50, 0xa)
void MarkRewardsPending(void) {
    s_rewardsPending = true;
}

static __inline void FinishLevelGain(Character* character) {
    ApplyLevelStatGrowth(character);
    FullyRestoreCharacter(character);
}

static __inline void ApplyPickedStatGain(Character* member) {
    member->stats.base[s_raisedStat]++;
    SaveGameState();
    SetGamePhase(6);
}

static __inline void ShowRaisedStat(Character* member, i16 highlighted) {
    FullyRestoreCharacter(member);
    DrawStatLine(member, s_raisedStat, highlighted, s_statWindow);
    PushWaitState(WAIT_FRAMES, 0xffff, 10, 0);
}

// Runs one frame of the level-up screen, by phase: 0 opens it, 2 picks the
// member and shows their stats, 4 hands out the levels and stat points and
// teaches new skills, 5 does the same without a choice (demons), 6 redraws the
// raised stat, and 1 closes the screen.
RVA(0x00018b60, 0x780)
b16 RunLevelUp(void) {
    Character* member;
    i16 key;
    i16 skill;

    SetStatusRenderMode();
    switch (GetGamePhase()) {
        case 0:
            s_savedMusic = PlayMusic(0x17, 1);
            CloseMessageWindow();
            SetGamePhase(2);
            AllocScreenSave(s_screenSave);
            CaptureScreenSaveWithState(s_screenSave);
            return false;
        case 1:
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                    s_pointPrompt = CloseTextWindow(s_pointPrompt);
                    PushScreenFade(SCREEN_FADE_TO_BLACK, 1);
                    PushWaitState(WAIT_INPUT, 0xffff, 0xffff, 0);
                    s_statWindow = CloseTextWindow(s_statWindow);
                    DrawStatusVitals(s_levelUpSlot);
                    return false;
                case 1:
                    ReturnFromGameState();
                    g_rewardExperience = 0;
                    RequestFieldRefresh();
                    LeaveStatusScreen(0);
                    ErasePictureSurface(0x36);
                    ClearStatusPicture();
                    RestoreScreenSave(s_screenSave);
                    FreeScreenSave(s_screenSave);
                    PlayMusic(s_savedMusic, 1);
                    return false;
            }
            break;
        case 2:
            CloseMessageWindow();
            EnterStatusScreen(0);
            member = GetRosterCharacter(s_levelUpSlot = FindLevelUpSlot());
            ClearConditionList(GetCharacterConditions(member), s_levelUpCures);
            DrawStatusScreen(s_levelUpSlot);
            s_statWindow = OpenStatListWindow(member);
            SetGamePhase(1);
            SaveGameState();
            SetGamePhase(4);
            if (!IsHumanCharacter(member)) {
                NextGamePhase();
            }
            FadeScreenAndWait(SCREEN_FADE_FROM_BLACK, 1);
            return false;
        case 3:
            NextGamePhase();
            return false;
        case 4:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                    s_remaining = CountPendingLevels(s_levelUpSlot);
                    return false;
                case 1:
                    NextGameStep();
                    if (s_remaining == 0 || !CountRaisableStats(member)) {
                        NextGameStep();
                        return false;
                    }
                    PickGrowthStats(member, s_statPicks, member->level + s_remaining);
                    DropTopStatPicks(member, s_statPicks);
                    s_remaining--;
                    return false;
                case 2:
                    if (GetGameSub() >= 3) {
                        PrevGameStep();
                        return false;
                    }
                    s_raisedStat = ResolveRaisableStat(member, s_statPicks[GetGameSub()]);
                    NextGameSub();
                    if (s_raisedStat < 0) {
                        PrevGameStep();
                        return false;
                    }
                    ApplyPickedStatGain(member);
                    return false;
                case 3:
                    NextGameStep();
                    ResetTextPlaneMenu(s_statWindow, 0, 0);
                    SetTextPlaneHighlightMode(s_statWindow, 1);
                    s_remaining = CountPendingLevels(s_levelUpSlot);
                    memset(s_statPicks, 0, sizeof(s_statPicks));
                    if (!CountRaisableStats(member)) {
                        NextGameStep();
                        return false;
                    }
                    ShowStatPointPrompt(s_remaining);
                    return false;
                case 4:
                    key = PollMenuInput(s_statWindow);
                    if (key == 0) {
                        break;
                    }
                    if (key == 1 && !IsStatCapped(member, g_selectedObjectId)) {
                        s_statPicks[g_selectedObjectId]++;
                        member->stats.base[g_selectedObjectId]++;
                        s_remaining--;
                        if (s_pointPrompt != -1) {
                            ShowStatPointPrompt(s_remaining);
                        }
                    } else if (key == 2 && s_statPicks[g_selectedObjectId] > 0) {
                        s_statPicks[g_selectedObjectId]--;
                        member->stats.base[g_selectedObjectId]--;
                        s_remaining++;
                        if (s_pointPrompt != -1) {
                            ShowStatPointPrompt(s_remaining);
                        }
                    } else {
                        break;
                    }
                    if (s_remaining == 0 || !CountRaisableStats(member)) {
                        NextGameStep();
                    }
                    ClearTextPlaneHighlight(s_statWindow);
                    s_raisedStat = g_selectedObjectId;
                    SaveGameState();
                    SetGamePhase(6);
                    return false;
                case 5:
                    while (CountPendingLevels(s_levelUpSlot)) {
                        member->level++;
                        FinishLevelGain(member);
                    }
                    s_statWindow = CloseTextWindow(s_statWindow);
                    DrawStatusVitals(s_levelUpSlot);
                    RaiseAffiliationLevels(member);
                    FullyRestoreCharacter(member);
                    NextGameStep();
                    if (!CollectLearnableSkills(member, -1)) {
                        ReturnFromGameState();
                        return false;
                    }
                    break;
                case 6:
                    skill = TakeLearnableSkill(member, s_learnableSkills);
                    if (skill == -1) {
                        ReturnFromGameState();
                        return false;
                    }
                    AddSkill(GetCharacterSkills(member), skill);
                    sprintf(
                        g_scratchBuffer,
                        "%s\202\360\211\357\223\276\202\265\202\275\201I",
                        GetSkillName(skill)
                    ); // %sを会得した！
                    PushMessageBox(0x19, g_scratchBuffer);
                    return false;
            }
            break;
        case 5:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case 0:
                    if (!CountPendingLevels(s_levelUpSlot)) {
                        ReturnFromGameState();
                        return false;
                    }
                    NextGameStep();
                    return false;
                case 1:
                    if (GetGameSub() >= 4) {
                        NextGameStep();
                        return false;
                    }
                    NextGameSub();
                    s_raisedStat = RollWeightedStat(member);
                    if (s_raisedStat == -1) {
                        NextGameStep();
                        return false;
                    }
                    ApplyPickedStatGain(member);
                    return false;
                case 2:
                    SetGameStep(0);
                    member->levelBonus += 2;
                    member->level++;
                    FinishLevelGain(member);
                    skill = LearnLevelSkill(member);
                    if (skill) {
                        _snprintf(
                            g_scratchBuffer,
                            0xff,
                            "%s\202\360\211\357\223\276\202\265\202\275\201I",
                            GetSkillName(skill)
                        ); // %sを会得した！
                        PushMessageBox(0x19, g_scratchBuffer);
                        return false;
                    }
                    break;
            }
            break;
        case 6:
            member = GetRosterCharacter(s_levelUpSlot);
            switch (GetGameStep()) {
                case 0:
                    NextGameStep();
                    ShowRaisedStat(member, 1);
                    break;
                case 1:
                    NextGameStep();
                    ShowRaisedStat(member, 0);
                    break;
                case 2:
                    ReturnFromGameState();
                    return false;
            }
            break;
    }
    return false;
}

RVA(0x000192e0, 0x8c)
void ShowStatPointPrompt(i16 points) {
    if (s_pointPrompt == -1) {
        s_pointPrompt = CreateTextPlane(0x10, 0);
    }
    ClearTextPlane(s_pointPrompt);
    PrintWindowText(
        s_pointPrompt,
        "\203|"
        "\203C\203\223\203g\202\360\220U\202\350\225\252\202\257\202\304\202\255\202\276\202\263"
        "\202\242\n",
        0x400,
        0,
        1
    );
    sprintf(g_scratchBuffer, "\214\343 %.1d \203|\203C\203\223\203g  \n", points);
    PrintWindowText(s_pointPrompt, g_scratchBuffer, 0x400, 0, 1);
    RepaintTextPlane(s_pointPrompt, -2);
}

static i16* BuildStatWeightRanges(Character* character, i16* ranges);

RVA(0x00019370, 0x5e)
i16 RollWeightedStat(Character* character) {
    i16 stat;
    i16 draw;
    if (BuildStatWeightRanges(character, s_statPicks) == NULL) {
        return -1;
    }
    draw = rand() * 10000 / RAND_MAX;
    for (stat = 0; stat < 10 && s_statPicks[stat] < draw; stat++) {
    }
    if (stat >= 10) {
        return -1;
    }
    return stat;
}

// @early-stop register allocation: the range cursor, total and stat index
// rotate across ebx, esi and edi. Calls, branches, stores and arithmetic
// align; cursor initialization order does not change the allocation.
RVA(0x000193d0, 0x9a)
static i16* BuildStatWeightRanges(Character* character, i16* ranges) {
    i16 total = 0;
    i16* range = ranges;
    i16 stat;
    stat = 0;
    while (stat < 10) {
        if (IsStatCapped(character, stat)) {
            *range = -1;
        } else {
            if (GetBaseStat(character, stat) == 0) {
                *range = ++total;
            } else {
                *range = total += GetBaseStat(character, stat) * 10;
            }
        }
        stat++;
        range++;
    }
    if (total == 0) {
        return NULL;
    }
    for (stat = 0; stat < 10; stat++) {
        if (ranges[stat] != -1) {
            ranges[stat] = ranges[stat] * 10000 / total;
        }
    }
    return ranges;
}

static i16 AppendLearnableSkills(Character* character, i16 count, i16 source);

RVA(0x00019470, 0x4e)
i16 CollectLearnableSkills(Character* character, i16 source) {
    i16 count;
    if (source == -1) {
        count = AppendLearnableSkills(character, 0, 0);
        if (character->id != 0) {
            return count;
        }
        count = AppendLearnableSkills(character, count, 1);
        return AppendLearnableSkills(character, count, 2);
    }
    return AppendLearnableSkills(character, 0, source);
}

RVA(0x000194c0, 0x87)
static i16 AppendLearnableSkills(Character* character, i16 count, i16 source) {
    i16* skills;
    i16 index;
    s_learnableSkills[count] = -1;
    if (character == NULL) {
        return count;
    }
    skills = GetLearnableSkillList(character->id, source);
    if (skills == NULL) {
        return count;
    }
    index = 0;
    while (skills[index] != -1) {
        if (count >= 63) {
            count = 63;
            break;
        }
        s_learnableSkills[count++] = skills[index++];
    }
    s_learnableSkills[count] = -1;
    return count;
}

// Teaches `character` every skill it can learn from `source`; returns how
// many it learned.
RVA(0x00019550, 0x63)
i16 LearnAllSkills(Character* character, i16 source) {
    i16 count = 0;
    i16 skill;

    if (CollectLearnableSkills(character, source) < 1) {
        return 0;
    }
    for (skill = TakeLearnableSkill(character, s_learnableSkills); skill != -1;
         skill = TakeLearnableSkill(character, s_learnableSkills)) {
        count++;
        AddSkill(GetCharacterSkills(character), skill);
    }
    return count;
}

RVA(0x000195c0, 0x199)
void GainLevels(Character* character, i16 count) {
    i16 index;
    i16 stat;
    u8 level;
    if (!character) {
        return;
    }
    if (!IsHumanCharacter(character)) {
        while (count > 0) {
            level = ClampLevel(character->level + 1);
            if (level == character->level) {
                break;
            }
            for (index = 0; index < 4; index++) {
                stat = RollWeightedStat(character);
                if (stat == -1) {
                    break;
                }
                character->stats.base[stat]++;
                FullyRestoreCharacter(character);
            }
            character->level++;
            character->levelBonus += 2;
            FinishLevelGain(character);
            LearnLevelSkill(character);
            count--;
        }
    } else {
        while (count > 0) {
            level = ClampLevel(character->level + 1);
            if (level == character->level || !CountRaisableStats(character)) {
                break;
            }
            PickGrowthStats(character, s_statPicks, character->level + s_remaining);
            DropTopStatPicks(character, s_statPicks);
            s_statPicks[3] = -1;
            for (index = 0; index < 4; index++) {
                stat = ResolveRaisableStat(character, s_statPicks[index]);
                if (stat >= 0) {
                    character->stats.base[stat]++;
                }
            }
            character->level++;
            FinishLevelGain(character);
            RaiseAffiliationLevels(character);
            count--;
        }
        LearnAllSkills(character, -1);
    }
    RaiseExperienceToLevel(character);
}

RVA(0x00019760, 0x40)
char* FormatLevelUpMessage(char* buf, i16 slot) {
    char name[64];
    FormatFullName(name, GetRosterCharacter(slot));
    sprintf(
        buf,
        "%s\202\315\203\214\203\170\203\213\202\252\217\343\202\252\202\301\202\275\n",
        name
    );
    return buf;
}
