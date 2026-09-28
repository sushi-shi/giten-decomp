// @identity-TODO: the owning TU is unproven. One retail object: the text
// tokens, the script operand reader and the script choice commands. In .data
// the choice command's hovered word precedes the token tables, against .text
// order, and in .bss the operand reader's static follows the choice
// commands' statics; each static is read only by its own part's code.

#include <rva.h>

#include <Game/Alignment.h>
#include <Game/AnalyzeData.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
#include <Game/Familiarity.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/ItemPool.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Game/StateStack.h>
#include <Game/Stats.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Script/TextToken.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Text/TextPlaneAttr.h>
#include <Text/WindowText.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

#include <stddef.h>
#include <string.h>

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
static char s_tokenText[0x100];

DATA(0x0008135c)
char g_shortNames[16][4] = {0};

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

// The slot ReadScriptOperand returns.
DATA(0x00081348)
static i32 s_operand;

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

DATA(0x00081330)
static i16 s_choiceWindow;
DATA(0x00081334)
static i16 s_keepChoices;
DATA(0x00081338)
static i16 s_choiceCancelMode;
DATA(0x0008133c)
static ScriptChoice* s_highlightedChoice;
DATA(0x00081340)
static ScriptChoice* s_choiceMenu;
DATA(0x00081344)
static ScriptChoice* s_hitChoice;

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
