#ifndef GITEN_GAME_ITEMEFFECT_H
#define GITEN_GAME_ITEMEFFECT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/AttackAttribute.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/InflictCode.h>
#include <Game/RestoreEffect.h>
#include <Ints.h>

// The condition a restorative item or a skill inflicts once its effect lands.
extern i16 g_pendingCondition;

// The item-effect handlers ApplyItemEffect picks by the used item's kind:
// kind 1 restores HP/MP (and may inflict a condition), kind 4 attacks, kind 5
// and kinds 6..19 change nothing.
// @identity-TODO: the mixed effects represented by kind 5 remain unrecovered.
void UseRestoreItem(Character* user, Character* target);
void UseAttackItem(Character* user, Character* target);
void UseKind5Item(Character* user, Character* target);
void UseInertItem(Character* user, Character* target);

// The amount a restore code gives against `max`: 0 none, 0xff all, 0xfe half,
// 0xfd a quarter, else scaled by `user`.
i16 ComputeRestoreAmount(i16 code, Character* user, u16 max);

// Applies an HP/MP restoration of kind `kind` to `target`; a reported effect
// lets a condition follow.
GZ_ENUM_RETURN(RestoreResult, i16)
ApplyRestoreEffect(GZ_ENUM_PARAM(RestoreEffect, i16) kind, i16 hp, Character* target, i16 mp);

#define RestoreEffectAllowsCondition(result)                                                       \
    ((result) >= RESTORE_RESULT_EFFECT_REPORTED && (result) < RESTORE_RESULT_NO_EFFECT_REPORTED)

// Clears a restoration group and records the last condition that was present.
static __inline void
ClearEffectConditions(ConditionSet* conditions, const GZ_ENUM_STORAGE(ConditionId, i16) * list) {
    g_effectCondition = LastConditionIn(conditions, list);
    ClearConditionList(conditions, list);
}

// The condition an inflict code gives `target`: codes 1..34 are conditions
// themselves, 57..65 pick one by chance, alignment or demon class; -1 none.
// @identity-TODO: what the coded conditions name is unrecovered.
GZ_ENUM_RETURN(ConditionId, i16) ResolveInflictedCondition(GZ_ENUM_PARAM(InflictCode, i16) code, Character* target);

// Gives `target` the condition of inflict code `code` (recomputing its stats
// when it newly gains condition 8).
void InflictCondition(GZ_ENUM_PARAM(InflictCode, i16) code, Character* target);

// Whether `target` resists inflict code `code` (1 when it gives no condition):
// whether an equipped item's resistance code (0x77..0x85) covers it.
i16 IsConditionResisted(Character* target, i16 code);
b16 ItemResistsCondition(i16 item, GZ_ENUM_PARAM(ConditionId, i16) condition);

// Rolls whether the item attack hits, recording resistance and action result.
b16 ResolveItemAttack(Character* user, Character* target, i16 sameSide);
i32 ComputeItemDamage(Character* attacker, Character* target, i16 hit);
b16 RollItemCondition(Character* attacker, Character* target, i16 resistance, i16 condition);

// Resolves an attack item's damage and returns whether it inflicted a condition.
b16 RunItemAttack(Character* user, Character* target);

// The equipment's per-element damage ratios: ApplyItemDamageRatio scales a
// ratio (percent, ten elements) by an item's code 0x40..0x67 (element code/4,
// step 0, 50, 150 or 200 percent by code%4), ScaleDamageByEquipment applies
// all eight slots' items to `damage` of element `element`.
// @identity-TODO: what the elements and codes name is unrecovered.
void ApplyItemDamageRatio(i16 item, i16* ratios);
i16 ScaleDamageByEquipment(Character* character, i16 damage, i16 element);

// Whether item `item` guards element `element` (2..5, its record byte +0x28);
// how many equipped items of `character` guard it.
b16 IsItemGuardingElement(i16 item, i16 element);
i16 CountElementGuards(Character* character, i16 element);

// The HP and MP an item's regeneration code (0x70..0x74) adds per turn.
typedef struct PoolRegen {
    i16 hp;
    i16 mp;
} PoolRegen;

void AddItemRegen(i16 item, PoolRegen* regen);

// Adds the regeneration of `character`'s equipped items to its HP and MP
// (unless a fatal condition holds); returns the amounts.
PoolRegen ApplyEquipmentRegen(Character* character);

// charpool's recomputation of the battle stats, which InflictCondition
// calls. Codegen constraint: also declared here because <Game/Stats.h>
// shifts itemrecord's SetBagEntry.
void RecalcDerivedStats(Character* character);

// Whether field-mode attacks suppress this condition on field actors.
// Codegen constraint: declared here; in <Game/Condition.h> it shifts
// fieldobj's TU state (RelativeFacing).
b16 IsFieldConditionRestricted(GZ_ENUM_PARAM(ConditionId, i16) condition);

#endif // GITEN_GAME_ITEMEFFECT_H
