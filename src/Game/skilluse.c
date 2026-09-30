// @identity-TODO: the owning TU is unproven. One retail object: the battle and
// field skill-use flows. Their .bss statics interleave in one run (the field
// skill user, pick and menu words sit between the battle action's), each read
// only by its own flow's code, and the field flow's code follows the battle
// flow's in .text.

#include <rva.h>

#include <Game/ActionOutcome.h>
#include <Game/Actor.h>
#include <Game/ActorFlag.h>
#include <Game/AreaNpc.h>
#include <Game/Attack.h>
#include <Game/Battle.h>
#include <Game/BattleEffect.h>
#include <Game/BattleStat.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/CombatantId.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DropTable.h>
#include <Game/EquipSlotIndex.h>
#include <Game/Familiarity.h>
#include <Game/Field.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/ItemBag.h>
#include <Game/ItemEffect.h>
#include <Game/ItemId.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecordId.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/PartyPick.h>
#include <Game/Skill.h>
#include <Game/SkillId.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/TargetCountMode.h>
#include <Game/TargetFlags.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/Shot.h>
#include <Input/Mouse.h>
#include <Math/Vec3.h>
#include <Script/EventFlags.h>
#include <Sound/Sound.h>
#include <Text/TextAttr.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/FieldMenus.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Ui/MessageScript.h>
#include <Util/BitChangeMode.h>
#include <Util/BitSet.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The conditions that block a skill as they would a spell: severe poison and
// sealed magic.
DATA(0x00064678)
static const i16 s_skillBlockingConditions[] =
    {CONDITION_SUFFOCATION, CONDITION_MAGIC_SEAL, CONDITION_LIST_END};

DATA(0x00064680)
static const i16 s_skillIdBlockingConditions[] =
    {CONDITION_SUFFOCATION, CONDITION_MAGIC_SEAL, CONDITION_LIST_END};

DATA(0x00064688)
static const u8 s_targetCellDistance[7][7] = {
    {6, 5, 4, 3, 4, 5, 6},
    {5, 4, 3, 2, 3, 4, 5},
    {4, 3, 2, 1, 2, 3, 4},
    {3, 2, 1, 0, 1, 2, 3},
    {4, 3, 2, 1, 2, 3, 4},
    {5, 4, 3, 2, 3, 4, 5},
    {6, 5, 4, 3, 4, 5, 6}
};

// @identity-TODO: the requested effect position and mode are stored but
// never read by the Windows action-state implementation.
DATA(0x000690c8)
static i16 s_promptX = -1;

DATA(0x000690cc)
static i16 s_promptY = -1;

DATA(0x000690d0)
static i16 s_promptZ = -1;

DATA(0x000690d4)
static i16 s_promptMode = -1;

// The game sub-state PushPromptState asks for (-1 for none).
DATA(0x000690d8)
static i16 s_promptSub = -1;

// The combatant defeated by the last action (0x7fff for none).
DATA(0x000690dc)
static i16 s_knockedOut = 0x7fff;

// @identity-TODO: the battle byte ReportBattleTally last reported (-1 for
// none) and a message flag checked with it after the action (-1 for none).
DATA(0x000690e0)
static i16 s_reportedTally = -1;

DATA(0x000690e4)
static i16 s_tallyMessage = -1;

DATA(0x00091542)
i16 g_battleOutcome;

DATA(0x00091546)
i16 g_pendingCondition;

DATA(0x0009154c)
i32 g_rewardMacca;

DATA(0x00091552)
i16 g_actionId;

DATA(0x00091986)
i16 g_drainAmount;

DATA(0x00091988)
i32 g_rewardMagnetite;

DATA(0x0009198c)
i16 g_mpChange;

DATA(0x00091990)
i32 g_rewardExperience;

DATA(0x00091994)
i16 g_actionResult;

DATA(0x00091996)
i16 g_targetId;

DATA(0x00091998)
i16 g_hpChange;

DATA(0x0009199a)
GZ_ENUM_STORAGE(InflictCode, i16) g_statusCondition;

DATA(0x000919f0)
i16 g_targetCount;

DATA(0x000919f6)
i16 g_actorId;

// The member (by id) using a skill on the field; -1 asks the picker.
DATA(0x00080a90)
static i16 s_skillUser = 0;

// The skill picked from the member's list.
DATA(0x00080a94)
static i16 s_skillPicked = 0;

DATA(0x00080a98)
static SkillHeader s_effectSkill = {0};

// Which objects the action marked (and whose stuns and conditions it clears
// and applies).
DATA(0x00080ab0)
static i16 s_objectMarks[16] = {0};

DATA(0x00080ad0)
static i16 s_targetList[270] = {0};

// The selected skill's attack range, passed to the target window.
DATA(0x00080cec)
static u8 s_pickRange = 0;

// The user's party position.
DATA(0x00080cf0)
static i16 s_userPosition = 0;

DATA(0x00080cf4)
static i32 s_effectSkillId = 0;

// The number of combatants left on the action's target list.
DATA(0x00080cfc)
static i16 s_targetListCount = 0;

// Set while PushPromptState's prompt is pending.
DATA(0x00080d00)
static b16 s_promptPending = false;

// The picked role of the action being played (PlayActionEffect reads it).
DATA(0x00080d04)
static i16 s_actionRole = 0;

// Set while effects are skipped (SetWorldMapActive): no shots, no redraw.
DATA(0x00080d08)
static i16 s_skipEffects = 0;

// The object-removal deferral saved over the action.
DATA(0x00080d0c)
static i16 s_savedRemovalDeferred = 0;

// @identity-TODO: when set, a hidden object's removal waits (0x42bd09 then
// calls 0x414750).
DATA(0x00080d10)
static b16 s_removalDeferred = false;

// Which hit sound the resolved action plays (0, 1 or 2 pick sounds 0x10,
// 0x36 and 0x24): set by the effect code (0x42ce57, 0x42d02a).
// @identity-TODO: what the three outcomes are is unrecovered.
DATA(0x00080d14)
static i16 s_actionOutcome = 0;

// The actor's and the target's HP before the action.
DATA(0x00080d18)
static i16 s_actorHpBefore = 0;

DATA(0x00080d1c)
static i16 s_targetHpBefore = 0;

// The action's actor and target, and the actor's role and pick, kept until
// the action ends.
DATA(0x00080d20)
static Character* s_actionActor = NULL;

DATA(0x00080d24)
static Character* s_actionTarget = NULL;

DATA(0x00080d28)
static i16 s_actionRoleKept = 0;

DATA(0x00080d2c)
static i16 s_actionPickKept = 0;

// The picker or skill list menu.
DATA(0x00080d30)
static MenuBox* s_fieldMenu = NULL;

DATA(0x00080d34)
char g_emptySkillMenuLabel[4] = {0};

RVA(0x0002aa20, 0x20)
i16 SetWorldMapActive(i16 active) {
    i16 previous = s_skipEffects;
    s_skipEffects = active;
    return previous;
}

static __inline void ClearCombatTargets(void) {
    s_targetListCount = 0;
}

RVA(0x0002aa40, 0x10)
i16 CountCombatTargets(void) {
    return s_targetListCount;
}

RVA(0x0002aa50, 0x50)
i16 AddCombatTarget(i16 id, i16 allowDuplicate) {
    i16 i;
    if (!allowDuplicate) {
        for (i = 0; i < s_targetListCount; i++) {
            if (s_targetList[i] == id) {
                return s_targetListCount;
            }
        }
    }
    if (s_targetListCount < 270) {
        s_targetList[s_targetListCount++] = id;
    }
    return s_targetListCount;
}

RVA(0x0002aaa0, 0x40)
i16 NextTarget(void) {
    i16 target;
    i16 i;
    if (s_targetListCount < 1) {
        return TARGET_LIST_END;
    }
    target = s_targetList[0];
    // Retail copies one extra entry, including past the array when full.
    for (i = 0; i < s_targetListCount; i++) {
        s_targetList[i] = s_targetList[i + 1];
    }
    s_targetListCount--;
    return target;
}

RVA(0x0002aae0, 0x60)
i16 RemoveCombatTarget(i16 id) {
    i16 i;
    i16 next;
    for (i = 0; i < s_targetListCount; i++) {
        if (s_targetList[i] == id) {
            for (next = i + 1; next < s_targetListCount; next++) {
                s_targetList[next - 1] = s_targetList[next];
            }
            s_targetListCount--;
            i--;
        }
    }
    return s_targetListCount;
}

