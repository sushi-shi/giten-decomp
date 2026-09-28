// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/EquipEffect.h>
#include <Game/FieldObject.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/SaveGame.h>
#include <Game/Stats.h>
#include <Game/StatUpdate.h>
#include <Mem/Alloc.h>
#include <Util/WordList.h>

#include <stddef.h>
#include <string.h>

// Only the first word of this retail read-only span is referenced by code.
DATA(0x000648fc)
const i16 g_ammoCountIndex = -1;

DATA(0x000816a8)
static FieldObject s_characterLoadObject;

DATA(0x000818e8)
Character g_characters[16] = {0};

DATA(0x00083b4c)
static char s_emptyCharacterName[1];

RVA(0x0003c8b0, 0xa6)
void RecalcCharacterStats(Character* character) {
    RecalcEquippedStatTotals(&character->stats, GetCharacterEquipment(character));
    ApplyEquipmentEffects(character, EQUIP_EFFECT_STAT_UPDATE);
    character->pools.hp.max = CalcMaxHp(character);
    character->pools.mp.max = CalcMaxMp(character);
    if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
        UpdateStatTotals(&character->stats);
    }
    RecalcDerivedStats(character);
    ResetBattleStatsToBase(character);
    ApplyStatFlags(character);
    character->actionSpeed = ComputeActionSpeed(character);
}

// Recomputes the character's stats and fills its HP and MP.
RVA(0x0003c960, 0x2c)
void FullyRestoreCharacter(Character* character) {
    RecalcCharacterStats(character);
    character->pools.hp.cur = character->pools.hp.max;
    character->pools.mp.cur = character->pools.mp.max;
}

RVA(0x0003c990, 0x7d)
Character* CopyCharacterCore(Character* source, Character* destination) {
    if (!destination) {
        destination = AllocCleared(sizeof(Character), 1);
    } else {
        FreeWordList(GetCharacterSkills(destination));
    }
    memcpy(destination, source, offsetof(Character, alignmentA));
    destination->skills.words = CopyWordArray(
        GetWordArray(GetCharacterSkills(source)),
        GetWordCount(GetCharacterSkills(source))
    );
    destination->skills.count = GetWordCount(GetCharacterSkills(source));
    destination->actionSpeed = ComputeActionSpeed(destination);
    return destination;
}

RVA(0x0003ca10, 0x25)
Character* LoadCharacterCore(i16 id, Character* destination) {
    LoadObjectRecord(id, &s_characterLoadObject);
    // The object loader exposes a character prefix beginning at its kind.
    return CopyCharacterCore((Character*)&s_characterLoadObject.kind, destination);
}

static __inline void CopyCharacterWithoutSkills(Character* destination, const Character* source) {
    *destination = *source;
    InitEmptyWordList(&destination->skills);
}

RVA(0x0003ca40, 0x1b9)
void InitCharacters(void) {
    i16 i;
    for (i = 0; i < 16; i++) {
        InitEmptyWordList(&g_characters[i].skills);
    }
    InitCharacterSlot(0, 0, "\212\213\217\351", "\216\152\220\154", 1);
    InitCharacterSlot(1, 2, "\213\153", "\227\122\211\106\215\201", 2);
    InitCharacterSlot(2, 4, "\224\362\222\271", "\237\243", 2);
    InitCharacterSlot(3, 5, "\220\274\226\354", "\213\140\227\131", 1);
    InitCharacterSlot(4, 6, "\221\201\215\342", "\222\102\226\347", 1);
    InitCharacterSlot(5, 10, "\216\122\220\243", "\227\105", 1);
    InitCharacterSlot(6, 11, "\213\313\223\207", "\211\160\224\374", 2);
    InitCharacterSlot(7, 13, "\203\152\203\205\201\133\203\147\203\223", s_emptyCharacterName, 0);
    InitCharacterSlot(8, 14, "\216\122\223\143", "\203\112\203\131\203\176", 1);
    InitCharacterSlot(9, 15, "\227\247\220\354", "\221\171\214\265", 1);
    InitCharacterSlot(10, 7, "\211\200\223\143", "\223\116\226\347", 1);
    InitCharacterSlot(11, 8, "\217\343\211\315", "\214\366\213\120", 1);
    InitCharacterSlot(12, 9, "\221\212\224\156", "\216\117\216\154\230\131", 1);
    CopyCharacterWithoutSkills(&g_characters[13], &g_characters[0]);
    CopyCharacterWithoutSkills(&g_characters[14], &g_characters[0]);
    CopyCharacterWithoutSkills(&g_characters[15], &g_characters[0]);
}

