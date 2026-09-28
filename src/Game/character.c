// @identity-TODO: the owning TU is unproven. One retail object: the
// character records and pools, the stat, affinity and condition helpers, the
// status panels and lines, the party and the character save. character's
// statics open and close one .bss run (0x4816a8..0x483b4f) with the status
// draw, party, status line and save statics between them, each read only by
// its own part's code; the .rdata tables interleave (condition, character,
// status line); the action speed's floating-point constants lead the
// object's constant run; and the code is contiguous in .text.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Alignment.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/EquipEffect.h>
#include <Game/EquipMagicDefense.h>
#include <Game/EquipScreen.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldSupport.h>
#include <Game/GameState.h>
#include <Game/Guest.h>
#include <Game/InfoBar.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemEffect.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/PartyPick.h>
#include <Game/PartyStatus.h>
#include <Game/Pool.h>
#include <Game/SaveGame.h>
#include <Game/SkillUse.h>
#include <Game/Stats.h>
#include <Game/StatUpdate.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenLayer.h>
#include <Input/Mouse.h>
#include <Ints.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/Menu.h>
#include <Ui/PartySlotSelection.h>
#include <Util/BitSet.h>
#include <Util/CurMax.h>
#include <Util/Level.h>
#include <Util/Range.h>
#include <Util/Scratch.h>
#include <Util/WordList.h>

#include <math.h>
#include <memory.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// A status condition's bit index and its display name (Shift-JIS).
typedef struct ConditionName {
    u8 bit;
    char name[7];
} ConditionName;

// The fatal conditions (ash, dead, dying).
DATA(0x000646e8)
static const i16 s_fatalConditions[] = {CONDITION_ASH, CONDITION_DEAD, CONDITION_DYING, -1};

// @identity-TODO: the conditions GetPickBlockingCondition reports (the last one
// set wins); named from its caller in the party picker.
DATA(0x000646f0)
static const i16 s_pickBlockingConditions[] = {
    21,
    6,
    CONDITION_DOZE,
    CONDITION_SLEEP,
    10,
    3,
    CONDITION_DYING,
    CONDITION_DEAD,
    CONDITION_ASH,
    20,
    12,
    5,
    4,
    19,
    -1
};

// Conditions suppressed when an attack targets a field actor in field mode.
DATA(0x00064710)
static const i16 s_fieldRestrictedConditions[] = {
    CONDITION_ASH,
    CONDITION_DEAD,
    CONDITION_DYING,
    3,
    4,
    5,
    6,
    7,
    CONDITION_ZOMBIE,
    9,
    10,
    11,
    12,
    CONDITION_SLEEP,
    14,
    CONDITION_POISON,
    17,
    19,
    21,
    30,
    31,
    CONDITION_SEVERE_POISON,
    33,
    -1,
};

// @identity-TODO: the conditions GetDisablingCondition reports (the last one
// set wins), those cleared after a battle, those cleared when a member leaves
// the party, and every condition in display order.
DATA(0x00064740)
static const i16 s_disablingConditions[] =
    {CONDITION_DYING, CONDITION_DEAD, CONDITION_ASH, 3, 4, 5, 6, -1};

DATA(0x00064750)
static const i16 s_battleConditions[] = {17, 20, 21, 22, CONDITION_DOZE, 26, 27, 28, -1};

DATA(0x00064768)
static const i16 s_leaveConditions[] = {
    10,
    11,
    CONDITION_SLEEP,
    14,
    16,
    18,
    19,
    20,
    21,
    22,
    23,
    CONDITION_DOZE,
    26,
    27,
    28,
    29,
    31,
    -1,
};

DATA(0x00064790)
static const i16 s_allConditions[] = {
    CONDITION_ASH,
    CONDITION_DEAD,
    CONDITION_DYING,
    3,
    4,
    5,
    6,
    7,
    CONDITION_ZOMBIE,
    9,
    10,
    11,
    12,
    CONDITION_SLEEP,
    14,
    CONDITION_SEVERE_POISON,
    CONDITION_POISON,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    CONDITION_DOZE,
    26,
    27,
    30,
    29,
    28,
    31,
    33,
    34,
    -1,
};

// Shown when no condition name applies: three full-width spaces.
DATA(0x000647d8)
static char* const s_noConditionName = "\201@\201@\201@";

DATA(0x000647e0)
static const ConditionName s_conditionNames[] = {
    {CONDITION_ASH, "\212D"},                      // 灰
    {CONDITION_DEAD, "\216\200"},                  // 死
    {CONDITION_DYING, "\225m\216\200"},            // 瀕死
    {3, "\215\250\223|"},                          // 昏倒
    {4, "\220\316\211\273"},                       // 石化
    {5, "\226\203\341\203"},                       // 麻痺
    {6, "\223\200\214\213"},                       // 凍結
    {7, "\234\337\210\313"},                       // 憑依
    {CONDITION_ZOMBIE, "\203]\203\223\203r"},      // ゾンビ
    {9, "\216\364\202\242"},                       // 呪い
    {10, "\213C\220\342"},                         // 気絶
    {11, "\222\202\221\247"},                      // 窒息
    {12, "\213\326\224\233"},                      // 禁縛
    {CONDITION_SLEEP, "\226\260\202\350"},         // 眠り
    {14, "\213\260\215Q"},                         // 恐慌
    {CONDITION_SEVERE_POISON, "\226\322\223\305"}, // 猛毒
    {CONDITION_POISON, "\223\305"},                // 毒
    {16, "\214\266\212o"},                         // 幻覚
    {17, "\226\243\227\271"},                      // 魅了
    {18, "\215\254\227\220"},                      // 混乱
    {19, "\225\221\223\245"},                      // 舞踏
    {20, "\212\264\223d"},                         // 感電
    {21, "\225X\214\213"},                         // 氷結
    {22, "\211\212\217\343"},                      // 炎上
    {23, "\226\323\226\332"},                      // 盲目
    {24, "\225\225\226\202"},                      // 封魔
    {CONDITION_DOZE, "\213\217\226\260\202\350"},  // 居眠り
    {26, "\213\266\220\355\216m"},                 // 狂戦士
    {27, "\203n\203C"},                            // ハイ
    {30, "\223D\220\214"},                         // 泥酔
    {29, "\202\331\202\353\220\214"},              // ほろ酔
    {28, "\215K\225\237"},                         // 幸福
    {31, "\275\327\262\321"},                      // ｽﾗｲﾑ
    {33, "\213z\214\214"},                         // 吸血
    {34, "\212O\217\235"},                         // 外傷
};

// Only the first word of this retail read-only span is referenced by code.
DATA(0x000648fc)
const i16 g_ammoCountIndex = -1;

// "CNL" / "DNL": the letters for alignment classes -1/0/1 on the two axes.
DATA(0x00064910)
static const char s_alignmentLetters[2][3] = {{'C', 'N', 'L'}, {'D', 'N', 'L'}};

DATA(0x00069ee0)
static i16 s_affinity[5][3] = {
    {3, 0, 0},
    {2, 1, 0},
    {1, 0, 1},
    {0, 1, 2},
    {0, 0, 3},
};

DATA(0x00069f00)
static i16 s_selectedPartySlot = -1;

// Each condition's base chance (of 256) to wear off per roll; 0 never does.
DATA(0x00069f08)
static i16 s_recoveryChance[35] = {
    0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  15, 40, 30, 40, 40, 0, 60, 10,
    50, 50, 128, 60, 50, 50, 10, 80, 10, 70, 80, 50, 10, 0,  0,  0, 0,
};

DATA(0x000816a8)
static FieldObject s_characterLoadObject = {0};

DATA(0x000818e8)
Character g_characters[16] = {0};

// The roster slots the status screen lists and how many there are.
DATA(0x00083ad8)
i16 g_statusSlots[32] = {0};

// @identity-TODO: while set, the party-status redraw requests are ignored.
DATA(0x00083b18)
static i16 s_statusRedrawLocked = 0;

DATA(0x00083b1c)
static b16 s_statusRedrawPending = false;

DATA(0x00083b20)
Character* g_panelMembers[6] = {0};

// Per character group, the 40-bit set of items the group can equip.
DATA(0x00083b38)
static i32 s_equipTable = 0;

DATA(0x00083b3c)
static MenuBox* s_statusListMenu = 0;

// Which of the five status-line columns the character status line shows.
DATA(0x00083b40)
static i16 s_statusColumn = 0;

DATA(0x00083b44)
i16 g_statusSlotCount = 0;

// The minutes toward the next party-timer tick.
DATA(0x00083b48)
static u32 s_timerMinutes = 0;

DATA(0x00083b4c)
static char s_emptyCharacterName[1] = "";

// Action speed from agility, reduced by armor defense scaled by vitality;
// never below 1.
RVA(0x0003c810, 0x97)
i16 ComputeActionSpeed(Character* character) {
    double base = sqrt(GetStatTotal(character, STAT_AGILITY)) * 4.0;
    double burden = (u16)SumArmorDefenseBonus(character);
    double root = sqrt(GetStatTotal(character, STAT_VITALITY));
    u16 speed;
    if (root < 1.0) {
        root = 1.0;
    }
    burden /= root;
    burden *= 0.25;
    base -= burden;
    speed = RoundToShort(base);
    if (speed < 1) {
        speed = 1;
    }
    return speed;
}

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

// Maximum HP: level * vitality / 2 + protection and fortune + 5, doubled by personal
// flag 37 and again by flag 38, at most 9999.
RVA(0x0003cec0, 0xa3)
i32 CalcMaxHp(Character* character) {
    double value = character->level;
    value *= character->stats.total[6];
    value *= 0.5;
    value += character->stats.total[4] + character->stats.total[10] + 5;
    if (TestBit(character->personalFlags, 37) == 1) {
        value += value;
    }
    if (TestBit(character->personalFlags, 38) == 1) {
        value += value;
    }
    if (value > 9999.0) {
        value = 9999.0;
    }
    return (i32)value;
}

