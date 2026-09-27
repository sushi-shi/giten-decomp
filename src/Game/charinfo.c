// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Game/PartyPick.h>
#include <Game/Stats.h>
#include <Mem/Alloc.h>
#include <Util/Range.h>

#include <stddef.h>
#include <string.h>

// Frees a non-human character's record (and its skill list); returns NULL.
RVA(0x0003f3c0, 0x2b)
Character* FreeCharacterRecord(Character* character) {
    if (character && !IsHumanCharacter(character)) {
        FreeWordList(GetCharacterSkills(character));
        FreeBlock(character);
    }
    return NULL;
}

// The roster slot of character `id`; with `inParty`, -1 unless it is in the
// party.
RVA(0x0003f3f0, 0x31)
i16 FindRosterSlotIn(i16 id, i16 inParty) {
    i16 slot = FindRosterSlotById(id);
    if (inParty && FindPartySlot(slot) == -1) {
        return -1;
    }
    return slot;
}

RVA(0x0003f430, 0x1c)
Character* GetRosterCharacterById(i16 id, i16 inParty) {
    return GetRosterCharacter(FindRosterSlotIn(id, inParty));
}

// An empty party position when `inParty`, else a free roster slot (-1: none).
RVA(0x0003f450, 0x1c)
i16 FindEmptySlot(i16 inParty) {
    if (inParty) {
        return FindPartySlot(-1);
    }
    return FindRosterSlotById(-1);
}

RVA(0x0003f470, 0xf)
i32 GetRankScore(Character* character) {
    return character->level * 10;
}

RVA(0x0003f480, 0x2e)
i16 CountRosterEntries(i16 all) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < 32; i++) {
        if (RosterMemberAt(i) != NULL && (all || !IsHumanCharacter(RosterMemberAt(i)))) {
            count++;
        }
    }
    return count;
}

RVA(0x0003f4b0, 0x59)
char* FormatFullName(char* buf, Character* character) {
    strcpy(buf, character->namePrefix);
    strcat(buf, character->name);
    return buf;
}

RVA(0x0003f510, 0x51)
i16 TickActionWait(ActionWait* wait, i16 speed) {
    u16 amount;
    if (!IsActionWaitPending(wait)) {
        return 0;
    }
    amount = RandomAverage(1, speed, 0) * 2 + 5;
    if (wait->remaining < amount) {
        wait->remaining = 0;
    } else {
        wait->remaining -= amount;
    }
    return IsActionWaitPending(wait);
}

// 1 when a condition keeps `character` from being picked, 2 while its field
// action wait runs, else 0.
RVA(0x0003f570, 0x50)
i16 TickPartyActionWaits(void) {
    i16 count = 0;
    i16 index;
    Character* actor;
    for (index = 0; index < 6; index++) {
        if (GetPartySlot(index) >= 0) {
            actor = GetPartyEntry(index);
            if (actor) {
                if (!TickActionWait(GetCharacterActionWait(actor), actor->actionSpeed)) {
                    CheckPickTarget(index);
                }
                count++;
            }
        }
    }
    return count;
}

RVA(0x0003f5c0, 0x30)
i16 GetPickState(Character* character) {
    if (GetPickBlockingCondition(GetCharacterConditions(character))) {
        return 1;
    }
    return IsActionWaitPending(GetCharacterActionWait(character)) ? 2 : 0;
}

// The first party member free to act (with `needMark`, also holding its field
// mark); -1 when none is.
RVA(0x0003f5f0, 0x4c)
i16 FindReadyMember(i16 needMark) {
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && !GetPickState(character)) {
            if (!needMark || IsActionWaitMarked(GetCharacterActionWait(character))) {
                return i;
            }
        }
    }
    return -1;
}

// Lists (count, then ids) up to `max` party members not blocked by a
// condition; with `idleOnly`, only those with no field mark or action wait.
// Allocates the list when NULL.
RVA(0x0003f640, 0x8e)
PartyMemberList* ListPickableMembers(PartyMemberList* list, i16 max, i16 idleOnly) {
    i16 i;
    Character* character;
    if (!list) {
        list = AllocCleared(1, max * sizeof(list->ids[0]) + offsetof(PartyMemberList, ids));
    }
    list->count = 0;
    for (i = 0; i < 6; i++) {
        if (list->count >= max) {
            break;
        }
        character = GetPartyCharacter(i);
        if (character && !GetPickBlockingCondition(GetCharacterConditions(character))) {
            if (!idleOnly
                || (!IsActionWaitPending(GetCharacterActionWait(character))
                    && !IsActionWaitMarked(GetCharacterActionWait(character)))) {
                list->ids[list->count++] = character->id;
            }
        }
    }
    return list;
}

// The first roster slot from `start` on that passes FilterPartyMember(`mode`)
// and whose HP (`pools` bit 0) or MP (bit 1) pool is in `state` (state 1 also
// takes an empty pool); -1 when none.
// @early-stop register residue: retail keeps the slot in edi and `state` in
// ebp, cl the reverse; the parameter as loop variable, an inline match helper
// and the permuter are flat or worse.
RVA(0x0003f6d0, 0xa6)
i16 FindMemberByPoolState(i16 start, i16 mode, i16 state, u8 pools) {
    i16 slot;
    Character* character;
    i16 pool;
    for (slot = start; slot < 32; slot++) {
        character = RosterMemberAt(slot);
        if (character && FilterPartyMember(slot, mode) != -1) {
            if (pools & POOL_MASK_HP) {
                pool = PoolState(&character->pools.hp);
                if (PoolStateMatches(pool, state)) {
                    return slot;
                }
            }
            if (pools & POOL_MASK_MP) {
                pool = PoolState(&character->pools.mp);
                if (PoolStateMatches(pool, state)) {
                    return slot;
                }
            }
        }
    }
    return -1;
}
