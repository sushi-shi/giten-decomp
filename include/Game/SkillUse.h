#ifndef GITEN_GAME_SKILLUSE_H
#define GITEN_GAME_SKILLUSE_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/Character.h>
#include <Game/GameState.h>
#include <Game/Skill.h>
#include <Ints.h>

#include <string.h>

// The member (negative id: party slot -1 - id) or field object `id` stands
// for; NULL for an object that is gone.
Character* GetCombatant(i16 id);
i16 CountUsableMemberSkills(Character* character, i16 checkCost);

// Makes field object `object` flash when `change` is not 0; 1 when it did.
b16 FlashHitObject(i16 object, i32 change);

// A combatant's map cell.
MapCoord GetCombatantCoord(i16 id);

// Resolves the actor's picked action on its target; returns the target's HP.
i16 ResolveCombatAction(void);

// Handles combatant `id` dropping from `previousHp` (defeat, knockout).
void ResolveKnockout(i16 previousHp, i16 id);

// Removes every occurrence of id and returns the remaining target count.
i16 RemoveCombatTarget(i16 id);
i16 AddCombatTarget(i16 id, i16 allowDuplicate);
i16 CountCombatTargets(void);

void SetActionOutcome(i16 outcome);
b32 CannotPaySkill(Character* character, SkillParameters* skill);
b32 IsSkillBlocked(Character* character, SkillParameters* skill);
b32 IsSkillIdBlocked(Character* character, i16 id);
void PaySkillCost(i16 who, i16 skill);

// NextTarget's value once the target list is empty.
#define TARGET_LIST_END (-0x8000)

// Loads the chosen skill's action values (condition, result, HP change) into
// the action globals before its prompt.
void PrepareSkillAction(void);

// The next combatant of the action's target list; 0x8000 when it is empty.
i16 NextTarget(void);

// Fills the action's target list from its area `area`, `flags` and `range`
// around `target` (for `actor`); returns the count.
// @identity-TODO: the area/flag/range codes (the skill header bytes +5..+7,
// the item record bytes +0xa..+0xc) are undecoded.
i16 CollectTargets(i16 area, i16 flags, i16 range, i16 target, i16 actor);

i16 CollectTargetsAlongLine(i16 area, i16 flags, i16 range, i16 target, i16 actor);
i16 CollectTargetsInView(i16 area, i16 flags, i16 range, i16 target, i16 actor);
i16 CollectTargetsAtCell(i16 area, i16 flags, i16 range, i16 target, i16 actor, i16 x, i16 y);
i16 AddRelatedCombatTargets(i16 x, i16 y, i16 flags, i16 target, i16 actor);
i16 AddObjectTargetsAtCell(i16 x, i16 y);
i16 AddPartyTargetsAtCell(i16 x, i16 y);

// The zero-initialized empty skill label.
extern char g_emptySkillMenuLabel[];

void UseAttackSkill(Character* user, Character* target);
void UseRestoreSkill(Character* user, Character* target);
void UseBattleTallySkill(Character* user, Character* target);
GZ_ENUM_BEGIN_SPLIT(BattleStatEffectKind, u8)
    BATTLE_STAT_EFFECT_WEAPON_GUN_POWER = 0,
    BATTLE_STAT_EFFECT_WEAPON_GUN_ACCURACY = 1,
    BATTLE_STAT_EFFECT_WEAPON_GUN_DEFENSE = 2,
    BATTLE_STAT_EFFECT_MAGIC_ATTACK = 3,
    BATTLE_STAT_EFFECT_MAGIC_DEFENSE = 4,
    BATTLE_STAT_EFFECT_MAGIC_ALL = 5
GZ_ENUM_END_SPLIT(BattleStatEffectKind)

GZ_ENUM_CONST_BEGIN(BattleStatEffectEncoding)
    BATTLE_STAT_EFFECT_LOWER = 0x80,
    BATTLE_STAT_EFFECT_KIND_MASK = 0x7f
GZ_ENUM_CONST_END(BattleStatEffectEncoding)

void UseBattleStatSkill(Character* user, Character* target);
i16 ChangeBattleStat(i16* value, i16 amount, i16 base);

