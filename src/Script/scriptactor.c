// @identity-TODO: the owning TU is unproven; this unit holds the actor
// opcodes' retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Actor.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/BattleEffect.h>
#include <Game/DropTable.h>
#include <Game/Familiarity.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldView.h>
#include <Game/ItemBonus.h>
#include <Game/LevelUp.h>
#include <Game/ModeFlags.h>
#include <Game/Party.h>
#include <Game/SkillUse.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Util/Range.h>

#include <stddef.h>

DATA(0x000646c8)
static const i16 s_rewardLevelThresholds[16] =
    {20, 30, 40, 50, 60, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140};

DATA(0x00081358)
static i16 s_spoilAdjustment;

RVA(0x00032cf0, 0x1b)
void DespawnScriptActor(void) {
    Character* actor = g_curScript->actor;
    if (actor != NULL) {
        actor->fieldHidden = 1;
        RequestFieldRefresh();
    }
}

RVA(0x00032d10, 0x20)
void RetireScriptActor(void) {
    Character* actor = g_curScript->actor;
    if (actor != NULL) {
        SetAnalyzed(actor->id, 1);
        DespawnScriptActor();
    }
}

RVA(0x00032d30, 0x41)
i16 StepScriptActor(i16 turn) {
    FieldActor* actor = (FieldActor*)g_curScript->actor;
    if (actor == NULL) {
        return 0;
    }
    StepMapCoord(&actor->pos.x, &actor->pos.y, actor->direction, turn);
    InvalidateSelectedHotspot();
    RequestFieldRefresh();
    return -3;
}

RVA(0x00032d80, 0x3b)
void OpStoreActorDistance(void) {
    i16 index = ReadLongVarIndex();
    FieldActor* actor = (FieldActor*)g_curScript->actor;
    if (actor != NULL) {
        SetScriptLongVar(index, DistanceFromParty(actor->pos.x, actor->pos.y));
    }
}

RVA(0x00032dc0, 0xd1)
void PlaceScriptActor(void) {
    i16 layer;
    if (g_curScript->actor == NULL) {
        return;
    }
    if (TestModeFlags(MODE_WORLD_MAP)) {
        layer = FindLayerOfKind(g_curScript->actor->id);
        if (layer == -1) {
            return;
        }
        SpawnFieldObject(
            layer,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            ((FieldActor*)g_curScript->actor)->direction,
            g_curScript->actor->id,
            0,
            -1,
            0
        );
    } else {
        layer = FindCellObject(
            g_curScript->actor->id,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y
        );
        if (layer == -1) {
            return;
        }
        SpawnMapObject(
            layer,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            ((FieldActor*)g_curScript->actor)->direction,
            -1
        );
    }
    RequestFieldRefresh();
}

static __inline void GrantAdjustedActorSpoil(i16 kind, i16 adjustment) {
    s_spoilAdjustment = adjustment;
    GrantActorSpoil(kind);
}

RVA(0x00032ea0, 0x2b8)
void GrantActorReward(i16 kind) {
    u16 reward = 0;
    for (;;) {
        switch (kind) {
            case 0:
            case 1:
                reward = GetItemRewardAt(kind);
                break;
            case 2: {
                i16 roll = RandomAverage(0, 100, 100);
                if (g_curScript->actor != NULL) {
                    roll += g_curScript->actor->level;
                }
                for (kind = 0; kind < 16; kind++) {
                    if (roll <= s_rewardLevelThresholds[kind]) {
                        reward = GetGemItemBase() + kind;
                        break;
                    }
                }
                kind = 2;
                if (!reward) {
                    reward = GetGemItemBase();
                }
                break;
            }
            case 3:
                if (g_curScript->actor == NULL) {
                    return;
                }
                reward = g_curScript->actor->pickItem;
                break;
            case 4:
            case 5:
            case 6:
                GrantActorSpoil(kind - 4);
                reward = 0;
                break;
            case 7: {
                i16 roll = RandomAverage(1, 100, 0);
                if (roll <= 20) {
                    GrantAdjustedActorSpoil(0, 2);
                    reward = 0;
                } else if (roll <= 40) {
                    GrantAdjustedActorSpoil(1, 2);
                    reward = 0;
                } else if (roll <= 58) {
                    kind = 1;
                    continue;
                } else if (roll <= 66) {
                    kind = 0;
                    continue;
                } else if (roll <= 74) {
                    kind = 2;
                    continue;
                } else if (roll <= 84) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    reward = PickEquipmentReward(g_curScript->actor);
                    kind = 3;
                    if (reward < 1) {
                        continue;
                    }
                } else if (roll <= 92) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    reward = g_curScript->actor->id;
                    kind = 7;
                    HealParty(reward);
                    RequestFieldRefresh();
                } else {
                    kind = 3;
                    continue;
                }
                break;
            }
            case 8: {
                i16 roll = RandomAverage(1, 100, 0);
                if (roll <= 20) {
                    GrantAdjustedActorSpoil(0, 1);
                    reward = 0;
                } else if (roll <= 40) {
                    GrantAdjustedActorSpoil(0, 3);
                    reward = 0;
                } else if (roll <= 60) {
                    GrantAdjustedActorSpoil(1, 3);
                    reward = 0;
                } else if (roll <= 70) {
                    kind = 1;
                    continue;
                } else if (roll <= 80) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    kind = 8;
                    reward = g_curScript->actor->level;
                } else {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    kind = 9;
                    reward = g_curScript->actor->level;
                }
                break;
            }
        }
        break;
    }
    if (reward) {
        CallScript(0xdf, 2);
        SetScriptLongVar(18, kind);
        SetScriptLongVar(19, reward);
    }
}

