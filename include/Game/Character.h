#ifndef GITEN_GAME_CHARACTER_H
#define GITEN_GAME_CHARACTER_H

#include <rva.h>

#include <Game/ActionWait.h>
#include <Game/Alignment.h>
#include <Game/BattleStat.h>
#include <Game/CharacterPools.h>
#include <Game/CharacterStat.h>
#include <Game/Condition.h>
#include <Game/EquipPart.h>
#include <Game/MapCoord.h>
#include <Game/PickFlags.h>
#include <Game/SavedMapPosition.h>
#include <Ints.h>
#include <Util/CurMax.h>
#include <Util/WordList.h>

#include <string.h>

// One of the eight packed equipment slots. The attachment is a signed
// index into the gem item group; -1 means no attachment. Quantity counts
// the equipped items, including the ammunition remaining in a loaded gun.
typedef struct ItemSlot {
    union {
        struct {
            i16 item : 11;
            i16 attachment : 5;
            i16 quantity;
        };
        u32 value;
    };
} ItemSlot;

#define SetItemSlotItem(slot, value) ((slot)->item = (value), (slot)->attachment = -1)

// Removes the item and quantity while retaining its attachment index.
#define EmptyItemSlot(slot) ((slot)->item = -1, (slot)->quantity = 0)

static __inline void ClearItemSlot(ItemSlot* slot) {
    SetItemSlotItem(slot, -1);
    slot->quantity = 0;
}

// The five eleven-stat arrays a character carries after its HP/MP pools;
// `total` is recomputed as half the sum of the other four (each clamped to
// 1..100).
// @identity-TODO: `base`/`bonus`/`equipment` are named from their place in
// that sum only; `modifiers` are the ones cleared together.
typedef struct StatBlock {
    i16 base[11];
    i16 bonus[11];
    i16 modifiers[11];
    i16 equipment[11];
    i16 total[11];
} StatBlock;