RVA(0x0003cc00, 0x15c)
void InitCharacterSlot(i16 slot, i16 id, const char* prefix, const char* name, i16 memberClass) {
    LoadCharacterCore(id, &g_characters[slot]);
    NormalizeAffiliations(&g_characters[slot]);
    g_characters[slot].byte069 = memberClass;
    g_characters[slot].memberClass = memberClass;
    g_characters[slot].id = id;
    memset(g_characters[slot].namePrefix, 0, sizeof(g_characters[slot].namePrefix));
    memset(g_characters[slot].name, 0, sizeof(g_characters[slot].name));
    memset(g_characters[slot].ammoCounts, 0, sizeof(g_characters[slot].ammoCounts));
    memset(&g_characters[slot].returnPosition, 0, sizeof(g_characters[slot].returnPosition));
    memset(&g_characters[slot].markPosition, 0, sizeof(g_characters[slot].markPosition));
    strcpy(g_characters[slot].namePrefix, prefix);
    strcpy(g_characters[slot].name, name);
    FullyRestoreCharacter(&g_characters[slot]);
    g_characters[slot].unusedValue = 10000;
    g_characters[slot].unusedBytes[0] = 0;
    g_characters[slot].unusedBytes[1] = 1;
    InitAlignmentInfo(&g_characters[slot].alignmentA);
    InitAlignmentInfo(&g_characters[slot].alignmentB);
    StripZeroWords(GetCharacterSkills(&g_characters[slot]));
}

// Out-of-range slots fall back to the last record.
RVA(0x0003cd60, 0x29)
Character* GetCharacter(i16 slot) {
    if (slot < 0 || slot >= 16) {
        slot = 15;
    }
    return &g_characters[slot];
}

RVA(0x0003cd90, 0x1a)
i16 GetCharacterId(i16 slot) {
    Character* character = GetCharacter(slot);
    if (character == NULL) {
        return -1;
    }
    return character->id;
}

RVA(0x0003cdb0, 0x2d)
i16 FindCharacter(i16 id) {
    i16 slot;
    for (slot = 0; slot < 16; slot++) {
        if (g_characters[slot].id == id) {
            return slot;
        }
    }
    return -1;
}

RVA(0x0003cde0, 0x2b)
Character* FindCharacterById(i16 id) {
    i16 slot = FindCharacter(id);
    if (slot == -1) {
        return NULL;
    }
    return &g_characters[slot];
}

RVA(0x0003ce10, 0x6)
Character* GetCharacters(void) {
    return g_characters;
}

// Recomputes every roster member's stats from scratch: clears the modifiers,
// reapplies the equipment bonuses and effects, recalculates the totals and
// the derived stats, and shows the new battle stats.
// @identity-TODO: what condition 8 changes here (an extra total update) is
// unrecovered.
RVA(0x0003ce20, 0x91)
void ResetRosterStatModifiers(void) {
    i16 slot;
    Character* character;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character) {
            ClearStatModifiers(&character->stats);
            RecalcEquippedStatTotals(&character->stats, GetCharacterEquipment(character));
            ApplyEquipmentEffects(character, EQUIP_EFFECT_STAT_UPDATE);
            if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                UpdateStatTotals(&character->stats);
            }
            RecalcDerivedStats(character);
            memcpy(
                character->battleStatsShown,
                character->battleStats,
                sizeof(character->battleStats)
            );
        }
    }
}