// Maximum MP: (charm and mental strength) / 2 + sqrt(level) * magic * 1.5, doubled by
// personal flag 38, at most 999.
RVA(0x0003cf70, 0x8c)
i32 CalcMaxMp(Character* character) {
    double scaled = sqrt(character->level);
    double value = GetStatTotal(character, 9) + GetStatTotal(character, 1);
    value *= 0.5;
    scaled *= GetStatTotal(character, 2);
    scaled *= 1.5;
    value += scaled;
    if (TestCharacterFlag(character, 38) == 1) {
        value += value;
    }
    if (value > 999.0) {
        value = 999.0;
    }
    return (i32)value;
}

// Codegen constraint: the operand order of CalcWeaponPowerStat, CalcMagicAccuracyStat,
// CalcMagicEvasionStat and ScalePercent999 shifts with the number of declarations
// this unit sees; memset comes from <memory.h> (<string.h> breaks
// CalcWeaponPowerStat), and a declaration added to an included header needs a
// recheck of all four.
// Bit 0: the first pool is below half; bit 1: the second one is.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003d000, 0x25)
i16 LowPoolMask(CharacterPools* pools) {
    i16 mask = pools->hp.cur < pools->hp.max / 2;
    mask |= pools->mp.cur < pools->mp.max / 2 ? POOL_MASK_MP : POOL_MASK_NONE;
    return mask;
}

// 2: full; 1: partly spent; 0: empty.
RVA(0x0003d030, 0x1d)
i16 PoolState(CurMax* pool) {
    if (pool->max <= pool->cur) {
        return 2;
    }
    return pool->cur > 0;
}

// Bit 0: the first pool is empty; bit 1: the second one is.
RVA(0x0003d050, 0x1d)
i16 EmptyPoolMask(CharacterPools* pools) {
    i16 mask = pools->hp.cur == 0;
    mask |= pools->mp.cur == 0 ? POOL_MASK_MP : POOL_MASK_NONE;
    return mask;
}

RVA(0x0003d070, 0x1e)
void DrainPool(CurMax* pool, i32 amount) {
    SubCapped(&pool->cur, ClampToUShort(amount), 0);
}

// Refills a pool: mode 0 up to its maximum, 1 without a cap, 2 up to twice
// its maximum; other modes do nothing and return 0.
RVA(0x0003d090, 0x97)
i16 FillPool(CurMax* pool, i32 amount, GZ_ENUM_STORAGE(PoolFillMode, i16) mode) {
    u16 limit;
    switch (mode) {
        case POOL_FILL_TO_MAX:
            AddCapped(&pool->cur, ClampToUShort(amount), pool->max);
            break;
        case POOL_FILL_TO_USHORT_MAX:
            AddCapped(&pool->cur, ClampToUShort(amount), 0xffff);
            break;
        case POOL_FILL_TO_DOUBLE_MAX:
            limit = pool->max;
            AddCapped(&limit, pool->max, 0xffff);
            AddCapped(&pool->cur, ClampToUShort(amount), limit);
            break;
        default:
            return 0;
    }
}

// Drains a negative amount, refills a positive one up to the maximum.
RVA(0x0003d130, 0x2a)
void ChangePool(CurMax* pool, i32 amount) {
    if (amount < 0) {
        DrainPool(pool, -amount);
    } else {
        FillPool(pool, amount, POOL_FILL_TO_MAX);
    }
}

RVA(0x0003d160, 0x30)
i32 ClampTo999(i16 value) {
    return max(min(value, 999), 1);
}

// Derived values built step by step from the eleven stats, rounded and
// clamped to 1..999. Codegen constraint: each is accumulated in one double through
// separate compound statements; single expressions schedule differently.
RVA(0x0003d190, 0x54)
i32 StatBlend80(i16* stats, i16 level) {
    double value = stats[STAT_DEXTERITY] * 0.5;
    value -= stats[STAT_INTUITION] * -0.25;
    value *= sqrt(level);
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d1f0, 0x4f)
i32 StatBlend57(i16* stats, i16 level) {
    double value = stats[STAT_STRENGTH] * 0.25;
    value *= sqrt(level);
    value += stats[STAT_AGILITY];
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d240, 0x75)
i32 CalcPhysicalDefenseStat(i16* stats, i16 bonus) {
    double value = stats[STAT_PROTECTION] * 0.2;
    value -= stats[STAT_STRENGTH] * -0.25;
    value -= stats[STAT_MENTAL_STRENGTH] * -0.1;
    value += bonus;
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d2c0, 0x7b)
i32 CalcGunAccuracyStat(i16* stats, i16 bonus, i16 extra) {
    double value = stats[STAT_DEXTERITY];
    value -= stats[STAT_INTUITION] * -0.2;
    value += bonus;
    if (extra > 0) {
        value += extra * 2 + 10;
    }
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d340, 0x41)
i32 CalcGunEvasionStat(i16* stats) {
    double value = stats[STAT_PROTECTION] + stats[STAT_INTUITION];
    value *= 0.4;
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d390, 0x5b)
i32 CalcWeaponAccuracyStat(i16* stats, i16 bonus) {
    double value = stats[STAT_AGILITY];
    value *= 0.5;
    value += bonus;
    value += stats[STAT_STRENGTH];
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d3f0, 0x80)
i32 CalcWeaponPowerStat(i16* stats, i16 bonus, i16 level, i16 penalty) {
    double value = stats[5];
    value += stats[6];
    value *= 0.2;
    value += bonus;
    if (level >= 0x20) {
        value -= penalty * -0.15;
    }
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d470, 0x5a)
i32 CalcWeaponEvasionStat(i16* stats, i16 bonus) {
    double value = stats[STAT_INTUITION];
    value *= 0.4;
    value += stats[STAT_AGILITY];
    value += bonus;
    return ClampTo999(RoundToShort(value));
}

// Derived values from pairs of an eleven-stat array, clamped to 1..999. CalcMagicAccuracyStat
// and CalcMagicEvasionStat ignore the item bonus RecalcDerivedStats passes them.
// @early-stop TU state: the same body alone in a probe TU loads stats[STAT_INTELLIGENCE] first
// as retail does; in this unit cl loads stats[STAT_MAGIC] first (commutative order).
RVA(0x0003d4d0, 0x1e)
i32 CalcMagicAccuracyStat(i16* stats, i16 bonus) {
    i16 sum = stats[3] + stats[2];
    return ClampTo999(sum / 2);
}

RVA(0x0003d4f0, 0x17)
i32 CalcMagicPowerStat(i16* stats, i16 amount) {
    return ClampTo999(stats[STAT_MAGIC] + amount);
}

RVA(0x0003d510, 0x1e)
i32 CalcMagicEvasionStat(i16* stats, i16 bonus) {
    i16 sum = stats[4] + stats[3];
    return ClampTo999(sum / 2);
}

RVA(0x0003d530, 0x24)
i32 CalcMagicDefenseStat(i16* stats, i32 amount) {
    i16 sum = stats[STAT_MENTAL_STRENGTH] + stats[STAT_PROTECTION];
    i32 average = sum / 2;
    return ClampTo999(average + amount);
}

// Recomputes a character's 24 battle stats from its total stats and the
// bonuses of its eight item slots (each slot's item, then its indexed gem
// item); stats 0, 6, 12 and 18 are kept, 13 and 19..23 are set to 1.
// @identity-TODO: which values the indices name is unrecovered.
RVA(0x0003d560, 0x3e0)
void RecalcDerivedStats(Character* character) {
    i16 kept12 = GetBattleStatBase(character, 12);
    i16 kept18 = GetBattleStatBase(character, 18);
    i16 level = GetBattleStatBase(character, 0);
    i16 kept6 = GetBattleStatBase(character, 6);
    i16 bonuses[24];
    i16* stats;

    memset(bonuses, 0, sizeof(bonuses));
    AddItemStatBonuses(GetCharacterEquipment(character)[0].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[5].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[6].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[7].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[0].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[5].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[6].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[7].attachment, bonuses, 1);

    stats = character->stats.total;
    character->battleStats[0] = level;
    character->battleStats[1] = StatBlend57(stats, level);
    character->battleStats[BATTLE_STAT_WEAPON_ACCURACY] =
        CalcWeaponAccuracyStat(stats, bonuses[BATTLE_STAT_WEAPON_ACCURACY]);
    character->battleStats[BATTLE_STAT_WEAPON_POWER] =
        CalcWeaponPowerStat(stats, bonuses[BATTLE_STAT_WEAPON_POWER], character->id, level);
    character->battleStats[BATTLE_STAT_WEAPON_EVASION] =
        CalcWeaponEvasionStat(stats, bonuses[BATTLE_STAT_WEAPON_EVASION]);
    character->battleStats[BATTLE_STAT_WEAPON_DEFENSE] =
        CalcPhysicalDefenseStat(stats, bonuses[BATTLE_STAT_WEAPON_DEFENSE]);
    character->battleStats[6] = kept6;
    character->battleStats[7] = StatBlend80(stats, kept6);
    character->battleStats[BATTLE_STAT_GUN_ACCURACY] = CalcGunAccuracyStat(
        stats,
        bonuses[BATTLE_STAT_GUN_ACCURACY],
        GetBattleStatBase(character, 6)
    );
    character->battleStats[BATTLE_STAT_GUN_POWER] = ClampTo999(bonuses[BATTLE_STAT_GUN_POWER]);
    character->battleStats[BATTLE_STAT_GUN_EVASION] = CalcGunEvasionStat(stats);
    character->battleStats[BATTLE_STAT_GUN_DEFENSE] =
        GetBattleStatBase(character, BATTLE_STAT_WEAPON_DEFENSE);
    character->battleStats[12] = kept12;
    character->battleStats[13] = 1;
    character->battleStats[BATTLE_STAT_MAGIC_POWER] =
        CalcMagicPowerStat(stats, GetItemMagicPowerBonus(GetCharacterEquipment(character)[5].item));
    character->battleStats[BATTLE_STAT_MAGIC_ACCURACY] = CalcMagicAccuracyStat(
        stats,
        GetItemMagicAccuracyBonus(GetCharacterEquipment(character)[5].item)
    );
    character->battleStats[BATTLE_STAT_MAGIC_EVASION] = CalcMagicEvasionStat(
        stats,
        GetItemPhysicalEvasionBonus(GetCharacterEquipment(character)[4].item)
    );
    character->battleStats[BATTLE_STAT_MAGIC_DEFENSE] =
        CalcMagicDefenseStat(stats, SumEquippedMagicDefenseBonus(character, 3));
    character->battleStats[18] = kept18;
    character->battleStats[19] = 1;
    character->battleStats[21] = 1;
    character->battleStats[20] = 1;
    character->battleStats[22] = 1;
    character->battleStats[23] = 1;
}