// @identity-TODO: a party member's 0x21f-byte record (the 16-entry table of
// them, the script objects resolved from negative ids, and the objects whose
// HP/MP pools and item slots the recovered code reads). `level` is raised by the
// level-up screen and shown as "L%2d" by the analyze window; `alignmentA`/`alignmentB` are the
// alignment histories whose values the party average reads (axes 1 and 0);
// `namePrefix`/`name` are the two 17-byte strings a full name is built from;
// `bloodType` indexes A/B/AB/O, `sign` a 12-entry and `affiliation` a signed
// name table (both named from their table sizes only);
// `conditions` contains the 35-condition bit set and how long each has
// been held, `personalFlags` the per-character event-flag bank (bank 14); `hundredths` is the carried remainder of a percentage accumulator, and
// `skills` is the word list saved after the record.
// `levelBonus` gains 2 with every level gained on the level-up screen.
// `lastChange` is the HP (else MP) change of the last effect applied, which
// the battle messages report. `attitude` indexes the analyze window's five
// attitude names (pleading, friendly, very hostile, hostile, normal).
// @identity-TODO: `alignmentLevelB`/`alignmentLevelA` are the signed values
// derived from `alignmentB`/`alignmentA` (0x43e0a0/0x43e0d0) that
// AlignmentClass classifies; `battleStats` are recomputed with the stats and
// copied to `battleStatsShown` (their 24 entries are unnamed).
// `experience` is what 0x4187e0 adds a battle's award to (below level 99) and
// 0x418aa0 raises to the level's minimum; `macca`/`magnetite` are what the
// script reads for an object (0x438cc0/0x438d10), and `levelGap` is the party
// leader's level minus the object's, clamped to 0..255 (0x410c00).
// @identity-TODO: `affiliation` is three signed bytes the script reads by
// index (the name table uses the first); `trainingPoints` are four counters
// 0x41c690 adds to, capped by 0x41c650(99); `title` picks the name 0x410180
// returns for a human member (id < 0x20); `familiarity` is an eighth of the
// per-id count GetFamiliarityCount reads, clamped to 0..63 (the script adds 2
// unless event flag 2/8 is set).
// @identity-TODO: `fieldState` is set to 6 on every map actor after a party turn
// of a field encounter and read as a script switch key; its values are unrecovered.
typedef struct Character {
    i16 id;
    char namePrefix[17];
    char name[17];
    // @identity-TODO: bag-entry details copied during equipment preview;
    // their meaning is unrecovered. The ammunition writer is disabled by
    // g_ammoCountIndex == -1. No consumer reads these entries directly.
    u8 ammoCounts[7];
    // The return point (area, level, x, y, direction) RecordWarpInLeader
    // copies into the roster leader and 0x41fa30 restores.
    SavedMapPosition returnPosition;
    // The marked position (area, level, x, y, direction) 0x403bf0 stores in
    // the roster leader and field effect 0x16 returns to (one cell in front);
    // skill 0x7a is blocked while no area is marked.
    SavedMapPosition markPosition;
    u8 bloodType;
    u8 sign;
    u8 pad037;
    // Per-attribute resistance; the special attribute beyond this array uses 50.
    u8 resistance[10];
    i8 affiliation[3];
    // The group whose row of the equipment table (0x483b38) says what it can
    // equip.
    i16 equipGroup;
    // PICK_ITEM_SKILL keeps the original item in pickItem while pickTarget names its skill.
    GZ_ENUM_STORAGE(PickFlags, u8) pickFlags;
    u32 trainingPoints[4];
    u8 pad058[3];
    u8 dropChance;
    i16 pickItem;
    i16 actionSpeed; // @identity-TODO: the speed a field actor's action wait uses (0x40f890)
    // @identity-TODO: cleared by each command-input step of 0x409620.
    u16 conditionActionTicks;
    u8 encounterRow; // @identity-TODO: a field actor's skill-roll row (0x40f1e0)
    // Absorbs HP damage until used up (and blocks the MP-draining skills).
    i16 shield;
    u8 pad065[4];
    u8 byte069; // @identity-TODO: copied to both object bytes +0x83/+0x84 (0x410930)
    // @identity-TODO: FindFavouredMember (0x43bc70) only weighs human members
    // with class 2; what the classes are is unrecovered.
    u8 memberClass;
    u8 level;
    u8 title;
    // @identity-TODO: the distance at which a field actor's script range
    // test holds (read as the script's trigger range only).
    u8 triggerRange;
    u32 experience;
    i32 macca;
    i32 magnetite;
    i8 alignmentLevelB;
    i8 alignmentLevelA;
    u8 pad07c;
    // @identity-TODO: set while a party member acts on the field (the view
    // shows it), cleared after its action.
    u8 acting;
    u8 pad07e[2];
    CharacterPools pools;
    StatBlock stats;
    i16 battleStats[24];
    i16 battleStatsShown[24];
    ConditionSet conditions;
    // @identity-TODO: cleared together on entering the field (0x407a70); the field
    // encounter's notes read them as a ready-to-act flag (0x43f5f0 finds the first
    // member with it set) and an action wait (OpSetActorAlert clamps it).
    ActionWait actionWait;
    // @identity-TODO: what the member does while picked on the field (set by the
    // picker 0x4095f0; 0, 7 and 8 show nothing) and the record it uses.
    i8 pickRole;
    i16 pickTarget : 15;
    i16 pickTargetHigh : 1;
    // @identity-TODO: the object (or -1 - party position) the pick targets.
    i16 pickObject : 14;
    // Set once the picked skill's cost is paid (or when an item stands in
    // for it).
    i16 pickCostPaid : 1;
    // @identity-TODO: when set, the resolved action changes nothing; cleared
    // after each action; its setter is unrecovered.
    i16 pickNoEffect : 1;
    // @identity-TODO: the action's result code; -6..-1 choose how
    // `lastChange` (and `selfChange`) is applied, 1 marks no effect.
    i8 result : 7;
    u8 resultFlag : 1;
    i32 lastChange;
    // @identity-TODO: the HP change an action costs its user (results
    // -5/-4).
    i32 selfChange;
    // The action tally reader accepts all fifteen entries, but the retail
    // reset clears only the first fourteen.
    u8 battleTally[15];
    u16 levelBonus;
    u16 hundredths;
    ItemSlot slots[8];
    u8 levelGap;
    u8 familiarity;
    u8 attitude;
    u8 fieldState;
    u8 mode;
    u8 pad1c7;
    u8 personalFlags[32];
    // @identity-TODO: two dwords cleared whenever a record is loaded (runtime
    // state that is not saved); `hpRollBonus` is added to the HP percentage
    // an HP-roll branch compares.
    u32 clearedOnLoad[2];
    u8 hpRollBonus;
    WordList skills;
    // @identity-TODO: COMP is available when the low three bits equal one.
    u8 compState : 3;
    u8 : 5;
    // @identity-TODO: the row of the 28-per-row moon table (0x47beac) the
    // member's moon value is read from (0x417a10).
    i8 moonRow;
    // Roster characters retain alignment history; field actors reuse these
    // bytes for their map position, facing and removal state.
    union {
        AlignmentInfo alignmentA;
        struct {
            MapCoord fieldPosition;
            i16 facing;
            u8 fieldStateReserved[10];
        };
    };
    union {
        AlignmentInfo alignmentB;
        i16 fieldHidden;
    };
    // @identity-TODO: initialized to 10000, 0 and 1 and retained in saved
    // records; no interpreted reader of these fields is known.
    i32 unusedValue;
    u8 unusedBytes[2];
} Character;

