// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/FieldMain.h>
#include <Game/GameState.h>
#include <Game/InfoBar.h>
#include <Game/ModeFlags.h>
#include <Game/Party.h>
#include <Game/Pool.h>
#include <Game/SaveGame.h>
#include <Game/Stats.h>
#include <Game/StatusDraw.h>
#include <Mem/Alloc.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Util/BitSet.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <string.h>

// Takes a step's upkeep for each living party member: a class-10 demon (and,
// under condition 8, any member) heals 2 HP per whole hundred its carried
// percentage reaches and costs eight times that; other demons (bar classes
// 12 and 13) cost the whole hundreds. Returns how many members died of it.
RVA(0x00040fa0, 0x183)
i16 PayStepUpkeep(void) {
    i16 died = 0;
    Character* hero = GetRosterCharacter(0);
    i16 i;
    for (i = 0; i < 6; i++) {
        Character* member = GetPartyCharacter(i);
        i16 whole;
        i16 class;
        i16 rate;
        if (member == NULL || GetFatalCondition(GetCharacterConditions(member))) {
            continue;
        }
        if (GetDemonClass(member->id) == 10) {
            whole = AddHundredths(member, member->levelBonus);
            if (whole == 0) {
                continue;
            }
            FillPool(&member->pools.hp, whole * 2, POOL_FILL_TO_MAX);
            died += DrainUpkeep(hero, member, whole * 8, i);
        } else if (HasCondition(GetCharacterConditions(member), CONDITION_ZOMBIE)) {
            class = GetDemonClass(member->id);
            if (class == 12 || class == 13) {
                rate = member->level;
            } else {
                rate = member->levelBonus;
            }
            whole = AddHundredths(member, rate);
            if (whole == 0) {
                continue;
            }
            FillPool(&member->pools.hp, whole * 2, POOL_FILL_TO_MAX);
            died += DrainUpkeep(hero, member, whole * 8, i);
        } else {
            class = GetDemonClass(member->id);
            if (class == 12 || class == 13) {
                continue;
            }
            whole = AddHundredths(member, member->levelBonus);
            if (whole == 0 || HasCondition(GetCharacterConditions(member), CONDITION_ZOMBIE)) {
                continue;
            }
            died += DrainUpkeep(hero, member, whole, i);
        }
    }
    DrawMoneyCounters(1);
    return died;
}

// Adds to the character's carried hundredths and returns the whole hundreds.
// @early-stop load order: retail loads `amount` first and adds the field into
// it (16-bit add into cx); every spelling tried (u16/i16 field and parameter,
// compound assignment, separate total, cast placement) loads the field first,
// and the permuter found a single island.
RVA(0x00041130, 0x42)
i16 AddHundredths(Character* character, i16 amount) {
    u16 total = amount + character->hundredths;
    i16 whole = total / 100;
    character->hundredths = total % 100;
    return whole;
}

// Pays `cost` from the hero's magnetite, then the hero's MP, then the
// member's MP and HP; a member drained of HP dies (and a demon leaves the
// party slot `position`). Returns 1 when the member died.
RVA(0x00041180, 0x118)
i16 DrainUpkeep(Character* hero, Character* member, i16 cost, i16 position) {
    i16 died = 0;
    if (hero->magnetite >= cost) {
        hero->magnetite -= cost;
        cost = 0;
    } else {
        cost -= hero->magnetite;
        hero->magnetite = 0;
    }
    if (cost < 1) {
        return 0;
    }
    PayPoolCost(&hero->pools.mp, cost);
    if (cost < 1) {
        return 0;
    }
    PayPoolCost(&member->pools.mp, cost);
    if (cost < 1) {
        return 0;
    }
    if (member->pools.hp.cur > (u16)cost) {
        member->pools.hp.cur -= cost;
    } else {
        died = 1;
        member->pools.hp.cur = 0;
        AddCondition(GetCharacterConditions(member), CONDITION_DYING);
        if (!IsHumanCharacter(member)) {
            ClearPartyPosition(position);
        }
    }
    return died;
}