// The physical-defense bonuses of armor slots 0..4 and their attached gems.
RVA(0x0003d940, 0x150)
i16 SumArmorDefenseBonus(Character* character) {
    i16 bonuses[24];

    memset(bonuses, 0, sizeof(bonuses));
    AddItemStatBonuses(GetCharacterEquipment(character)[0].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[0].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].attachment, bonuses, 1);
    return bonuses[BATTLE_STAT_WEAPON_DEFENSE];
}

// Scales the shown battle stats by the member's personal flags: flag 36
// halves stats 2, 4, 8 and 10, flag 35 raises them by half, flag 37 raises
// stats 3 and 5 by half.
// @identity-TODO: which stats the indices name and what the flags stand for
// are unrecovered.
RVA(0x0003da90, 0x14b)
void ApplyStatFlags(Character* character) {
    if (TestCharacterFlag(character, 0x24) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_ACCURACY), 50);
        character->battleStatsShown[BATTLE_STAT_WEAPON_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_EVASION), 50);
        character->battleStatsShown[BATTLE_STAT_GUN_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_ACCURACY), 50);
        character->battleStatsShown[BATTLE_STAT_GUN_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_EVASION), 50);
    }
    if (TestCharacterFlag(character, 0x23) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_ACCURACY), 150);
        character->battleStatsShown[BATTLE_STAT_WEAPON_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_EVASION), 150);
        character->battleStatsShown[BATTLE_STAT_GUN_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_ACCURACY), 150);
        character->battleStatsShown[BATTLE_STAT_GUN_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_EVASION), 150);
    }
    if (TestCharacterFlag(character, 0x25) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_POWER] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_POWER), 150);
        character->battleStatsShown[BATTLE_STAT_WEAPON_DEFENSE] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_DEFENSE), 150);
    }
}

RVA(0x0003dbe0, 0x28)
i32 ScalePercent999(i16 value, i16 percent) {
    return ClampTo999(value * percent / 100);
}

RVA(0x0003dc10, 0x2e)
i32 ClampTo100(i16 value) {
    return max(min(value, 100), 1);
}

RVA(0x0003dc40, 0x1a)
i32 ClampSum100(i16 a, i16 b, i16 c) {
    return ClampTo100(c + b + a);
}

// Recomputes each stat's total (base, equipment and bonus, modifiers; 1..100);
// returns how many changed.
RVA(0x0003dc60, 0x43)
i16 RecalcStatTotals(StatBlock* stats) {
    i16 changed = 0;
    i16 i;
    i16 total;
    for (i = 0; i < 11; i++) {
        total =
            ClampSum100(stats->base[i], stats->equipment[i] + stats->bonus[i], stats->modifiers[i]);
        if (total != stats->total[i]) {
            changed++;
        }
        stats->total[i] = total;
    }
    return changed;
}

// Clears the modifiers and returns how many were set.
RVA(0x0003dcb0, 0x21)
i16 ClearStatModifiers(StatBlock* stats) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < 11; i++) {
        if (stats->modifiers[i] != 0) {
            count++;
        }
        stats->modifiers[i] = 0;
    }
    return count;
}

// Clears `stats`' bonus and equipment arrays, then adds the stat points of
// each slot's item to the bonus array and those of each slot's indexed gem
// item to the equipment array.
RVA(0x0003dce0, 0x1c0)
b16 ApplyItemStatBonuses(StatBlock* stats, ItemSlot* slots) {
    i16 i;

    for (i = 0; i < 11; i++) {
        stats->bonus[i] = 0;
        stats->equipment[i] = 0;
    }
    AddItemStatPoints(slots[0].item, stats->bonus);
    AddItemStatPoints(slots[1].item, stats->bonus);
    AddItemStatPoints(slots[2].item, stats->bonus);
    AddItemStatPoints(slots[3].item, stats->bonus);
    AddItemStatPoints(slots[4].item, stats->bonus);
    AddItemStatPoints(slots[5].item, stats->bonus);
    AddItemStatPoints(slots[6].item, stats->bonus);
    AddItemStatPoints(slots[7].item, stats->bonus);
    AddItemStatPoints(GemItemId(slots[0].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[1].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[2].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[3].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[4].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[5].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[6].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[7].attachment), stats->equipment);
    return false;
}

RVA(0x0003dea0, 0x20)
i16 GemItemId(i16 index) {
    if (index == -1) {
        return -1;
    }
    return index + GetGemItemBase();
}

RVA(0x0003dec0, 0x14)
i32 ClampLevel(i16 level) {
    return min(level, 99);
}

RVA(0x0003dee0, 0x3b)
void UpdateStatTotals(StatBlock* stats) {
    i16 i;
    i16 sum;
    for (i = 0; i < 11; i++) {
        sum = stats->base[i] + stats->bonus[i] + stats->equipment[i] + stats->modifiers[i];
        stats->total[i] = ClampTo100(sum / 2);
    }
}

RVA(0x0003df20, 0x50)
i16 GetAlignmentAffinity(AlignmentInfo* info, i16 side) {
    i16 sum = info->last1 + info->last0;
    if (sum == 0 && info->last0 == 0 && side == 0) {
        return 2;
    }
    sum += 2;
    side++;
    return s_affinity[sum][side];
}

// Moves the alignment by the side's affinity times `weight` (a neutral move
// pulls it toward 0 by at most its size) and records the side; returns the
// new alignment class (or, for an unknown side, the unchanged one).
// @early-stop register residue: retail inserts `last0 = side` through the
// side's own register (xor bl,cl; and ebx,3) where this build uses eax;
// everything before the final insert matches.
RVA(0x0003df70, 0x110)
i16 MoveAlignment(AlignmentInfo* info, i16 weight, i16 side) {
    i16 delta = GetAlignmentAffinity(info, side) * weight;
    switch (side) {
        case -1:
            info->negativeMoves++;
            delta = -delta;
            break;
        case 0:
            info->neutralMoves++;
            if (delta > abs(info->value)) {
                delta = abs(info->value);
            }
            if (info->value > 0) {
                delta = -delta;
            }
            break;
        case 1:
            info->positiveMoves++;
            break;
        default:
            return AlignmentClass(info->value);
    }
    info->value += delta;
    info->value = ClampShort(info->value, -127, 127);
    info->last7 = info->last6;
    info->last6 = info->last5;
    info->last5 = info->last4;
    info->last4 = info->last3;
    info->last3 = info->last2;
    info->last2 = info->last1;
    info->last1 = info->last0;
    info->last0 = side;
    return AlignmentClass(info->value);
}

// -1 at or below -42, 1 at or above 42, 0 between.
RVA(0x0003e080, 0x1d)
i16 AlignmentClass(i16 value) {
    if (value <= -42) {
        return -1;
    }
    return value >= 42;
}

RVA(0x0003e0a0, 0x27)
void ShiftAlignmentB(Character* character, i16 amount, i16 step) {
    AlignmentInfo* alignment = &character->alignmentB;
    MoveAlignment(alignment, amount, step);
    character->alignmentLevelB = GetAlignmentValue(alignment);
}

RVA(0x0003e0d0, 0x27)
void ShiftAlignmentA(Character* character, i16 amount, i16 step) {
    AlignmentInfo* alignment = &character->alignmentA;
    MoveAlignment(alignment, amount, step);
    character->alignmentLevelA = GetAlignmentValue(alignment);
}

// The alignment class of the party's average alignment on one axis (0 reads
// `alignmentB`, any other `alignmentA`); 0 with nobody in the party.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003e100, 0x60)
i16 PartyAlignmentClass(i16 axis) {
    i32 sum = 0;
    i16 count = 0;
    i16 i;
    Character* member;
    for (i = 0; i < 6; i++) {
        member = GetPartyEntry(i);
        if (member != NULL) {
            if (axis == 0) {
                sum += GetAlignmentValue(&member->alignmentB);
            } else {
                sum += GetAlignmentValue(&member->alignmentA);
            }
            count++;
        }
    }
    if (count != 0) {
        return AlignmentClass(sum / count);
    }
    return 0;
}

static __inline i32 GetSelectedPartySlot(void) {
    i16 slot;
    if (s_selectedPartySlot == -1) {
        slot = -1;
    } else {
        slot = s_selectedPartySlot;
    }
    return slot;
}