RVA(0x0002ab40, 0x60)
b16 PushPromptState(i16 sub, i16 x, i16 y, i16 z, i16 mode) {
    if (s_promptPending) {
        return true;
    }
    PushGameState(GAME_STATE_BATTLE_ACTION);
    s_promptX = x;
    s_promptY = y;
    s_promptZ = z;
    s_promptMode = mode;
    s_promptSub = sub;
    s_promptPending = true;
    return false;
}

RVA(0x0002aba0, 0x40)
MapCoord GetCombatantCoord(i16 id) {
    if (id >= 0) {
        MapCoord point;
        FieldActor* actor = (FieldActor*)GetFieldActor(id);
        point.x = actor->pos.x;
        point.y = actor->pos.y;
        return point;
    }
    return GetMapCoord();
}

RVA(0x0002abe0, 0x33)
Character* GetCombatant(i16 id) {
    if (id < 0) {
        return GetPartyCharacter(CombatantPartyPosition(id));
    }
    id = GetLiveObject(id);
    if (id < 0) {
        return NULL;
    }
    return GetFieldActor(id);
}

RVA(0x0002ac20, 0x29)
b16 FlashHitObject(i16 object, i32 change) {
    if (change != 0) {
        GetFieldObject(object)->redraw = 2;
        RedrawFieldView();
        return true;
    }
    return false;
}

RVA(0x0002ac50, 0x40)
void AlertActor(Character* actor, GZ_ENUM_PARAM(Attitude, i16) state) {
    if (actor) {
        if (state >= 0) {
            actor->attitude = state;
        }
        SetCharacterFlag(actor, ACTOR_FLAG_BATTLE);
        SetCharacterFlag(actor, ACTOR_FLAG_NOTICED);
        actor->mode = ACTOR_MODE_PURSUE;
    }
}

#define CanAffectCombatant(id) (!IsEventFlagSet(1, 0x2d) || (id) >= 0)

static __inline void ApplyReflectedDamage(Character* actor) {
    g_hpChange = actor->selfChange;
    ChangePool(&actor->pools.hp, -actor->selfChange);
}

#define ApplyShieldedDamage(target, amount)                                                        \
    do {                                                                                           \
        if ((target)->shield != 0) {                                                               \
            AbsorbShieldDamage((target), (amount));                                                \
        } else {                                                                                   \
            ChangePool(&(target)->pools.hp, -(amount));                                            \
        }                                                                                          \
    } while (0)

static __inline void ApplyCombatDamage(Character* attacker, Character* target) {
    i16 kind;
    if (IsSkillAction(attacker)) {
        kind = GetSkillKind(attacker->pickTarget);
        switch (kind) {
            case SKILL_KIND_MP_DAMAGE:
                if (target->shield == 0) {
                    ChangePool(&target->pools.mp, -attacker->lastChange);
                }
                g_mpChange = attacker->lastChange;
                return;
            case SKILL_KIND_HP_DRAIN:
                ChangePool(&attacker->pools.hp, attacker->lastChange);
                g_actionResult |= 0x50;
                break;
            case SKILL_KIND_MP_DRAIN:
                ChangePool(&attacker->pools.mp, attacker->lastChange);
                ChangePool(&target->pools.mp, -attacker->lastChange);
                g_actionResult |= 0x70;
                g_mpChange = attacker->lastChange;
                return;
            case SKILL_KIND_EXPERIENCE_DRAIN:
                attacker->lastChange = min(target->experience, attacker->lastChange);
                target->experience -= attacker->lastChange;
                attacker->experience += attacker->lastChange;
                g_actionResult |= 0x80;
                g_drainAmount = attacker->lastChange;
                return;
        }
    }
    ApplyShieldedDamage(target, attacker->lastChange);
    if (attacker->pickRole == PICK_ROLE_ATTACK
        && GetCharacterEquipment(attacker)[EQUIP_SLOT_WEAPON].item >= 1) {
        kind = GetItemPassiveEffectCode(
            GetLoadedRecord(GetCharacterEquipment(attacker)[EQUIP_SLOT_WEAPON].item)
        );
        if (kind == 0x86) {
            ChangePool(&attacker->pools.hp, attacker->lastChange);
        } else if (kind == 0x87) {
            ChangePool(&attacker->pools.mp, attacker->lastChange);
        }
    }
}

// Resolves the actor's picked action (weapon, gun, item or skill) on its
// target: rolls it, pays its cost, plays the hit sound, applies the change by
// the action's result code (the draining skills move HP, MP or experience),
// then handles knockouts and turns the struck object toward the party.
// @early-stop CFG/register allocation: shared ChangePool tails merge at
// different points. Retail retains the fatal condition in ebx; this build
// spills it while retaining the knockout sentinel in ebp. Declaration order,
// register hints and the prior permutation frontier were byte-flat.
RVA(0x0002ac90, 0x800)
i16 ResolveCombatAction(void) {
    Character* attacker;
    Character* target;
    CurMax* targetHp;
    i16 fatal;
    i16 hit;
    i16 kind;

    s_actionOutcome = ACTION_OUTCOME_DEFAULT;
    s_knockedOut = 0x7fff;
    attacker = GetCombatant(g_actorId);
    if (attacker == NULL) {
        return 0;
    }
    g_targetId = attacker->pickObject;
    target = GetCombatant(attacker->pickObject);
    if (target == NULL) {
        return 0;
    }
    s_actorHpBefore = attacker->pools.hp.cur;
    s_targetHpBefore = target->pools.hp.cur;
    targetHp = &target->pools.hp;
    fatal = GetFatalCondition(GetCharacterConditions(target));
    if (attacker->id == OBJECT_RECORD_MARDUK && target->id == OBJECT_RECORD_PRIMROSE) {
        SetFieldCounts(-2, -2);
    }
    if (target->id == OBJECT_RECORD_PYANKARA && attacker->pickRole == PICK_ROLE_ITEM
        && attacker->pickTarget == SKILL_NOELEM) {
        SetFieldCounts(-2, -2);
    }
    ResetActionWait(GetCharacterActionWait(attacker));
    ResetActionOutcome();
    attacker->pickNoEffect = false;
    if (attacker->pickFlags & PICK_ITEM_SKILL) {
        attacker->pickCostPaid = true;
    }

    if (attacker->pickRole == PICK_ROLE_ATTACK) {
        attacker->pickTarget = GetCharacterEquipment(attacker)[EQUIP_SLOT_WEAPON].item;
        if (g_targetId >= 0) {
            ResolveWeaponAttack(attacker, target, IsFieldModeAtLeast(false));
        } else {
            ResolveWeaponAttack(attacker, target, 0);
        }
    } else if (attacker->pickRole == PICK_ROLE_GUN) {
        attacker->pickTarget = GetCharacterEquipment(attacker)[EQUIP_SLOT_AMMO].item;
        if (g_targetId >= 0) {
            ResolveGunAttack(attacker, target, IsFieldModeAtLeast(false));
        } else {
            ResolveGunAttack(attacker, target, 0);
        }
        SpendGunRounds(attacker);
    } else if (attacker->pickRole == PICK_ROLE_ITEM) {
        ApplyItemEffect(attacker->pickTarget, attacker, target);
    } else if (IsSkillAction(attacker)) {
        ApplySkillEffect(attacker->pickTarget, attacker, target);
        WearCachedSkill();
    }

    hit = attacker->lastChange != 0;
    attacker->lastChange = ScaleByFieldRate(g_actorId, attacker->pickObject, attacker->lastChange);
    if (hit && attacker->lastChange == 0) {
        SetActionResult(attacker, BATTLE_ACTION_NO_EFFECT);
    }
    TickFieldCount(g_actorId, false);
    g_hpChange = attacker->lastChange;
    if (g_hpChange >= 0x7fff) {
        g_actionResult = BATTLE_ACTION_LETHAL;
    }
    if (fatal && GetFatalCondition(GetCharacterConditions(target))) {
        SetActionResult(attacker, BATTLE_ACTION_NO_EFFECT);
        g_statusCondition = INFLICT_NONE;
        attacker->lastChange = 0;
        ResetPoolChanges();
        s_targetHpBefore = max(1, s_targetHpBefore);
    }

    if (IsSkillAction(attacker)) {
        if (CanAffectCombatant(g_actorId) && !attacker->pickCostPaid) {
            PaySkillCost(g_actorId, attacker->pickTarget);
            attacker->pickCostPaid = true;
        }
    }

    if (CanAffectCombatant(g_targetId) && !attacker->pickNoEffect) {
        if (g_actionResult >= BATTLE_ACTION_GRAZED && g_actionResult != BATTLE_ACTION_IMMUNE) {
            if (s_actionOutcome == ACTION_OUTCOME_DEFAULT) {
                PlaySoundEffect(0x10);
            } else if (s_actionOutcome == ACTION_OUTCOME_CONDITION) {
                PlaySoundEffect(0x36);
            } else if (s_actionOutcome == ACTION_OUTCOME_BATTLE_TALLY) {
                PlaySoundEffect(0x24);
            }
        }
        if (attacker->result == -6) {
            g_hpChange = 0;
        } else if (attacker->result == -4) {
            ApplyReflectedDamage(attacker);
        } else if (attacker->result == -5) {
            ApplyReflectedDamage(attacker);
            ApplyShieldedDamage(target, attacker->lastChange);
        } else if (attacker->result == -3) {
            ChangePool(&target->pools.mp, attacker->lastChange);
        } else if (attacker->result == -1) {
            ChangePool(targetHp, attacker->lastChange);
        } else if (attacker->result == -2) {
            ChangePool(targetHp, attacker->lastChange);
        } else {
            ApplyCombatDamage(attacker, target);
        }
    }

    attacker->pickNoEffect = false;
    if (HasCondition(GetCharacterConditions(target), CONDITION_ZOMBIE)
        && g_actionResult == BATTLE_ACTION_LETHAL) {
        AddCondition(GetCharacterConditions(target), CONDITION_DYING);
    }
    ApplyEmptyPools(attacker);
    ApplyEmptyPools(target);
    if (g_actorId < 0 || g_targetId < 0) {
        RequestFieldRefresh();
    }
    ResolveKnockout(s_targetHpBefore, g_targetId);
    if (g_actorId != g_targetId) {
        ResolveKnockout(s_actorHpBefore, g_actorId);
    }
    if (g_actorId >= 0) {
        GetCharacterFlags(attacker)[1] |= 0x80;
    }
    if (g_targetId >= 0) {
        GetFieldActor(g_targetId)->facing = OppositeDirection(g_party.field.pos.direction);
        AlertActor(target, ATTITUDE_VERY_HOSTILE);
        GetCharacterFlags(target)[1] |= 0x40;
        if (attacker->pickRole == PICK_ROLE_MAGIC) {
            kind = GetCachedSkill(attacker->pickTarget)->parameters.type;
            if (kind == 2 || kind == 3 || kind == 4 || kind == 10 || kind == 11) {
                return targetHp->cur;
            }
            if (FlashHitObject(g_targetId, attacker->lastChange)) {
                RequestFieldRefresh();
                return targetHp->cur;
            }
        } else if (FlashHitObject(g_targetId, attacker->lastChange)) {
            RequestFieldRefresh();
        }
    }
    return targetHp->cur;
}

