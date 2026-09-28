// @identity-TODO: the owning TU is unproven. One retail object: the script
// actor and flow opcodes, the comparison and clock opcodes, the text tokens,
// the operand reader and the choice commands. The flow and actor statics
// follow the token, choice and operand statics in one .bss run
// (0x481230..0x48135b), against .text order; the token tables follow the
// choice command's hovered word in .data; each static is read only by its
// own part's code, and the code is contiguous in .text.

#include <rva.h>

#include <Game/Actor.h>
#include <Game/Alignment.h>
#include <Game/AnalyzeData.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
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
#include <Game/ItemMenu.h>
#include <Game/ItemPool.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/ModeFlags.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/SaveGame.h>
#include <Game/Scene.h>
#include <Game/Skill.h>
#include <Game/SkillList.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Game/StatusDraw.h>
#include <Game/StatusScreen.h>
#include <Game/WorldMap.h>
#include <Gfx/Sprite.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
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

DATA(0x000646c8)
static const i16 s_rewardLevelThresholds[16] =
    {20, 30, 40, 50, 60, 70, 75, 80, 85, 90, 95, 100, 110, 120, 130, 140};

DATA(0x00069130)
static i16 s_hoveredChoice = -1;

// "Ａ", "Ｂ", "ＡＢ", "Ｏ".
DATA(0x00069138)
static char* s_bloodTypes[4] = {"\202`", "\202a", "\202`\202a", "\202n"};

DATA(0x00069148)
static char* s_signNames[12] = {
    g_shortNames[0],
    g_shortNames[1],
    g_shortNames[2],
    g_shortNames[3],
    g_shortNames[4],
    g_shortNames[5],
    g_shortNames[6],
    g_shortNames[7],
    g_shortNames[8],
    g_shortNames[9],
    g_shortNames[10],
    g_shortNames[11],
};

DATA(0x00069178)
static char* s_affiliationNames[4] = {
    g_shortNames[12],
    g_shortNames[13],
    g_shortNames[14],
    g_shortNames[15],
};

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
static ScriptChoice* s_highlightedChoice = 0;

DATA(0x00081340)
static ScriptChoice* s_choiceMenu = 0;

DATA(0x00081344)
static ScriptChoice* s_hitChoice = 0;

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

DATA(0x0008135c)
char g_shortNames[16][4] = {0};

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
    i32 lo;
    i32 hi;
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
    TransferFrameVars(frame, 0);
    return true;
}

