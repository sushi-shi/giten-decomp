// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldMain.h>
#include <Game/GameState.h>
#include <Game/SaveGame.h>
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
    i16 savedPoint[5];
    i16 point[5];
    i16 operation = ReadScriptValue();
    i16 slot = ReadScriptValue();
    i16 variable = ReadLongVarIndex();
    i16 field = ReadScriptValue();
    i16 result = 0;
    switch (operation) {
        case 0:
            ReadSaveSummary(slot, field);
            result = 1;
            break;
        case 1:
            if (GetReturnPoint(point) >= 0) {
                result = SaveGame(slot);
            } else {
                savedPoint[0] = g_party.field.pos.area;
                savedPoint[1] = g_party.field.pos.level;
                savedPoint[2] = g_party.field.pos.x;
                savedPoint[3] = g_party.field.pos.y;
                savedPoint[4] = g_party.field.pos.direction;
                g_party.field.pos.area = point[0];
                g_party.field.pos.level = point[1];
                g_party.field.pos.x = point[2];
                g_party.field.pos.y = point[3];
                g_party.field.pos.direction = point[4];
                result = SaveGame(slot);
                g_party.field.pos.area = savedPoint[0];
                g_party.field.pos.level = savedPoint[1];
                g_party.field.pos.x = savedPoint[2];
                g_party.field.pos.y = savedPoint[3];
                g_party.field.pos.direction = savedPoint[4];
            }
            break;
        case 2:
            result = LoadGame(slot, 1);
            break;
        case 3:
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
i16 SetActorMode(i16 mode) {
    Character* actor = g_curScript->actor;
    if (actor != NULL) {
        actor->mode = (u8)mode;
        g_scriptRegs[0] = 1;
    }
    return -1;
}

RVA(0x0002feb0, 0x82)
void OpIfFlags(i16 all) {
    u16 bank, index;
    i16 target = ReadBranchTarget();
    i16 every = -1;
    i16 any = 0;
    i16 invert;
    i16 matched;
    i32 skip;
    for (;;) {
        invert = ReadFlagOperand(&bank, &index);
        if (invert == -1 && bank == 0x7f) {
            break;
        }
        matched = (TestEventFlag(bank, index) != 0) ^ (invert & 1);
        any |= matched;
        every &= matched;
    }
    skip = 0;
    if (all) {
        if (!every) {
            skip = 1;
        }
    } else if (!any) {
        skip = 1;
    }
    ScriptJumpUnless(target, skip);
}