RVA(0x0002b490, 0x160)
void ResolveKnockout(i16 previousHp, i16 id) {
    Character* combatant = GetCombatant(id);
    if (!combatant) {
        return;
    }
    if (!TestCharacterFlag(combatant, ACTOR_FLAG_DESAMAN)) {
        if (previousHp == 0) {
            return;
        }
        if (!GetFatalCondition(GetCharacterConditions(combatant))) {
            if (HasCondition(GetCharacterConditions(combatant), CONDITION_ZOMBIE)) {
                if (combatant->pools.mp.cur != 0) {
                    return;
                }
            } else if (combatant->pools.hp.cur != 0) {
                return;
            }
        }
    }
    if (!TestCharacterFlag(combatant, ACTOR_FLAG_DESAMAN)) {
        combatant->pools.hp.cur = 0;
        s_knockedOut = id;
        if (id >= 0) {
            AddCondition(GetCharacterConditions(combatant), CONDITION_DEAD);
            RunFieldIdle();
        } else {
            ApplyEmptyPools(combatant);
        }
        SetAnalyzed(combatant->id, 1);
        if (id < 0) {
            RemoveCombatTarget(id);
            return;
        }
        g_rewardMacca += combatant->macca;
        g_rewardMagnetite += combatant->magnetite;
        g_rewardExperience += combatant->experience;
        if (combatant->dropChance > RandomUpTo(99)) {
            AddDropSlot(combatant->pickItem, 1);
        }
    } else {
        if (id < 0) {
            RemoveCombatTarget(id);
            return;
        }
        ClearCharacterFlag(combatant, ACTOR_FLAG_DESAMAN);
    }
    ResetObjectAnim(id);
    RunFieldIdle();
    RemoveCombatTarget(id);
}

RVA(0x0002b5f0, 0x82)
i16 ReportBattleTally(Character* combatant, i16 index, i16 mode) {
    i16 tally;
    u8* value;
    if (combatant == NULL) {
        return 0;
    }
    if (mode == 1) {
        s_reportedTally = index;
        return GetCharacterBattleTallies(combatant)[index];
    }
    tally = mode == -1 ? s_reportedTally : index;
    if (tally < 0) {
        return -1;
    }
    value = &GetCharacterBattleTallies(combatant)[tally];
    if (*value < RandomAverage(1, 40, 2)) {
        *value = 0;
        s_tallyMessage = tally;
    }
    return *value;
}

RVA(0x0002b680, 0x18)
void ClearBattleTally(Character* combatant) {
    i16 index;
    for (index = 0; index < 14; index++) {
        GetCharacterBattleTallies(combatant)[index] = 0;
    }
}

static __inline i16 CollectCurrentSkillTargets(const SkillParameters* skill) {
    return CollectTargets(
        skill->targetArea,
        skill->targetFlags,
        skill->targetCounts,
        g_targetId,
        g_actorId
    );
}

static __inline void ClearActionActors(void) {
    s_actionActor = NULL;
    s_actionTarget = NULL;
}

static __inline void ClearPendingAction(void) {
    s_promptPending = false;
    ClearActionActors();
}

static __inline void CancelPendingAction(void) {
    ClearPendingAction();
    ReturnFromGameState();
    RestoreSwappedMember();
}

static __inline void RefreshAfterDeferredRemoval(void) {
    if (s_removalDeferred) {
        RequestFieldRefresh();
    }
}

// Runs one phase of the actor's action on the field: 0 collects its targets
// and flies its shot, 1 shows a pending prompt, 2 resolves it on the current
// target, 3..5 apply and settle the change, 6 reports a battle byte, 7 moves
// to the next living target, 8 ends the action (the summoning skill swaps its
// demon into the command position).
static __inline void ResetReportedBattleTally(void) {
    s_tallyMessage = -1;
    s_reportedTally = -1;
}