// Restores the system variables from the newest call frame.
RVA(0x00033b50, 0x26)
b16 LoadFrameLocals(void) {
    i32 frame = TopCallFrame(g_curScript);
    if (!frame) {
        return false;
    }
    TransferFrameVars(frame, 1);
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
    TransferFrameVars(saved, 0);
    TransferFrameVars(frame, 1);
    TransferFrameVars(loaded, 0);
    TransferFrameVars(saved, 1);
    TransferFrameVars(frame, 0);
    TransferFrameVars(loaded, 1);
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
    } else if (file == 0x5b && entry == 0x22 && g_field.pos.x == 1 && g_field.pos.y == 4
               && g_field.pos.level == 1 && g_field.pos.area == 0x83) {
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
i32 ScriptJumpUnless(i16 pc, i32 condition) {
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
        return 0;
    }
    return -1;
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
        } else if (entry == 0x100 || (entry == 0x101 && g_field.pos.area == 0x83)) {
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
        } else if (entry == 0x101 && g_field.pos.area == 0x2e && g_field.pos.x == 7) {
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
            if ((g_field.pos.area == 0x06 && g_field.pos.y == 0x12)
                || (g_field.pos.area == 0x0a && g_field.pos.y == 0x0d)
                || (g_field.pos.area == 0x13 && g_field.pos.y == 0x01)
                || (g_field.pos.area == 0x1b && g_field.pos.y == 0x05)
                || (g_field.pos.area == 0x8a && g_field.pos.y == 0x02)
                || (g_field.pos.area == 0x1f && g_field.pos.level == 2)
                || (g_field.pos.area == 0x1a && g_field.pos.x == 0x0a)
                || (g_field.pos.area == 0x34 && g_field.pos.level == 0) || g_field.pos.area == 0x2e
                || (g_field.pos.area == 0x25 && g_field.pos.level == 0)
                || (g_field.pos.area == 0x21 && g_field.pos.y == 0x09)
                || (g_field.pos.area == 0x30 && g_field.pos.y == 0x0b)) {
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
            if (g_field.pos.area == 0x1a && g_field.pos.level == 3) {
                LoadSpriteImage(0x1f, 0x4f, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xd4);
                // 銀座地下街秘密区の薬屋でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\213\342\215\300\222n\211\272\212X\224\351\226\247\213\346\202\314\226\362"
                    "\211\256\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265"
                    "\202\304\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_field.pos.area == 0x06 || g_field.pos.area == 0x1b
                       || g_field.pos.area == 0x34 || g_field.pos.area == 0x25) {
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
            } else if (g_field.pos.area == 0x56) {
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
            if (g_field.pos.area == 0x21) {
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
            if (g_field.pos.x == 1 && g_field.pos.y == 5) {
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
                if (g_field.pos.y == 9) {
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
            if (g_field.pos.area == 0x21) {
                LoadSpriteImage(0x1f, 0x2c, 1);
                PlaceSprite(0x1f, 0x1f, 1, 0x28, 0xf0);
                // アメ屋プラザの病院でない場合は連絡して下さい。Takubo
                DebugTrace(
                    "\012\203A\203\201\211\256\203v\203\211\203U\202\314\225a\211@"
                    "\202\305\202\310\202\242\217\352\215\207\202\315\230A\227\215\202\265\202\304"
                    "\211\272\202\263\202\242\201BTakubo\012"
                );
            } else if (g_field.pos.area == 0x56) {
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
    ChangePool(&character->pools.hp, ReadScriptValue() * sign);
    RequestFieldRefresh();
}

RVA(0x000345e0, 0x2c)
void OpChangeMp(i16 sign) {
    Character* character = ReadScriptObject();
    ChangePool(&character->pools.mp, ReadScriptValue() * sign);
    RequestFieldRefresh();
}

// Retail uses the MP pair as the input even when writing the HP result.
RVA(0x00034610, 0x6b)
void OpBoostPool(void) {
    Character* character = ReadScriptObject();
    i16 which = ReadScriptValue();
    i16 amount = ReadScriptValue();
    i16 mode = ReadScriptValue();
    i16 limit;
    i16 current;
    if (which == 0) {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    } else {
        limit = character->pools.mp.max;
        current = character->pools.mp.cur;
    }
    if (mode == 1) {
        limit *= 2;
    } else if (mode == 2) {
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
    i32 matches = 0;
    if (ReadAndMatchEventFlag()) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

#define RollFixedContestValue(value, level)                                                        \
    do {                                                                                           \
        switch (level) {                                                                           \
            case 0:                                                                                \
                break;                                                                             \
            case 1:                                                                                \
                (value) = RandomAverage(5, 15, 0);                                                 \
                break;                                                                             \
            case 2:                                                                                \
                (value) = RandomAverage(12, 22, 0);                                                \
                break;                                                                             \
            case 3:                                                                                \
                (value) = RandomAverage(20, 40, 0);                                                \
                break;                                                                             \
        }                                                                                          \
    } while (0)

#define RollRelativeContestValue(value, level)                                                     \
    do {                                                                                           \
        switch (level) {                                                                           \
            case 0:                                                                                \
                break;                                                                             \
            case 1:                                                                                \
                (value) = RandomPercent((value), -20, 20);                                         \
                break;                                                                             \
            case 2:                                                                                \
                (value) = RandomPercent((value), 0, 30);                                           \
                break;                                                                             \
            case 3:                                                                                \
                (value) = RandomPercent((value), 10, 40);                                          \
                break;                                                                             \
        }                                                                                          \
    } while (0)

// Jumps unless the actor wins a contest of `stat` (the next operand) against
// the target, or with `invert` unless it loses: the target's side is its own
// value, a random spread around it, or a fixed random range chosen by the
// stat and the contest `level` (0..3); `swap` exchanges the sides.
RVA(0x000348b0, 0x520)
void OpJumpUnlessStatContest(i16 level, i16 invert, i16 swap) {
    i16 target = ReadBranchTarget();
    i16 stat = ReadScriptValue();
    i32 own;
    i32 other;
    i16 order;
    i32 won;

    ReadContestValues(stat, &own, &other, swap);
    won = 0;
    switch (stat) {
        case 0:
            RollFixedContestValue(other, level);
            break;
        case 1:
            RollFixedContestValue(other, level);
            break;
        case 2:
            RollRelativeContestValue(other, level);
            break;
        case 3:
            switch (level) {
                case 0:
                    break;
                case 1:
                    other = RandomPercent(other, -20, 20);
                    break;
                case 2:
                    other = RandomPercent(other, 10, 30);
                    break;
                case 3:
                    other = RandomPercent(other, 10, 40);
                    break;
            }
            break;
        case 4:
            RollFixedContestValue(other, level);
            break;
        case 5:
            switch (level) {
                case 0:
                case 1:
                case 2:
                case 3:
                    other = RandomAverage(0, 40, 2);
                    break;
            }
            break;
        case 6:
            RollRelativeContestValue(other, level);
            break;
        case 7:
            RollRelativeContestValue(other, level);
            break;
        case 8:
            RollFixedContestValue(other, level);
            break;
        case 9:
            RollFixedContestValue(other, level);
            break;
        case 10:
            RollFixedContestValue(other, level);
            break;
        case 11:
            switch (level) {
                case 0: {
                    i32 ownAgility;
                    i32 otherAgility;

                    ReadContestValues(4, &ownAgility, &otherAgility, swap);
                    other = -sqrt(otherAgility);
                    break;
                }
                case 1:
                    other = RandomPercent(other, 10, 25);
                    break;
                case 2:
                    other = RandomPercent(other, 25, 50);
                    break;
                case 3:
                    other = RandomPercent(other, -20, 20);
                    break;
            }
            break;
        case 12:
            switch (level) {
                case 0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case 1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case 2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case 3:
                    other = RandomAverage(10, 17, 0);
                    break;
            }
            break;
        case 13:
            switch (level) {
                case 0:
                    other = RandomAverage(0, 7, 0);
                    break;
                case 1:
                    other = RandomAverage(7, 10, 0);
                    break;
                case 2:
                    other = RandomAverage(6, 13, 0);
                    break;
                case 3:
                    other = RandomAverage(11, 18, 0);
                    break;
            }
            break;
        case 14:
            switch (level) {
                case 0:
                    other = RandomAverage(35, 70, 0);
                    break;
                case 1:
                    other = RandomAverage(60, 91, 0);
                    break;
                case 2:
                    other = RandomAverage(85, 116, 0);
                    break;
                case 3:
                    other = RandomAverage(120, 135, 0);
                    break;
            }
            break;
    }
    order = CompareInt(own, other);
    if (!invert && order >= 0) {
        won = 1;
    }
    if (invert && order < 0) {
        won = 1;
    }
    ScriptJumpUnless(target, won);
}

// Jumps unless the party is in the script actor's sight (with `invert`,
// unless it is not).
RVA(0x00034dd0, 0x81)
void OpJumpUnlessPlayerInView(i16 invert) {
    i32 jump = 0;
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
    if ((seen == 1 && invert == 0) || (seen == 0 && invert == 1)) {
        jump = 1;
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
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    MapCoord coord = GetMapCoord();
    if (GridDistance(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y
        )
        > 4) {
        if (invert != 0) {
            jump = 1;
        }
    } else {
        i16 side = RelativeDirection(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y,
            coord.x,
            coord.y,
            ((FieldActor*)g_curScript->actor)->direction
        );
        if ((side == 0 && invert == 0) || (side != 0 && invert == 1)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party is at the actor's trigger range (always, unless
// inverted, while the field marker is set).
RVA(0x00034ff0, 0x7a)
void OpJumpUnlessPlayerAtRange(i16 invert) {
    i32 jump = 0;
    i16 range = g_curScript->actor->triggerRange;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == 0;
    } else {
        i16 distance = DistanceToParty((FieldActor*)g_curScript->actor);
        if ((range != distance && invert) || (range == distance && !invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035070, 0x7b)
void OpJumpUnlessActorVisible(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (GetFieldMarker()) {
        jump = invert == 0;
    } else {
        b16 view = GetPartyView(
            ((FieldActor*)g_curScript->actor)->pos.x,
            ((FieldActor*)g_curScript->actor)->pos.y
        );
        if ((invert == 0 && view) || (invert == 1 && !view)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000350f0, 0x44)
void OpJumpUnlessInRoster(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 slot = RosterSlotOfId(ReadObjectId());
    if ((slot >= 0 && !invert) || (slot < 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the roster holds the roster capacity less 6 entries or more.
RVA(0x00035140, 0x4c)
void OpJumpUnlessRosterFull(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountRosterEntries(1);
    i16 limit = GetRosterCapacity() - 6;
    if ((count >= limit && !invert) || (count < limit && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object's alignment agrees with the leader's.
RVA(0x00035190, 0x44)
void OpJumpUnlessAlignmentMatch(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 conflict = AlignmentConflicts(ReadScriptObject());
    if (ScriptBooleanMatches(!conflict, invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party's macca covers the object's rank score.
RVA(0x000351e0, 0x5b)
void OpJumpUnlessCanAfford(i16 invert) {
    i32 price = 0x7fffffff;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (object) {
        price = GetRankScore(object);
    }
    price -= GetObjectMacca(-1);
    if ((price <= 0 && !invert) || (price > 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035240, 0x44)
void OpJumpUnlessInParty(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 position = FindPartyPositionOfId(ReadObjectId());
    if ((position >= 0 && !invert) || (position < 0 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035290, 0x3f)
void OpJumpUnlessRosterHasNoDemons(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(0);
    if (ScriptBooleanMatches(!demons, invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has no condition (no object counts as healthy
// only when inverted).
RVA(0x000352d0, 0x62)
void OpJumpUnlessHealthy(i16 invert) {
    i16 conditions = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    if (!object && invert) {
        jump = 1;
    } else {
        if (object) {
            AccumulateConditionBits(GetCharacterConditions(object), conditions);
        }
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

static __inline Character* GetResolvedPartyCharacter(i16 id) {
    return GetRosterCharacterById(ResolveObjectId(id), 1);
}

// The same for the first of the companions -2, -3 and -7 in the roster.
// @early-stop: with no companion and no invert, retail re-zeroes the jump
// flag in its register before the call; every spelling here passes the
// known-zero pointer instead.
RVA(0x00035340, 0xb5)
void OpJumpUnlessCompanionHealthy(i16 invert) {
    i16 conditions = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* companion = GetResolvedPartyCharacter(-2);
    if (!companion) {
        companion = GetResolvedPartyCharacter(-3);
    }
    if (!companion) {
        companion = GetResolvedPartyCharacter(-7);
    }
    if (!companion && invert) {
        jump = 1;
    } else {
        if (!companion) {
            ScriptJumpUnless(target, jump);
            return;
        }
        AccumulateConditionBits(GetCharacterConditions(companion), conditions);
        if (ScriptBooleanMatches(!conditions, invert)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the player has an item in equipment slot 6.
RVA(0x00035400, 0x4d)
void OpJumpUnlessHeroEquipped(i16 invert) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* player = ResolveScriptObject(-1);
    if ((GetCharacterEquipment(player)[6].item != -1 && !invert)
        || (GetCharacterEquipment(player)[6].item == -1 && invert)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the script has no actor.
// @identity-TODO: the state is 3 with an actor and 0 without; what the
// values 0..2 against which it is tested stood for is unrecovered.
RVA(0x00035450, 0x4a)
void OpIfNoActor(i16 negate) {
    i16 state = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (GetScriptActor()) {
        state = 3;
    }
    if ((state < 2 && !negate) || (state > 2 && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party faces the operand's direction.
RVA(0x000354a0, 0x45)
void OpIfFacing(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if ((direction == g_field.pos.direction && !negate)
        || (direction != g_field.pos.direction && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// The same against the direction saved with the return position (with none
// saved, only when negated).
RVA(0x000354f0, 0x64)
void OpIfReturnFacing(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 direction = ReadScriptValue() & 3;
    if (g_savedDirection == -1) {
        if (negate) {
            jump = 1;
        }
    } else if ((direction == g_savedDirection && !negate)
               || (direction != g_savedDirection && negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the object has condition `condition` (no object counts as
// lacking it).
RVA(0x00035560, 0x63)
void OpIfObjectHasCondition(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    Character* object = ReadScriptObject();
    i16 has = ReadScriptValue();
    if (!object && negate) {
        jump = 1;
    } else {
        if (object) {
            has = HasCondition(GetCharacterConditions(object), has);
        }
        if (ScriptBooleanMatches(has, negate)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds the item.
RVA(0x000355d0, 0x44)
void OpIfHasItem(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 held = CountHeldItem(ReadScriptValue());
    if (ScriptBooleanMatches(held, negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Jumps unless the party holds every item of the -1-terminated list (with
// `negate`, unless it holds none of them).
RVA(0x00035620, 0x67)
void OpIfHasAllItems(i16 negate) {
    i16 all = -1;
    i16 any = 0;
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 item;
    for (item = ReadScriptValue(); item != -1; item = ReadScriptValue()) {
        i16 held = CountHeldItem(item) ? -1 : 0;
        all &= held;
        any |= held;
    }
    if ((!negate && all) || (negate && !any)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

// Puts one of the item into the bag (without the full-bag prompt).
RVA(0x00035690, 0x33)
void OpGiveItem(void) {
    i16 item = ReadScriptValue();
    if (item > 0) {
        i16 quiet = SetBagQuiet(0);
        StoreBagItem(item, 1, -1);
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
    ScriptPanel* node = CreateScriptPanel(0x118, 8, 0, 0);
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
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountPoolEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x000357c0, 0x3b)
void OpIfBagHasEntries(i16 negate) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 count = CountBagEntries();
    if (ScriptBooleanMatches(count, negate)) {
        jump = 1;
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
    i16 restore = ReadScriptValue();
    i16 var = ReadScriptValue();
    var = min(var, 0xb0);
    if (!restore) {
        SaveGemItems((ItemStack*)&g_scriptVars[var]);
        ResetGemItems(GetGemItemBase());
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], 0);
        ClearBag();
    } else {
        RestoreGemItems((ItemStack*)&g_scriptVars[var]);
        SaveOrRestoreBag((ItemStack*)&g_scriptVars[var + 16], 1);
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
        moved = count - StoreBagItem(item, count, -1);
    }
    if (count > 0 && moved < 0) {
        moved = 0;
    }
    SetBagQuiet(quiet);
    SetScriptLongVar(index, moved);
}

// Lists the bag entries holding items of `category` (0: any; 1..19 an item
// kind; 20 excludes scenario items, 21 also requires a price, 22 is priceless items)
// into a new array handle, with `spare` extra entries; stores the handle and
// the count.
RVA(0x00035920, 0x11a)
void OpListBagByCategory(void) {
    i16 listVar = ReadLongVarIndex();
    i16 countVar = ReadLongVarIndex();
    i16 category = ReadScriptValue();
    i16 spare = ReadScriptValue();
    // Retail's frame holds more than the bag's 64 entries (65 words fit).
    i16 entries[65];
    i16 count = 0;
    i16 i;
    i32 handle;
    i32* list;
    for (i = 0; i < 64; i++) {
        i16 item = GetBagItem(i);
        if (item < 1) {
            continue;
        }
        if (category >= 0 && category <= 19) {
            if (category != 0 && GetItemKind(item) != category) {
                continue;
            }
        } else {
            if ((category == 20 || category == 21) && GetItemKind(item) == ITEM_KIND_SCENARIO) {
                continue;
            }
            if (category == 21 && GetItemPrice(item) == 0) {
                continue;
            }
            if (category == 22 && GetItemPrice(item) != 0) {
                continue;
            }
        }
        entries[count++] = i;
    }
    count += spare;
    handle = CreateArrayHandle(count + spare, 4);
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
    entry->hasAttachment = 0;
    entry->item = -1;
    entry->attachment = 0;
}

// @early-stop: the item and its remapped id swap registers (esi/edi); the
// permuter's search is flat.
RVA(0x00035a90, 0x88)
void OpTakeDropSlot(void) {
    i16 slot = ReadScriptValue();
    i16 itemVar = ReadLongVarIndex();
    i16 amountVar = ReadLongVarIndex();
    i16 item = GetDropSlot(slot)->item;
    i16 amount = GetDropSlot(slot)->amount;
    i16 remapped;
    ClearDropSlot(slot);
    remapped = RemapItem(item);
    if (remapped) {
        amount = RollDropAmount(item, amount);
    } else {
        remapped = item;
    }
    SetScriptLongVar(itemVar, remapped);
    SetScriptLongVar(amountVar, amount);
}

RVA(0x00035b20, 0xf)
i16 OpCallSubScene(void) {
    PushGameState(0x26);
    return -3;
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
void OpOpenFusionScreen(i16 kind) {
    PushFusionMenu(kind, ReadLongVarIndex());
}

RVA(0x00035bb0, 0x1c)
void OpRunFusion(i16 triple) {
    SetBlankStep(1);
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
void OpJumpIf(i16 cond) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    if (cond == 0) {
        jump = 1;
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00035c00, 0x11)
void OpSkipJumpTarget(i16 unused) {
    i16 target = ReadBranchTarget();
    ScriptJumpUnless(target, 1);
}

// Jumps unless the roster's demon count is above `limit` (mode 0) or at most
// `limit` (mode 1).
RVA(0x00035c20, 0x45)
void OpIfDemonCount(i16 mode, i16 limit) {
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 demons = CountRosterEntries(0);
    if ((mode == 0 && demons > limit) || (mode == 1 && demons <= limit)) {
        jump = 1;
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
    AddMagnetite(ResolveScriptObject(-1), ReadScriptValue() * sign);
    DrawMoneyCounters(1);
}

RVA(0x00035cc0, 0x2c)
void OpAddMacca(i16 sign) {
    AddMacca(ResolveScriptObject(-1), ReadScriptValue() * sign);
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
        if (i == 0 && g_field.pos.area == 0x82 && g_field.pos.level == 5 && g_field.pos.x == 4
            && g_field.pos.y == 9 && g_field.pos.direction == 3 && move == 3) {
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
    g_worldMapRequest = -1;
}

// Sets the world-map layer and spot the world map opens at.
RVA(0x00035df0, 0x2c)
void OpSetWorldMapSpot(void) {
    i16 layer = ReadScriptValue();
    i16 x = ReadScriptValue();
    SetWorldMapSpot(layer, x, ReadScriptValue());
    g_worldMapRequest = 1;
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
    SetScriptLongVar(index, g_field.pos.area);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.level);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.x);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.y);
    index = ReadLongVarIndex();
    SetScriptLongVar(index, g_field.pos.direction);
}

RVA(0x00035ef0, 0x23)
void OpSetPlayerPosition(void) {
    i16 x = ReadScriptValue();
    i16 y = ReadScriptValue();
    MovePartyTo(x, y, ReadScriptValue());
}

RVA(0x00035f20, 0xe8)
void OpIfBlockedToward(i16 negate, i16 turn) {
    i32 matches = 0;
    i16 target = ReadBranchTarget();
    i16 x = g_field.pos.x;
    i16 y = g_field.pos.y;
    i16 direction = g_field.pos.direction;
    i16 blocked;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        blocked = 1;
    } else {
        blocked = GetMapWallKind(x, y, (direction + turn) & 3);
    }
    if (!blocked) {
        StepMapCoord(&x, &y, direction, turn);
        blocked = IsCellBlocked(g_field.pos.level, 1, x, y);
        if (!blocked && g_curScript->actor != NULL) {
            blocked = DistanceToParty((FieldActor*)g_curScript->actor) == 0;
        }
    }
    if ((!blocked && !negate) || (blocked && negate)) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

// Runs move command `effect` as a screen transition and refreshes the field.
RVA(0x00036010, 0x19)
i16 PlayScreenTransition(i16 effect) {
    RunMoveCommand(effect, 0);
    RequestFieldRefresh();
    return -3;
}

// Plays a screen transition, then redraws the field screen in one long frame
// with the status redraw locked.
RVA(0x00036030, 0x41)
i16 OpScreenTransition(void) {
    i16 result = PlayScreenTransition(ReadScriptValue());
    i16 lock = LockStatusRedraw(1);
    UpdateFieldScreen(0);
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
i16 OpAddToRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    Character* character;
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id != -1) {
            DespawnScriptActor();
            LoadScriptCharacterToRoster(id);
        }
        return -1;
    }
    if (id >= 0) {
        LoadScriptCharacterToRoster(id);
        return 0;
    }
    character = GetCharacter(ObjectSlotOfId(ref));
    if (RosterSlotOfId(character->id) == -1) {
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    return 0;
}

RVA(0x00036160, 0x50)
void AddScriptCharacterToRoster(Character* character, i16 unused) {
    if (AddToRoster(character) < 0) {
        g_rosterPendingMember = character;
        SaveRosterReturnState();
        SetGameState(0x28);
        SetGamePhase(0);
    }
}

RVA(0x000361b0, 0x97)
i16 OpRemoveFromRoster(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (id >= 3000) {
        id = ref - 3000;
    } else if (id >= 2000) {
        RemoveFromRoster(id - 2000);
        return 0;
    } else if (id >= 1000) {
        i16 slot = GetPartySlot(id - 1000);
        if (slot != -1) {
            RemoveFromRoster(slot);
        }
        return 0;
    } else {
        if (id == -17 || id == -19) {
            return -1;
        }
        if (id == -18) {
            id = GetScriptActorId();
            if (id == -1) {
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
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (id >= 2000) {
        id = GetRosterId(id - 2000);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id == -1) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    if (id < 32 && RosterSlotOfId(id) == -1) {
        Character* character = FindCharacterById(id);
        AddScriptCharacterToRoster(character, 3);
        SetAnalyzed(character->id, 1);
        SortRoster();
    }
    slot = RosterSlotOfId(id);
    if (id >= 32 || FindPartyPositionOfId(id) == -1) {
        AddToParty(slot);
        RequestFieldRefresh();
    }
    return 0;
}

RVA(0x00036330, 0xa0)
i16 OpLeaveActiveParty(void) {
    i16 ref = ReadObjectRef();
    i16 id = ref;
    if (ref >= 3000) {
        id = ref - 3000;
    }
    if (id >= 2000) {
        id = GetRosterId(id - 2000);
        if (id < 0) {
            return -1;
        }
    }
    if (ref == -17 || ref == -19) {
        id = GetScriptActorId();
        if (id == -1) {
            return -1;
        }
    }
    if (id < 0) {
        id = ResolveObjectId(id);
    }
    RemoveFromParty(FindRosterSlotById(id));
    if (id < 32) {
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
    if (id == -20) {
        id = g_actorId;
    } else if (id == -21) {
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
    i32 matches;
    if (id == -20) {
        id = g_actorId;
    } else if (id == -21) {
        id = g_targetId;
    } else if (id == -16) {
        id = GetPartySlot(FindFavouredMember());
        if (id >= 0) {
            id = -1 - id;
        }
    }
    matches = 0;
    if ((id < 0 && !negate) || (id >= 0 && negate)) {
        matches = 1;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x00036560, 0x79)
void OpRebalanceMemberStats(void) {
    Character* character = GetRosterCharacter(ReadScriptValue());
    if (character) {
        i16 i;
        for (i = 0; i < 11; i++) {
            i16 sum = character->stats.bonus[i] + character->stats.equipment[i]
                      + GetBaseStat(character, i) + character->stats.modifiers[i];
            if (HasCondition(GetCharacterConditions(character), 8)) {
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
    SwitchOnValue(GetMoonPhase() + 1, call, 0);
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
char* GetTextToken(i16 kind, i16 byId, i16 id) {
    const char* text = NULL;
    Character* object;
    s_tokenText[0] = '\0';
    switch (kind) {
        case 0:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            FormatFullName(s_tokenText, object);
            return s_tokenText;
        case 1:
        case 2:
            if (byId != 1) {
                object = ResolveScriptObject(id);
                if (object != NULL) {
                    id = object->id;
                }
            }
            text = GetDemonRaceName(id);
            break;
        case 3:
            if (byId != 1) {
                object = ResolveScriptObject(id);
                if (object != NULL) {
                    id = object->id;
                }
            }
            text = GetDemonPantheonName(id);
            break;
        case 4:
            text = GetLoadedRecordName(id);
            break;
        case 7:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = object->namePrefix;
            break;
        case 8:
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
        case 9:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_bloodTypes[object->bloodType];
            break;
        case 10:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_signNames[object->sign];
            break;
        case 11:
            object = ResolveScriptObject(id);
            if (object == NULL) {
                return s_tokenText;
            }
            text = s_affiliationNames[GetCharacterAffiliation(object, 0)];
            break;
        case 12:
            text = GetConditionName(g_statusCondition);
            break;
        case 15:
            text = GetLoadedRecordName(id);
            break;
        case 16:
            text = GetDemonClassName(id);
            break;
        case 17:
            text = GetSkillName(id);
            break;
        case 18:
            text = GetConditionName(id);
            break;
        case 19:
            text = GetConditionName(g_statusCondition);
            break;
        case 5:
        case 6:
        case 13:
        case 14:
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
    byId = 0;
    switch (kind) {
        case 0:
            object = ReadScriptObject();
            if (object) {
                FormatFullName(s_tokenText, object);
            }
            ExchangeObjectCheckBypass(bypass);
            return s_tokenText;
        case 1:
            goto readIndexedToken;
        case 2:
            goto readIndexedToken;
        case 3:
            goto readIndexedToken;
        case 7:
            goto readIndexedToken;
        case 8:
            goto readIndexedToken;
        case 9:
            goto readIndexedToken;
        case 10:
            goto readIndexedToken;
        case 11:
        readIndexedToken:
            byId = ReadScriptByte();
            id = ReadScriptValue();
            break;
        case 4:
            id = ReadScriptValue();
            if (id == 0) {
                id = GetScriptLongVar(11);
            }
            break;
        case 15:
            ReadScriptValue();
            id = g_actionId;
            object = GetCombatant(g_actorId);
            if (object && g_actorId < 0 && (object->pickFlags & PICK_ITEM_SKILL)) {
                id = object->pickItem;
            }
            break;
        case 16:
            id = ReadObjectId();
            break;
        case 17:
            ReadScriptValue();
            id = g_actionId;
            object = GetCombatant(g_actorId);
            if (object && g_actorId < 0 && (object->pickFlags & PICK_ITEM_SKILL)) {
                id = object->pickItem;
            }
            break;
        case 18:
            id = GetFirstConditionIndex(ReadScriptObject());
            break;
        case 5:
            goto readTokenValue;
        case 6:
            goto readTokenValue;
        case 12:
            goto readTokenValue;
        case 13:
            goto readTokenValue;
        case 14:
            goto readTokenValue;
        case 19:
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
    i16 kind = ReadScriptByte();
    Character* object;
    i8 byteValue;
    i16 wordValue;

    switch (kind) {
        case 0:
            s_operand = ReadScriptByte();
            return &s_operand;
        case 2:
            s_operand = ReadScriptLong();
            return &s_operand;
        case 3:
            s_operand = GetScriptLongVar(ReadScriptByte());
            return &s_operand;
        case 4:
            byteValue = ReadScriptByte();
            s_operand = byteValue;
            return &s_operand;
        case 5:
            wordValue = ReadScriptWord();
            s_operand = wordValue;
            return &s_operand;
        case 6:
            s_operand = GetItemPrice(ReadScriptValue());
            return &s_operand;
        case 7:
            s_operand = GetItemPrice(ReadScriptValue()) / 4;
            return &s_operand;
        case 9:
            ReadScriptWord();
            s_operand = g_rolledMacca;
            return &s_operand;
        case 10:
            s_operand = GetObjectMacca(ReadObjectRef());
            return &s_operand;
        case 11:
            ReadScriptWord();
            s_operand = g_rolledMagnetite;
            return &s_operand;
        case 12:
            s_operand = GetObjectMagnetite(ReadObjectRef());
            return &s_operand;
        case 1:
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
            s_operand = ReadScriptWord();
            return &s_operand;
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
        case 35:
            s_operand = GetObjectStatTotal(ReadObjectRef(), kind - 25);
            return &s_operand;
        case 36:
            s_operand = GetObjectLevel(ReadObjectRef());
            return &s_operand;
        case 37:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->levelGap;
            return &s_operand;
        case 38:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->familiarity;
            if (IsEventFlagSet(2, 8)) {
                break;
            }
            s_operand += 2;
            return &s_operand;
        case 39:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetFamiliarityCount(object->id);
            return &s_operand;
        case 40:
            ReadScriptByte();
            s_operand = g_rolledMacca;
            strcpy(g_numberUnit, "\203}\203b\203J"); // マッカ
            return &s_operand;
        case 41:
            ReadScriptByte();
            s_operand = g_rolledMagnetite;
            strcpy(g_numberUnit, "\202l\202`\202f"); // ＭＡＧ
            return &s_operand;
        case 42:
            ReadScriptWord();
            return &s_operand;
        case 43:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.hp.cur;
            return &s_operand;
        case 44:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.mp.cur;
            return &s_operand;
        case 45:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.hp.max;
            return &s_operand;
        case 46:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->pools.mp.max;
            return &s_operand;
        case 47:
            s_operand = GetObjectAlignmentLevelB(ReadObjectRef());
            return &s_operand;
        case 48:
            s_operand = GetObjectAlignmentLevelA(ReadObjectRef());
            return &s_operand;
        case 49:
            s_operand = g_statusCondition;
            return &s_operand;
        case 56:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->id;
            return &s_operand;
        case 57:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatShown(object, BATTLE_STAT_WEAPON_DEFENSE);
            return &s_operand;
        case 58:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, 18);
            return &s_operand;
        case 59:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[5].item;
            return &s_operand;
        case 60:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[6].item;
            return &s_operand;
        case 61:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[7].item;
            return &s_operand;
        case 62:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetDemonRace(object->id);
            return &s_operand;
        case 63:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, 0);
            return &s_operand;
        case 64:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, 6);
            return &s_operand;
        case 65:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetBattleStatBase(object, 12);
            return &s_operand;
        case 66:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[0].item;
            return &s_operand;
        case 67:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[1].item;
            return &s_operand;
        case 68:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[2].item;
            return &s_operand;
        case 69:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[3].item;
            return &s_operand;
        case 70:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[4].item;
            return &s_operand;
        case 71:
        case 72:
        case 73:
        case 74:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetTrainingPoints(object, kind - 71);
            return &s_operand;
        case 75:
            s_operand = GetActionValue(ReadScriptValue());
            return &s_operand;
        case 76:
            ReadScriptValue();
            s_operand = g_hpChange;
            return &s_operand;
        case 77:
            ReadScriptValue();
            s_operand = g_mpChange;
            return &s_operand;
        case 78:
            if (ReadScriptValue() == 0) {
                s_operand = g_statusCondition;
                return &s_operand;
            }
            s_operand = g_effectCondition;
            return &s_operand;
        case 79:
            s_operand = GetBattleResultValue(ReadScriptValue());
            return &s_operand;
        case 80:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->experience;
            return &s_operand;
        case 81:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->levelBonus;
            return &s_operand;
        case 82:
        case 83:
        case 84:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterAffiliation(object, kind - 82);
            return &s_operand;
        case 85:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = object->title;
            return &s_operand;
        case 86:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[5].value;
            return &s_operand;
        case 87:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[6].value;
            return &s_operand;
        case 88:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[7].value;
            return &s_operand;
        case 89:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[0].value;
            return &s_operand;
        case 90:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[1].value;
            return &s_operand;
        case 91:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[2].value;
            return &s_operand;
        case 92:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[3].value;
            return &s_operand;
        case 93:
            object = ReadScriptObject();
            if (object == NULL) {
                break;
            }
            s_operand = GetCharacterEquipment(object)[4].value;
            break;
    }
    return &s_operand;
}

RVA(0x000373e0, 0x54)
i32 GetBattleResultValue(i16 which) {
    switch (which) {
        case 0:
            return g_rewardMacca;
        case 1:
            return g_rewardMagnetite;
        case 2:
            return 2u * g_rewardExperience / CountPartyMembers(1);
        case 3:
            return GetDropSlot(0)->item;
    }
    return 0;
}

RVA(0x00037440, 0x44)
i32 GetActionValue(i16 which) {
    switch (which) {
        case 0:
            return g_actionResult;
        case 1:
            return g_hpChange;
        case 2:
            return g_mpChange;
        case 3:
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
void ReadContestValues(i16 stat, i32* own, i32* other, i16 swap) {
    Character* object;
    i32 kept;

    switch (stat) {
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
            *own = GetObjectStatTotal(-1, stat);
            *other = GetObjectStatTotal(-17, stat);
            break;
        case 11:
            *own = GetObjectLevel(-1);
            *other = GetObjectLevel(-17);
            break;
        case 12:
            object = ResolveScriptObject(-17);
            if (object == NULL) {
                *own = 0;
            } else {
                *own = object->levelGap;
            }
            *other = -1;
            break;
        case 13:
            object = ResolveScriptObject(-17);
            if (object == NULL) {
                *own = 0;
            } else {
                *own = object->familiarity;
            }
            *other = -1;
            break;
        case 14:
            object = ResolveScriptObject(-17);
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
    i16 kind = ReadScriptByte();
    i32 value = ReadScriptValue();

    if (object == NULL) {
        return;
    }
    switch (kind) {
        case 10:
            object->macca = value;
            return;
        case 12:
            object->magnetite = value;
            return;
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
        case 35:
            SetStatTotal(object, kind - 25, value);
            return;
        case 36:
            object->level = value;
            return;
        case 37:
            object->levelGap = value;
            return;
        case 38:
            object->familiarity = value;
            return;
        case 43:
            object->pools.hp.cur = value;
            return;
        case 44:
            object->pools.mp.cur = value;
            return;
        case 45:
            object->pools.hp.max = value;
            return;
        case 46:
            object->pools.mp.max = value;
            return;
        case 47:
            object->alignmentLevelB = value;
            return;
        case 48:
            object->alignmentLevelA = value;
            return;
        case 56:
            object->id = value;
            return;
        case 57:
            object->battleStatsShown[BATTLE_STAT_WEAPON_DEFENSE] = value;
            return;
        case 58:
            object->battleStats[18] = value;
            return;
        case 59:
            GetCharacterEquipment(object)[5].item = value;
            return;
        case 60:
            GetCharacterEquipment(object)[6].item = value;
            return;
        case 61:
            GetCharacterEquipment(object)[7].item = value;
            GetCharacterEquipment(object)[7].quantity =
                GetGunMagazineSize(GetLoadedRecord(GetCharacterEquipment(object)[6].item));
            return;
        case 63:
            object->battleStats[0] = value;
            return;
        case 64:
            object->battleStats[6] = value;
            return;
        case 65:
            object->battleStats[12] = value;
            return;
        case 66:
            GetCharacterEquipment(object)[0].item = value;
            return;
        case 67:
            GetCharacterEquipment(object)[1].item = value;
            return;
        case 68:
            GetCharacterEquipment(object)[2].item = value;
            return;
        case 69:
            GetCharacterEquipment(object)[3].item = value;
            return;
        case 70:
            GetCharacterEquipment(object)[4].item = value;
            return;
        case 71:
        case 72:
        case 73:
        case 74:
            object->trainingPoints[kind - 71] = (i16)value;
            return;
        case 82:
        case 83:
        case 84:
            SetCharacterAffiliation(object, kind - 82, value);
            if (object->id == 0) {
                RaiseAffiliationLevels(object);
                if (value == 3) {
                    LearnAllSkills(object, kind - 82);
                }
            }
            return;
        case 85:
            object->title = value;
            return;
        case 86:
            GetCharacterEquipment(object)[5].value = value;
            return;
        case 87:
            GetCharacterEquipment(object)[6].value = value;
            return;
        case 88:
            GetCharacterEquipment(object)[7].value = value;
            return;
        case 89:
            GetCharacterEquipment(object)[0].value = value;
            return;
        case 90:
            GetCharacterEquipment(object)[1].value = value;
            return;
        case 91:
            GetCharacterEquipment(object)[2].value = value;
            return;
        case 92:
            GetCharacterEquipment(object)[3].value = value;
            return;
        case 93:
            GetCharacterEquipment(object)[4].value = value;
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
        while (slot >= 0 && slot < 32) {
            slot = FindMemberByPoolState(slot, mode, state, pools);
            if (slot != -1) {
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
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = RosterMemberAt(slot);
                if (character && HasCondition(GetCharacterConditions(character), condition)) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
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
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassB(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
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
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
                character = GetRosterCharacter(slot);
                if (character && GetAlignmentClassA(character) == alignment) {
                    result = slot;
                    break;
                }
            }
        }
    } else {
        result = 0;
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
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
        for (slot = 0; slot < 32; slot++) {
            if (FilterPartyMember(slot, mode) != -1) {
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
    i32 jump = 0;
    i16 target = ReadBranchTarget();
    i16 expect = ReadScriptValue();
    Character* character = GetRosterCharacter(ReadScriptValue());
    // The condition, then whether the member has it (no member: tested as
    // read).
    i16 has = ReadScriptValue();
    if (!character && expect) {
        jump = 1;
    } else {
        if (character) {
            has = HasCondition(GetCharacterConditions(character), has);
        }
        if (ScriptBooleanMatches(has, expect)) {
            jump = 1;
        }
    }
    ScriptJumpUnless(target, jump);
}

RVA(0x00037e80, 0x18)
void OpSaveObjectConditions(void) {
    Character* object = ReadScriptObject();
    if (object) {
        SetFlagTag(GetCharacterConditions(object));
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
i32 GetObjectStatTotal(i16 ref, i16 stat) {
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
    return object->alignmentLevelB;
}

RVA(0x00037f60, 0x17)
i32 GetObjectAlignmentLevelA(i16 ref) {
    Character* object = ResolveScriptObject(ref);
    if (!object) {
        return 0;
    }
    return object->alignmentLevelA;
}

// @identity-TODO: maps a script mode operand 0/1/2 to the step +1/0/-1 the
// caller passes on; the mode's meaning is unrecovered.
RVA(0x00037f80, 0x22)
i16 StepForMode(i16 mode) {
    switch (mode) {
        case 0:
            return 1;
        case 1:
            return 0;
        case 2:
            return -1;
    }
    return 0;
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
    for (i = 0; i < 32; i++) {
        if (FilterPartyMember(i, mode) != -1 && RosterMemberAt(i)
            && race == GetDemonRace(RosterMemberAt(i)->id)) {
            mask |= PowerOfTwo(i);
        }
    }
    SetScriptLongVar(index, mask);
}

// Fills pool `pool` (1: HP, else MP) by `amount` for each roster slot in the
// mask.
// @early-stop: retail loads the mask into a register before testing it
// against the bit; no spelling of the test reproduces that (the permuter's
// search is flat).
RVA(0x000382b0, 0x67)
void OpRecoverRosterPool(i16 pool) {
    u32 bit = 1;
    u32 mask = ReadScriptValue();
    i16 amount = ReadScriptValue();
    i16 i;
    for (i = 0; i < 32; i++) {
        if (RosterMemberAt(i) && (mask & bit)) {
            if (pool == 1) {
                FillPool(&RosterMemberAt(i)->pools.hp, amount, POOL_FILL_TO_MAX);
            } else {
                FillPool(&RosterMemberAt(i)->pools.mp, amount, POOL_FILL_TO_MAX);
            }
        }
        bit <<= 1;
    }
}

// Clears condition `condition` from each roster slot in the mask that has it.
// @early-stop: the same mask load as OpRecoverRosterPool.
RVA(0x00038320, 0x5f)
void OpCureRosterCondition(void) {
    u32 bit = 1;
    u32 mask = ReadScriptValue();
    i16 condition = ReadScriptValue();
    i16 i;
    for (i = 0; i < 32; i++) {
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
    PrintWindowText(window, text, 0x400, 0, 1);
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
    PushGameState(6);
    return NULL;
}

RVA(0x000384a0, 0x4c)
void InitScriptChoiceMenu(ScriptChoice* choices, i16 window, i16 keep, i16 cancelMode) {
    SetScriptChoiceMenu(choices, window, keep, cancelMode);
    if (g_mouseLeftClick) {
        g_mouseLeftClick = 0;
    }
    s_hoveredChoice = -1;
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
        return -1;
    }
    hovered = FindScriptChoiceAtMouse();
    if (hovered != s_hoveredChoice) {
        ToggleScriptChoiceHighlight();
        s_hoveredChoice = hovered;
        s_highlightedChoice = s_hitChoice;
        ToggleScriptChoiceHighlight();
    }
    if (!TakeMouseLeftClick()) {
        return 0;
    }
    if (s_hoveredChoice == -1) {
        return 0;
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
    return 1;
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
    return -1;
}

RVA(0x00038790, 0x53)
b16 RunScriptChoiceState(void) {
    switch (GetGameSub()) {
        case 0:
            NextGameSub();
            InitScriptChoiceMenu(s_choiceMenu, s_choiceWindow, s_keepChoices, s_choiceCancelMode);
        case 1:
            if (PollScriptChoiceMenu()) {
                ReturnFromGameState();
            }
            break;
    }
    return false;
}