static __inline void SetCharacterPickTarget(Character* actor, i16 target) {
    actor->pickTarget = target;
    actor->pickTargetHigh = 0;
}

#define GetCharacterEquipment(character) ((character)->slots)

#define GetCharacterAffiliation(character, index) ((character)->affiliation[(index)])
#define SetCharacterAffiliation(character, index, value)                                           \
    ((character)->affiliation[(index)] = (value))

static __inline u8* GetCharacterBattleTallies(Character* character) {
    return character->battleTally;
}

#define AbsorbShieldDamage(character, damage)                                                      \
    do {                                                                                           \
        (character)->shield -= (damage);                                                           \
        if ((character)->shield < 0) {                                                             \
            (character)->shield = 0;                                                               \
        }                                                                                          \
    } while (0)

static __inline b32 IsSkillAction(const Character* character) {
    return character->pickRole == 4 || character->pickRole == 6;
}

#define GetCharacterActionWait(character) (&(character)->actionWait)

static __inline ConditionSet* GetCharacterConditions(Character* character) {
    return &character->conditions;
}

static __inline WordList* GetCharacterSkills(Character* character) {
    return &character->skills;
}

static __inline u8* GetCharacterFlags(Character* character) {
    return character->personalFlags;
}

#define TestCharacterFlag(character, index) TestBit(GetCharacterFlags(character), (index))

#define SetCharacterFlag(character, index) SetBit(GetCharacterFlags(character), (index))

#define ClearCharacterFlag(character, index) ClearBit(GetCharacterFlags(character), (index))

#define ChangeCharacterFlag(character, index, op)                                                  \
    ChangeBit(GetCharacterFlags(character), (index), (op))

static __inline void SetCharacterChanges(Character* character, i32 targetChange, i32 selfChange) {
    character->lastChange = targetChange;
    character->selfChange = selfChange;
}

#define SetCharacterResult(character, value, flag)                                                 \
    do {                                                                                           \
        (character)->result = (value);                                                             \
        (character)->resultFlag = (flag);                                                          \
    } while (0)

#define IsCharacterHpLow(character) ((character)->pools.hp.cur * 5 < (character)->pools.hp.max)

#define IsHumanCharacter(character) ((character)->id < 32)

#define GetAlignmentClassA(character) AlignmentClass((character)->alignmentLevelA)
#define GetAlignmentClassB(character) AlignmentClass((character)->alignmentLevelB)

#define GetBattleStatBase(character, stat) ((character)->battleStats[(stat)])

#define GetBattleStatShown(character, stat) ((character)->battleStatsShown[(stat)])

static __inline i16* GetBattleStatGroup(Character* character, i16 group) {
    return &character->battleStatsShown[group * 6];
}

#define GetTrainingPoints(character, kind) ((character)->trainingPoints[(kind)])

#define ResetBattleStatsToBase(character)                                                          \
    memcpy(                                                                                        \
        (character)->battleStatsShown,                                                             \
        (character)->battleStats,                                                                  \
        sizeof((character)->battleStats)                                                           \
    )

static __inline i16 GetBaseStat(const Character* character, i16 stat) {
    return character->stats.base[stat];
}

static __inline i32 GetStatBonus(const Character* character, i16 stat) {
    return character->stats.bonus[stat] + character->stats.modifiers[stat];
}

#define GetStatEquipment(character, stat) ((character)->stats.equipment[(stat)])

#define GetStatTotal(character, stat) ((character)->stats.total[(stat)])

#define SetStatTotal(character, stat, value) ((character)->stats.total[(stat)] = (value))

static __inline i32 GetSummonMagnetiteCost(const Character* character) {
    return character->levelBonus * character->level;
}

extern Character g_characters[16];

Character* GetCharacter(i16 slot);
i16 GetCharacterId(i16 slot);

void RecalcCharacterStats(Character* character);

RVA_DECL(0x000404f0)
void UnequipPart(i16 slot, i16 part);

void FullyRestoreCharacter(Character* character);

void InitCharacters(void);
void InitCharacterSlot(i16 slot, i16 id, const char* prefix, const char* name, i16 memberClass);

Character* CopyCharacterCore(Character* source, Character* destination);
Character* LoadCharacterCore(i16 id, Character* destination);
i16 FindCharacter(i16 id);

#endif // GITEN_GAME_CHARACTER_H
