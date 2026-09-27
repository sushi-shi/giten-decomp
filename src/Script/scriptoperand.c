// @identity-TODO: the owning TU is unproven; this unit holds the script
// operand reader's span until link-order evidence names it.

#include <rva.h>

#include <Game/AnalyzeData.h>
#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
#include <Game/Familiarity.h>
#include <Game/FieldMain.h>
#include <Game/Growth.h>
#include <Game/ItemRecord.h>
#include <Game/LevelUp.h>
#include <Game/Party.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>

#include <stddef.h>
#include <string.h>

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