// Codegen constraint: keep next-target success outside the phase switch;
// an in-loop return or a post-loop sentinel test changes the shared tail.
RVA(0x0002b6a0, 0x9b0)
b16 RunBattleAction(void) {
    Character* actor;
    ItemRecord* record;
    SkillHeader* skill;
    MapCoord from;
    MapCoord to;
    i16 count;
    i16 shot;
    i16 slot;
    i16 skipEffects;

    actor = GetCombatant(g_actorId);
    if (actor != NULL) {
        g_actionId = actor->pickTarget;
    }
    switch (GetGamePhase()) {
        default:
            skipEffects = s_skipEffects;
            goto complete;
        case BATTLE_ACTION_PHASE_COLLECT_TARGETS:
            ResetReportedBattleTally();
            if (actor == NULL) {
                CancelPendingAction();
                return false;
            }
            if (actor->pickRole == PICK_ROLE_DEFENCE) {
                TickFieldCount(g_actorId, false);
                ClearPendingAction();
                ReturnFromGameState();
                ResetActionWaitDelay(GetCharacterActionWait(actor));
                RestoreSwappedMember();
                return false;
            }
            if (actor->pickRole == PICK_ROLE_RETURN && g_actorId < 0) {
                TickFieldCount(g_actorId, false);
                ResetActionWaitDelay(GetCharacterActionWait(actor));
                CheckPickTarget(CombatantPartyPosition(g_actorId));
                SetPartySlot(CombatantPartyPosition(g_actorId), PARTY_SLOT_EMPTY);
                RequestFieldRefresh();
                CancelPendingAction();
                PlaySoundEffect(0x55);
                return false;
            }
            if (GetCombatant(actor->pickObject) == NULL) {
                ResetActionWaitDelay(GetCharacterActionWait(actor));
                CancelPendingAction();
                return false;
            }
            s_actionActor = actor;
            s_actionTarget = GetCombatant(actor->pickObject);
            s_actionRoleKept = actor->pickRole;
            s_actionPickKept = actor->pickTarget;
            NextGamePhase();
            s_actionRole = actor->pickRole;
            actor->pickCostPaid = false;
            if (actor->pickRole == PICK_ROLE_ITEM) {
                g_battleOutcome = 0;
                record = GetLoadedRecord(actor->pickTarget);
                ClearCombatTargets();
                count = CollectTargets(
                    GetItemTargetArea(record),
                    GetItemTargetFlags(record),
                    GetItemTargetCounts(record),
                    g_targetId,
                    g_actorId
                );
                if (actor->pickTarget == ITEM_CORE_SHIELD) {
                    ModifyEventFlag(
                        EVENT_FLAG_BANK_ITEM_EFFECTS,
                        ITEM_EFFECT_CORE_SHIELD,
                        BIT_CHANGE_SET
                    );
                }
                if (actor->pickTarget == ITEM_KUSHINADA_JAR) {
                    ModifyEventFlag(
                        EVENT_FLAG_BANK_ITEM_EFFECTS,
                        ITEM_EFFECT_KUSHINADA_JAR_USED,
                        BIT_CHANGE_SET
                    );
                } else if (actor->pickTarget == ITEM_SOMA_CUP) {
                    ModifyEventFlag(
                        EVENT_FLAG_BANK_ITEM_EFFECTS,
                        ITEM_EFFECT_SOMA_CUP_USED,
                        BIT_CHANGE_SET
                    );
                } else if (GetItemValueHigh(actor->pickTarget)) {
                    TakeBagItems(actor->pickTarget, 1);
                }
            } else if (actor->pickRole == PICK_ROLE_GUN) {
                g_battleOutcome = 0;
                ClearCombatTargets();
                count =
                    FilterGunTargets(actor, CollectTargets(7, 0xa, 0xf1, g_targetId, g_actorId));
                s_targetListCount = count;
            } else if (actor->pickRole == PICK_ROLE_ATTACK) {
                g_battleOutcome = 0;
                ClearCombatTargets();
                if (actor->pickTarget < 1) {
                    skill = GetCachedSkill(SKILL_SWORD_ATTACK);
                    count = CollectCurrentSkillTargets(&skill->parameters);
                } else {
                    record = GetLoadedRecord(actor->pickTarget);
                    count = CollectTargets(
                        TARGET_AREA_WEAPON_HITS,
                        GetWeaponMinHits(record),
                        GetWeaponMaxHits(record),
                        g_targetId,
                        g_actorId
                    );
                }
            } else {
                if ((actor->pickFlags & PICK_ITEM_SKILL) && GetItemValueHigh(actor->pickItem)) {
                    TakeBagItems(actor->pickItem, 1);
                }
                g_battleOutcome = 1;
                ClearCombatTargets();
                skill = GetCachedSkill(actor->pickTarget);
                count = CollectCurrentSkillTargets(&skill->parameters);
            }
            if (count < 1) {
                count = 1;
            } else {
                g_targetId = NextTarget();
                actor->pickObject = g_targetId;
            }
            g_targetCount = count;
            if (g_actorId >= 0
                && (actor->pickRole != PICK_ROLE_MAGIC
                    || GetCachedSkill(actor->pickTarget)->parameters.kind != SKILL_KIND_INERT)) {
                actor->acting = true;
                RedrawFieldView();
                RequestFieldRefresh();
            }
            PlayActionEffect(0);
            if (actor->pickTarget == 0) {
                NextGamePhase();
                break;
            }
            if (actor->pickRole == PICK_ROLE_ITEM) {
                shot = GetItemShotId(GetLoadedRecord(actor->pickTarget));
            } else if (actor->pickRole == PICK_ROLE_GUN) {
                record = GetLoadedRecord(GetCharacterEquipment(actor)[EQUIP_SLOT_AMMO].item);
                if (GetItemShotId(record) == 0) {
                    record = GetLoadedRecord(GetCharacterEquipment(actor)[EQUIP_SLOT_GUN].item);
                }
                shot = GetItemShotId(record);
            } else if (actor->pickRole == PICK_ROLE_ATTACK) {
                CacheSkill(
                    SKILL_SWORD_ATTACK,
                    GetBattleStatShown(actor, BATTLE_STAT_MAGIC_ACCURACY)
                );
                shot = GetSkillShotId(SKILL_SWORD_ATTACK);
            } else {
                CacheSkill(
                    actor->pickTarget,
                    GetBattleStatShown(actor, BATTLE_STAT_MAGIC_ACCURACY)
                );
                shot = GetSkillShotId(actor->pickTarget);
            }
            if (s_skipEffects) {
                shot = 0;
            }
            if (shot == 0) {
                NextGamePhase();
                break;
            }
            from = GetCombatantCoord(g_actorId);
            to = GetCombatantCoord(g_targetId);
            LaunchShot(shot, s_promptSub, to.y - from.y, from.x, from.y, to.x, to.y);
            break;

        case BATTLE_ACTION_PHASE_PROMPT:
            NextGamePhase();
            PushGameState(GAME_STATE_CLOSING_EFFECT);
            SetGameSub(s_promptSub);
            break;

        case BATTLE_ACTION_PHASE_RESOLVE:
            NextGamePhase();
            s_removalDeferred = false;
            s_savedRemovalDeferred = ExchangeObjectRemovalDeferred(1);
            ResolveCombatAction();
            ClearObjectStuns(s_objectMarks);
            break;

        case BATTLE_ACTION_PHASE_APPLY_EFFECT:
            NextGamePhase();
            ApplyObjectConditions(s_objectMarks);
            PlayActionEffect(1);
            DropFlaggedMember(g_targetId);
            if (g_actorId != g_targetId) {
                DropFlaggedMember(g_actorId);
            }
            RefreshAfterDeferredRemoval();
            ClearObjectStuns(s_objectMarks);
            break;

        case BATTLE_ACTION_PHASE_APPLY_CONDITIONS:
            NextGamePhase();
            RefreshAfterDeferredRemoval();
            ApplyObjectConditions(s_objectMarks);
            break;

        case BATTLE_ACTION_PHASE_SHOW_KNOCKOUTS:
            NextGamePhase();
            ExchangeObjectRemovalDeferred(s_savedRemovalDeferred);
            ShowKnockoutMessage();
            RefreshAfterDeferredRemoval();
            break;

        case BATTLE_ACTION_PHASE_REPORT:
            NextGamePhase();
            if (actor == NULL) {
                break;
            }
            if (s_reportedTally != -1) {
                if (g_actionResult >= BATTLE_ACTION_GRAZED) {
                    Character* target = GetCombatant(g_targetId);
                    ReportBattleTally(target, s_reportedTally, -1);
                }
                if (s_tallyMessage != -1) {
                    RunMessageScript(0xdf, 3, -1);
                }
            }
            ResetReportedBattleTally();
            break;

        case BATTLE_ACTION_PHASE_NEXT_TARGET:
            if (actor == NULL) {
                NextGamePhase();
                while (NextTarget() != TARGET_LIST_END) {
                }
                skipEffects = s_skipEffects;
                goto complete;
            }
            for (slot = NextTarget(); slot != TARGET_LIST_END; slot = NextTarget()) {
                if (slot >= 0) {
                    if (GetLiveObject(slot) >= 0) {
                        goto nextTarget;
                    }
                } else if (IsPartyMemberFallen(CombatantPartyPosition(slot)) >= 0) {
                    goto nextTarget;
                }
            }
            NextGamePhase();
            if (g_actorId >= 0) {
                actor->acting = false;
                RequestFieldRefresh();
            }
            skipEffects = s_skipEffects;
            goto complete;

        case BATTLE_ACTION_PHASE_END:
            s_promptPending = false;
            ResetRecordCache();
            RestoreSwappedMember();
            ReturnFromGameState();
            if (actor != NULL) {
                if (s_actionRoleKept == PICK_ROLE_MAGIC && s_actionPickKept == SKILL_SABBATMA) {
                    slot = ExchangePartySlot(
                        g_commandPosition,
                        FindRosterSlotById(s_actionTarget->id)
                    );
                    ClearActionWait(GetCharacterActionWait(s_actionTarget));
                    AddMagnetite(
                        GetRosterCharacter(ROSTER_LEADER),
                        -GetSummonMagnetiteCost(s_actionTarget)
                    );
                    ResetBattleTally(s_actionTarget);
                    s_actionTarget = GetRosterCharacter(slot);
                    if (s_actionTarget != NULL) {
                        ClearBattleConditions(GetCharacterConditions(s_actionTarget));
                        ResetBattleTally(s_actionTarget);
                    }
                    RequestFieldRefresh();
                }
                ClearActionActors();
                s_actionRoleKept = 0;
                s_actionPickKept = 0;
                actor->pickFlags &= ~PICK_ITEM_SKILL;
                if (g_actorId < 0) {
                    actor->pickItem = 0;
                }
                if (actor->pickRole == PICK_ROLE_GUN) {
                    SpendAllGunRounds(actor);
                }
            }
            if (ProcessPartyCasualties()) {
                RequestFieldRefresh();
            }
            break;
    }
done:
    skipEffects = s_skipEffects;
complete:
    if (skipEffects) {
        return false;
    }
    return UpdateFieldScreen(false);

nextTarget:
    SetGamePhase(BATTLE_ACTION_PHASE_RESOLVE);
    g_targetId = slot;
    actor = GetCombatant(g_actorId);
    actor->pickObject = slot;
    goto done;
}

