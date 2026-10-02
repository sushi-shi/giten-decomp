#ifndef GITEN_GAME_ATTACK_H
#define GITEN_GAME_ATTACK_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/AttackAttribute.h>
#include <Game/Character.h>
#include <Game/Condition.h>
#include <Ints.h>

// Shared state of the attack being resolved, defined in Game/attack.c.
extern i16 g_attackResistance;
extern GZ_ENUM_STORAGE(AttackAttribute, i16) g_attackAttribute;
extern i16 g_attackCondition;

static __inline double GetExceptionalAttackLuck(Character* actor) {
    double value = GetStatTotal(actor, STAT_INTUITION);
    value *= 0.5;
    value += GetStatTotal(actor, STAT_FORTUNE);
    return value;
}

static __inline i32 GetExceptionalAttackBase(Character* actor) {
    return GetStatTotal(actor, STAT_PROTECTION) + GetStatTotal(actor, STAT_INTUITION);
}

#define ApplyAttackAccuracyConditions(character, accuracy)                                         \
    do {                                                                                           \
        if (HasCondition(GetCharacterConditions(character), CONDITION_BLIND)) {                    \
            (accuracy) /= 4;                                                                       \
            if ((accuracy) < 1) {                                                                  \
                (accuracy) = 1;                                                                    \
            }                                                                                      \
        }                                                                                          \
    } while (0)

// Both integer condition rolls and floating-point damage apply these boosts.
#define ApplyWeaponPowerConditions(character, power)                                               \
    do {                                                                                           \
        if (HasCondition(GetCharacterConditions(character), CONDITION_DANCE)) {                    \
            (power) *= 2;                                                                          \
        }                                                                                          \
        if (HasCondition(GetCharacterConditions(character), CONDITION_BERSERK)) {                  \
            (power) *= 2;                                                                          \
        }                                                                                          \
    } while (0)

#define FinalizeAttackDamage(damage, attacker)                                                      \
    do {                                                                                           \
        (damage) = ScaleActionValue((damage), g_attackResistance, 2);                              \
        (damage) = ScaleByMoonValue((damage), (attacker)->moonRow, 2);                              \
        (damage) = RandomPercent((damage), -20, 20);                                               \
        (damage) = ClampInt((damage) / 100, 0, 0x7fffffff);                                         \
    } while (0)

#define ApplyFacingDamageBonus(amount, facing)                                                     \
    do {                                                                                           \
        if ((facing) == FACING_FROM_BEHIND) {                                                       \
            (amount) *= 1.5;                                                                       \
        } else if ((facing) != FACING_FACE_TO_FACE) {                                               \
            (amount) *= 1.2;                                                                       \
        }                                                                                          \
    } while (0)

b16 RollWeaponCondition(
    Character* attacker,
    Character* target,
    i16 resistance,
    i16 condition,
    i16 mode
);
i32 ComputeWeaponDamage(Character* attacker, Character* target, i16 result);
b16 RollWeaponHit(Character* attacker, Character* target, i16 resistance);
i16 RollExceptionalWeaponAttack(Character* attacker, Character* target, i16 mode, i16 resistance);

i16 GetEquipmentHitModifier(Character* attacker, Character* target);
// GetCombatantFacingDifference: face to face, or `first` behind `second` (both
// facing the same way); the other two values are side-on.
GZ_ENUM_CONST_BEGIN(CombatFacing)
    FACING_FACE_TO_FACE = 0,
    FACING_FROM_BEHIND = 2
GZ_ENUM_CONST_END(CombatFacing)

i16 GetCombatantFacingDifference(i16 first, i16 second);
i16 GetCombatantDistance(i16 first, i16 second);
i16 GetCombatantAttackRange(i16 id);
i16 GetAttackRangeExcess(i16 first, i16 second);
i16 RollExceptionalAttack(Character* attacker, Character* target, i16 mode, i16 resistance);
b16 RollGunHit(Character* attacker, Character* target, i16 resistance);
i16 GetGunAttackPower(Character* attacker);
i32 ComputeGunDamage(Character* attacker, Character* target, i16 result);
b16 RollGunCondition(Character* attacker, Character* target, i16 resistance, i16 condition);

// The field battle's attack rolls; each sets the attacker's `lastChange` and
// `result` and the action globals of Game/BattleEffect.h.
// @identity-TODO: what `mode` (IsFieldModeAtLeast(0) for an object target, else
// 0) changes is unrecovered.

// An attack with the weapon (equipment slot 5).
b16 ResolveWeaponAttack(Character* attacker, Character* target, i16 mode);

// An attack with the gun (slot 6) and its ammunition (slot 7).
b16 ResolveGunAttack(Character* attacker, Character* target, i16 mode);

b16 RollSkillHit(Character* attacker, Character* target, b16 sameSide);
i32 ComputeSkillDamage(Character* attacker, Character* target, b16 hit);
b16 RollSkillCondition(Character* attacker, Character* target, i16 resistance, i16 condition);

// Whether a resistance result suppresses the follow-up, reflects it to the
// user, or leaves it for the target.
GZ_ENUM_BEGIN_SPLIT(ResistanceFollowup, i16)
    RESISTANCE_FOLLOWUP_REFLECT = -1,
    RESISTANCE_FOLLOWUP_SUPPRESS = 0,
    RESISTANCE_FOLLOWUP_TARGET = 1
GZ_ENUM_END_SPLIT(ResistanceFollowup)

GZ_ENUM_RETURN(ResistanceFollowup, i16)
ApplySkillResistanceOutcome(Character* attacker, i32 amount);
b16 ResolveSkillAttack(Character* attacker, Character* target);

// Spends the rounds a party member's gun attack used.
void SpendGunRounds(Character* attacker);

// Keeps the gun's targets up to the rounds loaded; returns the new count.
i16 FilterGunTargets(Character* attacker, i16 count);
i16 GetGunBurstRounds(Character* attacker);
i16 PrepareGunBurst(Character* attacker, i16 count);
i16 GetGunRequirementPenalty(i16 stat, i16 requirement);
i16 DistributeGunRounds(i16 rounds, i16 count);
i16 ComputeGunBurstPower(i16 rounds);

// Spends every round the gun attack used.
void SpendAllGunRounds(Character* attacker);

// The percentage weights that divide a gun burst among its targets.
void LoadGunDistributionTable(void);

#endif // GITEN_GAME_ATTACK_H