#define ChangeCharacterBattleStat(character, stat, amount)                                         \
    ChangeBattleStat(                                                                              \
        &(character)->battleStatsShown[(stat)],                                                    \
        (amount),                                                                                  \
        GetBattleStatBase(character, stat)                                                         \
    )

void UseClearBattleTallySkill(Character* user, Character* target);
void UseResetBattleStatsSkill(Character* user, Character* target);

// @identity-TODO: kinds 12..14 share the field-effect operation; their
// separate domain meanings are unproven.
void UseKind12Skill(Character* user, Character* target);
void UseKind13Skill(Character* user, Character* target);
void UseKind14Skill(Character* user, Character* target);
void UseFieldEffectSkill(Character* user, Character* target);
void UseInertSkill(Character* user, Character* target);

// Runs the action's message script (stage 0 before, 1 after the change).
void PlayActionEffect(i16 stage);

// Removes party member `id` from the party when its personal flag 0x3f is
// set, clearing the flag.
void DropFlaggedMember(i16 id);

// The battle protection or restriction stored at a combatant's tally index.
// Indices 4..6 and the attributes behind them still need fuller identity.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(BattleTallyIndex, i16)
    BATTLE_TALLY_MAGIC_SEAL = 0,
    BATTLE_TALLY_MAGIC_REFLECT = 1,
    BATTLE_TALLY_MAGIC_REFLECT_HALF = 2,
    BATTLE_TALLY_MAGIC_MP_ABSORB = 3,
    BATTLE_TALLY_GUN_BLOCK = 7,
    BATTLE_TALLY_FIRE_BLOCK = 8,
    BATTLE_TALLY_ICE_BLOCK = 9,
    BATTLE_TALLY_ELECTRIC_BLOCK = 10,
    BATTLE_TALLY_EXPEL_BLOCK = 11,
    BATTLE_TALLY_DARK_BLOCK = 12,
    BATTLE_TALLY_ALL_BLOCK = 13,
    BATTLE_TALLY_MAGIC_GUN_BLOCK = 14
GZ_ENUM_END_SPLIT(BattleTallyIndex)
// clang-format on

// Reads a battle tally byte; mode 1 remembers `index`, while mode -1 uses
// the remembered index and tests whether the byte clears.
i16 ReportBattleTally(Character* combatant, i16 index, i16 mode);

// Clears the combatant's 14 battle tally bytes.
void ClearBattleTally(Character* combatant);

static __inline void ClearAllBattleTallies(Character* combatant) {
    i16 index;
    for (index = 0; index < sizeof(combatant->battleTally); index++) {
        GetCharacterBattleTallies(combatant)[index] = 0;
    }
}

// Clears action tallies and the absorbed-damage shield.
#define ResetBattleTally(combatant)                                                                \
    do {                                                                                           \
        ClearBattleTally(combatant);                                                               \
        (combatant)->shield = 0;                                                                   \
    } while (0)

// Shows the knocked-out combatant's message.
void ShowKnockoutMessage(void);

// The phases of an actor's action on the field (RunBattleAction): collect its
// targets and fly its shot, show a pending prompt, resolve it on the current
// target, apply its effect and the object conditions, show knockouts, report
// a battle byte, move to
// the next living target, and end the action.
GZ_ENUM_BEGIN(BattleActionPhase)
    BATTLE_ACTION_PHASE_COLLECT_TARGETS = 0,
    BATTLE_ACTION_PHASE_PROMPT = 1,
    BATTLE_ACTION_PHASE_RESOLVE = 2,
    BATTLE_ACTION_PHASE_APPLY_EFFECT = 3,
    BATTLE_ACTION_PHASE_APPLY_CONDITIONS = 4,
    BATTLE_ACTION_PHASE_SHOW_KNOCKOUTS = 5,
    BATTLE_ACTION_PHASE_REPORT = 6,
    BATTLE_ACTION_PHASE_NEXT_TARGET = 7,
    BATTLE_ACTION_PHASE_END = 8
GZ_ENUM_END(BattleActionPhase)

b16 RunBattleAction(void);

#endif // GITEN_GAME_SKILLUSE_H