static __inline void CommitPartySlotSelection(void) {
    g_hoveredObjectId = GetSelectedPartySlot();
    g_selectedObjectId = g_hoveredObjectId;
}

RVA(0x0003e160, 0x85)
void RedrawPartyStatus(void) {
    i16 slot;
    i16 swapped;
    ClearTextPlane(g_infoPlane);
    for (slot = 0; slot < 6; slot++) {
        if (PartySlotAt(slot) == -1) {
            DrawPartyStatusSlot(slot, NULL);
        } else {
            swapped = GetSwappedMember(slot);
            if (swapped == -1) {
                DrawPartyStatusSlot(slot, NULL);
            } else if (swapped == -2) {
                DrawPartyStatusSlot(slot, GetPartyEntry(slot));
            } else {
                DrawPartyStatusSlot(slot, GetRosterCharacter(swapped));
            }
        }
    }
    ResetTextPlaneHighlight(g_infoPlane);
    s_selectedPartySlot = -1;
    s_statusRedrawPending = false;
}

RVA(0x0003e1f0, 0x164)
void DrawPartyStatusSlot(i16 slot, Character* character) {
    char name[36];
    i16 partySlot = slot;
    slot += 8;
    if (character == NULL) {
        g_panelMembers[partySlot] = NULL;
        ClearLayerSurface(slot);
        HideScreenLayer(slot);
        return;
    }
    g_panelMembers[partySlot] = character;
    ShowScreenLayer(slot);
    sprintf(g_scratchBuffer, "%-16s", FormatFullName(name, character));
    DrawLayerText(slot, 8, 8, g_scratchBuffer, 0x3450);
    sprintf(g_scratchBuffer, "%5d", character->pools.hp.cur);
    DrawLayerText(slot, 144, 24, g_scratchBuffer, 0x9450);
    sprintf(g_scratchBuffer, "%5d", character->pools.mp.cur);
    DrawLayerText(slot, 144, 40, g_scratchBuffer, 0x9450);
    DrawLayerGauge(slot, character->pools.hp.cur, character->pools.hp.max, 1);
    DrawLayerGauge(slot, character->pools.mp.cur, character->pools.mp.max, 0);
    sprintf(g_scratchBuffer, "%6s", GetFirstConditionName(GetCharacterConditions(character)));
    DrawLayerText(slot, 8, 44, g_scratchBuffer, 0x1450);
}

static __inline void RefreshPartyStatusIfNeeded(i16 force) {
    if (!s_statusRedrawLocked) {
        if (force || s_statusRedrawPending) {
            RedrawPartyStatus();
        }
        s_statusRedrawPending = false;
    }
}

RVA(0x0003e360, 0x2b)
void FlushStatusRedraw(i16 force) {
    RefreshPartyStatusIfNeeded(force);
}

RVA(0x0003e390, 0x2b)
void RefreshStatusPanel(i16 force) {
    RefreshPartyStatusIfNeeded(force);
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
// @early-stop: column and selection mode exchange ebx/edi; calls and semantic
// edges match. A separate filter result regresses both callers; C-safe TU states
// retain the register exchange, with one also extending the line-step lifetime.
RVA(0x0003e3c0, 0x1c1)
i16 PollTextPartySlotSelection(i16 mode) {
    i16 oldStep = ResetTextPlaneLineStep(g_infoPlane, 3);
    i16 x;
    i16 y;
    i16 column;
    i16 lineStep;
    i16 slot;
    SetTextPlaneHighlightMode(0, TEXT_HIGHLIGHT_OUTER);
    if (g_mouseLeftClick) {
        CommitPartySlotSelection();
        ClearMouseClicks();
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return 1;
    }
    if (g_mouseRightClick) {
        ClearMouseSelection();
        ClearMouseClicks();
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return -1;
    }
    column = TextPlaneCellAt(g_infoPlane, g_mousePosition.x, g_mousePosition.y, &x, &y);
    if (column == 0 || column == 39) {
        lineStep = GetTextPlaneLineStep(g_infoPlane);
        slot = y / lineStep + (column ? 3 : 0);
        slot = FilterPartySlotSelection(slot, mode);
    } else {
        slot = -1;
    }
    if (s_selectedPartySlot == slot) {
        ResetTextPlaneLineStep(g_infoPlane, oldStep);
        return 0;
    }
    s_selectedPartySlot = slot;
    ClearTextPlaneHighlight(g_infoPlane);
    if (slot != -1) {
        SetTextPlaneHighlight(g_infoPlane, column, y);
    }
    ResetTextPlaneLineStep(g_infoPlane, oldStep);
    return 0;
}

RVA(0x0003e590, 0xc9)
i16 PollPartySlotSelection(i16 mode) {
    i16 slot;
    if (g_mouseLeftClick) {
        CommitPartySlotSelection();
        ClearMouseClicks();
        return 1;
    }
    if (g_mouseRightClick) {
        ClearMouseSelection();
        ClearMouseClicks();
        return -1;
    }
    slot = PartyPanelAtPoint(g_mousePosition.x, g_mousePosition.y);
    slot = FilterPartySlotSelection(slot, mode);
    s_selectedPartySlot = slot;
    return 0;
}

RVA(0x0003e660, 0x19)
void ClearPartySlotSelection(void) {
    s_selectedPartySlot = -1;
    ClearTextPlaneHighlight(g_infoPlane);
}

RVA(0x0003e680, 0x12)
i16 LockStatusRedraw(i16 lock) {
    i16 prev = s_statusRedrawLocked;
    s_statusRedrawLocked = lock;
    return prev;
}

RVA(0x0003e6a0, 0xa)
void RequestStatusRedraw(void) {
    s_statusRedrawPending = true;
}

RVA(0x0003e6b0, 0x13)
b16 HasCondition(ConditionSet* conditions, i16 condition) {
    return TestBit(conditions->bits, condition);
}

#define AccumulateCollapseOrPetrification(blocked, conditions)                                     \
    do {                                                                                           \
        (blocked) |= HasCondition((conditions), 3);                                                \
        (blocked) |= HasCondition((conditions), 4);                                                \
    } while (0)

// The last condition of `list` that is set, or 0.
// Adds `condition` to a condition set under the conditions' precedence rules:
// returns 1 when added (its byte reset), -1 when already held, 0 when a
// fatal or overriding condition blocks it, and 2 when frozen and burning only
// cancel each other. Doze escalates to sleep, poison to severe poison,
// ice-bound to frozen and tipsy to drunk when already held.
// @early-stop tail merge: the eight `return 0` exits of the four escalating
// cases (15/21/25/29) jump to the shared return of cases 33/34 in retail but to
// case 5's here; instructions, calls and branch counts are identical. Loop
// form (for/goto), nested-if returns, case 33/34 spelling and order were tried.
RVA(0x0003e6d0, 0x700)
i16 AddCondition(ConditionSet* conditions, i16 condition) {
    i16 blocked;
    i16 i;

    HasCondition(conditions, CONDITION_ZOMBIE);
    for (;;) {
        blocked = 0;
        switch (condition) {
            case CONDITION_POISON:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                blocked |= HasCondition(conditions, CONDITION_SEVERE_POISON);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, CONDITION_POISON)) {
                    break;
                }
                condition = CONDITION_SEVERE_POISON;
                continue;
            case 21:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                blocked |= HasCondition(conditions, 6);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, 21)) {
                    break;
                }
                condition = 6;
                continue;
            case CONDITION_DOZE:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, CONDITION_SLEEP);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, CONDITION_DOZE)) {
                    break;
                }
                condition = CONDITION_SLEEP;
                continue;
            case 29:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 30);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, 29)) {
                    break;
                }
                condition = 30;
                continue;
            case CONDITION_ASH:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                for (i = 0; i < 35; i++) {
                    ClearCondition(conditions, i);
                }
                break;
            case CONDITION_DEAD:
            case CONDITION_DYING:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                for (i = 0; i < 35; i++) {
                    ClearCondition(conditions, i);
                }
                break;
            case 3:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                if (HasCondition(conditions, 26)) {
                    return 0;
                }
                ClearCondition(conditions, 10);
                ClearCondition(conditions, CONDITION_SLEEP);
                ClearCondition(conditions, 18);
                ClearCondition(conditions, 20);
                ClearCondition(conditions, CONDITION_DOZE);
                ClearCondition(conditions, 27);
                ClearCondition(conditions, 28);
                ClearCondition(conditions, 29);
                ClearCondition(conditions, 30);
                break;
            case 4:
            case 9:
            case 24:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                break;
            case 5:
            case 12:
            case 17:
            case 19:
            case 20:
            case 28:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                break;
            case 26:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 5);
                blocked |= HasCondition(conditions, 7);
                blocked |= HasCondition(conditions, CONDITION_ZOMBIE);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, CONDITION_SLEEP);
                blocked |= HasCondition(conditions, CONDITION_DOZE);
                if (blocked) {
                    return 0;
                }
                break;
            case 30:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, 29);
                break;
            case CONDITION_SEVERE_POISON:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_POISON);
                break;
            case 33:
            case 34:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                break;
            case 6:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                if (HasCondition(conditions, 22)) {
                    ClearCondition(conditions, 22);
                    return 2;
                }
                ClearCondition(conditions, 21);
                break;
            case 7:
            case CONDITION_ZOMBIE:
            case 11:
            case 31:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                if (HasCondition(conditions, 4)) {
                    return 0;
                }
                break;
            case 10:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_SLEEP);
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case CONDITION_SLEEP:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case 14:
            case 16:
            case 18:
            case 27:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case 22:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                if (HasCondition(conditions, 6)) {
                    ClearCondition(conditions, 6);
                    return 2;
                }
                break;
            case 23:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                break;
        }
        break;
    }
    SetBit(conditions->bits, condition);
    SetConditionAge(conditions, condition, 0);
    return 1;
}