// The minutes toward the next party-timer tick.
DATA(0x00083b48)
static u32 s_timerMinutes;

// Advances the party timers by `minutes` for the party's roster-2 member:
// each whole period (240 minutes in mode 2, else 60) costs it 1 MP and 1 HP,
// and the moon sets or clears its condition 3. -1 when the timers are off or
// no such member is in the party, 0 when no period passed (or it is down).
RVA(0x000412a0, 0x150)
i16 TickPartyTimers(u16 minutes) {
    Character* character;
    i16 i;
    i16 count;
    if (IsEventFlagSet(1, 0xc) || GetGameState() == 5) {
        return -1;
    }
    for (i = 0; i < 6; i++) {
        if (PartySlotAt(i) != -1 && (character = RosterMemberAt(PartySlotAt(i))) != NULL
            && character->id == 2) {
            goto found;
        }
    }
    return -1;
found:
    if (TestModeFlags(MODE_WORLD_MAP)) {
        s_timerMinutes += minutes;
        count = s_timerMinutes / 240;
        s_timerMinutes %= 240;
    } else {
        s_timerMinutes += minutes;
        count = s_timerMinutes / 60;
        s_timerMinutes %= 60;
    }
    if (count == 0) {
        return 0;
    }
    if (GetFatalCondition(GetCharacterConditions(character))) {
        return 0;
    }
    ChangePool(&character->pools.mp, -count);
    ChangePool(&character->pools.hp, -count);
    ApplyEmptyPools(character);
    RequestStatusRedraw();
    if (g_clock.moonPhase <= 14) {
        ClearCondition(GetCharacterConditions(character), 3);
    } else {
        AddCondition(GetCharacterConditions(character), 3);
    }
    return 1;
}

// Clears the character's moon-driven personal flags as the moon moves on
// (flag 0x23 steps to 0x24; unless `keep`, 0x25 and 0x26 clear, 0x26 adding
// condition 0). Returns how many flags changed.
// @early-stop prologue: retail pushes esi up front and forms the flags
// pointer after the NULL test; assigning it after the test defers the push,
// initialising it at the declaration hoists the lea (direct field use,
// if-wrapped body and return-variable spellings tried).
RVA(0x000413f0, 0xb6)
i16 ApplyMoonPhase(Character* character, i16 keep) {
    u8* flags = GetCharacterFlags(character);
    i16 changed = 0;
    if (character == NULL) {
        return 0;
    }
    if (TestBit(flags, 0x24) == 1) {
        changed = 1;
        ClearBit(flags, 0x24);
    }
    if (TestBit(flags, 0x23) == 1) {
        changed++;
        ClearBit(flags, 0x23);
        SetBit(flags, 0x24);
    }
    if (keep == 0) {
        if (TestBit(flags, 0x25) == 1) {
            changed++;
            ClearBit(flags, 0x25);
        }
        if (TestBit(flags, 0x26) == 1) {
            changed++;
            ClearBit(flags, 0x26);
            AddCondition(GetCharacterConditions(character), CONDITION_ASH);
        }
    }
    return changed;
}

// Writes the field state LoadFieldState reads (the roster sorted first);
// nonzero on failure.
RVA(0x000414b0, 0xf4)
i16 WriteFieldState(FILE* fp) {
    i16 failed;
    Character** slot;
    i32 i;
    i16 id;
    SortRoster();
    failed = 1 - fwrite(&g_field, 0x10, 1, fp);
    failed |= 1 - fwrite(&g_savedDirection, 2, 1, fp);
    failed |= 6 - fwrite(g_party, 2, 6, fp);
    failed |= 1 - fwrite(&g_fieldStatus, 2, 1, fp);
    slot = g_roster;
    for (i = 32; i != 0; i--) {
        id = -1;
        if (*slot == NULL) {
            failed |= 1 - fwrite(&id, 2, 1, fp);
        } else {
            id = (*slot)->id;
            failed |= 1 - fwrite(&id, 2, 1, fp);
            if (id >= 0x20) {
                failed |= WriteCharacter(fp, *slot);
            }
        }
        slot++;
    }
    return failed;
}

