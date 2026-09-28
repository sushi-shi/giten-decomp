// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/BattleEffect.h>
#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/DemonTable.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/ItemRecord.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Script/TextToken.h>

#include <stddef.h>
#include <string.h>

// The expansion of the last text token.
DATA(0x00081230)
static char s_tokenText[0x100];

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
