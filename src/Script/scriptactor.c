// @identity-TODO: the owning TU is unproven. One retail object: the script
// actor and flow opcodes, the comparison and clock opcodes, the text tokens,
// the operand reader and the choice commands. The flow and actor statics
// follow the token, choice and operand statics in one .bss run, against .text
// order; the token tables follow the choice command's hovered word in .data;
// each static is read only by its own part's code, and the code is contiguous
// in .text.

#include <rva.h>

#include <Game/ActionMark.h>
#include <Game/Actor.h>
#include <Game/Alignment.h>
#include <Game/AnalyzeData.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharacterStat.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
#include <Game/EquipSlotIndex.h>
#include <Game/Familiarity.h>
#include <Game/Field.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldView.h>
#include <Game/FusionMenu.h>
#include <Game/FusionScreen.h>
#include <Game/GameLoop.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/Growth.h>
#include <Game/InfoBar.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemId.h>
#include <Game/ItemMenu.h>
#include <Game/ItemPool.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/MapArea.h>
#include <Game/ModeFlags.h>
#include <Game/ObjectRecordId.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/Skill.h>
#include <Game/SkillId.h>
#include <Game/SkillList.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/WorldMap.h>
#include <Gfx/Sprite.h>
#include <Giten/Resource.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/ObjectRef.h>
#include <Script/OwnedFlag.h>
#include <Script/Script.h>
#include <Script/ScriptBlock.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptPanel.h>
#include <Script/ScriptVars.h>
#include <Script/TextToken.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Text/TextPlaneAttr.h>
#include <Text/WindowText.h>
#include <Ui/Panel.h>
#include <Ui/PartySlotSelection.h>
#include <Util/BitSet.h>
#include <Util/Debug.h>
#include <Util/Range.h>

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define SCRIPT_CHOICE_NONE (-1)

GZ_ENUM_BEGIN_SPLIT(ScriptChoicePollResult, i16)
    SCRIPT_CHOICE_CANCELLED = -1,
    SCRIPT_CHOICE_WAITING = 0,
    SCRIPT_CHOICE_SELECTED = 1
GZ_ENUM_END_SPLIT(ScriptChoicePollResult)

DATA(0x000646c8)
static const i16 s_rewardLevelThresholds[GEM_ITEM_COUNT] =
    {20, 30, 40, 50, 60, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140};

DATA(0x00069130)
static i16 s_hoveredChoice = SCRIPT_CHOICE_NONE;

// "Ａ", "Ｂ", "ＡＢ", "Ｏ".
DATA(0x00069138)
static char* s_bloodTypes[BLOOD_TYPE_COUNT] = {"\202`", "\202a", "\202`\202a", "\202n"};

// The expansion of the last text token.
DATA(0x00081230)
static char s_tokenText[0x100] = {0};

DATA(0x00081330)
static i16 s_choiceWindow = 0;

DATA(0x00081334)
static i16 s_keepChoices = 0;

DATA(0x00081338)
static i16 s_choiceCancelMode = 0;

DATA(0x0008133c)
static ScriptChoice* s_highlightedChoice = NULL;

DATA(0x00081340)
static ScriptChoice* s_choiceMenu = NULL;

DATA(0x00081344)
static ScriptChoice* s_hitChoice = NULL;

// The slot ReadScriptOperand returns.
DATA(0x00081348)
static i32 s_operand = 0;

// The long-variable accumulator, the second operand and the variable index.
DATA(0x0008134c)
static i32 s_longAcc = 0;

DATA(0x00081350)
static i32 s_longOperand = 0;

DATA(0x00081354)
static i16 s_longVarIndex = 0;

DATA(0x00081358)
static i16 s_spoilAdjustment = 0;

// @identity-TODO: the empty names the sign and affiliation tables point at;
// each is its own 4-byte .bss item, and no code writes them.
DATA(0x0008135c)
static char s_signName0[4] = "";

DATA(0x00081360)
static char s_signName1[4] = "";

DATA(0x00081364)
static char s_signName2[4] = "";

DATA(0x00081368)
static char s_signName3[4] = "";

DATA(0x0008136c)
static char s_signName4[4] = "";

DATA(0x00081370)
static char s_signName5[4] = "";

DATA(0x00081374)
static char s_signName6[4] = "";

DATA(0x00081378)
static char s_signName7[4] = "";

DATA(0x0008137c)
static char s_signName8[4] = "";

DATA(0x00081380)
static char s_signName9[4] = "";

DATA(0x00081384)
static char s_signName10[4] = "";

DATA(0x00081388)
static char s_signName11[4] = "";

DATA(0x0008138c)
static char s_affiliationName0[4] = "";

DATA(0x00081390)
static char s_affiliationName1[4] = "";

DATA(0x00081394)
static char s_affiliationName2[4] = "";

DATA(0x00081398)
static char s_affiliationName3[4] = "";

DATA(0x00069148)
static char* s_signNames[12] = {
    s_signName0,
    s_signName1,
    s_signName2,
    s_signName3,
    s_signName4,
    s_signName5,
    s_signName6,
    s_signName7,
    s_signName8,
    s_signName9,
    s_signName10,
    s_signName11,
};

DATA(0x00069178)
static char* s_affiliationNames[4] = {
    s_affiliationName0,
    s_affiliationName1,
    s_affiliationName2,
    s_affiliationName3,
};

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
GZ_ENUM_RETURN(ScriptStatus, i16) StepScriptActor(GZ_ENUM_PARAM(MoveCommand, i16) turn) {
    FieldActor* actor = (FieldActor*)g_curScript->actor;
    if (actor == NULL) {
        return SCRIPT_CONTINUE;
    }
    StepMapCoord(&actor->pos.x, &actor->pos.y, actor->direction, turn);
    InvalidateSelectedHotspot();
    RequestFieldRefresh();
    return SCRIPT_YIELD;
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
        if (layer == FIELD_LAYER_NONE) {
            return;
        }
        SpawnFieldObject(
            layer,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            ((FieldActor*)g_curScript->actor)->direction,
            g_curScript->actor->id,
            false,
            FIELD_OBJECT_NO_EVENT,
            false
        );
    } else {
        layer = FindCellObject(
            g_curScript->actor->id,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y
        );
        if (layer == CELL_OBJECT_INDEX_NONE) {
            return;
        }
        SpawnMapObject(
            layer,
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            ((FieldActor*)g_curScript->actor)->direction,
            FIELD_OBJECT_NO_EVENT
        );
    }
    RequestFieldRefresh();
}

static __inline void
GrantAdjustedActorSpoil(GZ_ENUM_PARAM(ActorSpoilKind, i16) kind, i16 adjustment) {
    s_spoilAdjustment = adjustment;
    GrantActorSpoil(kind);
}

RVA(0x00032ea0, 0x2b8)
void GrantActorReward(GZ_ENUM_PARAM(ActorRewardKind, i16) kind) {
    u16 reward = 0;
    for (;;) {
        switch (kind) {
            case ACTOR_REWARD_FIRST_ITEM:
            case ACTOR_REWARD_SECOND_ITEM:
                reward = GetItemRewardAt(kind);
                break;
            case ACTOR_REWARD_GEM: {
                i16 roll = RandomAverage(0, 100, 100);
                if (g_curScript->actor != NULL) {
                    roll += g_curScript->actor->level;
                }
                for (kind = 0; kind < GEM_ITEM_COUNT; kind++) {
                    if (roll <= s_rewardLevelThresholds[kind]) {
                        reward = GetGemItemBase() + kind;
                        break;
                    }
                }
                kind = ACTOR_REWARD_GEM;
                if (!reward) {
                    reward = GetGemItemBase();
                }
                break;
            }
            case ACTOR_REWARD_PICK_ITEM:
                if (g_curScript->actor == NULL) {
                    return;
                }
                reward = g_curScript->actor->pickItem;
                break;
            case ACTOR_REWARD_SPOIL_MACCA:
            case ACTOR_REWARD_SPOIL_MAGNETITE:
            case ACTOR_REWARD_SPOIL_EXPERIENCE:
                GrantActorSpoil(kind - ACTOR_REWARD_SPOIL_MACCA);
                reward = 0;
                break;
            case ACTOR_REWARD_RANDOM: {
                i16 roll = RandomAverage(1, 100, 0);
                if (roll <= 20) {
                    GrantAdjustedActorSpoil(ACTOR_SPOIL_MACCA, 2);
                    reward = 0;
                } else if (roll <= 40) {
                    GrantAdjustedActorSpoil(ACTOR_SPOIL_MAGNETITE, 2);
                    reward = 0;
                } else if (roll <= 58) {
                    kind = ACTOR_REWARD_SECOND_ITEM;
                    continue;
                } else if (roll <= 66) {
                    kind = ACTOR_REWARD_FIRST_ITEM;
                    continue;
                } else if (roll <= 74) {
                    kind = ACTOR_REWARD_GEM;
                    continue;
                } else if (roll <= 84) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    reward = PickEquipmentReward(g_curScript->actor);
                    kind = ACTOR_REWARD_PICK_ITEM;
                    if (reward < 1) {
                        continue;
                    }
                } else if (roll <= 92) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    reward = g_curScript->actor->id;
                    kind = ACTOR_REWARD_HEALED;
                    HealParty(reward);
                    RequestFieldRefresh();
                } else {
                    kind = ACTOR_REWARD_PICK_ITEM;
                    continue;
                }
                break;
            }
            case ACTOR_REWARD_RANDOM_B: {
                i16 roll = RandomAverage(1, 100, 0);
                if (roll <= 20) {
                    GrantAdjustedActorSpoil(ACTOR_SPOIL_MACCA, 1);
                    reward = 0;
                } else if (roll <= 40) {
                    GrantAdjustedActorSpoil(ACTOR_SPOIL_MACCA, 3);
                    reward = 0;
                } else if (roll <= 60) {
                    GrantAdjustedActorSpoil(ACTOR_SPOIL_MAGNETITE, 3);
                    reward = 0;
                } else if (roll <= 70) {
                    kind = ACTOR_REWARD_SECOND_ITEM;
                    continue;
                } else if (roll <= 80) {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    kind = ACTOR_REWARD_BOMB_ATTACK;
                    reward = g_curScript->actor->level;
                } else {
                    if (g_curScript->actor == NULL) {
                        return;
                    }
                    kind = ACTOR_REWARD_PUNCH_ATTACK;
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
    for (i = 0; i < EQUIP_SLOT_COUNT; i++) {
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
        items[count] = ITEM_ID_EMPTY;
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
void GrantActorSpoil(GZ_ENUM_PARAM(ActorSpoilKind, i16) kind) {
    if (g_curScript->actor != NULL) {
        i32 amount = 0;
        switch (kind) {
            case ACTOR_SPOIL_EXPERIENCE:
                amount = GetCharacterExperience(g_curScript->actor);
                g_rewardExperience += amount;
                MarkRewardsPending();
                break;
            case ACTOR_SPOIL_MAGNETITE:
                amount = g_curScript->actor->magnetite;
                AdjustActorSpoilAmount(amount);
                AddMagnetite(GetRosterCharacter(ROSTER_LEADER), amount);
                break;
            case ACTOR_SPOIL_MACCA:
                amount = g_curScript->actor->macca;
                AdjustActorSpoilAmount(amount);
                AddMacca(GetRosterCharacter(ROSTER_LEADER), amount);
                break;
        }
        CallScript(0xdf, 2);
        SetScriptLongVar(18, kind + ACTOR_REWARD_SPOIL_MACCA);
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
    if (g_actionId == SKILL_FUSION) {
        FlashHitObject(g_targetId, 0x37);
        ResetObjectAnim(g_targetId);
    } else {
        GetFieldActor(g_targetId)->fieldHidden = 1;
        RequestFieldRefresh();
    }
}

RVA(0x00033390, 0xf6)
void OpJumpUnlessActorCanStep(
    GZ_ENUM_PARAM(ScriptTestPolarity, i16) invert,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
) {
    b32 matches = false;
    i16 blocked = 1;
    i16 target = ReadBranchTarget();
    if (g_curScript->actor != NULL) {
        i16 x = ((FieldActor*)g_curScript->actor)->pos.x;
        i16 y = ((FieldActor*)g_curScript->actor)->pos.y;
        i16 direction = ((FieldActor*)g_curScript->actor)->direction;
        i16 relative = RelativeDirection(x, y, g_party.field.pos.x, g_party.field.pos.y, direction);
        turn = (relative + turn) & 3;
        blocked = GetMapWallKind(x, y, (direction + turn) & 3);
        if (!blocked) {
            StepMapCoord(&x, &y, direction, turn);
            blocked = IsCellBlocked(g_party.field.pos.level, CELL_SCAN_TEST, x, y);
        }
    }
    if ((!blocked && !invert) || (blocked && invert)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x00033490, 0x76)
GZ_ENUM_RETURN(ScriptStatus, i16) OpSetActorAlert(GZ_ENUM_PARAM(ActorAlertMode, i16) level) {
    ReadScriptValue();
    if (g_curScript->actor != NULL) {
        if (level != ACTOR_ALERT_DELAY) {
            AlertActor(g_curScript->actor, ATTITUDE_VERY_HOSTILE);
        }
        if (level == ACTOR_ALERT_IMMEDIATE) {
            if (GetCharacterActionWait(g_curScript->actor)->remaining > ACTION_WAIT_QUEUED) {
                GetCharacterActionWait(g_curScript->actor)->remaining = ACTION_WAIT_QUEUED;
            }
        } else if (level == ACTOR_ALERT_DELAY) {
            if (GetCharacterActionWait(g_curScript->actor)->remaining < ACTION_WAIT_EXTENDED) {
                GetCharacterActionWait(g_curScript->actor)->remaining = ACTION_WAIT_EXTENDED;
            }
        }
    }
    return SCRIPT_END;
}

RVA(0x00033510, 0x87)
void OpJumpUnlessPlayerInLine(i16 invert) {
    b32 matches = false;
    i16 target = ReadBranchTarget();
    FieldActor* actor = (FieldActor*)g_curScript->actor;
    if (actor != NULL) {
        MapCoord offset = RelativeOffset(
            actor->pos.x,
            actor->pos.y,
            actor->direction,
            g_party.field.pos.x,
            g_party.field.pos.y
        );
        if ((offset.x == 0 && offset.y <= 0 && invert == false)
            || ((offset.x != 0 || offset.y > 0) && invert != false)) {
            matches = true;
        }
    }
    ScriptJumpUnless(target, matches);
}

DATA(0x000911b0)
i16 g_scriptArgA;

DATA(0x000911b2)
i16 g_scriptArgB;

// The byte before the index is a kind byte the long ops ignore.
RVA(0x000335a0, 0x15)
i16 ReadLongVarIndex(void) {
    ReadScriptByte();
    s_longVarIndex = (i8)ReadScriptByte();
    return s_longVarIndex;
}

RVA(0x000335c0, 0x1d)
i32 ReadLongOperand(i16 inPlace) {
    if (inPlace == 0) {
        return ReadScriptValue();
    }
    return GetScriptLongVar(s_longVarIndex);
}

RVA(0x000335e0, 0x22)
void LoadLongOperands(i16 inPlace) {
    ReadLongVarIndex();
    s_longAcc = ReadLongOperand(inPlace);
    s_longOperand = ReadScriptValue();
}

RVA(0x00033610, 0x1c)
i32 StoreLongResult(void) {
    SetScriptLongVar(s_longVarIndex, s_longAcc);
    return s_longAcc;
}

RVA(0x00033630, 0x25)
void OpMulLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc *= s_longOperand;
    StoreLongResult();
}

RVA(0x00033660, 0x34)
void OpDivLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    if (s_longOperand == 0) {
        s_longOperand = 1;
    }
    s_longAcc /= s_longOperand;
    StoreLongResult();
}

RVA(0x000336a0, 0x24)
void OpAddLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc += s_longOperand;
    StoreLongResult();
}

RVA(0x000336d0, 0x24)
void OpSubLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc -= s_longOperand;
    StoreLongResult();
}

RVA(0x00033700, 0x24)
void OpAndLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc &= s_longOperand;
    StoreLongResult();
}

RVA(0x00033730, 0x24)
void OpOrLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc |= s_longOperand;
    StoreLongResult();
}