RVA(0x0002c050, 0x1c0)
void PlayActionEffect(i16 stage) {
    SkillMessage before;
    SkillMessage after;
    ItemRecord* item;
    SkillMessage* message;
    Character* user;
    i16 weapon;
    if (s_actionRole == PICK_ROLE_ITEM) {
        item = GetLoadedRecord(g_actionId);
        before.script = item->beforeMessage.script;
        before.entry = item->beforeMessage.entry;
        after.script = item->afterMessage.script;
        after.entry = item->afterMessage.entry;
    } else if (s_actionRole == PICK_ROLE_GUN) {
        before.script = 0xde;
        before.entry = 4;
        after.script = 0xdd;
        after.entry = 5;
    } else if (s_actionRole == PICK_ROLE_ATTACK) {
        before.script = 0xde;
        after.script = 0xdd;
        weapon = GetCharacterEquipment(GetCombatant(g_actorId))[EQUIP_SLOT_WEAPON].item;
        if (weapon < 1) {
            before.entry = 3;
            after.entry = 5;
        } else {
            item = GetLoadedRecord(weapon);
            before.script = item->beforeMessage.script;
            before.entry = item->beforeMessage.entry;
            after.entry = GetSkillMessage(1, 1)->entry;
        }
    } else {
        user = GetCombatant(g_actorId);
        if (user->pickFlags & PICK_ITEM_SKILL) {
            item = GetLoadedRecord(user->pickItem);
            if (item->kind == ITEM_KIND_WEAPON) {
                before.script = 0xde;
                before.entry = 0x44;
                message = GetSkillMessage(g_actionId, 1);
                after.script = message->script;
                after.entry = message->entry;
            } else {
                before.script = item->beforeMessage.script;
                before.entry = item->beforeMessage.entry;
                message = GetSkillMessage(g_actionId, 1);
                after.script = message->script;
                after.entry = message->entry;
            }
        } else {
            before.script = 0xde;
            before.entry = GetSkillMessage(g_actionId, 0)->entry;
            after.script = 0xdd;
            after.entry = GetSkillMessage(g_actionId, 1)->entry;
        }
    }
    if (stage == 0) {
        if (before.script) {
            RunMessageScript(before.script, before.entry, -1);
        }
    } else if (after.script) {
        if (g_actionId >= SKILL_TRAESTO && g_actionId <= SKILL_TRAFURI) {
            RunMessageTextScript(after.script, after.entry, -1);
        } else {
            RunMessageScript(after.script, after.entry, -1);
        }
    }
}

RVA(0x0002c210, 0x3d)
void ShowKnockoutMessage(void) {
    i16 savedTarget;
    if (s_knockedOut != 0x7fff) {
        savedTarget = g_targetId;
        g_targetId = s_knockedOut;
        if (s_knockedOut < 0) {
            RunMessageScript(221, 80, -1);
        } else {
            RunMessageScript(221, 79, -1);
        }
        g_targetId = savedTarget;
    }
}

RVA(0x0002c250, 0x50)
void DropFlaggedMember(i16 id) {
    Character* character;
    if (id < 0) {
        character = GetCombatant(id);
        if (character && TestCharacterFlag(character, ACTOR_FLAG_DESAMAN)) {
            ClearCharacterFlag(character, ACTOR_FLAG_DESAMAN);
            SetPartySlot(CombatantPartyPosition(id), PARTY_SLOT_EMPTY);
            RequestFieldRefresh();
        }
    }
}

RVA(0x0002c2a0, 0xa)
void DeferObjectRemoval(void) {
    s_removalDeferred = true;
}

RVA(0x0002c2b0, 0xc)
void SetActionOutcome(i16 outcome) {
    s_actionOutcome = outcome;
}

// Nonzero when `character` cannot pay the skill's HP or MP cost.
RVA(0x0002c2c0, 0x22)
b32 CannotPaySkill(Character* character, SkillParameters* skill) {
    return HpMpLeftAfterCost(GetSkillParameterCost(skill), character) < 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
// Nonzero when a condition or the member's lock keeps it from using `skill`;
// skills with a mode are never blocked here.
RVA(0x0002c2f0, 0x5d)
b32 IsSkillBlocked(Character* character, SkillParameters* skill) {
    if (GetPickBlockingCondition(GetCharacterConditions(character))) {
        return true;
    }
    if (!skill->mode) {
        if (GetCharacterBattleTallies(character)[BATTLE_TALLY_MAGIC_SEAL]) {
            return true;
        }
        if (LastConditionIn(GetCharacterConditions(character), s_skillBlockingConditions)) {
            return true;
        }
    }
    return false;
}

// The same check by skill id; Traport is also blocked while the first
// roster member's byte +0x30 is clear.
RVA(0x0002c350, 0x69)
b32 IsSkillIdBlocked(Character* character, i16 id) {
    if (GetSkillMode(id)) {
        return false;
    }
    if (GetCharacterBattleTallies(character)[BATTLE_TALLY_MAGIC_SEAL]) {
        return true;
    }
    if (LastConditionIn(GetCharacterConditions(character), s_skillIdBlockingConditions)) {
        return true;
    }
    if (id == SKILL_TRAPORT && !GetRosterCharacter(ROSTER_LEADER)->markPosition.area) {
        return true;
    }
    return false;
}

// Takes the skill's cost from `who`: MP for a positive cost, HP for a
// negative one; 0x7f (MP) and -0x80 (HP) take the whole pool. Ids below 16 are
// free.
RVA(0x0002c3c0, 0x71)
void PaySkillCost(i16 who, i16 skill) {
    Character* character;
    i16 cost;
    if (skill < 0x10) {
        return;
    }
    character = GetCombatant(who);
    cost = GetSkillCost(skill);
    if (cost >= 0) {
        if (cost == SKILL_COST_WHOLE_MP) {
            cost = character->pools.mp.cur;
        }
        DrainPool(&character->pools.mp, cost);
    } else {
        cost = -cost;
        if (cost == SKILL_COST_WHOLE_HP) {
            cost = character->pools.hp.cur;
        }
        DrainPool(&character->pools.hp, cost);
    }
}

RVA(0x0002c440, 0x54)
i16 IsSkillUsableNow(GZ_ENUM_PARAM(SkillUseModes, u16) usable) {
    if (g_fieldBattleActive && (usable & SKILL_USE_FIELD_BATTLE)) {
        return 1;
    }
    if (TestModeFlags(MODE_FIELD) && (usable & SKILL_USE_FIELD)) {
        return 1;
    }
    if (TestModeFlags(MODE_WORLD_MAP) && (usable & SKILL_USE_WORLD_MAP)) {
        return 1;
    }
    return -1;
}

RVA(0x0002c4a0, 0x60)
MenuBox* OpenMemberSkillMenu(i16 id) {
    Character* character = GetCharacterById(id);
    MenuBox* menu = CreateMenuBox(NULL, 5, 2);
    menu->flags |= 0x1e;
    SetMenuItems(
        menu,
        7,
        character,
        GetWordCount(GetCharacterSkills(character)),
        MemberSkillMenuHandler
    );
    SetTextPlaneFirstSelectableRow(menu->plane, 1, true);
    return menu;
}

RVA(0x0002c500, 0x1c0)
void MemberSkillMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    Character* character = menu->items.character;
    SkillView* skill;
    i16 disabled;
    i16 style;
    i16 blocked;
    i16 cost;
    switch (event) {
        case MENU_EVENT_ADD_ROW:
            disabled = false;
            style = TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
            if (GetWord(GetCharacterSkills(character), index) == 0) {
                AddMenuLine(menu->plane, g_emptySkillMenuLabel, style, 0, MENU_LINE_DISABLED);
                return;
            }
            blocked = IsSkillIdBlocked(character, GetWord(GetCharacterSkills(character), index));
            skill = GetSkillView(GetWord(GetCharacterSkills(character), index));
            if (blocked == 1) {
                style =
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
                disabled = true;
            } else if (IsSkillUsableNow(GetSkillUseModes(skill)) < 1) {
                style =
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
                disabled = true;
            } else if (CheckSkillArea(GetWord(GetCharacterSkills(character), index)) < 1) {
                style =
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_BLACK, TEXT_COLOR_BLACK);
                disabled = true;
            } else if (CannotPaySkill(character, &skill->parameters)) {
                style =
                    TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_GREEN, TEXT_COLOR_RED, TEXT_COLOR_BLACK);
                disabled = true;
            }
            if (SkillCostsFullPool(&skill->parameters)) {
                sprintf(g_scratchBuffer, "%-16.16s MAX", skill->name);
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    style,
                    GetWord(GetCharacterSkills(character), index),
                    disabled
                );
            } else {
                cost = GetSkillParameterCost(&skill->parameters) < 0
                           ? -GetSkillParameterCost(&skill->parameters)
                           : GetSkillParameterCost(&skill->parameters);
                sprintf(g_scratchBuffer, "%-16.16s %3d", skill->name, cost);
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    style,
                    GetWord(GetCharacterSkills(character), index),
                    disabled
                );
            }
            break;
        case MENU_EVENT_BEGIN_PAGE:
            FormatFullName(g_scratchBuffer, character);
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                0,
                MENU_LINE_DISABLED
            );
            break;
        case MENU_EVENT_DESTROY:
            menu->items.character = NULL;
            menu->itemCount = 0;
            break;
    }
}