RVA(0x0003edd0, 0x13)
i16 GetFatalCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_fatalConditions);
}

RVA(0x0003edf0, 0x43)
i16 LastConditionIn(ConditionSet* conditions, const i16* list) {
    i16 found = 0;
    i16 i;
    for (i = 0; list[i] != -1; i++) {
        if (HasCondition(conditions, list[i])) {
            found = list[i];
        }
    }
    return found;
}

RVA(0x0003ee40, 0x32)
void ClearConditionList(ConditionSet* conditions, const i16* list) {
    i16 i;
    for (i = 0; list[i] != -1; i++) {
        ClearCondition(conditions, list[i]);
    }
}

RVA(0x0003ee80, 0x13)
i16 GetPickBlockingCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_pickBlockingConditions);
}

RVA(0x0003eea0, 0x36)
b16 IsFieldConditionRestricted(i16 condition) {
    i16 i;
    for (i = 0; s_fieldRestrictedConditions[i] != -1; i++) {
        if (s_fieldRestrictedConditions[i] == condition) {
            return true;
        }
    }
    return false;
}

RVA(0x0003eee0, 0x13)
i16 GetDisablingCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_disablingConditions);
}

RVA(0x0003ef00, 0x13)
void ClearBattleConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_battleConditions);
}

RVA(0x0003ef20, 0x13)
void ClearLeaveConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_leaveConditions);
}

RVA(0x0003ef40, 0x13)
void ClearAllConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_allConditions);
}

// Ages every held condition by `amount`; nonzero when any aged.
RVA(0x0003ef60, 0x37)
i16 AgeConditions(ConditionSet* conditions, i16 amount) {
    i16 aged = 0;
    i16 i;
    if (!amount) {
        return 0;
    }
    for (i = 0; i < 35; i++) {
        aged |= AgeCondition(amount, conditions, i);
    }
    return aged;
}

// Ages `condition` by `amount` (kept in 0..255) when it is held and can wear
// off; 1 when aged.
RVA(0x0003efa0, 0x57)
b16 AgeCondition(i16 amount, ConditionSet* conditions, i16 condition) {
    if (!HasCondition(conditions, condition)) {
        return false;
    }
    if (s_recoveryChance[condition] == 0) {
        return false;
    }
    SetConditionAge(
        conditions,
        condition,
        ClampUShort(GetConditionAge(conditions, condition) + amount, 0, 0xff)
    );
    return true;
}

// Rolls every held condition of `character` for recovery; nonzero when any
// wore off.
RVA(0x0003f000, 0x25)
i16 RecoverConditions(Character* character) {
    i16 recovered = 0;
    i16 i;
    for (i = 0; i < 35; i++) {
        recovered |= RecoverCondition(character, i);
    }
    return recovered;
}

// Rolls `condition` for recovery: its chance plus an eighth of its age against
// 0..255. A condition that stays can hurt: dancing (19) drains 1..5 HP,
// suffocation (11) 1..33.
RVA(0x0003f030, 0xb6)
b16 RecoverCondition(Character* character, i16 condition) {
    i16 chance;
    if (!HasCondition(GetCharacterConditions(character), condition)) {
        return false;
    }
    if (s_recoveryChance[condition] == 0) {
        return false;
    }
    chance = (GetConditionAge(GetCharacterConditions(character), condition) >> 3)
             + s_recoveryChance[condition];
    if (chance <= RandomUpTo(0xff)) {
        if (condition == 19) {
            DrainPool(&character->pools.hp, RandomUpTo(4) + 1);
        } else if (condition == 11) {
            DrainPool(&character->pools.hp, RandomUpTo(0x20) + 1);
        }
        return false;
    }
    ClearCondition(GetCharacterConditions(character), condition);
    return true;
}

// Applies the conditions an empty HP or MP pool causes: 1 when HP ran out
// (dying, unless already dying (2) or the condition 8 blocks it), 3 when a
// pool ran out and a condition was added, 4 when both are out while already
// dying, 2 when HP is out while dying, else 0.
// @identity-TODO: condition 8's role here is unrecovered.
RVA(0x0003f0f0, 0xd0)
i16 ApplyEmptyPools(Character* character) {
    switch (EmptyPoolMask(&character->pools)) {
        case POOL_MASK_NONE:
            break;
        case POOL_MASK_BOTH:
            if (!HasCondition(GetCharacterConditions(character), CONDITION_DYING)) {
                AddCondition(GetCharacterConditions(character), CONDITION_DYING);
                return 3;
            }
            return 4;
        case POOL_MASK_MP:
            if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                AddCondition(GetCharacterConditions(character), CONDITION_DEAD);
                return 3;
            }
            break;
        case POOL_MASK_HP:
            if (!HasCondition(GetCharacterConditions(character), CONDITION_DYING)) {
                if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                    break;
                }
                AddCondition(GetCharacterConditions(character), CONDITION_DYING);
                return 1;
            }
            return 2;
    }
    return 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
// Escalates `mild` to `severe`: 0 when `severe` is held, 1 when `mild` was
// added, 2 when `mild` became `severe`.
RVA(0x0003f1c0, 0x60)
i16 EscalateCondition(ConditionSet* conditions, i16 mild, i16 severe) {
    if (HasCondition(conditions, severe)) {
        return 0;
    }
    if (!HasCondition(conditions, mild)) {
        AddCondition(conditions, mild);
        return 1;
    }
    ClearCondition(conditions, mild);
    AddCondition(conditions, severe);
    return 2;
}

// Eases `severe` to `mild` (2), or clears `mild` (1); 0 when neither is held.
RVA(0x0003f220, 0x6c)
i16 EaseCondition(ConditionSet* conditions, i16 mild, i16 severe) {
    if (HasCondition(conditions, severe)) {
        ClearCondition(conditions, severe);
        ClearCondition(conditions, mild);
        AddCondition(conditions, mild);
        return 2;
    }
    if (HasCondition(conditions, mild)) {
        ClearCondition(conditions, mild);
        return 1;
    }
    return 0;
}

// Eases sleep (13) to doze (25).
RVA(0x0003f290, 0x12)
i16 EaseSleep(ConditionSet* conditions) {
    return EaseCondition(conditions, CONDITION_DOZE, CONDITION_SLEEP);
}

// The display name of the first held condition in display order.
RVA(0x0003f2b0, 0x3e)
const char* GetFirstConditionName(ConditionSet* conditions) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (TestBit(conditions->bits, s_conditionNames[i].bit)) {
            return s_conditionNames[i].name;
        }
    }
    return s_noConditionName;
}

// The name of the next held condition from `*cursor` on (display order),
// leaving `*cursor` on it; -1 and the blank name after the last.
RVA(0x0003f2f0, 0x5a)
const char* NextConditionName(ConditionSet* conditions, i16* cursor) {
    for (; *cursor < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); (*cursor)++) {
        if (TestBit(conditions->bits, s_conditionNames[*cursor].bit)) {
            return s_conditionNames[*cursor].name;
        }
    }
    *cursor = -1;
    return s_noConditionName;
}

RVA(0x0003f350, 0x30)
const char* GetConditionName(i16 bit) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (s_conditionNames[i].bit == bit) {
            return s_conditionNames[i].name;
        }
    }
    return s_noConditionName;
}

// The display-order index of the first condition `character` holds; -1 when
// none.
RVA(0x0003f380, 0x3c)
i16 GetFirstConditionIndex(Character* character) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (TestBit(GetCharacterConditions(character)->bits, s_conditionNames[i].bit)) {
            return i;
        }
    }
    return -1;
}

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

// Returns `slot` when it is in the party and `mode` bit 0 is set, or when it is
// not and bit 1 is set; -1 otherwise.
RVA(0x0003f780, 0x38)
i16 FilterPartyMember(i16 slot, i16 mode) {
    i16 index = FindPartySlot(slot);
    if (index != -1 && (mode & 1)) {
        return slot;
    }
    if (index == -1 && (mode & 2)) {
        return slot;
    }
    return -1;
}

// Installs `character` in a free roster slot; a human member (id below 32)
// also joins the party. Non-humans are refused (-1) once 26 entries exist.
// Returns the slot, or -1.
RVA(0x0003f7c0, 0x8a)
i16 AddToRoster(Character* character) {
    i16 slot;
    if (!IsHumanCharacter(character) && CountRosterEntries(0) >= 26) {
        return -1;
    }
    slot = FindEmptySlot(0);
    if (slot != -1) {
        SetRosterEntry(slot, character);
        if (IsHumanCharacter(character)) {
            StripZeroWords(GetCharacterSkills(character));
            AddToParty(slot);
            RaiseExperienceToLevel(character);
            return slot;
        }
        KeepFirstSixWords(GetCharacterSkills(character));
        RaiseExperienceToLevel(character);
    }
    return slot;
}

// Takes roster slot `slot` out of the party and the roster; a non-human's
// record is freed. Returns the slot, or -1 when it was empty.
RVA(0x0003f850, 0x5f)
i16 RemoveFromRoster(i16 slot) {
    i16 index;
    Character* character = GetRosterEntry(slot);
    if (!character) {
        return -1;
    }
    index = FindPartySlot(slot);
    SetPartySlot(index, -1);
    if (!IsHumanCharacter(character)) {
        FreeWordList(GetCharacterSkills(character));
        FreeBlock(character);
    }
    SetRosterEntry(slot, NULL);
    return slot;
}