RVA(0x00033760, 0x24)
void OpXorLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc ^= s_longOperand;
    StoreLongResult();
}

RVA(0x00033790, 0x24)
void OpShlLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc <<= s_longOperand;
    StoreLongResult();
}

RVA(0x000337c0, 0x24)
void OpSarLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc >>= s_longOperand;
    StoreLongResult();
}

// The accumulator as a percentage of the operand.
RVA(0x000337f0, 0x3d)
void OpPercentLongVar(i16 inPlace) {
    i32 scaled;
    LoadLongOperands(inPlace);
    scaled = s_longAcc * 100;
    if (s_longOperand == 0) {
        s_longOperand = 1;
    }
    s_longAcc = scaled / s_longOperand;
    StoreLongResult();
}

RVA(0x00033830, 0x2b)
void OpSqrtLongVar(i16 inPlace) {
    ReadLongVarIndex();
    s_longAcc = (i32)sqrt(ReadLongOperand(inPlace));
    StoreLongResult();
}

RVA(0x00033860, 0x24)
void OpModLongVar(i16 inPlace) {
    LoadLongOperands(inPlace);
    s_longAcc %= s_longOperand;
    StoreLongResult();
}

RVA(0x00033890, 0x1a)
void LoadLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = GetScriptLongVar(s_longVarIndex);
}

RVA(0x000338b0, 0x17)
void StoreLongVar(void) {
    SetScriptLongVar(s_longVarIndex, s_longAcc);
}

// Sets a variable to a value. Setting variable 23 to 15 while slot 0 is empty
// and track 7 plays shows picture 0x73 in slot 31.
RVA(0x000338d0, 0x6d)
void OpSetLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = ReadScriptValue();
    if (s_longVarIndex == 23 && s_longAcc == 15 && !IsSpritePlaced(0) && CurrentMusicTrack() == 7) {
        LoadSpriteImage(31, 0x73, 0);
        PlaceSprite(31, 31, 0, 40, 240);
        // 邪教のところ以外このメッセージをみたらお知らせください
        DebugTrace(
            "\n\216\327\213\263\202\314\202\306\202\261\202\353\210\310\212O\202\261\202\314\203"
            "\201\203b\203Z\201["
            "\203W\202\360\202\335\202\275\202\347\202\250\222m\202\347\202\271\202\255\202\276\202"
            "\263\202\242 S.Iseki \n"
        );
    }
    StoreLongVar();
}

RVA(0x00033940, 0x1f)
void OpSwapLongVars(void) {
    i16 first = ReadLongVarIndex();
    ReadLongVarIndex();
    SwapScriptLongVars(first, s_longVarIndex);
}

RVA(0x00033960, 0x1f)
void OpCopyLongVar(void) {
    i16 dst = ReadLongVarIndex();
    ReadLongVarIndex();
    CopyScriptLongVar(dst, s_longVarIndex);
}

RVA(0x00033980, 0x15)
void OpUnsetLongVar(void) {
    ReadLongVarIndex();
    ClearScriptLongVar(s_longVarIndex);
}

RVA(0x000339a0, 0x17)
void OpZeroLongVar(void) {
    ReadLongVarIndex();
    SetScriptLongVar(s_longVarIndex, 0);
}

RVA(0x000339c0, 0x16)
void OpNegLongVar(void) {
    LoadLongVar();
    s_longAcc = -s_longAcc;
    StoreLongVar();
}

RVA(0x000339e0, 0x16)
void OpNotLongVar(void) {
    LoadLongVar();
    s_longAcc = ~s_longAcc;
    StoreLongVar();
}

RVA(0x00033a00, 0x10)
void OpIncLongVar(void) {
    LoadLongVar();
    s_longAcc++;
    StoreLongVar();
}

RVA(0x00033a10, 0x10)
void OpDecLongVar(void) {
    LoadLongVar();
    s_longAcc--;
    StoreLongVar();
}

RVA(0x00033a20, 0x26)
void OpIncLongVarBelow(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc < s_longOperand) {
        s_longAcc++;
    }
    StoreLongVar();
}

RVA(0x00033a50, 0x26)
void OpDecLongVarAbove(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc > s_longOperand) {
        s_longAcc--;
    }
    StoreLongVar();
}

RVA(0x00033a80, 0x3c)
void OpClampLongVar(void) {
    LoadLongVar();
    s_longOperand = ReadScriptValue();
    if (s_longAcc < s_longOperand) {
        s_longAcc = s_longOperand;
    }
    s_longOperand = ReadScriptValue();
    if (s_longAcc > s_longOperand) {
        s_longAcc = s_longOperand;
    }
    StoreLongVar();
}

RVA(0x00033ac0, 0x35)
void OpRollLongVar(void) {
    i16 lo;
    i16 hi;
    ReadLongVarIndex();
    lo = ReadScriptValue();
    hi = ReadScriptValue();
    s_longAcc = RandomAverage(lo, hi, ReadScriptValue());
    StoreLongVar();
}

RVA(0x00033b00, 0x14)
void OpRandLongVar(void) {
    ReadLongVarIndex();
    s_longAcc = rand();
    StoreLongVar();
}

// Saves the system variables into the newest call frame.
RVA(0x00033b20, 0x26)
b16 StoreFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    if (!frame) {
        return false;
    }
    TransferFrameVars(frame, false);
    return true;
}

// Restores the system variables from the newest call frame.
RVA(0x00033b50, 0x26)
b16 LoadFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    if (!frame) {
        return false;
    }
    TransferFrameVars(frame, true);
    return true;
}

// Exchanges the system variables with the newest call frame's, through two
// scratch frames.
RVA(0x00033b80, 0x86)
b16 SwapFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    i32 saved;
    i32 loaded;
    if (!frame) {
        return false;
    }
    saved = NewCallFrame();
    loaded = NewCallFrame();
    TransferFrameVars(saved, false);
    TransferFrameVars(frame, true);
    TransferFrameVars(loaded, false);
    TransferFrameVars(saved, true);
    TransferFrameVars(frame, false);
    TransferFrameVars(loaded, true);
    FreeCallFrames(loaded);
    FreeCallFrames(saved);
    return true;
}

// Reads two byte operands into words.
RVA(0x00033c10, 0x22)
void ReadScriptBytePair(i16* first, i16* second) {
    *first = ReadScriptByte();
    *second = ReadScriptByte();
}

RVA(0x00033c40, 0x10)
void ScriptJump(i16 pc) {
    g_curScript->pc = pc;
}

// Continues the script at `pc` of the code block `codeBase`.
RVA(0x00033c50, 0x1e)
b16 ScriptJumpTo(u32 codeBase, i16 pc) {
    g_curScript->codeBase = codeBase;
    ScriptJump(pc);
    return false;
}

// Continues the script at `pc` of `codeBase`, passing the two call arguments.
RVA(0x00033c70, 0x2a)
b16 ScriptJumpWithArgs(u32 codeBase, i16 pc, i16 argA, i16 argB) {
    g_scriptArgA = argA;
    g_scriptArgB = argB;
    return ScriptJumpTo(codeBase, pc);
}

// Where `entry` of script file `file` starts: files 0xe0..0xff are the
// current actor's field-layer scripts; any other file is taken from the cache,
// loading it on a miss.
RVA(0x00033ca0, 0xca)
ScriptEntry ResolveScriptEntry(i16 file, i16 entry) {
    ScriptEntry result;
    if (file >= 0xe0 && file <= 0xff) {
        i16 layer = FindLayerOfKind(g_curScript->actor->id);
        return FindLayerScriptEntry(layer + 1, file, entry);
    }
    result = FindCachedScriptEntry(file, entry);
    if (result.code) {
        return result;
    }
    return MakeScriptEntry(CacheScriptFile(file), entry);
}

// Continues the current script at `entry` of script file `file`, passing both
// as the call arguments.
// @identity-TODO: the three special cases (a redirected entry, a sprite reset
// and a picture shown at one map place) are unexplained.
RVA(0x00033d70, 0xc1)
void GotoScript(i16 file, i16 entry) {
    ScriptEntry target;
    if (file == 0xde && entry == 1) {
        entry = 9;
    } else if (file == 0x2c && entry == 4) {
        ResetSprites(SPRITE_LAYERS_PARTY_AND_TEXT);
    } else if (file == 0x5b && entry == 0x22 && g_party.field.pos.x == 1 && g_party.field.pos.y == 4
               && g_party.field.pos.level == 1
               && g_party.field.pos.area == MAP_AREA_RESISTANCE_FRONTLINE_BASE) {
        LoadSpriteImage(0, 0x42, 0);
        PlaceSprite(0, 0, 0, 40, 240);
    }
    target = ResolveScriptEntry(file, entry);
    ScriptJumpWithArgs(target.code, target.pc, file, entry);
}

// Calls `entry` of script file `file`, returning to the current position.
RVA(0x00033e40, 0x23)
void CallScript(i16 file, i16 entry) {
    PushCallFrame(g_curScript, 0);
    GotoScript(file, entry);
}

// Reads a script file and entry and jumps (or with `call`, calls) there.
RVA(0x00033e70, 0x49)
void OpJumpScript(i16 call) {
    i16 file;
    i16 entry;
    ReadScriptBytePair(&file, &entry);
    if (!call) {
        GotoScript(file, entry);
    } else {
        CallScript(file, entry);
    }
}

// Reads a jump offset; returns the position it names.
RVA(0x00033ec0, 0x13)
i16 ReadJumpTarget(void) {
    u16 offset = ReadScriptWord();
    return g_curScript->pc + offset;
}

RVA(0x00033ee0, 0xf)
void OpJump(void) {
    ScriptJump(ReadJumpTarget());
}

// Jumps to `pc` when `condition` is zero; passes `condition` through.
RVA(0x00033ef0, 0x1a)
b32 ScriptJumpUnless(i16 pc, b32 condition) {
    if (!condition) {
        ScriptJump(pc);
    }
    return condition;
}

// Pushes a call frame whose return position is the jump target read from the
// script, and continues after the operand.
RVA(0x00033f10, 0x38)
b16 OpPushReturnTarget(void) {
    i16 target = ReadJumpTarget();
    i16 next = g_curScript->pc;
    ScriptJump(target);
    PushCallFrame(g_curScript, 1);
    ScriptJump(next);
    return false;
}

RVA(0x00033f50, 0x14)
b16 DropCallFrame(void) {
    PopCallFrame(g_curScript, 1);
    return false;
}

RVA(0x00033f70, 0x12)
b16 SwapCallFrames(void) {
    SwapTopCallFrames(g_curScript);
    return false;
}

RVA(0x00033f90, 0x12)
b16 ClearCallStack(void) {
    UnwindCallFrames(g_curScript);
    return false;
}

// Returns from the current call; -1 when that ends the script.
RVA(0x00033fb0, 0x21)
i16 ReturnFromCall(void) {
    PopCallFrame(g_curScript, 0);
    if (g_curScript->codeBase) {
        return SCRIPT_CONTINUE;
    }
    return SCRIPT_END;
}

// Discards every call frame and continues at `entry` of script file `file`.
RVA(0x00033fe0, 0x18)
void RestartScript(i16 file, i16 entry) {
    ClearCallStack();
    GotoScript(file, entry);
}