RVA(0x0002c6c0, 0x7b)
i16 CountUsableMemberSkills(Character* character, i16 checkCost) {
    i16 index;
    i16 count = 0;
    for (index = 0; index < GetWordCount(GetCharacterSkills(character)); index++) {
        if (checkCost) {
            if (CanUseSkill(GetWord(GetCharacterSkills(character), index), character) < 1) {
                continue;
            }
        } else {
            if (IsSkillUsableNow(
                    GetSkillUseModes(GetSkillView(GetWord(GetCharacterSkills(character), index)))
                )
                < 1) {
                continue;
            }
        }
        count++;
    }
    return count;
}

RVA(0x0002c740, 0xc0)
i16 CollectTargets(
    GZ_ENUM_PARAM(TargetArea, i16) area,
    i16 flags,
    i16 range,
    i16 target,
    i16 actor
) {
    MapCoord point;
    switch (area) {
        case TARGET_AREA_LINE:
            return CollectTargetsAlongLine(area, flags, range, target, actor);
        case 6:
        case 7:
            return CollectTargetsInView(area, flags, range, target, actor);
        case 3:
        case 4:
        case 5:
        default:
            if (target >= 0) {
                point = GetObjectCoord(target);
            } else {
                point = GetMapCoord();
            }
            return CollectTargetsAtCell(area, flags, range, target, actor, point.x, point.y);
    }
}

#define ReturnCombatTargetsOrDefault(count, target)                                                \
    do {                                                                                           \
        if (!(count)) {                                                                            \
            (count) = AddCombatTarget((target), 0);                                                \
        }                                                                                          \
        return (count);                                                                            \
    } while (0)

RVA(0x0002c800, 0x1e0)
i16 CollectTargetsAtCell(
    GZ_ENUM_PARAM(TargetArea, i16) area,
    i16 flags,
    i16 range,
    i16 target,
    i16 actor,
    i16 x,
    i16 y
) {
    i16 targets[128];
    i16 hits;
    i16 mode;
    i16 selected;
    i16 count;
    i16 id;
    i16 i;
    i16 repeat;
    i16 result;
    if (area == TARGET_AREA_WEAPON_HITS) {
        mode = range;
        selected = 1;
        hits = RandomAverage(flags, range, 0);
        AddCombatTarget(target, 0);
    } else {
        if (area == TARGET_AREA_SELECTED_ONLY) {
            result = AddCombatTarget(target, 0);
        } else {
            result = AddRelatedCombatTargets(x, y, flags, target, actor);
        }
        hits = range & TARGET_COUNT_NIBBLE_MASK;
        if (hits < 1) {
            hits = 1;
        }
        mode = (range >> TARGET_COUNT_MODE_SHIFT) & TARGET_COUNT_NIBBLE_MASK;
        selected = mode;
        if (mode == TARGET_COUNT_ALL_TARGETS) {
            selected = result;
        } else if (mode == TARGET_COUNT_RANDOM_TARGETS) {
            selected = RandomPercent(hits, -20, 20);
        }
    }
    count = 0;
    while ((id = NextTarget()) != TARGET_LIST_END) {
        targets[count++] = id;
    }
    if (mode == TARGET_COUNT_RANDOM_TARGETS) {
        result = 0;
        for (i = 0; i < selected; i++) {
            id = RandomUpTo(count - 1);
            result = AddCombatTarget(targets[id], 1);
        }
        ReturnCombatTargetsOrDefault(result, target);
    }
    if (mode == TARGET_COUNT_ALL_TARGETS || selected > count) {
        selected = count;
    }
    result = 0;
    for (i = 0; i < selected; i++) {
        id = RandomPercent(hits, -20, 20);
        for (repeat = 0; repeat < id; repeat++) {
            result = AddCombatTarget(targets[i], 1);
        }
    }
    ReturnCombatTargetsOrDefault(result, target);
}

RVA(0x0002c9e0, 0xc0)
i16 AddRelatedCombatTargets(i16 x, i16 y, i16 flags, i16 target, i16 actor) {
    i16 count = AddCombatTarget(target, 0);
    if (flags & TARGET_EXPAND_GROUP) {
        if ((flags & (TARGET_ACTOR_SIDE | TARGET_TARGET_SIDE))
            == (TARGET_ACTOR_SIDE | TARGET_TARGET_SIDE)) {
            if (target >= 0) {
                count = AddObjectTargetsAtCell(x, y);
                count = AddPartyTargetsAtCell(x, y);
            } else {
                count = AddPartyTargetsAtCell(x, y);
                count = AddObjectTargetsAtCell(x, y);
            }
            return count;
        }
        if (flags & TARGET_TARGET_SIDE) {
            if (target >= 0) {
                count = AddObjectTargetsAtCell(x, y);
            } else {
                count = AddPartyTargetsAtCell(x, y);
            }
        }
        if (flags & TARGET_ACTOR_SIDE) {
            if (actor >= 0) {
                count = AddObjectTargetsAtCell(x, y);
            } else {
                count = AddPartyTargetsAtCell(x, y);
            }
        }
    }
    return count;
}

RVA(0x0002caa0, 0x60)
i16 AddObjectTargetsAtCell(i16 x, i16 y) {
    i16 count = CountCombatTargets();
    i16 object = FindObjectAt(x, y, 0, OBJECT_MATCH_ANY, 0);
    while (object != -1) {
        count = AddCombatTarget(object, 0);
        object = FindObjectAt(x, y, object + 1, OBJECT_MATCH_ANY, 0);
    }
    return count;
}

RVA(0x0002cb00, 0x60)
i16 AddPartyTargetsAtCell(i16 x, i16 y) {
    MapCoord point = GetMapCoord();
    i16 count = CountCombatTargets();
    i16 target;
    i16 slot;
    if (point.x == x && point.y == y) {
        slot = 0;
        for (target = -1; target > -7; target--) {
            if (!IsPartyMemberFallen(slot)) {
                count = AddCombatTarget(target, 0);
            }
            slot++;
        }
    }
    return count;
}

RVA(0x0002cb60, 0x120)
i16 CollectTargetsAlongLine(
    GZ_ENUM_PARAM(TargetArea, i16) area,
    i16 flags,
    i16 range,
    i16 target,
    i16 actor
) {
    MapCoord origin;
    MapCoord offset;
    i16 direction;
    i16 x;
    i16 y;
    i16 count = 0;
    if (actor >= 0) {
        origin = GetObjectCoord(target);
        direction = GetObjectDirection(target);
    } else {
        origin = GetMapCoord();
        direction = g_party.field.pos.direction;
    }
    if (target >= 0) {
        offset = GetObjectCoord(target);
    } else {
        offset = GetMapCoord();
    }
    offset = RelativeOffset(origin.x, origin.y, direction, offset.x, offset.y);
    offset.y = offset.y ? -1 : 0;
    for (; offset.y >= -3; offset.y--) {
        x = origin.x;
        y = origin.y;
        OffsetMapCoord(&x, &y, direction, offset.y, offset.x);
        count = CollectTargetsAtCell(area, flags, range, target, actor, x, y);
        if (GetMapWallKind(x, y, direction)) {
            break;
        }
    }
    return count;
}