// Puts roster slot `slot` into an empty party position; when the party is
// full, a human member replaces the first non-human one.
RVA(0x0003f8b0, 0x5a)
i16 AddToParty(i16 slot) {
    i16 index = FindEmptySlot(1);
    if (index == -1) {
        i16 id = GetRosterId(slot);
        if (id >= 0 && id < 32) {
            for (index = 0; index < 6; index++) {
                if (GetPartyRosterId(index) >= 32) {
                    break;
                }
            }
            if (index == 6) {
                index = -1;
            }
        } else {
            index = -1;
        }
    }
    return ExchangePartySlot(index, slot);
}

RVA(0x0003f910, 0x25)
i16 ExchangePartySlot(i16 index, i16 slot) {
    i16 prev = GetPartySlot(index);
    SetPartySlot(index, slot);
    return prev;
}

RVA(0x0003f940, 0x10)
void ClearPartyPosition(i16 index) {
    SetPartySlot(index, -1);
}

// Takes roster slot `slot` out of the party.
RVA(0x0003f950, 0x17)
void RemoveFromParty(i16 slot) {
    ClearPartyPosition(FindPartySlot(slot));
}

// The members present; with `skipDisabled`, only those without a disabling
// condition.
RVA(0x0003f970, 0x40)
i16 CountPartyMembers(i16 skipDisabled) {
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            if (!skipDisabled || !GetDisablingCondition(GetCharacterConditions(character))) {
                count++;
            }
        }
    }
    return count;
}

// Ages every member's conditions by one and rolls them for recovery; nonzero
// when any wore off.
RVA(0x0003f9b0, 0x40)
i16 TickPartyConditions(void) {
    i16 recovered = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            AgeConditions(GetCharacterConditions(character), 1);
            recovered |= RecoverConditions(character);
        }
    }
    return recovered;
}

static __inline i32 PoolPercentAmount(const CurMax* pool, i16 percent) {
    i32 amount = pool->max * percent / 100;
    amount = max(1, amount);
    return amount;
}

// Takes `percent` of each living member's maximum HP (at least 1; a negative
// `percent` takes that many points), then applies what an empty pool brings.
// With `skipId13`, character 13 is spared. Returns how many were hit.
// @identity-TODO: who character 13 is is unrecovered.
RVA(0x0003f9f0, 0xa1)
i16 DamageParty(i16 percent, i16 skipId13) {
    i16 hit = 0;
    i16 i;
    i32 amount;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && !GetFatalCondition(GetCharacterConditions(character))) {
            if (skipId13 && character->id == 13) {
                continue;
            }
            if (percent < 0) {
                ChangePool(&character->pools.hp, percent);
            } else {
                amount = PoolPercentAmount(&character->pools.hp, percent);
                ChangePool(&character->pools.hp, -amount);
            }
            ApplyEmptyPools(character);
            hit++;
        }
    }
    return hit;
}

// Restores `percent` of each living member's maximum HP (at least 1; a
// negative `percent` restores that many points). Returns how many were healed.
RVA(0x0003faa0, 0x90)
i16 HealParty(i16 percent) {
    i16 healed = 0;
    i16 i;
    i32 amount;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character && !GetFatalCondition(GetCharacterConditions(character))) {
            if (percent < 0) {
                ChangePool(&character->pools.hp, -percent);
            } else {
                amount = PoolPercentAmount(&character->pools.hp, percent);
                ChangePool(&character->pools.hp, amount);
            }
            ApplyEmptyPools(character);
            healed++;
        }
    }
    return healed;
}

// Ends a battle for every roster member: clears its battle conditions and
// tally, its shield and its field mark.
RVA(0x0003fb30, 0x4a)
void ResetRosterBattleState(void) {
    i16 slot;
    Character* character;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character) {
            ClearBattleConditions(GetCharacterConditions(character));
            ResetBattleTally(character);
            ClearActionWait(GetCharacterActionWait(character));
        }
    }
}

// Clears every roster member's conditions and recomputes its stats.
RVA(0x0003fb80, 0x35)
void ClearRosterConditions(void) {
    i16 slot;
    Character* character;
    for (slot = 0; slot < 32; slot++) {
        character = GetRosterCharacter(slot);
        if (character) {
            ClearAllConditions(GetCharacterConditions(character));
            RecalcCharacterStats(character);
        }
    }
}

// Copies `src` into `dst` (a new record when NULL; an existing one loses its
// skill list first) with its own copy of the skill list. Returns `dst`.
RVA(0x0003fbc0, 0x70)
Character* CopyCharacter(Character* src, Character* dst) {
    if (!src) {
        return dst;
    }
    if (!dst) {
        dst = AllocCleared(1, sizeof(Character));
    } else {
        FreeWordList(GetCharacterSkills(dst));
    }
    *dst = *src;
    InitEmptyWordList(&dst->skills);
    CopySkillList(src, GetCharacterSkills(dst));
    return dst;
}

// @identity-TODO: That 0x556b0 (word 0x90b10 while layer 1 is shown) is the character the
// layer-1 panel shows is inferred; also hides layer 1 as a side effect.
// Counts the fallen human members; hides layer 1 when the character it shows
// has fallen or left.
RVA(0x0003fc30, 0x74)
i16 CountFallenHumans(void) {
    i16 shown = GetShownPanelCharacter();
    i16 fallen = 0;
    b16 found = false;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (character) {
            if (!GetFatalCondition(GetCharacterConditions(character))) {
                if (character->id == shown) {
                    found = true;
                }
            } else if (IsHumanCharacter(character)) {
                fallen++;
            }
        }
    }
    if (shown != -1 && !found) {
        HideScreenLayer(1);
    }
    return fallen;
}

RVA(0x0003fcb0, 0x2c)
i16 IsPartyMemberFallen(i16 index) {
    Character* character = GetPartyCharacter(index);
    if (!character) {
        return -1;
    }
    return GetFatalCondition(GetCharacterConditions(character)) != 0;
}

// Handles fallen and emptied members after a fight: a fallen non-human (not
// the guest) leaves the party; a member whose pool ran out gets its
// conditions, and leaves too unless human. Returns how many were handled.
// @identity-TODO: what CheckPickTarget does for them is only partly decoded.
RVA(0x0003fce0, 0x8e)
i16 ProcessPartyCasualties(void) {
    i16 count = 0;
    i16 i;
    Character* character;
    for (i = 0; i < 6; i++) {
        character = GetPartyCharacter(i);
        if (!character) {
            continue;
        }
        if (GetFatalCondition(GetCharacterConditions(character))) {
            if (IsHumanCharacter(character) || IsGuestIndex(i) >= 0) {
                continue;
            }
            CheckPickTarget(i);
            SetPartySlot(i, -1);
        } else {
            if (!ApplyEmptyPools(character) || IsGuestIndex(i) >= 0) {
                continue;
            }
            CheckPickTarget(i);
            if (!IsHumanCharacter(character)) {
                SetPartySlot(i, -1);
            }
        }
        count++;
    }
    return count;
}

RVA(0x0003fd70, 0xbb)
i16 TickPartySteps(void) {
    i16 changed = 0;
    i16 index;
    Character* character;
    for (index = 0; index < 6; index++) {
        character = GetPartyCharacter(index);
        if (character) {
            ApplyEquipmentRegen(character);
            ApplyEquipmentEffects(character, EQUIP_EFFECT_STEP_TICK);
            if (character->id != 13) {
                if (HasCondition(GetCharacterConditions(character), CONDITION_SEVERE_POISON)
                    && !IsConditionResisted(character, 32)) {
                    changed |= 1;
                    DrainPool(&character->pools.hp, 4);
                }
                if (HasCondition(GetCharacterConditions(character), CONDITION_POISON)
                    && !IsConditionResisted(character, 15)) {
                    changed |= 1;
                    DrainPool(&character->pools.hp, 1);
                }
            }
        }
    }
    RedrawPartyStatus();
    return changed;
}

// @identity-TODO: an identity conversion every script-object lookup ends in
// (for the party table and the pointer slots alike).
RVA(0x0003fe30, 0x5)
Character* AsCharacter(Character* character) {
    return character;
}

RVA(0x0003fe40, 0x6)
Character* GetRosterLeader(void) {
    return RosterMemberAt(0);
}

RVA(0x0003fe50, 0x1e)
Character* GetRosterEntry(i16 slot) {
    if (slot >= 0 && slot < 32) {
        return RosterMemberAt(slot);
    }
    return NULL;
}

RVA(0x0003fe70, 0x17)
Character* GetRosterCharacter(i16 slot) {
    return AsCharacter(GetRosterEntry(slot));
}

RVA(0x0003fe90, 0x20)
i16 FindPartySlot(i16 slot) {
    i16 i;
    for (i = 0; i < 6; i++) {
        if (PartySlotAt(i) == slot) {
            return i;
        }
    }
    return -1;
}

RVA(0x0003feb0, 0x1a)
i16 GetRosterId(i16 slot) {
    Character* character = GetRosterCharacter(slot);
    if (character == NULL) {
        return -1;
    }
    return character->id;
}

RVA(0x0003fed0, 0x1e)
Character* SetRosterEntry(i16 slot, Character* character) {
    Character* prev = GetRosterEntry(slot);
    g_roster[slot] = character;
    return prev;
}

RVA(0x0003fef0, 0x21)
i16 GetPartySlot(i16 index) {
    if (index >= 0 && index < 6) {
        return PartySlotAt(index);
    }
    return -1;
}

RVA(0x0003ff20, 0x17)
Character* GetPartyEntry(i16 index) {
    return GetRosterEntry(GetPartySlot(index));
}

RVA(0x0003ff40, 0x17)
Character* GetPartyCharacter(i16 index) {
    return AsCharacter(GetPartyEntry(index));
}