// Loads script file `file` entry `entry` (skipping files 0xe0..0xff); a shop
// entry of file 0x7f first shows its keeper, and traces which shop it should
// be.
RVA(0x00034000, 0x599)
void OpLoadRecord(void) {
    i16 file = ReadScriptValue() & 0xff;
    i16 entry = ReadScriptValue();

    if (file >= 0xe0 && file <= 0xff) {
        return;
    }
    if (file == 0x7f && entry >= 0xb0) {
        if (entry < 0xb7) {
            LoadSpriteImage(0x1f, 0x74, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
            // マイシティ・シャンシャンシティ２・神田地下街・御茶ノ水・秋葉原・銀座地下街・恵比寿ガーデンプレイスの酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\203}"
                "\203C\203V\203e\203B\201E\203V\203\203\203\223\203V\203\203\203\223\203V\203e\203B"
                "\202Q\201E\220_"
                "\223c\222n\211\272\212X\201E\214\344\222\203\203m\220\205\201E\217H\227t\214\264"
                "\201E\213\342\215\300\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                "\203f\203\223\203v\203\214\203C\203X\202\314\216\360\217\352\202\305\202\310\202"
                "\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242"
                "\201BTakubo\012"
            );
        } else if (entry == 0xb7) {
            LoadSpriteImage(0x1f, 0xc7, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xd5);
            // 大歓楽街の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\221\345\212\275\212y\212X\202\314\216\360\217\352\202\305\202\310\202\242\217"
                "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTaku"
                "bo\012"
            );
        } else if (entry == 0xb8) {
            LoadSpriteImage(0x1f, 0x56, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
            // 六本木の酒場でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\230Z\226{"
                "\226\330\202\314\216\360\217\352\202\305\202\310\202\242\217\352\215\207\202\315"
                "\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x100
                   || (entry == 0x101
                       && g_party.field.pos.area == MAP_AREA_RESISTANCE_FRONTLINE_BASE)) {
            LoadSpriteImage(0x1f, 0x79, 0);
            PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xd3);
            // 初台以外の道具屋＆レジスタンス前線基地の薬屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\210\310\212O\202\314\223\271\213\357\211\256\201\225\203\214"
                "\203W\203X\203^"
                "\203\223\203X\221O\220\374\212\356\222n\202\314\226\362\211\256\202\305\202\310"
                "\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202"
                "\242\201BTakubo\012"
            );
        } else if (entry == 0x101 && g_party.field.pos.area == MAP_AREA_ROPPONGI
                   && g_party.field.pos.x == 7) {
            LoadSpriteImage(0x1f, 0xc7, 1);
            PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xe1);
            // 六本木の防具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\230Z\226{"
                "\226\330\202\314\226h\213\357\211\256\202\305\202\310\202\242\217\352\215\207\202"
                "\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x101) {
            LoadSpriteImage(0x1f, 0x79, 1);
            if ((g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND
                 && g_party.field.pos.y == 0x12)
                || (g_party.field.pos.area == MAP_AREA_MY_CITY && g_party.field.pos.y == 0x0d)
                || (g_party.field.pos.area == MAP_AREA_SHANSHAN_CITY && g_party.field.pos.y == 0x01)
                || (g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND
                    && g_party.field.pos.y == 0x05)
                || (g_party.field.pos.area == MAP_AREA_OCHANOMIZU && g_party.field.pos.y == 0x02)
                || (g_party.field.pos.area == MAP_AREA_AKIHABARA_STATION_BUILDING
                    && g_party.field.pos.level == 2)
                || (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND
                    && g_party.field.pos.x == 0x0a)
                || (g_party.field.pos.area == MAP_AREA_EBISU_GARDEN && g_party.field.pos.level == 0)
                || g_party.field.pos.area == MAP_AREA_ROPPONGI
                || (g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING
                    && g_party.field.pos.level == 0)
                || (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA && g_party.field.pos.y == 0x09)
                || (g_party.field.pos.area == MAP_AREA_SHIBUYA && g_party.field.pos.y == 0x0b)) {
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xda);
                // 臨海コロシアム以外の武器屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\210\310\212O\202\314\225\220"
                    "\212\355\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215"
                    "\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xe6);
                // 臨海コロシアム・六本木以外の防具屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\201E\230Z\226{"
                    "\226\330\210\310\212O\202\314\226h\213\357\211\256\202\305\202\310\202\242\217"
                    "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201B"
                    "Takubo\012"
                );
            }
        } else if (entry == 0x102 || entry == 0x104) {
            if (g_party.field.pos.area == MAP_AREA_GINZA_UNDERGROUND
                && g_party.field.pos.level == 3) {
                LoadSpriteImage(0x1f, 0x4f, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xd4);
                // 銀座地下街秘密区の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\213\342\215\300\222n\211\272\212X\224\351\226\247\213\346\202\314\226\362"
                    "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                    "\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_party.field.pos.area == MAP_AREA_SHINJUKU_UNDERGROUND
                       || g_party.field.pos.area == MAP_AREA_KANDA_UNDERGROUND
                       || g_party.field.pos.area == MAP_AREA_EBISU_GARDEN
                       || g_party.field.pos.area == MAP_AREA_ASAKUSA_SUBWAY_BUILDING) {
                LoadSpriteImage(0x1f, 0x79, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xda);
                // 新宿地下街・神田地下街・恵比寿ガーデン・浅草地下鉄ビルの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\220V\217h\222n\211\272\212X\201E\220_"
                    "\223c\222n\211\272\212X\201E\214b\224\344\216\365\203K\201["
                    "\203f\203\223\201E\220\363\221\220\222n\211\272\223S\203r\203\213\202\314\226"
                    "\362\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202"
                    "\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM) {
                LoadSpriteImage(0x1f, 0x2b, 3);
                PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd4);
                // 臨海コロシアムの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226\362\211\256\202"
                    "\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                    "\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x2b, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd4);
                // レジスタンス前線基地・新宿地下街・神田地下街・銀座地下街秘密区・恵比寿ガーデン・浅草地下街・臨海コロシアム以外の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203\214\203W\203X\203^"
                    "\203\223\203X\221O\220\374\212\356\222n\201E\220V\217h\222n\211\272\212X\201E"
                    "\220_"
                    "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\224\351\226\247"
                    "\213\346\201E\214b\224\344\216\365\203K\201["
                    "\203f\203\223\201E\220\363\221\220\222n\211\272\212X\201E\227\325\212C\203R"
                    "\203\215\203V\203A\203\200\210\310\212O\202\314\226\362\211\256\202\305\202"
                    "\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202"
                    "\263\202\242\201BTakubo\012"
                );
            }
        } else if (entry == 0x103) {
            LoadSpriteImage(0x1f, 0x25, 3);
            PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd5);
            // 初台の道具屋でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\202\314\223\271\213\357\211\256\202\305\202\310\202\242\217"
                "\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202\263\202\242\201BTaku"
                "bo\012"
            );
        } else if (entry == 0x10a || entry == 0x10b) {
            LoadSpriteImage(0x1f, 0x2c, 4);
            PlaceSprite(0x1f, 0x1f, 4, 0x28, 0xd5);
            // コンピュータショップでない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\203R\203\223\203s\203\205\201[\203^"
                "\203V\203\207\203b\203v\202\305\202\310\202\242\217\352\215\207\202\315\230A\227"
                "\215\202\265\202\304\211\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x105) {
            if (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA) {
                LoadSpriteImage(0x1f, 0x79, 2);
                PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xda);
                // アメ屋プラザ２階の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203A\203\201\211\256\203v\203\211\203U\202Q\212K\202\314\226\362\211\256"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0xb7, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xda);
                // 大歓楽街２Ｆの薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\221\345\212\275\212y\212X\202Q\202e\202\314\226\362\211\256\202\305\202"
                    "\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211\272\202"
                    "\263\202\242\201BTakubo\012"
                );
            }
        } else if (entry == 0x106) {
            if (g_party.field.pos.x == 1 && g_party.field.pos.y == 5) {
                LoadSpriteImage(0x1f, 0x7f, 3);
                PlaceSprite(0x1f, 0x1f, 3, 0x28, 0xd5);
                // 臨海コロシアムの道具屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\223\271\213\357\211"
                    "\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202"
                    "\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x7f, 2);
                if (g_party.field.pos.y == 9) {
                    PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd5);
                    // 臨海コロシアムの武器屋でない場合は連絡して下さい。Takubo
                    DebugTrace(
                        "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225\220\212\355"
                        "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202"
                        "\265\202\304\211\272\202\263\202\242\201BTakubo\012"
                    );
                } else {
                    PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xe1);
                    // 臨海コロシアムの防具屋でない場合は連絡して下さい。Takubo
                    DebugTrace(
                        "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\226h\213\357\211"
                        "\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                        "\202\304\211\272\202\263\202\242\201BTakubo\012"
                    );
                }
            }
        } else if (entry == 0x110 || entry == 0x111) {
            LoadSpriteImage(0x1f, 0x2b, 2);
            PlaceSprite(0x1f, 0x1f, 2, 0x28, 0xd4);
            // 初台・マイシティ・御茶ノ水・ミレニアム病院の病院でない場合は連絡して下さい。Takubo
            DebugTrace(
                "\012\217\211\221\344\201E\203}"
                "\203C\203V\203e\203B\201E\214\344\222\203\203m\220\205\201E\203~"
                "\203\214\203j\203A\203\200\225a\211@\202\314\225a\211@"
                "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304\211"
                "\272\202\263\202\242\201BTakubo\012"
            );
        } else if (entry == 0x115) {
            if (g_party.field.pos.area == MAP_AREA_AMEYA_PLAZA) {
                LoadSpriteImage(0x1f, 0x2c, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xf0);
                // アメ屋プラザの病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203A\203\201\211\256\203v\203\211\203U\202\314\225a\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_party.field.pos.area == MAP_AREA_RINKAI_COLISEUM) {
                LoadSpriteImage(0x1f, 0x76, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xf0);
                // 臨海コロシアムの病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\227\325\212C\203R\203\215\203V\203A\203\200\202\314\225a\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else {
                LoadSpriteImage(0x1f, 0x78, 0);
                PlaceSprite(0x1f, 0x1f, 0, 0x28, 0xf0);
                // 神田地下街・銀座地下街の病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\220_"
                    "\223c\222n\211\272\212X\201E\213\342\215\300\222n\211\272\212X\202\314\225a"
                    "\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            }
        }
    }
    LoadCachedScriptFile(file, entry);
}

RVA(0x000345a0, 0x10)
void RequestQuit(void) {
    g_quitRequest = 1;
}

RVA(0x000345b0, 0x2c)
void OpChangeHp(i16 sign) {
    Character* character = ReadScriptObject();
    ChangePool(GetCharacterHpPool(character), ReadScriptValue() * sign);
    RequestFieldRefresh();
}

RVA(0x000345e0, 0x2c)
void OpChangeMp(i16 sign) {
    Character* character = ReadScriptObject();
    ChangePool(GetCharacterMpPool(character), ReadScriptValue() * sign);
    RequestFieldRefresh();
}

GZ_ENUM_BEGIN_SPLIT(ScriptPoolBoostMode, i16)
    SCRIPT_POOL_BOOST_NORMAL = 0,
    SCRIPT_POOL_BOOST_DOUBLE_MAX = 1,
    SCRIPT_POOL_BOOST_SIGNED_MAX = 2
GZ_ENUM_END_SPLIT(ScriptPoolBoostMode)

// Retail uses the MP pair as the input even when writing the HP result.
RVA(0x00034610, 0x6b)
void OpBoostPool(void) {
    Character* character = ReadScriptObject();
    i16 which = ReadScriptValue();
    i16 amount = ReadScriptValue();
    GZ_ENUM_LOCAL(ScriptPoolBoostMode, i16) mode = ReadScriptValue();
    i16 limit;
    i16 current;
    if (which == 0) {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    } else {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    }
    if (mode == SCRIPT_POOL_BOOST_DOUBLE_MAX) {
        limit *= 2;
    } else if (mode == SCRIPT_POOL_BOOST_SIGNED_MAX) {
        limit = 0x7fff;
    }
    current = AddClampShort(current, amount, 0, limit);
    if (which == 0) {
        character->pools.hp.cur = current;
    } else {
        character->pools.mp.cur = current;
    }
}

RVA(0x00034680, 0x10)
i16 ReadBranchTarget(void) {
    return ReadJumpTarget();
}

RVA(0x00034690, 0x1c)
i16 CompareInt(i32 a, i32 b) {
    if (a < b) {
        return -1;
    }
    return a > b;
}

// Applies a comparison operator to CompareInt(a, b): 0 !=, 1 ==, 2 <=, 3 >=,
// 4 <, 5 >; 1 when it holds, 0 otherwise (also for an unknown operator).
RVA(0x000346b0, 0x8c)
b32 CompareByOp(ComparisonOperator op, i32 a, i32 b) {
    i16 order = CompareInt(a, b);
    b32 holds = false;
    switch (op) {
        case COMPARE_EQUAL:
            if (order == 0) {
                holds = true;
            }
            break;
        case COMPARE_NOT_EQUAL:
            if (order != 0) {
                holds = true;
            }
            break;
        case COMPARE_LESS_EQUAL:
            if (order <= 0) {
                holds = true;
            }
            break;
        case COMPARE_GREATER_EQUAL:
            if (order >= 0) {
                holds = true;
            }
            break;
        case COMPARE_LESS:
            if (order < 0) {
                holds = true;
            }
            break;
        case COMPARE_GREATER:
            if (order > 0) {
                holds = true;
            }
            break;
    }
    return holds;
}

RVA(0x00034740, 0x3c)
void OpJumpUnlessCompare(ComparisonOperator op, i32 withRhs) {
    i16 target = ReadBranchTarget();
    i32 lhs = ReadScriptValue();
    i32 rhs = 0;
    if (withRhs == 1) {
        rhs = ReadScriptValue();
    }
    ScriptJumpUnless(target, CompareByOp(op, lhs, rhs));
}

RVA(0x00034780, 0xc8)
b32 OpApplyEventFlag(ScriptFlagAction action, i32 expect) {
    u16 bank;
    u16 index;
    i32 result;
    ReadFlagOperand(&bank, &index);
    result = 0;
    switch (action) {
        case SCRIPT_FLAG_TEST:
            result = TestEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_SET:
            result = SetEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_CLEAR:
            result = ClearEventFlag(bank, index);
            break;
        case SCRIPT_FLAG_TOGGLE:
            result = ToggleEventFlag(bank, index);
            break;
    }
    return result == expect;
}

RVA(0x00034850, 0x26)
void OpJumpUnlessEventFlag(ScriptFlagAction action, i32 expect) {
    i16 target = ReadBranchTarget();
    ScriptJumpUnless(target, OpApplyEventFlag(action, expect));
}