RVA(0x0002cc80, 0x100)
i16 CollectTargetsInView(
    GZ_ENUM_PARAM(TargetArea, i16) area,
    i16 flags,
    i16 range,
    i16 target,
    i16 actor
) {
    MapCoord origin;
    i16 count = 0;
    i16 direction = g_party.field.pos.direction;
    i16 distance;
    i16 across;
    i16 along;
    i16 x;
    i16 y;
    if (target >= 0) {
        origin = GetObjectCoord(target);
    } else {
        origin = GetMapCoord();
    }
    for (distance = 0; distance <= 6; distance++) {
        for (across = 0; across < 7; across++) {
            for (along = 0; along < 7; along++) {
                if (s_targetCellDistance[across][along] == distance) {
                    x = origin.x;
                    y = origin.y;
                    OffsetMapCoord(&x, &y, direction, across - 3, along - 3);
                    if (GetPartyView(x, y)) {
                        count = CollectTargetsAtCell(area, flags, range, target, actor, x, y);
                    }
                }
            }
        }
    }
    return count;
}

RVA(0x0002cd80, 0x130)
void UseAttackSkill(Character* user, Character* target) {
    ResetActionOutcome();
    ResetPoolChanges();
    user->lastChange = 0;
    if (GetSkillValueB(&s_effectSkill)) {
        ResolveSkillAttack(user, target);
        return;
    }
    if (g_targetId >= 0 && IsFieldModeAtLeast(false)
        && IsFieldConditionRestricted(GetSkillInflictedCondition(&s_effectSkill))) {
        return;
    }
    if (IsConditionResisted(target, GetSkillInflictedCondition(&s_effectSkill))) {
        return;
    }
    if (RollSkillHit(user, target, false) <= 0) {
        return;
    }
    AddTrainingPoints(user, BATTLE_GROUP_MAGIC, 3);
    g_statusCondition = GetSkillInflictedCondition(&s_effectSkill);
    switch (ApplySkillResistanceOutcome(user, 0)) {
        case 0:
            g_statusCondition = INFLICT_NONE;
            break;
        case -1:
            SetActionOutcome(ACTION_OUTCOME_CONDITION);
            InflictCondition(GetSkillInflictedCondition(&s_effectSkill), user);
            break;
        default:
            SetActionOutcome(ACTION_OUTCOME_CONDITION);
            InflictCondition(GetSkillInflictedCondition(&s_effectSkill), target);
            break;
    }
}

RVA(0x0002ceb0, 0xe0)
void UseRestoreSkill(Character* user, Character* target) {
    i16 hit;
    i16 amount;
    i16 result;
    g_statusCondition = INFLICT_NONE;
    hit = RollSkillHit(user, target, true);
    amount = ComputeRestoreAmount(GetSkillValueB(&s_effectSkill), user, target->pools.hp.max);
    user->lastChange = amount;
    result = ApplyRestoreEffect(GetSkillEffectCode(&s_effectSkill), amount, target, 0);
    user->lastChange = target->lastChange;
    user->pickNoEffect = true;
    g_actionResult = result;
    user->result = result;
    if (RestoreEffectAllowsCondition(result) && hit > 0
        && !IsConditionResisted(target, GetSkillInflictedCondition(&s_effectSkill))) {
        g_statusCondition = g_pendingCondition = GetSkillInflictedCondition(&s_effectSkill);
        InflictCondition(GetSkillInflictedCondition(&s_effectSkill), target);
    }
}

RVA(0x0002cf90, 0xd0)
void UseBattleTallySkill(Character* user, Character* target) {
    i16 tally;
    target->pickNoEffect = true;
    user->pickNoEffect = true;
    g_statusCondition = INFLICT_NONE;
    g_hpChange = 0;
    g_actionResult = 0;
    user->lastChange = 0;
    target->pickNoEffect = true;
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
    g_hpChange = 0;
    user->lastChange = 0;
    g_statusCondition = GetSkillInflictedCondition(&s_effectSkill);
    tally = GetSkillEffectCode(&s_effectSkill);
    if (tally < 0 || tally > 15) {
        tally = BATTLE_TALLY_ALL_BLOCK;
    }
    GetCharacterBattleTallies(target)[tally] = GetSkillValueA(&s_effectSkill);
    if (tally == BATTLE_TALLY_FIRE_BLOCK) {
        GetCharacterBattleTallies(target)[BATTLE_TALLY_ICE_BLOCK] = 0;
    } else if (tally == BATTLE_TALLY_ICE_BLOCK) {
        GetCharacterBattleTallies(target)[BATTLE_TALLY_FIRE_BLOCK] = 0;
    }
    SetActionOutcome(ACTION_OUTCOME_BATTLE_TALLY);
}

static __inline void PrepareBattleStatSkill(Character* user) {
    user->pickNoEffect = true;
    g_statusCondition = INFLICT_NONE;
    g_hpChange = 0;
    g_actionResult = 0;
    SetCharacterResult(user, 0, 0);
}

RVA(0x0002d060, 0x2e0)
void UseBattleStatSkill(Character* user, Character* target) {
    double power = sqrt(GetStatTotal(user, STAT_MAGIC)) + GetSkillValueB(&s_effectSkill);
    i32 changed = 0;
    i16 amount = RoundToShort(RandomAverage(80, 120, 0) * power * 0.01);
    if (GetSkillEffectCode(&s_effectSkill) & BATTLE_STAT_EFFECT_LOWER) {
        amount = -amount;
    }
    PrepareBattleStatSkill(user);
    if (amount < 0) {
        if (RollSkillHit(user, target, false) <= 0) {
            return;
        }
    } else {
        if (RollSkillHit(user, target, true) <= 0) {
            return;
        }
    }
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
    user->lastChange = amount;
    g_hpChange = amount;
    switch (GetSkillEffectCode(&s_effectSkill) & BATTLE_STAT_EFFECT_KIND_MASK) {
        case BATTLE_STAT_EFFECT_WEAPON_GUN_POWER:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_WEAPON_POWER, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_GUN_POWER, amount);
            break;
        case BATTLE_STAT_EFFECT_WEAPON_GUN_ACCURACY:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_WEAPON_ACCURACY, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_GUN_ACCURACY, amount);
            break;
        case BATTLE_STAT_EFFECT_WEAPON_GUN_DEFENSE:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_WEAPON_DEFENSE, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_GUN_DEFENSE, amount);
            break;
        case BATTLE_STAT_EFFECT_MAGIC_ATTACK:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_POWER, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_ACCURACY, amount);
            break;
        case BATTLE_STAT_EFFECT_MAGIC_DEFENSE:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_EVASION, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_DEFENSE, amount);
            break;
        case BATTLE_STAT_EFFECT_MAGIC_ALL:
            changed = ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_POWER, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_ACCURACY, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_EVASION, amount);
            changed += ChangeCharacterBattleStat(target, BATTLE_STAT_MAGIC_DEFENSE, amount);
            break;
    }
    if (!changed) {
        g_actionResult = BATTLE_ACTION_NO_EFFECT;
        user->result = 1;
    }
}

RVA(0x0002d340, 0x50)
i16 ChangeBattleStat(i16* value, i16 amount, i16 base) {
    i16 limit = ClampTo999(base * 4);
    i16 next = *value + amount;
    i16 change;
    if (next > limit) {
        next = limit;
    }
    if (next > *value * 2) {
        next = *value * 2;
    }
    next = ClampTo999(next);
    change = next - *value;
    *value = next;
    return change;
}

static __inline void PrepareNonDamageSkill(Character* user, Character* target) {
    target->pickNoEffect = true;
    user->pickNoEffect = true;
    g_statusCondition = INFLICT_NONE;
    g_hpChange = 0;
    user->lastChange = 0;
}

RVA(0x0002d390, 0x70)
void UseClearBattleTallySkill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
    if (RollSkillHit(user, target, false) > 0) {
        ClearAllBattleTallies(target);
    }
}

RVA(0x0002d400, 0x80)
void UseResetBattleStatsSkill(Character* user, Character* target) {
    PrepareBattleStatSkill(user);
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
    RecalcDerivedStats(target);
    ResetBattleStatsToBase(target);
    ClearAllBattleTallies(target);
}

