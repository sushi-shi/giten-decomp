// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldMain.h>
#include <Game/GameState.h>
#include <Game/SaveGame.h>
#include <Game/WorldMap.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptText.h>
#include <Script/ScriptVars.h>
#include <Util/Scratch.h>

#include <stddef.h>

DATA(0x00091180)
i16 g_scriptRegs[16];

RVA(0x0002fd00, 0x174)
b16 OpSaveDataCommand(void) {
    ReturnPoint savedPoint;
    ReturnPoint point;
    GZ_ENUM_LOCAL(SaveDataOperation, i16) operation = ReadScriptValue();
    i16 slot = ReadScriptValue();
    i16 variable = ReadLongVarIndex();
    i16 field = ReadScriptValue();
    i16 result = 0;
    switch (operation) {
        case SAVE_DATA_SUMMARY:
            ReadSaveSummary(slot, field);
            result = 1;
            break;
        case SAVE_DATA_SAVE:
            if (GetReturnPoint(&point) >= WORLD_MAP_REQUEST_NONE) {
                result = SaveGame(slot);
            } else {
                savedPoint.area = g_party.field.pos.area;
                savedPoint.level = g_party.field.pos.level;
                savedPoint.x = g_party.field.pos.x;
                savedPoint.y = g_party.field.pos.y;
                savedPoint.direction = g_party.field.pos.direction;
                g_party.field.pos.area = point.area;
                g_party.field.pos.level = point.level;
                g_party.field.pos.x = point.x;
                g_party.field.pos.y = point.y;
                g_party.field.pos.direction = point.direction;
                result = SaveGame(slot);
                g_party.field.pos.area = savedPoint.area;
                g_party.field.pos.level = savedPoint.level;
                g_party.field.pos.x = savedPoint.x;
                g_party.field.pos.y = savedPoint.y;
                g_party.field.pos.direction = savedPoint.direction;
            }
            break;
        case SAVE_DATA_LOAD:
            result = LoadGame(slot, true);
            break;
        case SAVE_DATA_SUMMARY_TEXT:
            EndSaveRenderMode();
            result = ReadSaveSummary(slot, field);
            if (result >= 0) {
                SetCapturedText(g_scratchBuffer);
            }
            break;
    }
    SetScriptLongVar(variable, result);
    return false;
}

RVA(0x0002fe80, 0x24)
GZ_ENUM_RETURN(ScriptStatus, i16) SetActorMode(GZ_ENUM_PARAM(ActorMode, i16) mode) {
    Character* actor = g_curScript->actor;
    if (actor != NULL) {
        actor->mode = (u8)mode;
        g_scriptRegs[0] = 1;
    }
    return SCRIPT_END;
}

RVA(0x0002feb0, 0x82)
void OpIfFlags(b16 all) {
    u16 bank, index;
    i16 target = ReadBranchTarget();
    i16 every = -1;
    i16 any = 0;
    i16 invert;
    i16 matched;
    b32 skip;
    for (;;) {
        invert = ReadFlagOperand(&bank, &index);
        if (invert == FLAG_OPERAND_NEGATED && bank == FLAG_BANK_MASK) {
            break;
        }
        matched = (TestEventFlag(bank, index) != false) ^ (invert & 1);
        any |= matched;
        every &= matched;
    }
    skip = false;
    if (all) {
        if (!every) {
            skip = true;
        }
    } else if (!any) {
        skip = true;
    }
    ScriptJumpUnless(target, skip);
}