RVA(0x00034880, 0x27)
void OpJumpUnlessFlagSet(void) {
    i16 target = ReadBranchTarget();
    b32 matches = false;
    if (ReadAndMatchEventFlag()) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

#define RollFixedContestValue(value, level)                                                        \
    do {                                                                                           \
        switch (level) {                                                                           \
            case STAT_CONTEST_LEVEL_0:                                                             \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_1:                                                             \
                (value) = RandomAverage(5, 15, 0);                                                 \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_2:                                                             \
                (value) = RandomAverage(12, 22, 0);                                                \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_3:                                                             \
                (value) = RandomAverage(20, 40, 0);                                                \
                break;                                                                             \
        }                                                                                          \
    } while (0)

#define RollRelativeContestValue(value, level)                                                     \
    do {                                                                                           \
        switch (level) {                                                                           \
            case STAT_CONTEST_LEVEL_0:                                                             \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_1:                                                             \
                (value) = RandomPercent((value), -20, 20);                                         \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_2:                                                             \
                (value) = RandomPercent((value), 0, 30);                                           \
                break;                                                                             \
            case STAT_CONTEST_LEVEL_3:                                                             \
                (value) = RandomPercent((value), 10, 40);                                          \
                break;                                                                             \
        }                                                                                          \
    } while (0)

// Jumps unless the actor wins a contest of `stat` (the next operand) against
// the target, or with `invert` unless it loses: the target's side is its own
// value, a random spread around it, or a fixed random range chosen by the
// stat and the contest `level` (0..3); `swap` exchanges the sides.
RVA(0x000348b0, 0x520)
void OpJumpUnlessStatContest(
    GZ_ENUM_PARAM(StatContestLevel, i16) level,
    GZ_ENUM_PARAM(ScriptTestPolarity, i16) invert,
    b16 swap
) {
    i16 target = ReadBranchTarget();
    i16 stat = ReadScriptValue();
    i32 own;
    i32 other;
    i16 order;
    i32 won;

    ReadContestValues(stat, &own, &other, swap);
    won = false;
    switch (stat) {
        case STAT_INTUITION:
            RollFixedContestValue(other, level);
            break;
        case STAT_MENTAL_STRENGTH:
            RollFixedContestValue(other, level);
            break;
        case STAT_MAGIC:
            RollRelativeContestValue(other, level);
            break;
        case STAT_INTELLIGENCE:
            switch (level) {
                case STAT_CONTEST_LEVEL_0:
                    break;
                case STAT_CONTEST_LEVEL_1:
                    other = RandomPercent(other, -20, 20);
                    break;
                case STAT_CONTEST_LEVEL_2:
                    other = RandomPercent(other, 10, 30);
                    break;
                case STAT_CONTEST_LEVEL_3:
                    other = RandomPercent(other, 10, 40);
                    break;
            }
            break;
        case STAT_PROTECTION:
            RollFixedContestValue(other, level);
            break;
        case STAT_STRENGTH:
            switch (level) {
                case STAT_CONTEST_LEVEL_0:
                case STAT_CONTEST_LEVEL_1:
                case STAT_CONTEST_LEVEL_2:
                case STAT_CONTEST_LEVEL_3:
                    other = RandomAverage(0, 40, 2);
                    break;
            }
            break;
        case STAT_VITALITY:
            RollRelativeContestValue(other, level);
            break;
        case STAT_AGILITY:
            RollRelativeContestValue(other, level);
            break;
        case STAT_DEXTERITY:
            RollFixedContestValue(other, level);
            break;
        case STAT_CHARM:
            RollFixedContestValue(other, level);
            break;
        case STAT_FORTUNE:
            RollFixedContestValue(other, level);
            break;
        case CONTEST_LEVEL:
            switch (level) {
                case STAT_CONTEST_LEVEL_0: {
                    i32 ownProtection;
                    i32 otherProtection;

                    ReadContestValues(STAT_PROTECTION, &ownProtection, &otherProtection, swap);
                    other = -sqrt(otherProtection);
                    break;
                }
                case STAT_CONTEST_LEVEL_1:
                    other = RandomPercent(other, 10, 25);
                    break;
                case STAT_CONTEST_LEVEL_2:
                    other = RandomPercent(other, 25, 50);
                    break;
                case STAT_CONTEST_LEVEL_3:
                    other = RandomPercent(other, -20, 20);
                    break;
            }
            break;
        case CONTEST_LEVEL_GAP:
            switch (level) {
                case STAT_CONTEST_LEVEL_0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case STAT_CONTEST_LEVEL_1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case STAT_CONTEST_LEVEL_2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case STAT_CONTEST_LEVEL_3:
                    other = RandomAverage(10, 17, 0);
                    break;
            }
            break;
        case CONTEST_FAMILIARITY:
            switch (level) {
                case STAT_CONTEST_LEVEL_0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case STAT_CONTEST_LEVEL_1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case STAT_CONTEST_LEVEL_2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case STAT_CONTEST_LEVEL_3:
                    other = RandomAverage(11, 18, 0);
                    break;
            }
            break;
        case CONTEST_FAMILIARITY_COUNT:
            switch (level) {
                case STAT_CONTEST_LEVEL_0:
                    other = RandomAverage(35, 70, 0);
                    break;
                case STAT_CONTEST_LEVEL_1:
                    other = RandomAverage(60, 91, 0);
                    break;
                case STAT_CONTEST_LEVEL_2:
                    other = RandomAverage(85, 116, 0);
                    break;
                case STAT_CONTEST_LEVEL_3:
                    other = RandomAverage(120, 135, 0);
                    break;
            }
            break;
    }
    order = CompareInt(own, other);
    if (!invert && order >= 0) {
        won = true;
    }
    if (invert && order < 0) {
        won = true;
    }
    ScriptJumpUnless(target, won);
}

// Jumps unless the party is in the script actor's sight (with `invert`,
// unless it is not).
RVA(0x00034dd0, 0x81)
void OpJumpUnlessPlayerInView(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 seen;
    BuildSightGrid(
        ((FieldActor*)g_curScript->actor)->pos.x,
        ((FieldActor*)g_curScript->actor)->pos.y,
        ((FieldActor*)g_curScript->actor)->direction
    );
    seen = IsPartyInSight(
        ((FieldActor*)g_curScript->actor)->pos.x,
        ((FieldActor*)g_curScript->actor)->pos.y
    );
    if ((seen == true && invert == false) || (seen == false && invert == true)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Compares (operator `op`) a roll of 0..100 with the actor's HP percentage
// plus its byte +0x1f0 and a roll of 0..15.
// @identity-TODO: what the byte at +0x1f0 adds is unrecovered.
RVA(0x00034e60, 0x7f)
void OpJumpUnlessHpPercentRoll(ComparisonOperator op) {
    i16 target = ReadBranchTarget();
    Character* actor = g_curScript->actor;
    i32 roll = RandomAverage(0, 100, 0);
    i32 value = actor->pools.hp.cur * 100 / actor->pools.hp.max + actor->hpRollBonus;
    value += RandomAverage(0, 15, 0);
    ScriptJumpUnless(target, CompareByOp(op, roll, value));
}

// Compares (operator `op`) the actor's HP with a quarter of its maximum plus
// a roll up to that quarter.
RVA(0x00034ee0, 0x56)
void OpJumpUnlessHpQuarterRoll(ComparisonOperator op) {
    i16 target = ReadBranchTarget();
    Character* actor = g_curScript->actor;
    i32 quarter = actor->pools.hp.max;
    i32 hp = actor->pools.hp.cur;
    quarter >>= 2;
    quarter += RandomAverage(0, quarter, 0);
    ScriptJumpUnless(target, CompareByOp(op, hp, quarter));
}

// Jumps unless the party stands within 4 cells in front of the actor.
RVA(0x00034f40, 0xab)
void OpJumpUnlessPlayerNearFront(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    MapCoord coord = GetMapCoord();
    if (GridDistance(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y
        )
        > 4) {
        if (invert != false) {
            jump = true;
        }
    } else {
        i16 side = RelativeDirection(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y,
            ((FieldActor*)g_curScript->actor)->direction
        );
        if ((side == 0 && invert == false) || (side != 0 && invert == true)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party is at the actor's trigger range (always, unless
// inverted, while the field marker is set).
RVA(0x00034ff0, 0x7a)
void OpJumpUnlessPlayerAtRange(i16 invert) {
    b32 jump = false;
    i16 range = g_curScript->actor->triggerRange;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == false;
    } else {
        i16 distance = DistanceToParty((FieldActor*)g_curScript->actor);
        if ((range != distance && invert) || (range == distance && !invert)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035070, 0x7b)
void OpJumpUnlessActorVisible(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == false;
    } else {
        b16 view = GetPartyView(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y
        );
        if ((invert == false && view) || (invert == true && !view)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000350f0, 0x44)
void OpJumpUnlessInRoster(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 slot = RosterSlotOfId(ReadObjectId());
    if ((slot >= 0 && !invert) || (slot < 0 && invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the roster holds the roster capacity less 6 entries or more.
RVA(0x00035140, 0x4c)
void OpJumpUnlessRosterFull(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 count = CountRosterEntries(true);
    i16 limit = GetRosterCapacity() - 6;
    if ((count >= limit && !invert) || (count < limit && invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object's alignment agrees with the leader's.
RVA(0x00035190, 0x44)
void OpJumpUnlessAlignmentMatch(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 conflict = AlignmentConflicts(ReadScriptObject());
    if (ScriptBooleanMatches(!conflict, invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party's macca covers the object's rank score.
RVA(0x000351e0, 0x5b)
void OpJumpUnlessCanAfford(i16 invert) {
    i32 price = 0x7fffffff;
    b32 jump = false;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (object) {
        price = GetRankScore(object);
    }
    price -= GetObjectMacca(SCRIPT_REF_SLOT_BASE);
    if ((price <= 0 && !invert) || (price > 0 && invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035240, 0x44)
void OpJumpUnlessInParty(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 position = FindPartyPositionOfId(ReadObjectId());
    if ((position >= 0 && !invert) || (position < 0 && invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035290, 0x3f)
void OpJumpUnlessRosterHasNoDemons(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(false);
    if (ScriptBooleanMatches(!demons, invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has no condition (no object counts as healthy
// only when inverted).
RVA(0x000352d0, 0x62)
void OpJumpUnlessHealthy(i16 invert) {
    i16 conditions = 0;
    b32 jump = false;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (!object && invert) {
        jump = true;
    } else {
        if (object) {
            AccumulateConditionBits(GetCharacterConditions(object), conditions);
        }
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

static __inline Character* GetResolvedPartyCharacter(i16 id) {
    return GetRosterCharacterById(ResolveObjectId(id), 1);
}

// The first of the companions in script object slots 1, 2 and 6.
RVA(0x00035340, 0xb5)
void OpJumpUnlessCompanionHealthy(i16 invert) {
    i16 conditions = 0;
    b32 jump = false;
    i16 target = ReadBranchTarget();
    Character* companion = GetResolvedPartyCharacter(ScriptObjectRefFromSlot(1));
    if (!companion) {
        companion = GetResolvedPartyCharacter(ScriptObjectRefFromSlot(2));
    }
    if (!companion) {
        companion = GetResolvedPartyCharacter(ScriptObjectRefFromSlot(6));
    }
    if (!companion && invert) {
        jump = true;
    } else if (!companion) {
        jump = false;
    } else {
        AccumulateConditionBits(GetCharacterConditions(companion), conditions);
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the player has an item in equipment slot 6.
RVA(0x00035400, 0x4d)
void OpJumpUnlessHeroEquipped(i16 invert) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    Character* player = ResolveScriptObject(SCRIPT_REF_SLOT_BASE);
    if ((GetCharacterEquipment(player)[EQUIP_SLOT_GUN].item != ITEM_ID_EMPTY && !invert)
        || (GetCharacterEquipment(player)[EQUIP_SLOT_GUN].item == ITEM_ID_EMPTY && invert)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the script has no actor.
// @identity-TODO: the state is 3 with an actor and 0 without; what the
// values 0..2 against which it is tested stood for is unrecovered.
RVA(0x00035450, 0x4a)
void OpIfNoActor(i16 negate) {
    i16 state = 0;
    b32 jump = false;
    i16 target = ReadBranchTarget();
    if (GetScriptActor()) {
        state = 3;
    }
    if ((state < 2 && !negate) || (state > 2 && negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party faces the operand's direction.
RVA(0x000354a0, 0x45)
void OpIfFacing(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if ((direction == g_party.field.pos.direction && !negate)
        || (direction != g_party.field.pos.direction && negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// The same against the direction saved with the return position (with none
// saved, only when negated).
RVA(0x000354f0, 0x64)
void OpIfReturnFacing(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if (g_party.savedDirection == -1) {
        if (negate) {
            jump = true;
        }
    } else if ((direction == g_party.savedDirection && !negate)
               || (direction != g_party.savedDirection && negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has condition `condition` (no object counts as
// lacking it).
RVA(0x00035560, 0x63)
void OpIfObjectHasCondition(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    i16 has = ReadScriptValue();
    if (!object && negate) {
        jump = true;
    } else {
        if (object) {
            has = HasCondition(GetCharacterConditions(object), has);
        }
        if (ScriptBooleanMatches(has, negate)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds the item.
RVA(0x000355d0, 0x44)
void OpIfHasItem(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 held = CountHeldItem(ReadScriptValue());
    if (ScriptBooleanMatches(held, negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds every item of the -1-terminated list (with
// `negate`, unless it holds none of them).
RVA(0x00035620, 0x67)
void OpIfHasAllItems(i16 negate) {
    i16 all = -1;
    i16 any = 0;
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 item;
    for (item = ReadScriptValue(); item != ITEM_ID_EMPTY; item = ReadScriptValue()) {
        i16 held = CountHeldItem(item) ? -1 : 0;
        all &= held;
        any |= held;
    }
    if ((!negate && all) || (negate && !any)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

// Puts one of the item into the bag (without the full-bag prompt).
RVA(0x00035690, 0x33)
void OpGiveItem(void) {
    i16 item = ReadScriptValue();
    if (item > 0) {
        i16 quiet = SetBagQuiet(0);
        StoreBagItem(item, 1, GEM_ITEM_INDEX_NONE);
        SetBagQuiet(quiet);
    }
}

RVA(0x000356d0, 0x16)
void OpTakeItem(void) {
    i16 item = ReadScriptValue();
    if (item > 0) {
        TakeBagItems(item, 1);
    }
}

RVA(0x000356f0, 0x55)
void OpOpenItemListWindow(void) {
    i16 totalVar = ReadScriptValue();
    i16 selling = ReadScriptValue();
    ScriptPanel* node = CreateScriptPanel(IDB_BITMAP56, 8, 0, 0);
    i16 i;
    node->panel->flags |= PANEL_ALLOW_RIGHT_CLICK;
    node->panel->flags &= ~PANEL_IGNORE_RIGHT_CLICK;
    OpenScriptItemMenu(totalVar, selling);
    for (i = 0; i < 8; i++) {
        SetLastPanelRowState(i, PANEL_HANDLER_LOCKED);
    }
}

RVA(0x00035750, 0x12)
void OpCloseItemListWindow(void) {
    CloseScriptPanelByImage(0x115);
    CloseItemMenu();
}

RVA(0x00035770, 0x5)
void OpRedrawItemListTotal(void) {
    RefreshScriptItemMenuTotal();
}

RVA(0x00035780, 0x3b)
void OpIfPoolHasItems(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 count = CountPoolEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000357c0, 0x3b)
void OpIfBagHasEntries(i16 negate) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 count = CountBagEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035800, 0x26)
void OpGetItemPrice(void) {
    i16 item = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetItemPrice(item));
}

// Saves (0) or restores the kind-9 item table and the bag through the
// script variables from the operand on (capped at 176): the table at the
// operand, the bag 16 words on. Saving also resets the table and empties
// the bag.
RVA(0x00035830, 0x7d)
void OpStashItemLists(void) {
    GZ_ENUM_STORAGE(ItemStashAction, i16) action = ReadScriptValue();
    i16 var = ReadScriptValue();
    var = min(var, 0xb0);
    if (action == ITEM_STASH_SAVE) {
        SaveGemItems((ItemStack*)&g_scriptVars[var]);
        ResetGemItems(GetGemItemBase());
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], ITEM_STASH_SAVE);
        ClearBag();
    } else {
        RestoreGemItems((ItemStack*)&g_scriptVars[var]);
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], ITEM_STASH_RESTORE);
    }
}

// Stores (a positive count) or takes (a negative one) the item quietly and
// sets a long variable to how many were moved (never below 0 for a store).
RVA(0x000358b0, 0x6d)
void OpAdjustItemCount(void) {
    i16 item = ReadScriptValue();
    i16 count = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    i16 quiet = SetBagQuiet(1);
    i16 moved;
    if (count < 0) {
        moved = count - TakeBagItems(item, count);
    } else {
        moved = count - StoreBagItem(item, count, GEM_ITEM_INDEX_NONE);
    }
    if (count > 0 && moved < 0) {
        moved = 0;
    }
    SetBagQuiet(quiet);
    SetScriptLongVar(index, moved);
}

// Lists the bag entries holding items of `category` (1..19 selects an item
// kind; the other selectors choose all, non-scenario, priced non-scenario or
// zero-price items)
// into a new array handle, with `spare` extra entries; stores the handle and
// the count.
RVA(0x00035920, 0x11a)
void OpListBagByCategory(void) {
    i16 listVar = ReadLongVarIndex();
    i16 countVar = ReadLongVarIndex();
    GZ_ENUM_LOCAL(ScriptBagCategory, i16) category = ReadScriptValue();
    i16 spare = ReadScriptValue();
    // Retail's frame holds more than the bag's 64 entries (65 words fit).
    i16 entries[65];
    i16 count = 0;
    i16 i;
    i32 handle;
    i32* list;
    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        i16 item = GetBagItem(i);
        if (item < 1) {
            continue;
        }
        if (category >= SCRIPT_BAG_CATEGORY_ALL && category <= ITEM_KIND_ACCESSORY) {
            if (category != SCRIPT_BAG_CATEGORY_ALL && GetItemKind(item) != category) {
                continue;
            }
        } else {
            if ((category == SCRIPT_BAG_CATEGORY_NON_SCENARIO
                 || category == SCRIPT_BAG_CATEGORY_PRICED_NON_SCENARIO)
                && GetItemKind(item) == ITEM_KIND_SCENARIO) {
                continue;
            }
            if (category == SCRIPT_BAG_CATEGORY_PRICED_NON_SCENARIO && GetItemPrice(item) == 0) {
                continue;
            }
            if (category == SCRIPT_BAG_CATEGORY_ZERO_PRICE && GetItemPrice(item) != 0) {
                continue;
            }
        }
        entries[count++] = i;
    }
    count += spare;
    handle = CreateArrayHandle(count + spare, sizeof(*list));
    SetScriptLongVar(listVar, handle);
    SetScriptLongVar(countVar, count);
    list = HandleWritePtr(handle);
    for (i = 0; i < count; i++) {
        list[i] = entries[i];
    }
}

// Stores a whole bag entry (ItemStack) in a long variable.
RVA(0x00035a40, 0x28)
void OpGetBagEntry(void) {
    i16 index = ReadScriptValue();
    i16 var = ReadLongVarIndex();
    SetScriptLongVar(var, GetBagEntry(index)->value);
}

RVA(0x00035a70, 0x1a)
void OpClearBagEntry(void) {
    ItemStack* entry = GetBagEntry(ReadScriptValue());
    entry->count = 0;
    entry->hasAttachment = false;
    entry->item = ITEM_ID_EMPTY;
    entry->attachment = 0;
}

RVA(0x00035a90, 0x88)
void OpTakeDropSlot(void) {
    i16 slot = ReadScriptValue();
    i16 itemVar = ReadLongVarIndex();
    i16 amountVar = ReadLongVarIndex();
    i16 item = GetDropSlot(slot)->item;
    i16 count = GetDropSlot(slot)->amount;
    i16 remapped;
    i16 amount;
    ClearDropSlot(slot);
    remapped = RemapItem(item);
    if (remapped) {
        amount = RollDropAmount(item, count);
    } else {
        remapped = item;
        amount = count;
    }
    SetScriptLongVar(itemVar, remapped);
    SetScriptLongVar(amountVar, amount);
}

RVA(0x00035b20, 0xf)
GZ_ENUM_RETURN(ScriptStatus, i16) OpCallSubScene(void) {
    PushGameState(GAME_STATE_GEM_ITEM_GIFT);
    return SCRIPT_YIELD;
}

// Stores in a long variable a handle to a copy of the item's decoded record
// (without its name and description pointers).
RVA(0x00035b30, 0x52)
void OpCopyItemRecord(void) {
    i16 index = ReadLongVarIndex();
    ItemRecord* record = GetLoadedRecord(ReadScriptValue());
    i32 handle = AllocHandle(sizeof(ItemRecord));
    ItemRecord* copy = HandleWritePtr(handle);
    *copy = *record;
    copy->name = NULL;
    copy->description = NULL;
    SetScriptLongVar(index, handle);
}

RVA(0x00035b90, 0x14)
void OpOpenFusionScreen(GZ_ENUM_PARAM(FusionMenuStep, i16) kind) {
    PushFusionMenu(kind, ReadLongVarIndex());
}

RVA(0x00035bb0, 0x1c)
void OpRunFusion(b16 triple) {
    SetBlankStep(BLANK_STEP_FIRST);
    if (!triple) {
        RunPairFusion();
    } else {
        RunTripleFusion();
    }
}

RVA(0x00035bd0, 0x5)
void OpEndFusion(void) {
    EndFusion();
}

// Jumps unless `cond` is zero.
RVA(0x00035be0, 0x20)
void OpJumpIf(b16 cond) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    if (!cond) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035c00, 0x11)
void OpSkipJumpTarget(i16 unused) {
    i16 target = ReadBranchTarget();
    ScriptJumpUnless(target, true);
}

// Jumps unless the roster's demon count is above `limit` (mode 0) or at most
// `limit` (mode 1).
RVA(0x00035c20, 0x45)
void OpIfDemonCount(GZ_ENUM_PARAM(ScriptTestPolarity, i16) mode, i16 limit) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(false);
    if ((mode == SCRIPT_TEST_NORMAL && demons > limit)
        || (mode == SCRIPT_TEST_INVERTED && demons <= limit)) {
        jump = true;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035c70, 0x1c)
void OpGetFusionResult(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, GetFusionResultKind());
}

RVA(0x00035c90, 0x2c)
void OpAddMagnetite(i16 sign) {
    AddMagnetite(ResolveScriptObject(SCRIPT_REF_SLOT_BASE), ReadScriptValue() * sign);
    DrawMoneyCounters(1);
}

RVA(0x00035cc0, 0x2c)
void OpAddMacca(i16 sign) {
    AddMacca(ResolveScriptObject(SCRIPT_REF_SLOT_BASE), ReadScriptValue() * sign);
    DrawMoneyCounters(1);
}

// Queues the 0xff-terminated operand bytes as automatic moves and holds the
// scene. At area 0x82 level 5, cell 4/9 facing 3, a first move of 3 becomes 1.
// @identity-TODO: why that one spot's first move is rewritten is unrecovered.
RVA(0x00035cf0, 0xb8)
void OpQueueAutoMoves(void) {
    u16 pc = g_curScript->pc;
    i16 count = 0;
    i32 i;
    i16 move;
    while (ReadScriptByte() != 0xff) {
        count++;
    }
    GrowAutoMoves(count);
    g_curScript->pc = pc;
    i = 0;
    move = ReadScriptByte();
    while (move != 0xff) {
        if (i == 0 && g_party.field.pos.area == MAP_AREA_HATSUDAI && g_party.field.pos.level == 5
            && g_party.field.pos.x == 4 && g_party.field.pos.y == 9
            && g_party.field.pos.direction == VIEW_WEST && move == 3) {
            move = 1;
        }
        PushAutoMove(move);
        i++;
        move = ReadScriptByte();
    }
    ExchangeSceneHold(1);
}

// Sets the return point (area, level, x, y, direction) the field leaves to.
// @identity-TODO: the operand order is read from SetReturnPoint's stores.
RVA(0x00035db0, 0x40)
void OpChangeMap(void) {
    i16 area = ReadScriptValue();
    i16 level = ReadScriptValue();
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    SetReturnPoint(area, level, x, y, ReadScriptValue());
    g_worldMapRequest = WORLD_MAP_REQUEST_EXIT;
}

// Sets the world-map layer and spot the world map opens at.
RVA(0x00035df0, 0x2c)
void OpSetWorldMapSpot(void) {
    i16 layer = ReadScriptValue();
    i16 x = ReadScriptValue();
    SetWorldMapSpot(layer, x, ReadScriptValue());
    g_worldMapRequest = WORLD_MAP_REQUEST_SAVED_SPOT;
}

// Adds a world-map route point: layer `layer`'s origin plus an x/y offset
// (layer 0 drops the route instead).
RVA(0x00035e20, 0x4c)
void OpAddRoutePoint(void) {
    MapCoord point;
    i16 layer = ReadScriptValue();
    if (!layer) {
        ReadScriptValue();
        ReadScriptValue();
        FreeRoute();
        return;
    }
    point = GetLayerOrigin(layer);
    point.x += ReadScriptValue();
    point.y += ReadScriptValue();
    PushRoutePoint(point);
}

// Stores the party's area, level, x, y and direction in five long variables.
RVA(0x00035e70, 0x75)
void OpGetPlayerLocation(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_party.field.pos.area);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_party.field.pos.level);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_party.field.pos.x);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_party.field.pos.y);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_party.field.pos.direction);
}

RVA(0x00035ef0, 0x23)
void OpSetPlayerPosition(void) {
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    MovePartyTo(x, y, ReadScriptValue());
}

RVA(0x00035f20, 0xe8)
void OpIfBlockedToward(
    GZ_ENUM_PARAM(ScriptTestPolarity, i16) negate,
    GZ_ENUM_PARAM(MoveCommand, i16) turn
) {
    b32 matches = false;
    i16 target = ReadBranchTarget();
    i16 x = g_party.field.pos.x;
    i16 y = g_party.field.pos.y;
    i16 direction = g_party.field.pos.direction;
    i16 blocked;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        blocked = 1;
    } else {
        blocked = GetMapWallKind(x, y, (direction + turn) & 3);
    }
    if (!blocked) {
        StepMapCoord(&x, &y, direction, turn);
        blocked = IsCellBlocked(g_party.field.pos.level, CELL_SCAN_TEST, x, y);
        if (!blocked && g_curScript->actor != NULL) {
            blocked = DistanceToParty((FieldActor*)g_curScript->actor) == 0;
        }
    }
    if ((!blocked && !negate) || (blocked && negate)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

// Runs move command `effect` as a screen transition and refreshes the field.
RVA(0x00036010, 0x19)
GZ_ENUM_RETURN(ScriptStatus, i16) PlayScreenTransition(i16 effect) {
    RunMoveCommand(effect, 0);
    RequestFieldRefresh();
    return SCRIPT_YIELD;
}

// Plays a screen transition, then redraws the field screen in one long frame
// with the status redraw locked.
RVA(0x00036030, 0x41)
GZ_ENUM_RETURN(ScriptStatus, i16) OpScreenTransition(void) {
    i16 result = PlayScreenTransition(ReadScriptValue());
    i16 lock = LockStatusRedraw(true);
    UpdateFieldScreen(false);
    SetLongFrame(1);
    LockStatusRedraw(lock);
    return result;
}

static __inline void LoadScriptCharacterToRoster(i16 id) {
    Character* character = LoadCharacterCore(id, NULL);
    SetAnalyzed(character->id, 1);
    AddScriptCharacterToRoster(character, 3);
}

RVA(0x00036080, 0xd7)
GZ_ENUM_RETURN(ScriptStatus, i16) OpAddToRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    Character* character;
    if (ref >= SCRIPT_REF_CHARACTER_BASE) {
        id = ref - SCRIPT_REF_CHARACTER_BASE;
    }
    if (ref == SCRIPT_REF_ACTOR || ref == SCRIPT_REF_ACTOR_ALIAS) {
        id = GetScriptActorId();
        if (id != CHARACTER_ID_NONE) {
            DespawnScriptActor();
            LoadScriptCharacterToRoster(id);
        }
        return SCRIPT_END;
    }
    if (id >= 0) {
        LoadScriptCharacterToRoster(id);
        return SCRIPT_CONTINUE;
    }
    character = GetCharacter(ObjectSlotOfId(ref));
    if (RosterSlotOfId(character->id) == ROSTER_SLOT_NONE) {
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    return SCRIPT_CONTINUE;
}

RVA(0x00036160, 0x50)
void AddScriptCharacterToRoster(Character* character, i16 unused) {
    if (AddToRoster(character) < 0) {
        g_rosterPendingMember = character;
        SaveRosterReturnState();
        SetGameState(GAME_STATE_REPLACE_ROSTER_MEMBER);
        SetGamePhase(ROSTER_REPLACEMENT_OPEN_LIST);
    }
}

RVA(0x000361b0, 0x97)
i16 OpRemoveFromRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (id >= SCRIPT_REF_CHARACTER_BASE) {
        id = ref - SCRIPT_REF_CHARACTER_BASE;
    } else if (id >= SCRIPT_REF_ROSTER_BASE) {
        RemoveFromRoster(id - SCRIPT_REF_ROSTER_BASE);
        return 0;
    } else if (id >= SCRIPT_REF_PARTY_BASE) {
        i16 slot = GetPartySlot(id - SCRIPT_REF_PARTY_BASE);
        if (slot != ROSTER_SLOT_NONE) {
            RemoveFromRoster(slot);
        }
        return 0;
    } else {
        if (id == SCRIPT_REF_ACTOR || id == SCRIPT_REF_ACTOR_ALIAS) {
            return -1;
        }
        if (id == SCRIPT_REF_ACTOR_BY_ID) {
            id = GetScriptActorId();
            if (id == CHARACTER_ID_NONE) {
                return -1;
            }
        }
        if (id < 0) {
            id = ResolveObjectId(id);
        }
    }
    RemoveFromRoster(RosterSlotOfId(id));
    return 0;
}

RVA(0x00036250, 0xdd)
i16 OpJoinActiveParty(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    i16 slot;
    if (ref >= SCRIPT_REF_CHARACTER_BASE) {
        id = ref - SCRIPT_REF_CHARACTER_BASE;
    }
    if (id >= SCRIPT_REF_ROSTER_BASE) {
        id = GetRosterId(id - SCRIPT_REF_ROSTER_BASE);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == SCRIPT_REF_ACTOR || ref == SCRIPT_REF_ACTOR_ALIAS) {
        id = GetScriptActorId();
        if (id == CHARACTER_ID_NONE) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    if (id < HUMAN_ID_LIMIT && RosterSlotOfId(id) == ROSTER_SLOT_NONE) {
        Character* character = FindCharacterById(id);
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    slot = RosterSlotOfId(id);
    if (id >= 32 || FindPartyPositionOfId(id) == PARTY_POSITION_NONE) {
        AddToParty(slot);
        RequestFieldRefresh();
    }
    return 0;
}

RVA(0x00036330, 0xa0)
i16 OpLeaveActiveParty(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (ref >= SCRIPT_REF_CHARACTER_BASE) {
        id = ref - SCRIPT_REF_CHARACTER_BASE;
    }
    if (id >= SCRIPT_REF_ROSTER_BASE) {
        id = GetRosterId(id - SCRIPT_REF_ROSTER_BASE);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == SCRIPT_REF_ACTOR || ref == SCRIPT_REF_ACTOR_ALIAS) {
        id = GetScriptActorId();
        if (id == CHARACTER_ID_NONE) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    RemoveFromParty(FindRosterSlotById(id));
    if (id < HUMAN_ID_LIMIT) {
        RemoveFromRoster(RosterSlotOfId(id));
    }
    RequestFieldRefresh();
    return 0;
}

RVA(0x000363d0, 0x28)
b16 OpSelectPartySlot(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, PollPartySlotSelection(ReadScriptValue()));
    return false;
}

RVA(0x00036400, 0x9)
b16 OpEndPartySlotSelect(void) {
    ClearPartySlotSelection();
    return false;
}

RVA(0x00036410, 0x2c)
i16 OpCountActiveParty(void) {
    i16 index = ReadLongVarIndex();
    i16 count = CountPartyMembers(ReadScriptValue());
    SetScriptLongVar(index, count);
    return count;
}

RVA(0x00036440, 0x7b)
i16 OpGetCombatantId(void) {
    i16 index = ReadLongVarIndex();
    i16 id = ReadObjectRef();
    if (id == SCRIPT_REF_BATTLE_ACTOR) {
        id = g_actorId;
    } else if (id == SCRIPT_REF_BATTLE_TARGET) {
        id = g_targetId;
    } else {
        id = -1;
    }
    SetScriptLongVar(index, id);
    return id;
}

RVA(0x000364c0, 0x92)
void OpIfObjectIsAlly(i16 negate) {
    i16 target = ReadBranchTarget();
    i16 id = ReadObjectRef();
    b32 matches;
    if (id == SCRIPT_REF_BATTLE_ACTOR) {
        id = g_actorId;
    } else if (id == SCRIPT_REF_BATTLE_TARGET) {
        id = g_targetId;
    } else if (id == SCRIPT_REF_FAVOURED_MEMBER) {
        id = GetPartySlot(FindFavouredMember());
        if (id >= 0) {
            id = ScriptObjectRefFromSlot(id);
        }
    }
    matches = false;
    if ((id < 0 && !negate) || (id >= 0 && negate)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x00036560, 0x79)
void OpRebalanceMemberStats(void) {
    Character* character = GetRosterCharacter(ReadScriptValue());
    if (character) {
        i16 i;
        for (i = 0; i < STAT_COUNT; i++) {
            i16 sum = character->stats.bonus[i] + character->stats.modifiers[i]
                      + character->stats.equipment[i] + GetBaseStat(character, i);
            if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                sum /= 2;
            }
            character->stats.base[i] += GetStatTotal(character, i) - sum;
        }
        FullyRestoreCharacter(character);
    }
}

RVA(0x000365e0, 0x2f)
void OpAddMemberSkill(void) {
    i16 index = ReadScriptValue();
    i16 skill = ReadScriptValue();
    Character* character = GetRosterCharacter(index);
    if (character != NULL) {
        AddSkill(GetCharacterSkills(character), skill);
    }
}

RVA(0x00036610, 0x18)
void OpSwitchOnMoonPhase(i16 call) {
    SwitchOnValue(GetMoonPhase() + 1, call, false);
}

// @identity-TODO: what table 0x47beac (28 bytes per row, row = actor byte +0x1f8, column =
// phase) holds is unrecovered.
RVA(0x00036630, 0x38)
void OpGetActorMoonValue(void) {
    i16 index = ReadLongVarIndex();
    i16 half = GetMoonValue(g_curScript->actor->moonRow) / 2;
    SetScriptLongVar(index, half);
}

RVA(0x00036670, 0xf)
void OpAdvanceClock(void) {
    AdvanceClock(ReadScriptValue());
}

RVA(0x00036680, 0x2b)
void OpGetTicksUntilMoonPhase(void) {
    i16 phase = ReadScriptValue();
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, TimeUntilMoonPhase(phase));
}

RVA(0x000366b0, 0x16)
void OpGetDayCount(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_clock.days);
}

// Stores the time of day in minutes.
RVA(0x000366d0, 0x2c)
void OpGetTimeOfDay(void) {
    i16 index = ReadLongVarIndex();
    i16 minutes = g_clock.hour * 60 + g_clock.minute;
    SetScriptLongVar(index, minutes);
}

// @identity-TODO: the token kinds are named from what they read; which script
// escape selects each is unrecovered. `byId` makes kinds 1/2 take `id` as a
// character id instead of a script object id.
RVA(0x00036700, 0x218)
char* GetTextToken(GZ_ENUM_PARAM(TextTokenKind, i16) kind, b16 byId, i16 id) {
    const char* text = NULL;
    Character* object;
    s_tokenText[0] = '\0';
    switch (kind) {
        case TEXT_TOKEN_FULL_NAME:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            FormatFullName(s_tokenText, object);
            return s_tokenText;
        case TEXT_TOKEN_RACE_NAME:
        case TEXT_TOKEN_RACE_NAME_ALIAS:
            if (byId != true) {
                object = ResolveScriptObject(id);
                if (object != NULL) {
                    id = object->id;
                }
            }
            text = GetDemonRaceName(id);
            break;
        case TEXT_TOKEN_PANTHEON_NAME:
            if (byId != true) {
                object = ResolveScriptObject(id);
                if (object != NULL) {
                    id = object->id;
                }
            }
            text = GetDemonPantheonName(id);
            break;
        case TEXT_TOKEN_RECORD_NAME:
            text = GetLoadedRecordName(id);
            break;
        case TEXT_TOKEN_NAME_PREFIX:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = object->namePrefix;
            break;
        case TEXT_TOKEN_NONHUMAN_NAME_PREFIX:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            if (!IsHumanCharacter(object)) {
                text = object->namePrefix;
            } else {
                text = object->name;
            }
            break;
        case TEXT_TOKEN_BLOOD_TYPE:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_bloodTypes[object->bloodType];
            break;
        case TEXT_TOKEN_SIGN:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_signNames[object->sign];
            break;
        case TEXT_TOKEN_AFFILIATION:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_affiliationNames[GetCharacterAffiliation(object, 0)];
            break;
        case TEXT_TOKEN_STATUS_CONDITION:
            text = GetConditionName(g_statusCondition);
            break;
        case TEXT_TOKEN_RECORD_NAME_ALIAS:
            text = GetLoadedRecordName(id);
            break;
        case TEXT_TOKEN_DEMON_CLASS:
            text = GetDemonClassName(id);
            break;
        case TEXT_TOKEN_SKILL_NAME:
            text = GetSkillName(id);
            break;
        case TEXT_TOKEN_CONDITION_NAME:
            text = GetConditionName(id);
            break;
        case TEXT_TOKEN_STATUS_CONDITION_ALIAS:
            text = GetConditionName(g_statusCondition);
            break;
        case TEXT_TOKEN_EMPTY_5:
        case TEXT_TOKEN_EMPTY_6:
        case TEXT_TOKEN_EMPTY_13:
        case TEXT_TOKEN_EMPTY_14:
            break;
        default:
            return s_tokenText;
    }
    if (text != NULL) {
        strcpy(s_tokenText, text);
    }
    return s_tokenText;
}

// Codegen constraint: keep the separate switch arms. Grouping these cases
// changes the dispatch table and the shared ReadScriptValue tail.
RVA(0x00036920, 0x17c)
char* ReadTextToken(void) {
    i16 bypass = ExchangeObjectCheckBypass(1);
    i16 kind;
    i16 byId;
    i16 id;
    Character* object;
    char* text;
    s_tokenText[0] = '\0';
    kind = ReadScriptByte();
    id = 0;
    byId = false;
    switch (kind) {
        case TEXT_TOKEN_FULL_NAME:
            object = ReadScriptObject();
            if (object) {
                FormatFullName(s_tokenText, object);
            }
            ExchangeObjectCheckBypass(bypass);
            return s_tokenText;
        case TEXT_TOKEN_RACE_NAME:
            goto readIndexedToken;
        case TEXT_TOKEN_RACE_NAME_ALIAS:
            goto readIndexedToken;
        case TEXT_TOKEN_PANTHEON_NAME:
            goto readIndexedToken;
        case TEXT_TOKEN_NAME_PREFIX:
            goto readIndexedToken;
        case TEXT_TOKEN_NONHUMAN_NAME_PREFIX:
            goto readIndexedToken;
        case TEXT_TOKEN_BLOOD_TYPE:
            goto readIndexedToken;
        case TEXT_TOKEN_SIGN:
            goto readIndexedToken;
        case TEXT_TOKEN_AFFILIATION:
        readIndexedToken:
            byId = ReadScriptByte();
            id = ReadScriptValue();
            break;
        case TEXT_TOKEN_RECORD_NAME:
            id = ReadScriptValue();
            if (id == 0) {
                id = GetScriptLongVar(11);
            }
            break;
        case TEXT_TOKEN_RECORD_NAME_ALIAS:
            ReadScriptValue();
            id = g_actionId;
            object = GetCombatant(g_actorId);
            if (object && g_actorId < 0 && (object->pickFlags.itemSkill)) {
                id = object->pickItem;
            }
            break;
        case TEXT_TOKEN_DEMON_CLASS:
            id = ReadObjectId();
            break;
        case TEXT_TOKEN_SKILL_NAME:
            ReadScriptValue();
            id = g_actionId;
            object = GetCombatant(g_actorId);
            if (object && g_actorId < 0 && (object->pickFlags.itemSkill)) {
                id = object->pickItem;
            }
            break;
        case TEXT_TOKEN_CONDITION_NAME:
            id = GetFirstConditionIndex(ReadScriptObject());
            break;
        case TEXT_TOKEN_EMPTY_5:
            goto readTokenValue;
        case TEXT_TOKEN_EMPTY_6:
            goto readTokenValue;
        case TEXT_TOKEN_STATUS_CONDITION:
            goto readTokenValue;
        case TEXT_TOKEN_EMPTY_13:
            goto readTokenValue;
        case TEXT_TOKEN_EMPTY_14:
            goto readTokenValue;
        case TEXT_TOKEN_STATUS_CONDITION_ALIAS:
        readTokenValue:
            id = ReadScriptValue();
            break;
    }
    text = GetTextToken(kind, byId, id);
    ExchangeObjectCheckBypass(bypass);
    return text;
}

DATA(0x000911b4)
i32 g_rolledMacca;

DATA(0x000911b8)
i32 g_rolledMagnetite;

RVA(0x00036aa0, 0x23)
void OpRollActorMagnetite(void) {
    Character* actor = GetScriptActor();
    if (actor != NULL) {
        g_rolledMagnetite = RollCharacterMagnetite(actor);
    } else {
        g_rolledMagnetite = 0;
    }
}

RVA(0x00036ad0, 0x23)
void OpRollActorMacca(void) {
    Character* actor = GetScriptActor();
    if (actor != NULL) {
        g_rolledMacca = RollCharacterMacca(actor);
    } else {
        // Retail clears magnetite here, leaving the previous macca roll intact.
        g_rolledMagnetite = 0;
    }
}

// Reads an operand kind byte and its arguments: literals (0-5), a long
// variable (3), item prices (6, 7), macca and magnetite (9-12, 40, 41), object
// stats and fields (25-39, 43-48, 56-74, 80-93) and battle results (49,
// 75-79). Unknown kinds leave the slot unchanged.
RVA(0x00036b00, 0x8e0)
i32* ReadScriptOperand(void) {
    GZ_ENUM_LOCAL(ScriptOperandKind, i16) kind = ReadScriptByte();
    Character* object;
    i8 byteValue;
    i16 wordValue;

    switch (kind) {
        case SCRIPT_OPERAND_BYTE:
            s_operand = ReadScriptByte();
            return &s_operand;
        case SCRIPT_OPERAND_LONG:
            s_operand = ReadScriptLong();
            return &s_operand;
        case SCRIPT_OPERAND_LONG_VAR:
            s_operand = GetScriptLongVar(ReadScriptByte());
            return &s_operand;
        case SCRIPT_OPERAND_SIGNED_BYTE:
            byteValue = ReadScriptByte();
            s_operand = byteValue;
            return &s_operand;
        case SCRIPT_OPERAND_SIGNED_WORD:
            wordValue = ReadScriptWord();
            s_operand = wordValue;
            return &s_operand;
        case SCRIPT_OPERAND_ITEM_PRICE:
            s_operand = GetItemPrice(ReadScriptValue());
            return &s_operand;
        case SCRIPT_OPERAND_ITEM_SELL_PRICE:
            s_operand = GetItemPrice(ReadScriptValue()) / 4;
            return &s_operand;
        case SCRIPT_OPERAND_ROLLED_MACCA:
            ReadScriptWord();
            s_operand = g_rolledMacca;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MACCA:
            s_operand = GetObjectMacca(ReadObjectRef());
            return &s_operand;
        case SCRIPT_OPERAND_ROLLED_MAGNETITE:
            ReadScriptWord();
            s_operand = g_rolledMagnetite;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MAGNETITE:
            s_operand = GetObjectMagnetite(ReadObjectRef());
            return &s_operand;
        case SCRIPT_OPERAND_WORD:
        case SCRIPT_OPERAND_WORD_ALIAS_1:
        case SCRIPT_OPERAND_WORD_ALIAS_2:
        case SCRIPT_OPERAND_WORD_ALIAS_3:
        case SCRIPT_OPERAND_WORD_ALIAS_4:
        case SCRIPT_OPERAND_WORD_ALIAS_5:
        case SCRIPT_OPERAND_WORD_ALIAS_6:
        case SCRIPT_OPERAND_WORD_ALIAS_7:
        case SCRIPT_OPERAND_WORD_ALIAS_8:
        case SCRIPT_OPERAND_WORD_ALIAS_9:
        case SCRIPT_OPERAND_WORD_ALIAS_10:
        case SCRIPT_OPERAND_WORD_ALIAS_11:
        case SCRIPT_OPERAND_WORD_ALIAS_12:
            s_operand = ReadScriptWord();
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_STAT_0:
        case SCRIPT_OPERAND_OBJECT_STAT_1:
        case SCRIPT_OPERAND_OBJECT_STAT_2:
        case SCRIPT_OPERAND_OBJECT_STAT_3:
        case SCRIPT_OPERAND_OBJECT_STAT_4:
        case SCRIPT_OPERAND_OBJECT_STAT_5:
        case SCRIPT_OPERAND_OBJECT_STAT_6:
        case SCRIPT_OPERAND_OBJECT_STAT_7:
        case SCRIPT_OPERAND_OBJECT_STAT_8:
        case SCRIPT_OPERAND_OBJECT_STAT_9:
        case SCRIPT_OPERAND_OBJECT_STAT_10:
            s_operand = GetObjectStatTotal(ReadObjectRef(), kind - SCRIPT_OPERAND_OBJECT_STAT_0);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_LEVEL:
            s_operand = GetObjectLevel(ReadObjectRef());
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_LEVEL_GAP:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->levelGap;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_FAMILIARITY:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->familiarity;
            if (IsEventFlagSet(EVENT_FLAG_BANK_OWNED, OWNED_DCS_MABUDACHI)) {
                break;
            }
            s_operand += 2;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_FAMILIARITY_COUNT:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetFamiliarityCount(object->id);
            return &s_operand;
        case SCRIPT_OPERAND_ROLLED_MACCA_WITH_UNIT:
            ReadScriptByte();
            s_operand = g_rolledMacca;
            strcpy(g_numberUnit, "\203}\203b\203J"); // マッカ
            return &s_operand;
        case SCRIPT_OPERAND_ROLLED_MAGNETITE_WITH_UNIT:
            ReadScriptByte();
            s_operand = g_rolledMagnetite;
            strcpy(g_numberUnit, "\202l\202`\202f"); // ＭＡＧ
            return &s_operand;
        case SCRIPT_OPERAND_IGNORED_WORD:
            ReadScriptWord();
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_HP:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.hp.cur;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MP:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.mp.cur;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MAX_HP:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.hp.max;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MAX_MP:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.mp.max;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ALIGNMENT_B:
            s_operand = GetObjectAlignmentLevelB(ReadObjectRef());
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ALIGNMENT_A:
            s_operand = GetObjectAlignmentLevelA(ReadObjectRef());
            return &s_operand;
        case SCRIPT_OPERAND_STATUS_CONDITION:
            s_operand = g_statusCondition;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ID:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->id;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_WEAPON_DEFENSE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatShown(object, BATTLE_STAT_WEAPON_DEFENSE);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_FOURTH_GROUP_BASE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, BATTLE_STAT_DEMON_INTERACTION_LEVEL);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_WEAPON:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_WEAPON].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_GUN:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_GUN].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_AMMO:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_AMMO].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_RACE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetDemonRace(object->id);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_WEAPON_GROUP_BASE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, BATTLE_STAT_WEAPON_LEVEL);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_GUN_GROUP_BASE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, BATTLE_STAT_GUN_LEVEL);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_MAGIC_GROUP_BASE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, BATTLE_STAT_MAGIC_LEVEL);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_HEAD:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_HEAD].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_BODY:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_BODY].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ARMS:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_ARMS].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_LEGS:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_LEGS].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ACCESSORY:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_ACCESSORY].item;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_TRAINING_0:
        case SCRIPT_OPERAND_OBJECT_TRAINING_1:
        case SCRIPT_OPERAND_OBJECT_TRAINING_2:
        case SCRIPT_OPERAND_OBJECT_TRAINING_3:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetTrainingPoints(object, kind - SCRIPT_OPERAND_OBJECT_TRAINING_0);
            return &s_operand;
        case SCRIPT_OPERAND_ACTION_VALUE:
            s_operand = GetActionValue(ReadScriptValue());
            return &s_operand;
        case SCRIPT_OPERAND_HP_CHANGE:
            ReadScriptValue();
            s_operand = g_hpChange;
            return &s_operand;
        case SCRIPT_OPERAND_MP_CHANGE:
            ReadScriptValue();
            s_operand = g_mpChange;
            return &s_operand;
        case SCRIPT_OPERAND_EFFECT_CONDITION:
            if (ReadScriptValue() == 0) {
                s_operand = g_statusCondition;
                return &s_operand;
            }
            s_operand = g_effectCondition;
            return &s_operand;
        case SCRIPT_OPERAND_BATTLE_RESULT_VALUE:
            s_operand = GetBattleResultValue(ReadScriptValue());
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_EXPERIENCE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterExperience(object);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_LEVEL_BONUS:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->levelBonus;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_0:
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_1:
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_2:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterAffiliation(object, kind - SCRIPT_OPERAND_OBJECT_AFFILIATION_0);
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_TITLE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->title;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_WEAPON_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_WEAPON].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_GUN_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_GUN].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_AMMO_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_AMMO].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_HEAD_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_HEAD].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_BODY_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_BODY].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ARMS_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_ARMS].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_LEGS_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_LEGS].value;
            return &s_operand;
        case SCRIPT_OPERAND_OBJECT_ACCESSORY_VALUE:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[EQUIP_SLOT_ACCESSORY].value;
            break;
    }
    return &s_operand;
}

RVA(0x000373e0, 0x54)
i32 GetBattleResultValue(GZ_ENUM_PARAM(ScriptBattleResultSelector, i16) which) {
    switch (which) {
        case SCRIPT_BATTLE_RESULT_MACCA:
            return g_rewardMacca;
        case SCRIPT_BATTLE_RESULT_MAGNETITE:
            return g_rewardMagnetite;
        case SCRIPT_BATTLE_RESULT_EXPERIENCE_PER_MEMBER:
            return 2u * g_rewardExperience / CountPartyMembers(1);
        case SCRIPT_BATTLE_RESULT_FIRST_DROP_ITEM:
            return GetDropSlot(0)->item;
    }
    return 0;
}

RVA(0x00037440, 0x44)
i32 GetActionValue(GZ_ENUM_PARAM(ScriptActionValueSelector, i16) which) {
    switch (which) {
        case SCRIPT_ACTION_VALUE_RESULT:
            return g_actionResult;
        case SCRIPT_ACTION_VALUE_HP_CHANGE:
            return g_hpChange;
        case SCRIPT_ACTION_VALUE_MP_CHANGE:
            return g_mpChange;
        case SCRIPT_ACTION_VALUE_DRAIN_AMOUNT:
            return g_drainAmount;
    }
    return 0;
}

RVA(0x00037490, 0x8)
i32 ReadScriptValue(void) {
    return *ReadScriptOperand();
}

// The two sides of a contest of `stat` between the actor (object -1) and its
// target (object -17): stats 0..10 are the stat totals, 11 the levels, and
// 12..14 the target's level gap, familiarity and familiarity count against -1;
// `swap` exchanges the sides.
RVA(0x000374a0, 0x110)
void ReadContestValues(GZ_ENUM_PARAM(ContestStat, i16) stat, i32* own, i32* other, i16 swap) {
    Character* object;
    i32 kept;

    switch (stat) {
        case STAT_INTUITION:
        case STAT_MENTAL_STRENGTH:
        case STAT_MAGIC:
        case STAT_INTELLIGENCE:
        case STAT_PROTECTION:
        case STAT_STRENGTH:
        case STAT_VITALITY:
        case STAT_AGILITY:
        case STAT_DEXTERITY:
        case STAT_CHARM:
        case STAT_FORTUNE:
            *own = GetObjectStatTotal(SCRIPT_REF_SLOT_BASE, stat);
            *other = GetObjectStatTotal(SCRIPT_REF_ACTOR, stat);
            break;
        case CONTEST_LEVEL:
            *own = GetObjectLevel(SCRIPT_REF_SLOT_BASE);
            *other = GetObjectLevel(SCRIPT_REF_ACTOR);
            break;
        case CONTEST_LEVEL_GAP:
            object = ResolveScriptObject(SCRIPT_REF_ACTOR);
            if (object == NULL) {
                *own = 0;
            } else {
                *own = object->levelGap;
            }
            *other = -1;
            break;
        case CONTEST_FAMILIARITY:
            object = ResolveScriptObject(SCRIPT_REF_ACTOR);
            if (object == NULL) {
                *own = 0;
            } else {
                *own = object->familiarity;
            }
            *other = -1;
            break;
        case CONTEST_FAMILIARITY_COUNT:
            object = ResolveScriptObject(SCRIPT_REF_ACTOR);
            if (object == NULL) {
                *own = 0;
            } else {
                *own = GetFamiliarityCount(object->id);
            }
            *other = -1;
            break;
    }
    if (swap) {
        kept = *own;
        *own = *other;
        *other = kept;
    }
}

// Writes an object field chosen by a kind byte (the same kinds
// ReadScriptOperand reads) from the next operand; unknown kinds and a missing
// object write nothing. Setting a party leader's affiliation raises its
// levels, and to 3 also teaches it every skill of that axis.
RVA(0x000375b0, 0x3a0)
void OpSetObjectField(void) {
    Character* object = ReadScriptObject();
    GZ_ENUM_LOCAL(ScriptOperandKind, i16) kind = ReadScriptByte();
    i32 value = ReadScriptValue();

    if (object == NULL) {
        return;
    }
    switch (kind) {
        case SCRIPT_OPERAND_OBJECT_MACCA:
            object->macca = value;
            return;
        case SCRIPT_OPERAND_OBJECT_MAGNETITE:
            object->magnetite = value;
            return;
        case SCRIPT_OPERAND_OBJECT_STAT_0:
        case SCRIPT_OPERAND_OBJECT_STAT_1:
        case SCRIPT_OPERAND_OBJECT_STAT_2:
        case SCRIPT_OPERAND_OBJECT_STAT_3:
        case SCRIPT_OPERAND_OBJECT_STAT_4:
        case SCRIPT_OPERAND_OBJECT_STAT_5:
        case SCRIPT_OPERAND_OBJECT_STAT_6:
        case SCRIPT_OPERAND_OBJECT_STAT_7:
        case SCRIPT_OPERAND_OBJECT_STAT_8:
        case SCRIPT_OPERAND_OBJECT_STAT_9:
        case SCRIPT_OPERAND_OBJECT_STAT_10:
            SetStatTotal(object, kind - SCRIPT_OPERAND_OBJECT_STAT_0, value);
            return;
        case SCRIPT_OPERAND_OBJECT_LEVEL:
            object->level = value;
            return;
        case SCRIPT_OPERAND_OBJECT_LEVEL_GAP:
            object->levelGap = value;
            return;
        case SCRIPT_OPERAND_OBJECT_FAMILIARITY:
            object->familiarity = value;
            return;
        case SCRIPT_OPERAND_OBJECT_HP:
            object->pools.hp.cur = value;
            return;
        case SCRIPT_OPERAND_OBJECT_MP:
            object->pools.mp.cur = value;
            return;
        case SCRIPT_OPERAND_OBJECT_MAX_HP:
            object->pools.hp.max = value;
            return;
        case SCRIPT_OPERAND_OBJECT_MAX_MP:
            object->pools.mp.max = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ALIGNMENT_B:
            object->alignmentLevelB = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ALIGNMENT_A:
            object->alignmentLevelA = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ID:
            object->id = value;
            return;
        case SCRIPT_OPERAND_OBJECT_WEAPON_DEFENSE:
            object->battleStatsShown[BATTLE_STAT_WEAPON_DEFENSE] = value;
            return;
        case SCRIPT_OPERAND_OBJECT_FOURTH_GROUP_BASE:
            object->battleStats[18] = value;
            return;
        case SCRIPT_OPERAND_OBJECT_WEAPON:
            GetCharacterEquipment(object)[EQUIP_SLOT_WEAPON].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_GUN:
            GetCharacterEquipment(object)[EQUIP_SLOT_GUN].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_AMMO:
            GetCharacterEquipment(object)[EQUIP_SLOT_AMMO].item = value;
            GetCharacterEquipment(object)[EQUIP_SLOT_AMMO].quantity = GetGunMagazineSize(
                GetLoadedRecord(GetCharacterEquipment(object)[EQUIP_SLOT_GUN].item)
            );
            return;
        case SCRIPT_OPERAND_OBJECT_WEAPON_GROUP_BASE:
            object->battleStats[BATTLE_STAT_WEAPON_LEVEL] = value;
            return;
        case SCRIPT_OPERAND_OBJECT_GUN_GROUP_BASE:
            object->battleStats[6] = value;
            return;
        case SCRIPT_OPERAND_OBJECT_MAGIC_GROUP_BASE:
            object->battleStats[12] = value;
            return;
        case SCRIPT_OPERAND_OBJECT_HEAD:
            GetCharacterEquipment(object)[EQUIP_SLOT_HEAD].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_BODY:
            GetCharacterEquipment(object)[EQUIP_SLOT_BODY].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ARMS:
            GetCharacterEquipment(object)[EQUIP_SLOT_ARMS].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_LEGS:
            GetCharacterEquipment(object)[EQUIP_SLOT_LEGS].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ACCESSORY:
            GetCharacterEquipment(object)[EQUIP_SLOT_ACCESSORY].item = value;
            return;
        case SCRIPT_OPERAND_OBJECT_TRAINING_0:
        case SCRIPT_OPERAND_OBJECT_TRAINING_1:
        case SCRIPT_OPERAND_OBJECT_TRAINING_2:
        case SCRIPT_OPERAND_OBJECT_TRAINING_3:
            object->trainingPoints[kind - SCRIPT_OPERAND_OBJECT_TRAINING_0] = (i16)value;
            return;
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_0:
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_1:
        case SCRIPT_OPERAND_OBJECT_AFFILIATION_2:
            SetCharacterAffiliation(object, kind - SCRIPT_OPERAND_OBJECT_AFFILIATION_0, value);
            if (object->id == HUMAN_KATSURAGI) {
                RaiseAffiliationLevels(object);
                if (value == 3) {
                    LearnAllSkills(object, kind - SCRIPT_OPERAND_OBJECT_AFFILIATION_0);
                }
            }
            return;
        case SCRIPT_OPERAND_OBJECT_TITLE:
            object->title = value;
            return;
        case SCRIPT_OPERAND_OBJECT_WEAPON_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_WEAPON].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_GUN_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_GUN].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_AMMO_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_AMMO].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_HEAD_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_HEAD].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_BODY_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_BODY].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ARMS_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_ARMS].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_LEGS_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_LEGS].value = value;
            return;
        case SCRIPT_OPERAND_OBJECT_ACCESSORY_VALUE:
            GetCharacterEquipment(object)[EQUIP_SLOT_ACCESSORY].value = value;
            return;
    }
}

RVA(0x00037950, 0x9f)
void OpFindMemberByPoolState(i16 all, i16 pools) {
    i16 index = ReadLongVarIndex();
    i16 state = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i16 slot = 0;
    i32 result;
    if (!all) {
        result = FindMemberByPoolState(slot, mode, state, pools);
    } else {
        result = 0;
        while (slot >= 0 && slot < ROSTER_SIZE) {
            slot = FindMemberByPoolState(slot, mode, state, pools);
            if (slot != ROSTER_SLOT_NONE) {
                result |= PowerOfTwo(slot);
                slot++;
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x000379f0, 0xde)
void OpFindMemberWithCondition(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 condition = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = RosterMemberAt(slot);
                if (character && HasCondition(GetCharacterConditions(character), condition)) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = RosterMemberAt(slot);
                if (character && HasCondition(GetCharacterConditions(character), condition)) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037ad0, 0xeb)
void OpFindMemberByAlignmentA(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 alignment = 1 - ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassB(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassB(character) == alignment) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037bc0, 0xeb)
void OpFindMemberByAlignmentB(i16 all) {
    i16 index = ReadLongVarIndex();
    i16 alignment = 1 - ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    i32 result = -1;
    i16 slot;
    Character* character;
    if (!all) {
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassA(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassA(character) == alignment) {
                    result |= PowerOfTwo(slot);
                }
            }
        }
    }
    SetScriptLongVar(index, result);
}

RVA(0x00037cb0, 0xa3)
void OpCountItemOwned(void) {
    i32 count = 0;
    i16 index = ReadLongVarIndex();
    i16 item = ReadScriptValue();
    i16 mode = ReadScriptValue();
    GZ_ENUM_STORAGE(ItemCountScope, i16) scope = ReadScriptValue();
    i16 slot;
    Character* character;
    mode++;
    if (scope == ITEM_COUNT_EQUIPMENT || scope == ITEM_COUNT_BAG_AND_EQUIPMENT) {
        for (slot = 0; slot < ROSTER_SIZE; slot++) {
            if (FilterPartyMember(slot, mode) != ROSTER_SLOT_NONE) {
                character = GetRosterCharacter(slot);
                if (character) {
                    count += CountItemInSlots(item, GetCharacterEquipment(character));
                }
            }
        }
    }
    if (scope == ITEM_COUNT_BAG || scope == ITEM_COUNT_BAG_AND_EQUIPMENT) {
        count += CountHeldItem(item);
    }
    SetScriptLongVar(index, count);
}

RVA(0x00037d60, 0xa1)
i16 CountItemInSlots(i16 item, ItemSlot* slots) {
    i16 count = 0;
    if (slots[0].item == item) {
        count++;
    }
    if (slots[1].item == item) {
        count++;
    }
    if (slots[2].item == item) {
        count++;
    }
    if (slots[3].item == item) {
        count++;
    }
    if (slots[4].item == item) {
        count++;
    }
    if (slots[5].item == item) {
        count++;
    }
    if (slots[6].item == item) {
        count++;
    }
    if (slots[7].item == item) {
        count++;
    }
    return count;
}

// Jumps unless roster member `slot` has condition `condition` (or, with
// `expect` set, lacks it); no member counts as lacking it only with `expect`.
RVA(0x00037e10, 0x6e)
void OpIfMemberHasCondition(void) {
    b32 jump = false;
    i16 target = ReadBranchTarget();
    i16 expect = ReadScriptValue();
    Character* character = GetRosterCharacter(ReadScriptValue());
    // The condition, then whether the member has it (no member: tested as
    // read).
    i16 has = ReadScriptValue();
    if (!character && expect) {
        jump = true;
    } else {
        if (character) {
            has = HasCondition(GetCharacterConditions(character), has);
        }
        if (ScriptBooleanMatches(has, expect)) {
            jump = true;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00037e80, 0x18)
void OpSaveObjectConditions(void) {
    Character* object = ReadScriptObject();
    if (object) {
        SetFlagTag(GetCharacterConditions(object)->bits);
    }
}

RVA(0x00037ea0, 0x28)
void OpApplyObjectCondition(void) {
    Character* object = ReadScriptObject();
    i16 condition = ReadScriptValue();
    if (object) {
        AddCondition(GetCharacterConditions(object), condition);
        RequestFieldRefresh();
    }
}

RVA(0x00037ed0, 0x28)
void OpClearObjectCondition(void) {
    Character* object = ReadScriptObject();
    i16 condition = ReadScriptValue();
    if (object) {
        ClearCondition(GetCharacterConditions(object), condition);
        RequestFieldRefresh();
    }
}

RVA(0x00037f00, 0x20)
i32 GetObjectStatTotal(i16 ref, GZ_ENUM_PARAM(CharacterStat, i16) stat) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return GetStatTotal(object, stat);
}

RVA(0x00037f20, 0x1a)
i32 GetObjectLevel(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return object->level;
}

RVA(0x00037f40, 0x17)
i32 GetObjectAlignmentLevelB(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return GetAlignmentLevelB(object);
}

RVA(0x00037f60, 0x17)
i32 GetObjectAlignmentLevelA(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return GetAlignmentLevelA(object);
}

RVA(0x00037f80, 0x22)
GZ_ENUM_RETURN(AlignmentSide, i16) StepForMode(GZ_ENUM_PARAM(AlignmentStepMode, i16) mode) {
    switch (mode) {
        case ALIGNMENT_STEP_TO_POSITIVE:
            return ALIGNMENT_POSITIVE;
        case ALIGNMENT_STEP_TO_NEUTRAL:
            return ALIGNMENT_NEUTRAL;
        case ALIGNMENT_STEP_TO_NEGATIVE:
            return ALIGNMENT_NEGATIVE;
    }
    return ALIGNMENT_NEUTRAL;
}

// Shifts the player's alignment B by an amount, towards the side the mode
// operand (0, 1 or 2; 1 counts as 2) steps to.
RVA(0x00037fb0, 0x40)
void OpShiftPlayerAlignmentB(void) {
    Character* player = GetCharacter(0);
    i16 mode = ReadScriptValue();
    i16 amount;
    if (mode == 1) {
        mode = 2;
    }
    amount = ReadScriptValue();
    ShiftAlignmentB(player, amount, StepForMode(mode));
}

RVA(0x00037ff0, 0x40)
void OpShiftPlayerAlignmentA(void) {
    Character* player = GetCharacter(0);
    i16 mode = ReadScriptValue();
    i16 amount;
    if (mode == 1) {
        mode = 2;
    }
    amount = ReadScriptValue();
    ShiftAlignmentA(player, amount, StepForMode(mode));
}

RVA(0x00038030, 0x25)
b16 OpLevelUpMember(void) {
    i16 slot = ReadScriptValue();
    GainLevels(GetRosterCharacter(slot), ReadScriptValue());
    return false;
}

static __inline i16 ReadScriptDelta(i16 negate) {
    i16 delta = ReadScriptValue();
    if (negate) {
        delta = -delta;
    }
    return delta;
}

// The script actor's familiarity count (negated with `negate`).
RVA(0x00038060, 0x27)
void OpAddFamiliarityCount(i16 negate) {
    Character* actor = g_curScript->actor;
    i16 delta = ReadScriptDelta(negate);
    AddFamiliarityCount(actor->id, delta);
}

RVA(0x00038090, 0x22)
void OpAddActorFamiliarity(i16 negate) {
    i16 delta = ReadScriptDelta(negate);
    AddFamiliarity(g_curScript->actor, delta);
}

RVA(0x000380c0, 0x22)
void OpAddActorLevelGap(i16 negate) {
    i16 delta = ReadScriptDelta(negate);
    AddLevelGap(g_curScript->actor, delta);
}

RVA(0x000380f0, 0x18)
void OpSetActorFamiliarity(void) {
    SetFamiliarity(g_curScript->actor, ReadScriptValue());
}

RVA(0x00038110, 0x18)
void OpSetActorLevelGap(void) {
    SetLevelGap(g_curScript->actor, ReadScriptValue());
}

RVA(0x00038130, 0x15)
void OpSetActorAttitude(void) {
    g_curScript->actor->attitude = ReadScriptValue();
}

RVA(0x00038150, 0x15)
void OpSetActorFieldState(void) {
    g_curScript->actor->fieldState = ReadScriptValue();
}

// Sets an object's familiarity (0..255; negated with `negate`) and its
// personal flag 0.
RVA(0x00038170, 0x44)
void OpSetObjectFamiliarity(i16 negate) {
    Character* object = ReadScriptObject();
    i32 value = ReadScriptValue();
    u8 familiarity;
    if (negate) {
        value = -value;
    }
    familiarity = ClampInt(value, 0, 0xff);
    if (object) {
        object->familiarity = familiarity;
        SetCharacterFlag(object, 0);
    }
}

// Jumps unless the pooled items fit in the bag (with `invert`, unless they
// do not).
RVA(0x000381c0, 0x2a)
void OpBranchOnItemsFit(i16 invert) {
    i16 target = ReadBranchTarget();
    i32 fit = PooledItemsFit();
    if (invert) {
        fit = !fit;
    }
    ScriptJumpUnless(target, fit);
}

RVA(0x000381f0, 0x19)
void OpRemovePendingItem(void) {
    i16 item = ReadScriptValue();
    TakeFromPool(item, ReadScriptValue());
}

RVA(0x00038210, 0x19)
void OpAddPendingItem(void) {
    i16 item = ReadScriptValue();
    AddToPool(item, ReadScriptValue());
}

// Stores in a long variable the mask of roster slots (passing
// FilterPartyMember with `mode` + 1) whose demon race is `race`.
RVA(0x00038230, 0x7e)
void OpMaskRosterByKind(void) {
    i16 index = ReadLongVarIndex();
    i16 race = ReadScriptValue();
    i16 mode = ReadScriptValue() + 1;
    u32 mask = 0;
    i16 i;
    for (i = 0; i < ROSTER_SIZE; i++) {
        if (FilterPartyMember(i, mode) != ROSTER_SLOT_NONE && RosterMemberAt(i)
            && race == GetDemonRace(RosterMemberAt(i)->id)) {
            mask |= PowerOfTwo(i);
        }
    }
    SetScriptLongVar(index, mask);
}

// Fills pool `pool` (1: HP, else MP) by `amount` for each roster slot in the
// mask.
RVA(0x000382b0, 0x67)
void OpRecoverRosterPool(GZ_ENUM_PARAM(CharacterPoolMask, i16) pool) {
    u32 mask;
    u32 bit = 1;
    i16 amount;
    i16 i;
    mask = ReadScriptValue();
    amount = ReadScriptValue();
    for (i = 0; i < ROSTER_SIZE; i++) {
        if (RosterMemberAt(i) && (mask & bit)) {
            if (pool == POOL_MASK_HP) {
                FillPool(GetCharacterHpPool(RosterMemberAt(i)), amount, POOL_FILL_TO_MAX);
            } else {
                FillPool(GetCharacterMpPool(RosterMemberAt(i)), amount, POOL_FILL_TO_MAX);
            }
        }
        bit <<= 1;
    }
}

// Clears condition `condition` from each roster slot in the mask that has it.
RVA(0x00038320, 0x5f)
void OpCureRosterCondition(void) {
    u32 mask;
    u32 bit = 1;
    i16 condition;
    i16 i;
    mask = ReadScriptValue();
    condition = ReadScriptValue();
    for (i = 0; i < ROSTER_SIZE; i++) {
        if (mask & bit) {
            Character* character = GetRosterCharacter(i);
            if (character && HasCondition(GetCharacterConditions(character), condition)) {
                ClearCondition(GetCharacterConditions(character), condition);
            }
        }
        bit <<= 1;
    }
}

RVA(0x00038380, 0xf)
ScriptChoice* ListTail(ScriptChoice* node) {
    while (1) {
        if (node->next == NULL) {
            break;
        }
        node = node->next;
    }
    return node;
}

RVA(0x00038390, 0x2e)
ScriptChoice* AppendScriptChoice(ScriptChoice** head) {
    ScriptChoice* choice = AllocCleared(1, sizeof(ScriptChoice));
    if (*head == NULL) {
        *head = choice;
    } else {
        ListTail(*head)->next = choice;
    }
    return choice;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000383c0, 0x7b)
ScriptChoice*
PrintScriptChoice(i16 window, ScriptChoice** head, const char* text, i16 value, i16 disabled) {
    ScriptChoice* choice = AppendScriptChoice(head);
    choice->x = GetTextPlaneCursorX(window);
    choice->y = GetTextPlaneCursorY(window);
    choice->width = strlen(text);
    choice->value = value;
    choice->disabled = disabled;
    PrintWindowText(window, text, TEXT_ATTR_DEFAULT, 0, true);
    return choice;
}

RVA(0x00038440, 0x1e)
ScriptChoice* FreeScriptChoices(ScriptChoice* head) {
    while (head) {
        ScriptChoice* choice = head;
        head = head->next;
        FreeBlock(choice);
    }
    return head;
}

static __inline void
SetScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    s_choiceMenu = choices;
    s_choiceWindow = window;
    s_keepChoices = keep;
    s_choiceCancelMode = cancelMode;
}

RVA(0x00038460, 0x39)
ScriptChoice* PushScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    SetScriptChoiceMenu(choices, window, keep, cancelMode);
    PushGameState(GAME_STATE_SCRIPT_CHOICE);
    return NULL;
}

RVA(0x000384a0, 0x4c)
void InitScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    SetScriptChoiceMenu(choices, window, keep, cancelMode);
    if (g_mouseLeftClick) {
        g_mouseLeftClick = MOUSE_CLICK_NONE;
    }
    s_hoveredChoice = SCRIPT_CHOICE_NONE;
    s_highlightedChoice = NULL;
}

static __inline void ToggleScriptChoiceHighlight(void) {
    if (s_highlightedChoice) {
        ReverseTextRun(
            s_choiceWindow,
            s_highlightedChoice->x,
            s_highlightedChoice->y,
            s_highlightedChoice->width
        );
        RedrawTextRun(
            s_choiceWindow,
            s_highlightedChoice->x,
            s_highlightedChoice->y,
            s_highlightedChoice->width
        );
    }
}

RVA(0x000384f0, 0x1d2)
i16 PollScriptChoiceMenu(void) {
    i16 index = 0;
    i16 hovered;
    if (TakeMouseCancel(s_choiceCancelMode)) {
        if (!s_keepChoices) {
            s_choiceMenu = FreeScriptChoices(s_choiceMenu);
        }
        PlaySoundEffect(2);
        return SCRIPT_CHOICE_CANCELLED;
    }
    hovered = FindScriptChoiceAtMouse();
    if (hovered != s_hoveredChoice) {
        ToggleScriptChoiceHighlight();
        s_hoveredChoice = hovered;
        s_highlightedChoice = s_hitChoice;
        ToggleScriptChoiceHighlight();
    }
    if (!TakeMouseLeftClick()) {
        return SCRIPT_CHOICE_WAITING;
    }
    if (s_hoveredChoice == SCRIPT_CHOICE_NONE) {
        return SCRIPT_CHOICE_WAITING;
    }
    g_hoveredObjectId = s_hoveredChoice;
    g_selectedObjectId = s_highlightedChoice->value;
    if (!s_keepChoices) {
        s_hitChoice = s_choiceMenu;
        while (s_hitChoice) {
            if (index != s_hoveredChoice) {
                BlankTextRun(s_choiceWindow, s_hitChoice->x, s_hitChoice->y, s_hitChoice->width);
                RedrawTextRun(s_choiceWindow, s_hitChoice->x, s_hitChoice->y, s_hitChoice->width);
            }
            s_hitChoice = s_hitChoice->next;
            index++;
        }
        s_choiceMenu = FreeScriptChoices(s_choiceMenu);
    }
    PlaySoundEffect(1);
    return SCRIPT_CHOICE_SELECTED;
}

RVA(0x000386d0, 0xbf)
i16 FindScriptChoiceAtMouse(void) {
    i16 originX, originY;
    i16 index;
    ScriptChoice* choice;
    GetTextPlaneOrigin(s_choiceWindow, &originX, &originY);
    choice = s_choiceMenu;
    index = 0;
    while (choice) {
        if (!choice->disabled) {
            i16 x = choice->x * 8;
            i16 y = choice->y * 16;
            x = g_mousePosition.x - x - originX;
            y = g_mousePosition.y - y - originY;
            if (x >= 0 && x < choice->width * 8 && y >= 0 && y < 16) {
                s_hitChoice = choice;
                return index;
            }
        }
        choice = choice->next;
        index++;
    }
    s_hitChoice = NULL;
    return SCRIPT_CHOICE_NONE;
}

RVA(0x00038790, 0x53)
b16 RunScriptChoiceState(void) {
    switch (GetGameSub()) {
        case SCRIPT_CHOICE_SUBSTEP_INITIALIZE:
            NextGameSub();
            InitScriptChoiceMenu(s_choiceMenu, s_choiceWindow, s_keepChoices, s_choiceCancelMode);
        case SCRIPT_CHOICE_SUBSTEP_POLL:
            if (PollScriptChoiceMenu()) {
                ReturnFromGameState();
            }
            break;
    }
    return false;
}