// Returns nonzero when anything failed to write.
RVA(0x000415b0, 0x52)
i16 WriteCharacter(FILE* fp, Character* character) {
    i16 failed = 1 - fwrite(character, sizeof(Character), 1, fp);
    i16 count = GetWordCount(GetCharacterSkills(character));
    if (count > 0) {
        failed |= count - fwrite(GetWordArray(GetCharacterSkills(character)), 2, count, fp);
    }
    return failed;
}

// Writes the character count and all sixteen records; nonzero on failure.
RVA(0x00041610, 0x4e)
i16 WriteCharacters(FILE* fp) {
    i16 count = 16;
    i16 i;
    i16 failed = 1 - fwrite(&count, 2, 1, fp);
    for (i = 0; i < 16; i++) {
        failed |= WriteCharacter(fp, &g_characters[i]);
    }
    return failed;
}

// Reads the field state: the party's position, saved direction, party slots
// and field status, then the roster (each slot's id: a human's shared record,
// or a demon's own record read after it).
RVA(0x00041660, 0xec)
i16 LoadFieldState(FILE* fp) {
    i16 failed = 1 - fread(&g_field, 0x10, 1, fp);
    Character** slot;
    i32 i;
    i16 id;
    failed |= 1 - fread(&g_savedDirection, 2, 1, fp);
    failed |= 6 - fread(g_party, 2, 6, fp);
    failed |= 1 - fread(&g_fieldStatus, 2, 1, fp);
    slot = g_roster;
    for (i = 32; i != 0; i--) {
        *slot = FreeCharacterRecord(*slot);
        failed |= 1 - fread(&id, 2, 1, fp);
        if (id != -1) {
            if (id >= 0x20) {
                *slot = AllocCleared(1, sizeof(Character));
                failed |= LoadCharacter(fp, *slot);
            } else {
                *slot = FindCharacterById(id);
            }
        }
        slot++;
    }
    return failed;
}

static __inline void ClearLoadedNameTail(char* name, i16 size) {
    strcpy(g_scratchBuffer, name);
    memset(name, 0, size);
    strcpy(name, g_scratchBuffer);
}

// Reads a character record and its skill list (the runtime words cleared,
// the action speed recomputed and the names re-terminated).
RVA(0x00041750, 0x13f)
i16 LoadCharacter(FILE* fp, Character* character) {
    i16 failed = 1 - fread(character, sizeof(Character), 1, fp);
    i16 count = GetWordCount(GetCharacterSkills(character));
    character->clearedOnLoad[0] = 0;
    character->clearedOnLoad[1] = 0;
    character->skills.words = NULL;
    if (count > 0) {
        character->skills.words = AllocCleared(count, 2);
        failed |= count - fread(GetWordArray(GetCharacterSkills(character)), 2, count, fp);
        StripZeroWords(GetCharacterSkills(character));
    }
    character->actionSpeed = ComputeActionSpeed(character);
    ClearLoadedNameTail(character->namePrefix, sizeof(character->namePrefix));
    ClearLoadedNameTail(character->name, sizeof(character->name));
    return failed;
}

// Reads the character count and that many records (each normalised after).
RVA(0x00041890, 0x73)
i16 LoadCharacters(FILE* fp) {
    i16 count;
    i16 failed = 1 - fread(&count, 2, 1, fp);
    i16 i;
    for (i = 0; i < count; i++) {
        FreeWordList(GetCharacterSkills(&g_characters[i]));
        failed |= LoadCharacter(fp, &g_characters[i]);
        NormalizeAffiliations(&g_characters[i]);
    }
    return failed;
}
