// @identity-TODO: the owning TU is unproven; this unit holds the restoration
// effect span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/Condition.h>
#include <Game/EquipEffect.h>
#include <Game/ItemEffect.h>
#include <Game/Stats.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <math.h>
#include <string.h>

DATA(0x000919fc)
i16 g_effectCondition;

// The condition groups selected by restoration kinds 53..59, 62 and 64.
DATA(0x00064558)
static const i16 s_mentalRecoveryConditions[] = {14, 18, 19, 27, 28, 29, -1};

DATA(0x00064568)
static const i16 s_extendedMentalRecoveryConditions[] =
    {14, 18, 19, 27, 28, 29, 23, 26, 17, 16, 10, -1};

DATA(0x00064580)
static const i16 s_poisonParalysisConditions[] = {15, 5, -1};

DATA(0x00064588)
static const i16 s_extendedPoisonParalysisConditions[] = {15, 5, 32, 4, -1};

DATA(0x00064598)
const i16 g_physicalRecoveryConditions[] = {21, 22, 20, 11, 4, 15, 5, -1};

DATA(0x000645a8)
static const i16 s_faintRecoveryConditions[] = {2, 3, 10, -1};

DATA(0x000645b0)
static const i16 s_deathRecoveryConditions[] = {1, 2, 3, 10, -1};

DATA(0x000645c0)
static const i16 s_generalRecoveryConditions[] = {2,  3,  4,  5,  6,  10, 11, 12, 13, 14,
                                                  15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                                                  25, 26, 27, 28, 29, 30, 32, 33, 34, -1};

DATA(0x00064600)
static const i16 s_specialRecoveryConditions[] = {0, 1, 8, 9, 31, 14, 18, 19, 27, 28, 29, -1};

RVA(0x0001fdc0, 0xc1)
i16 ComputeRestoreAmount(i16 code, Character* user, u16 max) {
    double amount;
    if (code == 0) {
        return 0;
    }
    if (code == 255) {
        return max;
    }
    if (code == 254) {
        return max / 2;
    }
    if (code == 253) {
        return max / 4;
    }
    amount = GetStatTotal(user, STAT_MAGIC);
    if (amount < 0.0) {
        amount = 0.0;
    }
    amount = sqrt(amount);
    amount += code;
    amount *= RandomAverage(80, 120, 0);
    amount *= 0.01;
    return RoundToShort(amount);
}

static __inline void FillRestorePools(CurMax* hpPool, CurMax* mpPool, i16 hp, i16 mp) {
    g_hpChange = hp;
    FillPool(hpPool, hp, POOL_FILL_TO_MAX);
    FillPool(mpPool, mp, POOL_FILL_TO_MAX);
    g_mpChange = mp;
}