// Puts roster slot `slot` (-1 empties it) into party position `index`; a
// member displaced from a non-guest position loses its leave conditions.
RVA(0x0003ff60, 0x6b)
void SetPartySlot(i16 index, i16 slot) {
    Character* character;
    if (index < 0 || index >= 6) {
        return;
    }
    if (slot < -1 || slot >= 32) {
        return;
    }
    if (PartySlotAt(index) != -1) {
        character = GetPartyCharacter(index);
        if (character != NULL && IsGuestIndex(index) < 0) {
            ClearLeaveConditions(GetCharacterConditions(character));
        }
    }
    g_party[index] = slot;
}

RVA(0x0003ffd0, 0x17)
i16 GetPartyRosterId(i16 index) {
    return GetRosterId(GetPartySlot(index));
}

// The roster slot holding character `id`; -1 when none does.
RVA(0x0003fff0, 0x2b)
i16 FindRosterSlotById(i16 id) {
    i16 slot;
    for (slot = 0; slot < 32; slot++) {
        if (GetRosterId(slot) == id) {
            return slot;
        }
    }
    return -1;
}

RVA(0x00040020, 0x18)
i16 RosterSlotOfId(i16 id) {
    if (id == -1) {
        return id;
    }
    return FindRosterSlotById(id);
}

RVA(0x00040040, 0x17)
Character* GetCharacterById(i16 id) {
    return GetRosterEntry(RosterSlotOfId(id));
}

// The party position of character `id`; -1 when not in the party.
RVA(0x00040060, 0x21)
i16 FindPartyPositionOfId(i16 id) {
    i16 slot = RosterSlotOfId(id);
    if (slot == -1) {
        return slot;
    }
    return FindPartySlot(slot);
}

// Sorts the roster by character id (empty slots last) and puts the party
// back on the same characters.
// Codegen constraint: the search for the insertion point leaves through
// `goto next`; as a for loop cl rotates it and the layout differs.
RVA(0x00040090, 0xcd)
b16 SortRoster(void) {
    i16 ids[6];
    i16 i;
    i16 j;
    i16 id;
    i16 other;
    for (i = 0; i < 6; i++) {
        ids[i] = GetPartyRosterId(i);
    }
    for (i = 0; i < 32; i++) {
        id = GetRosterId(i);
        while (id >= 0 && id < 32) {
            j = 0;
            while (1) {
                if (j >= i) {
                    goto next;
                }
                other = GetRosterId(j);
                if (other < 0 || other >= 32 || other > id) {
                    break;
                }
                j++;
            }
            SetRosterEntry(i, SetRosterEntry(j, GetRosterEntry(i)));
            i = j;
            id = GetRosterId(i);
        }
    next:;
    }
    for (i = 0; i < 6; i++) {
        if (ids[i] == -1) {
            SetPartySlot(i, -1);
        } else {
            SetPartySlot(i, FindRosterSlotById(ids[i]));
        }
    }
    return false;
}

// The id of the character at party position `index` (unchecked).
RVA(0x00040160, 0x17)
i16 GetPartyMemberId(i16 index) {
    return GetRosterId(PartySlotAt(index));
}

// Swaps two party slots; when the front three end up empty, the back three
// move forward.
RVA(0x00040180, 0x96)
void SwapPartySlots(i16 a, i16 b) {
    i16 slot;
    if (a != b) {
        slot = PartySlotAt(a);
        g_party[a] = PartySlotAt(b);
        g_party[b] = slot;
        if (PartySlotAt(0) == -1 && PartySlotAt(1) == -1 && PartySlotAt(2) == -1) {
            g_party[0] = PartySlotAt(3);
            g_party[1] = PartySlotAt(4);
            g_party[2] = PartySlotAt(5);
            g_party[3] = -1;
            g_party[4] = -1;
            g_party[5] = -1;
        }
    }
}

// Loads the equipment table: per character group, a 40-bit set of the items
// the group can equip.
RVA(0x00040220, 0x2a)
void LoadEquipTable(void) {
    FILE* fp = OpenDataFile(6, 12, 0);
    s_equipTable = ReadRawHandle(fp);
    CloseDataFile(fp);
}

// Nonzero when characters of `group` can equip item `item`.
RVA(0x00040250, 0x39)
b16 CanGroupEquip(i16 group, i16 item) {
    u8* table;
    if (item < 0) {
        return false;
    }
    table = HandleReadPtr(s_equipTable);
    return TestBit(table + group * 5, item) != 0;
}

RVA(0x00040290, 0x74)
i16 EquipPartOfItem(ItemRecord* item) {
    i16 part;
    i16 kind = item->kind;
    kind -= ITEM_KIND_WEAPON;
    switch (kind) {
        case 0:
            part = EQUIP_PART_WEAPON;
            break;
        case 1:
            part = EQUIP_PART_GUN;
            break;
        case 2:
            part = EQUIP_PART_AMMO;
            break;
        case 4:
            part = EQUIP_PART_HEAD;
            break;
        case 3:
        case 5:
            part = EQUIP_PART_BODY;
            break;
        case 6:
            part = EQUIP_PART_ARMS;
            break;
        case 7:
            part = EQUIP_PART_LEGS;
            break;
        case 8:
            part = EQUIP_PART_ACCESSORY;
            break;
    }
    return part;
}

RVA(0x00040310, 0x90)
ItemSlot GetEquipSlot(Character* character, i16 part) {
    ItemSlot none = {-1, -1, 0};
    if (character != NULL) {
        switch (part) {
            case EQUIP_PART_WEAPON:
                return GetCharacterEquipment(character)[5];
            case EQUIP_PART_GUN:
                return GetCharacterEquipment(character)[6];
            case EQUIP_PART_AMMO:
                return GetCharacterEquipment(character)[7];
            case EQUIP_PART_HEAD:
                return GetCharacterEquipment(character)[0];
            case EQUIP_PART_BODY:
                return GetCharacterEquipment(character)[1];
            case EQUIP_PART_ARMS:
                return GetCharacterEquipment(character)[2];
            case EQUIP_PART_LEGS:
                return GetCharacterEquipment(character)[3];
            case EQUIP_PART_ACCESSORY:
                return GetCharacterEquipment(character)[4];
        }
    }
    return none;
}

RVA(0x000403a0, 0x1c)
ItemSlot GetRosterEquipSlot(i16 slot, i16 part) {
    return GetEquipSlot(GetRosterCharacter(slot), part);
}

RVA(0x000403c0, 0x1b)
i16 GetEquipItem(Character* character, i16 part) {
    return GetEquipSlot(character, part).item;
}

RVA(0x000403e0, 0x110)
i16 SetEquipSlot(i16 slot, i16 part, ItemSlot item, i16 check) {
    Character* character = GetRosterCharacter(slot);
    if (!character) {
        return -1;
    }
    switch (part) {
        case EQUIP_PART_WEAPON:
            GetCharacterEquipment(character)[5] = item;
            break;
        case EQUIP_PART_GUN:
            GetCharacterEquipment(character)[6] = item;
            if (check && CanEquipItem(character, GetCharacterEquipment(character)[7].item) < 1) {
                UnequipPart(slot, EQUIP_PART_AMMO);
            }
            break;
        case EQUIP_PART_AMMO:
            GetCharacterEquipment(character)[7] = item;
            break;
        case EQUIP_PART_HEAD:
            GetCharacterEquipment(character)[0] = item;
            break;
        case EQUIP_PART_BODY:
            GetCharacterEquipment(character)[1] = item;
            break;
        case EQUIP_PART_ARMS:
            GetCharacterEquipment(character)[2] = item;
            break;
        case EQUIP_PART_LEGS:
            GetCharacterEquipment(character)[3] = item;
            break;
        case EQUIP_PART_ACCESSORY:
            GetCharacterEquipment(character)[4] = item;
            break;
        default:
            part = -1;
    }
    return part;
}

RVA(0x000404f0, 0x93)
void UnequipPart(i16 slot, i16 part) {
    ItemSlot empty;
    ItemSlot item;
    ClearItemSlot(&empty);
    item = GetRosterEquipSlot(slot, part);
    SetEquipSlot(slot, part, empty, 1);
    if (item.item != -1 && item.item != 0) {
        if (item.quantity < 1) {
            item.quantity = 1;
        }
        StoreBagItem(item.item, item.quantity, item.attachment);
    }
}

RVA(0x00040590, 0x4d)
i16 AttachEquipItem(i16 member, i16 part, i16 index) {
    ItemSlot item = GetRosterEquipSlot(member, part);
    i16 previous = item.attachment;
    item.attachment = index;
    SetEquipSlot(member, part, item, 0);
    return previous;
}

// The ammunition type of the gun in equipment slot 6 (item parameter 0x27);
// -1 without a character or a gun.
// @identity-TODO: that slot 6 holds the gun is inferred from this use.
RVA(0x000405e0, 0x36)
i16 GetGunAmmoType(Character* character) {
    if (!character) {
        return -1;
    }
    if (GetCharacterEquipment(character)[6].item < 1) {
        return -1;
    }
    return GetItemAmmoType(GetLoadedRecord(GetCharacterEquipment(character)[6].item));
}

// The equipment part item `item` goes in when `character` can equip it, else
// -1. Ammunition (kind 13) must match the equipped gun; anything else must be
// allowed for the character's equipment group.
RVA(0x00040620, 0x82)
i16 CanEquipItem(Character* character, i16 item) {
    ItemRecord* record;
    i16 part;
    if (item < 1) {
        return -1;
    }
    record = GetLoadedRecord(item);
    part = EquipPartOfItem(record);
    if (part < 0) {
        return -1;
    }
    if (record->kind != ITEM_KIND_AMMO) {
        if (!CanGroupEquip(character->equipGroup, GetItemEquipCode(record))) {
            return -1;
        }
    } else {
        i16 ammo = GetItemAmmoType(record);
        if (GetGunAmmoType(character) != ammo) {
            return -1;
        }
    }
    return part;
}