RVA(0x00033160, 0xaf)
i16 PickEquipmentReward(Character* character) {
    i16 items[8];
    i16 count = 8;
    i16 i;
    i16 pick;
    for (i = 0; i < 8; i++) {
        items[i] = GetEquipItem(character, i);
    }
    for (;;) {
        pick = RandomAverage(0, count - 1, 0);
        if (items[pick] >= 1) {
            return items[pick];
        }
        count--;
        for (i = pick; i < count; i++) {
            items[i] = items[i + 1];
        }
        items[count] = -1;
        if (count <= 3) {
            return 0;
        }
    }
}

#define AdjustActorSpoilAmount(amount)                                                             \
    do {                                                                                           \
        if (s_spoilAdjustment == 1) {                                                              \
            (amount) = 1;                                                                          \
        } else if (s_spoilAdjustment == 2) {                                                       \
            (amount) += 5;                                                                         \
        } else if (s_spoilAdjustment == 3) {                                                       \
            (amount) = (amount) / 8 + 1;                                                           \
        }                                                                                          \
        s_spoilAdjustment = 0;                                                                     \
    } while (0)

RVA(0x00033210, 0x110)
void GrantActorSpoil(i16 kind) {
    if (g_curScript->actor != NULL) {
        i32 amount = 0;
        switch (kind) {
            case 2:
                amount = g_curScript->actor->experience;
                g_rewardExperience += amount;
                MarkRewardsPending();
                break;
            case 1:
                amount = g_curScript->actor->magnetite;
                AdjustActorSpoilAmount(amount);
                AddMagnetite(GetRosterCharacter(0), amount);
                break;
            case 0:
                amount = g_curScript->actor->macca;
                AdjustActorSpoilAmount(amount);
                AddMacca(GetRosterCharacter(0), amount);
                break;
        }
        CallScript(0xdf, 2);
        SetScriptLongVar(18, kind + 4);
        SetScriptLongVar(19, amount);
    }
}

RVA(0x00033320, 0x20)
void OpSetObjectPresence(void) {
    i16 id = ReadScriptValue();
    SetAnalyzed(id, 1 - ReadScriptValue());
}

RVA(0x00033340, 0x48)
void DismissTalkTarget(void) {
    if (g_targetId < 0) {
        return;
    }
    if (g_actionId == 0x10e) {
        FlashHitObject(g_targetId, 0x37);
        ResetObjectAnim(g_targetId);
    } else {
        GetFieldActor(g_targetId)->fieldHidden = 1;
        RequestFieldRefresh();
    }
}

RVA(0x00033390, 0xf6)
void OpJumpUnlessActorCanStep(i16 invert, i16 turn) {
    i32 matches = 0;
    i16 blocked = 1;
    i16 target = ReadBranchTarget();
    if (g_curScript->actor != NULL) {
        i16 x = ((FieldActor*)g_curScript->actor)->pos.x;
        i16 y = ((FieldActor*)g_curScript->actor)->pos.y;
        i16 direction = ((FieldActor*)g_curScript->actor)->direction;
        i16 relative = RelativeDirection(x, y, g_field.pos.x, g_field.pos.y, direction);
        turn = (relative + turn) & 3;
        blocked = GetMapWallKind(x, y, (direction + turn) & 3);
        if (!blocked) {
            StepMapCoord(&x, &y, direction, turn);
            blocked = IsCellBlocked(g_field.pos.level, 1, x, y);
        }
    }
    if ((!blocked && !invert) || (blocked && invert)) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x00033490, 0x76)
i16 OpSetActorAlert(i16 level) {
    ReadScriptValue();
    if (g_curScript->actor != NULL) {
        if (level != 2) {
            AlertActor(g_curScript->actor, 2);
        }
        if (level == 1) {
            if ((u16)GetCharacterActionWait(g_curScript->actor)->remaining > 1) {
                GetCharacterActionWait(g_curScript->actor)->remaining = 1;
            }
        } else if (level == 2) {
            if ((u16)GetCharacterActionWait(g_curScript->actor)->remaining < 0x200) {
                GetCharacterActionWait(g_curScript->actor)->remaining = 0x200;
            }
        }
    }
    return -1;
}

RVA(0x00033510, 0x87)
void OpJumpUnlessPlayerInLine(i16 invert) {
    i32 matches = 0;
    i16 target = ReadBranchTarget();
    FieldActor* actor = (FieldActor*)g_curScript->actor;
    if (actor != NULL) {
        MapCoord offset = RelativeOffset(
            actor->pos.x,
            actor->pos.y,
            actor->direction,
            g_field.pos.x,
            g_field.pos.y
        );
        if ((offset.x == 0 && offset.y <= 0 && invert == 0)
            || ((offset.x != 0 || offset.y > 0) && invert != 0)) {
            matches = 1;
        }
    }
    ScriptJumpUnless(target, matches);
}
