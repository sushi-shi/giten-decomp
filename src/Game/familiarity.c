// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The saved per-id counts
// that become a demon's familiarity, the per-id analyze bits, and the
// familiarity / level-gap bytes derived from them.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/AnalyzeData.h>
#include <Game/Character.h>
#include <Game/Familiarity.h>
#include <Game/GameState.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stdio.h>
#include <string.h>

// @identity-TODO: a count per id (0..255) whose eighth is the familiarity.
DATA(0x0007add8)
static u8 s_familiarityCounts[0x200];

// One bit per id: whether it has been analyzed.
DATA(0x00078878)
static u8 s_analyzed[0x40];

RVA(0x00010ba0, 0x20)
void SetFamiliarityCount(i16 id, i16 count) {
    s_familiarityCounts[id] = ClampShort(count, 0, 0xff);
}

RVA(0x00010bc0, 0xe)
i16 GetFamiliarityCount(i16 id) {
    return s_familiarityCounts[id];
}

RVA(0x00010bd0, 0x20)
void AddFamiliarityCount(i16 id, i16 delta) {
    delta += GetFamiliarityCount(id);
    SetFamiliarityCount(id, delta);
}

// Derives a record's familiarity and level gap once (personal flag 0).
RVA(0x00010bf0, 0x7b)
void RefreshFamiliarity(Character* character) {
    i16 value;
    i16 leaderLevel;
    if (TestCharacterFlag(character, 0)) {
        return;
    }
    value = GetFamiliarityCount(character->id) / 8;
    character->familiarity = ClampShort(value, 0, 0x3f);
    leaderLevel = GetRosterLeader()->level;
    value = leaderLevel - character->level;
    character->levelGap = ClampShort(value, 0, 0xff);
    SetCharacterFlag(character, 0);
}

RVA(0x00010c70, 0x2a)
void SetLevelGap(Character* character, i16 gap) {
    RefreshFamiliarity(character);
    character->levelGap = ClampShort(gap, 0, 0xff);
}

RVA(0x00010ca0, 0x28)
void AddLevelGap(Character* character, i16 delta) {
    RefreshFamiliarity(character);
    SetLevelGap(character, character->levelGap + delta);
}

// The familiarity, two more unless event flag 2/8 is set.
RVA(0x00010cd0, 0x2e)
i16 GetFamiliarity(Character* character) {
    i16 familiarity;
    RefreshFamiliarity(character);
    familiarity = character->familiarity;
    if (!IsEventFlagSet(2, 8)) {
        familiarity += 2;
    }
    return familiarity;
}

RVA(0x00010d00, 0x27)
void SetFamiliarity(Character* character, i16 familiarity) {
    RefreshFamiliarity(character);
    character->familiarity = ClampShort(familiarity, 0, 0x3f);
}

RVA(0x00010d30, 0x20)
void AddFamiliarity(Character* character, i16 delta) {
    delta += GetFamiliarity(character);
    SetFamiliarity(character, delta);
}

static __inline i32 RollCharacterFunds(Character* character) {
    i32 score = character->level * 10;
    score += RandomUpTo(10);
    return score;
}

// The actor's script magnetite roll: ten times its level plus 0..10.
RVA(0x00010d50, 0x22)
i32 RollCharacterMagnetite(Character* character) {
    return RollCharacterFunds(character);
}

// The actor's script macca roll uses the same level-based distribution.
RVA(0x00010d80, 0x22)
i32 RollCharacterMacca(Character* character) {
    return RollCharacterFunds(character);
}

// @early-stop register scheduling: the alignment argument loads cross the
// saved-result moves differently, and the final sum uses different scratch
// registers. Calls, branch destinations and return paths agree.
RVA(0x00010db0, 0x8e)
i16 AlignmentConflicts(Character* character) {
    i16 leaderClass;
    i16 characterClass;
    if (character == NULL) {
        return -1;
    }
    leaderClass = GetAlignmentClassA(GetRosterLeader());
    characterClass = GetAlignmentClassA(character);
    if ((characterClass < 0 && leaderClass >= 0) || (characterClass >= 0 && leaderClass < 0)) {
        return -1;
    }
    leaderClass = GetAlignmentClassB(GetRosterLeader());
    characterClass = GetAlignmentClassB(character);
    if (characterClass + leaderClass == 0 && leaderClass != 0) {
        return -1;
    }
    return 0;
}

RVA(0x00010e40, 0x14)
i16 ClearAnalyzed(void) {
    memset(s_analyzed, 0, sizeof(s_analyzed));
    return 0;
}

RVA(0x00010e60, 0x2e)
void SetAnalyzed(i16 id, i16 on) {
    if (on == 0) {
        ClearBit(s_analyzed, id);
        return;
    }
    SetBit(s_analyzed, id);
}

RVA(0x00010e90, 0x13)
i16 HasAnalyzeData(i16 id) {
    return TestBit(s_analyzed, id);
}

// Save and load the counts and the analyze bits; each returns how many bytes
// fell short (0 on success).
RVA(0x00010eb0, 0x24)
i16 SaveFamiliarityCounts(FILE* fp) {
    return 0x200 - fwrite(s_familiarityCounts, 1, 0x200, fp);
}

RVA(0x00010ee0, 0x24)
i16 LoadFamiliarityCounts(FILE* fp) {
    return 0x200 - fread(s_familiarityCounts, 1, 0x200, fp);
}

RVA(0x00010f10, 0x21)
i16 SaveAnalyzed(FILE* fp) {
    return 0x40 - fwrite(s_analyzed, 1, 0x40, fp);
}

RVA(0x00010f40, 0x26)
i16 LoadAnalyzed(FILE* fp) {
    ClearAnalyzed();
    return 0x40 - fread(s_analyzed, 1, 0x40, fp);
}