// Puts `item` on its equipment part of roster member `slot` (SetEquipSlot's
// result in `*result`) and returns the item slot it replaces.
RVA(0x000406b0, 0x52)
ItemSlot SwapEquipSlot(i16 slot, ItemSlot item, i16* result) {
    i16 part = EquipPartOfItem(GetLoadedRecord(item.item));
    ItemSlot old = GetRosterEquipSlot(slot, part);
    *result = SetEquipSlot(slot, part, item, 1);
    return old;
}

// Equips `item` from bag entry `index` on roster member `slot`, taking it out
// of the bag and putting the replaced item back; returns the replaced slot.
// Ammunition (kind 13) records `count` for the character; kind 14 clears
// parts 3, 5 and 6.
RVA(0x00040710, 0x11a)
ItemSlot EquipItem(i16 slot, ItemSlot item, i16 count, i16 index) {
    i16 result;
    ItemSlot old = SwapEquipSlot(slot, item, &result);
    if (result != -1) {
        Character* character = GetRosterCharacter(slot);
        if (item.item != -1) {
            i16 kind = GetItemKind(item.item);
            if (kind != 13) {
                TakeBagItemsAt(index, item.item, item.quantity);
                if (kind == ITEM_KIND_FULL_BODY_ARMOR) {
                    UnequipPart(slot, EQUIP_PART_HEAD);
                    UnequipPart(slot, EQUIP_PART_ARMS);
                    UnequipPart(slot, EQUIP_PART_LEGS);
                }
            } else {
                TakeBagItems(item.item, item.quantity);
                if (character && g_ammoCountIndex >= 0) {
                    i16 at = g_ammoCountIndex;
                    character->ammoCounts[at] = (u8)count;
                }
            }
        }
        if (old.item != -1) {
            if (old.quantity < 1) {
                old.quantity = 1;
            }
            StoreBagItem(old.item, old.quantity, old.attachment);
        }
        CompactBag();
    }
    return old;
}

// Normalises all eight equipment slots.
RVA(0x00040830, 0x7f)
void NormalizeEquipSlots(Character* character) {
    NormalizeItemSlot(&GetCharacterEquipment(character)[0]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[1]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[2]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[3]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[4]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[5]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[6]);
    NormalizeItemSlot(&GetCharacterEquipment(character)[7]);
}

// Empties a slot without an item; a non-ammunition item gets quantity 1.
RVA(0x000408b0, 0x39)
void NormalizeItemSlot(ItemSlot* slot) {
    if (slot->item < 1) {
        ClearItemSlot(slot);
        return;
    }
    if (GetItemKind(slot->item) != ITEM_KIND_AMMO) {
        slot->quantity = 1;
    }
}

// Adds `amount` to the character's macca, kept in 0..9999999; returns the new
// total (0 without a character).
RVA(0x000408f0, 0x2a)
i32 AddMacca(Character* character, i32 amount) {
    if (!character) {
        return 0;
    }
    return character->macca = AddClampInt(character->macca, amount, 0, 9999999);
}

// The same for magnetite.
RVA(0x00040920, 0x2a)
i32 AddMagnetite(Character* character, i32 amount) {
    if (!character) {
        return 0;
    }
    return character->magnetite = AddClampInt(character->magnetite, amount, 0, 9999999);
}

// -1 when script object `who` has less macca than `amount`, 0 when exactly
// that much, else 1.
RVA(0x00040950, 0x28)
i16 CompareMacca(i16 who, i32 amount) {
    i32 macca = ResolveScriptObject(who)->macca;
    if (macca < amount) {
        return -1;
    }
    return macca != amount;
}

RVA(0x00040980, 0x12)
i16 SetStatusColumn(i16 column) {
    i16 prev = s_statusColumn;
    s_statusColumn = column;
    return prev;
}

RVA(0x000409a0, 0x74)
i16 RunStatusListPicker(i16 close) {
    i16 result = -2;
    if (!close) {
        if (!s_statusListMenu) {
            s_statusListMenu = CreateStatusListMenu(NULL);
        }
        result = RunListMenu(s_statusListMenu);
        if (result == -1) {
            return -1;
        }
        if (result != -2) {
            result = g_selectedObjectId;
        }
    }
    s_statusListMenu = CloseListMenu(s_statusListMenu);
    return result;
}

RVA(0x00040a20, 0x57)
MenuBox* CreateStatusListMenu(MenuBox* parent) {
    MenuBox* menu;
    BuildStatusSlots();
    menu = CreateMenuBox(parent, 26, 2);
    MoveMenuBox(menu, 45, 86);
    SetMenuItems(menu, 16, g_statusSlots, g_statusSlotCount, StatusListMenuHandler);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    return menu;
}

RVA(0x00040a80, 0xa8)
void StatusListMenuHandler(MenuBox* menu, i16 index, i16 event) {
    i16 slot = g_statusSlots[index];
    i16 result;
    switch (event) {
        case MENU_EVENT_DESTROY:
            menu->items.entries = NULL;
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW:
            result = FormatStatusLine(slot, index);
            if (result == -1) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, slot, 1);
            } else if (result >= 0) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2450, slot, 0);
            } else {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x2500, slot, 1);
            }
            break;
    }
}

// Formats roster slot `slot` as status-list row `row` into the scratch
// buffer (a party mark, the name, alignment, level, HP, MP, condition, and in
// column 1 the summoning cost); returns the member's id, -1 when the row
// cannot be picked in this column, -2 when its cost cannot be paid.
RVA(0x00040b30, 0x394)
i16 FormatStatusLine(i16 slot, i16 row) {
    char text[36];
    Character* member;
    i32 cost;

    member = GetRosterCharacter(slot);
    if (member == NULL) {
        sprintf(g_scratchBuffer, " %2d", row + 1);
        return -1;
    }
    sprintf(
        g_scratchBuffer,
        "%c%2d %-17.17s ",
        FindPartySlot(slot) != -1 ? '*' : ' ',
        row + 1,
        FormatFullName(text, member)
    );
    strcat(g_scratchBuffer, FormatAlignmentLetter(1, member->alignmentLevelA, text));
    strcat(g_scratchBuffer, "/");
    strcat(g_scratchBuffer, FormatAlignmentLetter(0, member->alignmentLevelB, text));
    sprintf(text, " %2d ", member->level);
    strcat(g_scratchBuffer, text);
    strcat(g_scratchBuffer, FormatCurMax(member->pools.hp, text, 4));
    strcat(g_scratchBuffer, FormatCurMax(member->pools.mp, text, 3));
    strcat(g_scratchBuffer, " ");
    sprintf(text, "%-6.6s", GetFirstConditionName(GetCharacterConditions(member)));
    strcat(g_scratchBuffer, text);
    switch (s_statusColumn) {
        case 0:
            break;
        case 1:
            cost = GetSummonMagnetiteCost(member);
            sprintf(text, " %6ld", cost);
            strcat(g_scratchBuffer, text);
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            if (GetFatalCondition(GetCharacterConditions(member))) {
                return -1;
            }
            if (GetRosterCharacter(0)->magnetite < cost) {
                return -2;
            }
            break;
        case 2:
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            if (TestCharacterFlag(member, 0x40)) {
                return -1;
            }
            break;
        case 3:
            if (TestCharacterFlag(member, 0x40)) {
                return -1;
            }
            break;
        case 4:
            if (FindPartySlot(slot) != -1) {
                return -1;
            }
            break;
    }
    return member->id;
}

RVA(0x00040ed0, 0x2b)
char* FormatAlignmentLetter(i16 axis, i16 value, char* out) {
    i16 column = AlignmentClass(value) + 1;
    out[0] = s_alignmentLetters[axis][column];
    out[1] = '\0';
    return out;
}

RVA(0x00040f00, 0x34)
char* FormatCurMax(CurMax value, char* buf, i16 width) {
    sprintf(buf, " %*d/%*d", width, value.cur, width, value.max);
    return buf;
}

// Lists the roster slots the status screen shows: every slot in column 0,
// otherwise only filled slots holding ids from 32 up.
RVA(0x00040f40, 0x51)
i16 BuildStatusSlots(void) {
    i16 slot;
    Character* character;
    g_statusSlotCount = 0;
    for (slot = 0; slot < 32; slot++) {
        if (s_statusColumn != 0) {
            character = GetRosterCharacter(slot);
            if (character == NULL || IsHumanCharacter(character)) {
                continue;
            }
        }
        g_statusSlots[g_statusSlotCount++] = slot;
    }
    return g_statusSlotCount;
}

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
b16 DrainUpkeep(Character* hero, Character* member, i16 cost, i16 position) {
    b16 died = false;
    if (hero->magnetite >= cost) {
        hero->magnetite -= cost;
        cost = 0;
    } else {
        cost -= hero->magnetite;
        hero->magnetite = 0;
    }
    if (cost < 1) {
        return false;
    }
    PayPoolCost(&hero->pools.mp, cost);
    if (cost < 1) {
        return false;
    }
    PayPoolCost(&member->pools.mp, cost);
    if (cost < 1) {
        return false;
    }
    if (member->pools.hp.cur > (u16)cost) {
        member->pools.hp.cur -= cost;
    } else {
        died = true;
        member->pools.hp.cur = 0;
        AddCondition(GetCharacterConditions(member), CONDITION_DYING);
        if (!IsHumanCharacter(member)) {
            ClearPartyPosition(position);
        }
    }
    return died;
}

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
    }
    return -1;
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