RVA(0x0002d480, 0x160)
void ApplySkillEffect(i16 skill, Character* user, Character* target) {
    CopySkillHeader(skill, &s_effectSkill);
    s_effectSkillId = skill;
    if (skill == 250) {
        s_effectSkill.parameters.type = 2;
    }
    switch (s_effectSkill.parameters.kind) {
        case SKILL_KIND_RESTORE:
            UseRestoreSkill(user, target);
            break;
        case SKILL_KIND_BATTLE_TALLY:
            UseBattleTallySkill(user, target);
            break;
        case SKILL_KIND_BATTLE_STAT:
            UseBattleStatSkill(user, target);
            break;
        case 9:
        case 10:
            UseClearBattleTallySkill(user, target);
            break;
        case SKILL_KIND_RESET_BATTLE_STATS:
            UseResetBattleStatsSkill(user, target);
            break;
        case SKILL_KIND_FIELD_TRAVEL:
            UseKind12Skill(user, target);
            break;
        case SKILL_KIND_SUMMON:
            UseKind13Skill(user, target);
            break;
        case 14:
            UseKind14Skill(user, target);
            break;
        case SKILL_KIND_FIELD_EFFECT:
            UseFieldEffectSkill(user, target);
            break;
        case SKILL_KIND_INERT:
            UseInertSkill(user, target);
            break;
        default:
            UseAttackSkill(user, target);
            break;
    }
}

RVA(0x0002d5e0, 0x60)
void UseKind12Skill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    RunFieldEffect(GetSkillEffectCode(&s_effectSkill));
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
}

RVA(0x0002d640, 0x60)
void UseKind13Skill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    RunFieldEffect(GetSkillEffectCode(&s_effectSkill));
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
}

RVA(0x0002d6a0, 0x60)
void UseKind14Skill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    RunFieldEffect(GetSkillEffectCode(&s_effectSkill));
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
}

RVA(0x0002d700, 0x50)
void UseFieldEffectSkill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    RunFieldEffect(GetSkillEffectCode(&s_effectSkill));
}

RVA(0x0002d750, 0x50)
void UseInertSkill(Character* user, Character* target) {
    PrepareNonDamageSkill(user, target);
    SetFlaggedActionResult(user, BATTLE_ACTION_SUCCESS);
}

// @identity-TODO: PrepareSkillAction only clears this word; no reader survives.
DATA(0x00091980)
i16 g_skillActionResetValue;

RVA(0x0002d7a0, 0x20)
i16 CancelFieldTargetMenu(i16 command) {
    if (command == -1) {
        s_fieldMenu = CloseListMenu(s_fieldMenu);
    }
    return -1;
}

static __inline void LocateFieldSkillUser(void) {
    s_userPosition = FindRosterSlotIn(s_skillUser, 1);
    s_userPosition = FindPartySlot(s_userPosition);
}

// Runs the field skill-use flow one phase: pick the member (unless one is
// set), pick a skill from its list, pick the skill's target, then hand the
// member's pick to the action prompt. Returns 0.
RVA(0x0002d7c0, 0x370)
b16 RunFieldSkillUse(void) {
    i16 flags;
    i16 picked;

    switch (GetGamePhase()) {
        case SKILL_USE_PHASE_START:
            HideScreenLayer(1);
            if (s_skillUser < 0) {
                SetGamePhase(SKILL_USE_PHASE_END);
                return false;
            }
            LocateFieldSkillUser();
            SetGamePhase(SKILL_USE_PHASE_OPEN_SKILL_LIST);
            return false;

        case SKILL_USE_PHASE_CLOSE_MEMBER_PICKER:
            s_fieldMenu = ClosePickerMenu(s_fieldMenu);
            SetGamePhase(SKILL_USE_PHASE_END);
            return false;

        case SKILL_USE_PHASE_PICK_MEMBER:
            s_skillUser = RunPickerMenu(s_fieldMenu);
            if (s_skillUser == -2) {
                PrevGamePhase();
            }
            if (s_skillUser < 0) {
                break;
            }
            LocateFieldSkillUser();
            NextGamePhase();
            s_fieldMenu = ClosePickerMenu(s_fieldMenu);
            return false;

        case SKILL_USE_PHASE_OPEN_SKILL_LIST:
            NextGamePhase();
            NextGamePhase();
            s_fieldMenu = OpenMemberSkillMenu(s_skillUser);
            return false;

        case SKILL_USE_PHASE_CLOSE_SKILL_LIST:
            SetGamePhase(SKILL_USE_PHASE_END);
            s_fieldMenu = CloseListMenu(s_fieldMenu);
            return false;

        case SKILL_USE_PHASE_PICK_SKILL:
            s_skillPicked = RunListMenu(s_fieldMenu);
            if (s_skillPicked == LIST_MENU_CANCELLED) {
                PrevGamePhase();
            }
            if (s_skillPicked < 0) {
                break;
            }
            NextGamePhase();
            s_fieldMenu = CloseListMenu(s_fieldMenu);
            g_actionId = s_skillPicked;
            return false;

        case SKILL_USE_PHASE_PICK_TARGET:
            flags = GetSkillTargetFlags(s_skillPicked);
            if (TargetFlagsSelectSelf(flags)) {
                g_targetId = PartyCombatantId(s_userPosition);
                NextGamePhase();
                return false;
            }
            if (TargetFlagsSelectActorGroup(flags)) {
                g_targetId = PartyCombatantId(s_userPosition);
                NextGamePhase();
                return false;
            }
            s_pickRange = GetSkillAttackRange(g_actionId);
            if (flags == TARGET_SELECT_FIELD_OR_ROSTER) {
                picked = RunPickTargetWindow(
                    0,
                    s_pickRange,
                    TARGET_PICK_FIELD_OBJECT | TARGET_PICK_ROSTER_LIST,
                    0
                );
            } else if (flags == TARGET_SELECT_ROSTER_ONLY) {
                picked = RunPickTargetWindow(0, s_pickRange, TARGET_PICK_ROSTER_LIST, 0);
            } else if (flags == TARGET_SELECT_PARTY_OR_ROSTER) {
                picked = RunPickTargetWindow(
                    0,
                    s_pickRange,
                    TARGET_PICK_PARTY_SLOT | TARGET_PICK_ROSTER_LIST,
                    0
                );
            } else {
                flags = 0;
                picked = RunPickTargetWindow(
                    0,
                    s_pickRange,
                    TARGET_PICK_FIELD_OBJECT | TARGET_PICK_PARTY_SLOT,
                    0
                );
            }
            if (picked == TARGET_PICK_CANCELLED) {
                SetGamePhase(SKILL_USE_PHASE_OPEN_SKILL_LIST);
            }
            if (picked <= TARGET_PICK_WAITING) {
                break;
            }
            if (flags) {
                g_selectedObjectId = SwapInForPick(s_userPosition, g_selectedObjectId);
            }
            g_targetId = g_selectedObjectId;
            NextGamePhase();
            return false;

        case SKILL_USE_PHASE_PROMPT_ACTION:
            NextGamePhase();
            g_actorId = PartyCombatantId(s_userPosition);
            SetSkillPick(s_userPosition);
            PrepareSkillAction();
            PushFieldUsePrompt();
            return false;

        case SKILL_USE_PHASE_END:
            RestoreSwappedMember();
            ReturnFromGameState();
            break;
    }
    return false;
}

// Makes party member `position`'s pick the chosen skill on the current target.
// @early-stop operand order: retail loads g_targetId into a register and xors the
// old pick word into it; cl here xors g_targetId from memory into the old word
// (same bits); the permuter found one compiler island.
RVA(0x0002db30, 0x60)
void SetSkillPick(i16 position) {
    Character* user = GetPartyCharacter(position);

    if (user != NULL) {
        user->pickObject = g_targetId;
        user->pickRole = PICK_ROLE_MAGIC;
        g_actionId = s_skillPicked;
        user->pickTarget = s_skillPicked;
    }
}

RVA(0x0002db90, 0x4e)
void PrepareSkillAction(void) {
    SkillHeader* skill;
    g_skillActionResetValue = 0;
    skill = GetCachedSkill(g_actionId);
    g_statusCondition = GetSkillInflictedCondition(skill);
    g_actionResult = GetSkillEffectCode(skill);
    g_drainAmount = 0;
    g_hpChange = GetSkillValueB(skill);
    g_mpChange = 0;
}

RVA(0x0002dbe0, 0x10)
void SetFieldSkillUser(i16 id) {
    s_skillUser = id;
}