RVA(0x0001fe90, 0x6f0)
i16 ApplyRestoreEffect(i16 kind, i16 hp, Character* target, i16 mp) {
    i16 reportCondition;
    i16 revival;
    i16 sleep;
    CurMax* hpPool;
    CurMax* mpPool;
    u16 oldHp;
    u16 oldMp;
    ConditionSet* conditions;
    i16 wasZombie;
    sleep = 0;
    reportCondition = 1;
    revival = 0;
    hpPool = &target->pools.hp;
    mpPool = &target->pools.mp;
    oldHp = hpPool->cur;
    oldMp = mpPool->cur;
    conditions = GetCharacterConditions(target);
    g_effectCondition = 0;
    wasZombie = HasCondition(conditions, CONDITION_ZOMBIE);
    switch (kind) {
        case 0:
            sleep = 0;
            FillRestorePools(hpPool, mpPool, hp, mp);
            reportCondition = 0;
            break;
        case 1:
        case 2:
            revival = 1;
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
            if (HasCondition(conditions, kind)) {
                g_effectCondition = kind;
            }
            ClearCondition(conditions, kind);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 49:
            g_hpChange = hp;
            FillPool(hpPool, hp, POOL_FILL_TO_USHORT_MAX);
            FillPool(mpPool, mp, POOL_FILL_TO_MAX);
            g_mpChange = mp;
            break;
        case 50:
            g_mpChange = hp;
            FillPool(mpPool, hp, POOL_FILL_TO_MAX);
            reportCondition = 0;
            break;
        case 51:
            g_hpChange = hp;
            FillPool(hpPool, hp, POOL_FILL_TO_MAX);
            g_mpChange = hp / 4;
            FillPool(mpPool, g_mpChange, POOL_FILL_TO_MAX);
            reportCondition = 0;
            break;
        case 52:
            g_mpChange = hp;
            FillPool(mpPool, hp, POOL_FILL_TO_MAX);
            g_hpChange = hp / 4;
            FillPool(hpPool, g_hpChange, POOL_FILL_TO_MAX);
            reportCondition = 0;
            break;
        case 53:
            ClearEffectConditions(conditions, s_mentalRecoveryConditions);
            sleep = EaseSleep(conditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 54:
            ClearEffectConditions(conditions, s_extendedMentalRecoveryConditions);
            sleep = EaseSleep(conditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 55:
            ClearEffectConditions(conditions, s_poisonParalysisConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 56:
            ClearEffectConditions(conditions, s_extendedPoisonParalysisConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 57:
            ClearEffectConditions(conditions, g_physicalRecoveryConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 58:
            ClearEffectConditions(conditions, s_faintRecoveryConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            revival = 1;
            break;
        case 59:
            ClearEffectConditions(conditions, s_deathRecoveryConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            revival = 1;
            break;
        case 60:
            g_mpChange = -hp;
            DrainPool(mpPool, hp);
            FillPool(mpPool, mp, POOL_FILL_TO_MAX);
            g_mpChange = mp;
            reportCondition = 0;
            break;
        case 61:
            g_hpChange = -hp;
            DrainPool(hpPool, hp);
            FillPool(mpPool, mp, POOL_FILL_TO_MAX);
            g_mpChange = mp;
            reportCondition = 0;
            break;
        case 62:
            ClearEffectConditions(conditions, s_generalRecoveryConditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            revival = 1;
            break;
        case 63:
            if (GetAlignmentClassB(target) > 0) {
                g_hpChange = hp;
                FillPool(hpPool, target->pools.hp.max, POOL_FILL_TO_MAX);
            } else if (hpPool->cur > (target->pools.hp.max >> 3)) {
                hpPool->cur = target->pools.hp.max >> 3;
            }
            FillPool(mpPool, mp, POOL_FILL_TO_MAX);
            g_mpChange = mp;
            break;
        case 64:
            ClearEffectConditions(conditions, s_specialRecoveryConditions);
            sleep = EaseSleep(conditions);
            FillRestorePools(hpPool, mpPool, hp, mp);
            break;
        case 65:
            if (GetAlignmentClassB(target) < 0) {
                g_hpChange = target->pools.hp.max - hpPool->cur;
                FillPool(hpPool, target->pools.hp.max, POOL_FILL_TO_MAX);
            }
            FillPool(mpPool, mp, POOL_FILL_TO_MAX);
            g_mpChange = mp;
            break;
    }
    if (!HasCondition(conditions, CONDITION_ZOMBIE) && wasZombie
        && !GetFatalCondition(conditions)) {
        ApplyItemStatBonuses(&target->stats, GetCharacterEquipment(target));
        RecalcStatTotals(&target->stats);
        ApplyEquipmentEffects(target, EQUIP_EFFECT_STAT_UPDATE);
        RecalcDerivedStats(target);
        ResetBattleStatsToBase(target);
    }
    g_hpChange = hpPool->cur - oldHp;
    g_mpChange = mpPool->cur - oldMp;
    if (g_hpChange) {
        target->lastChange = g_hpChange;
    } else {
        target->lastChange = g_mpChange;
    }
    if (GetFatalCondition(conditions)) {
        hpPool->cur = 0;
        mpPool->cur = oldMp;
        g_mpChange = 0;
        g_hpChange = 0;
        target->lastChange = 0;
        return revival == 1 ? 6 : 2;
    }
    if (reportCondition) {
        if (!g_effectCondition) {
            if (sleep == 2) {
                g_effectCondition = 13;
            } else if (sleep == 1) {
                g_effectCondition = 25;
            }
        }
        return g_effectCondition ? 3 : 6;
    }
    return target->lastChange ? 3 : 6;
}

RVA(0x00020580, 0x160)
i16 ConditionKindApplies(i16 kind, ConditionSet* conditions) {
    i16 result = 0;
    switch (kind) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19:
        case 20:
        case 21:
        case 22:
        case 23:
        case 24:
        case 25:
        case 26:
        case 27:
        case 28:
        case 29:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
            result = HasCondition(conditions, kind);
            break;
        case 53:
            result = LastConditionIn(conditions, s_mentalRecoveryConditions);
            break;
        case 54:
            result = LastConditionIn(conditions, s_extendedMentalRecoveryConditions);
            break;
        case 55:
            result = LastConditionIn(conditions, s_poisonParalysisConditions);
            break;
        case 56:
            result = LastConditionIn(conditions, s_extendedPoisonParalysisConditions);
            break;
        case 57:
            result = LastConditionIn(conditions, g_physicalRecoveryConditions);
            break;
        case 58:
            result = LastConditionIn(conditions, s_faintRecoveryConditions);
            break;
        case 59:
            result = LastConditionIn(conditions, s_deathRecoveryConditions);
            break;
        case 62:
            result = LastConditionIn(conditions, s_generalRecoveryConditions);
            break;
        case 64:
            result = LastConditionIn(conditions, s_specialRecoveryConditions);
            break;
    }
    return result;
}
