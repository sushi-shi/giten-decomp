// @identity-TODO: the owning TU is unproven; this unit holds the field
// opcodes' contiguous retail span until link-order evidence names it.

#include <rva.h>

#include <Game/BattleEffect.h>
#include <Game/Condition.h>
#include <Game/Field.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/StateStack.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptState.h>
#include <Script/ScriptVars.h>

RVA(0x000324b0, 0xae)
void OpEnterFieldMap(void) {
    i16 query = ReadScriptByte();
    if (query) {
        i16 index = ReadLongVarIndex();
        SetScriptLongVar(index, GetFieldEntryState());
    } else {
        i16 map = ReadScriptValue();
        i16 hold = 1 - ReadScriptValue();
        i16 countA = ReadScriptValue();
        i16 rateA = ReadScriptValue();
        i16 countB = ReadScriptValue();
        i16 rateB = ReadScriptValue();
        SetGameStep(SCRIPT_SCENE_STEP_RESUME_FIELD_MAP);
        SetGameSub(ExchangeViewHold(hold));
        ThawObjectsForScript();
        EnterFieldMap(map, countA, rateA, countB, rateB, FIELD_MAP_SCRIPT_EVENT);
        HoldScriptFiles();
        SaveScriptState();
    }
}

// @early-stop register allocation: retail keeps the predicate in ecx and
// the actor ids in dx/si; this build uses esi and cx/dx. State locals,
// flag widths and declaration order do not recover that allocation.
RVA(0x00032560, 0x44)
void OpIfEventObjectIs(void) {
    i16 target = ReadBranchTarget();
    i16 different = ReadScriptValue();
    b32 matches = false;
    if ((g_actorId == g_targetId && !different) || (g_actorId != g_targetId && different)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x000325b0, 0x3e)
void OpIfBattleResult(void) {
    i16 target = ReadBranchTarget();
    i16 multiple = ReadScriptValue();
    b32 matches = false;
    if ((g_targetCount == 1 && multiple == false) || (g_targetCount >= 2 && multiple == true)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x000325f0, 0x62)
i16 OpCountObjectsAt(void) {
    i16 index = ReadLongVarIndex();
    i16 ref = ReadObjectRef();
    i16 mode = ReadScriptValue();
    MapCoord point = ResolveScriptObjectCoord(ref);
    i16 count = CountObjectsAt(point.x, point.y, mode + 1, ResolveScriptObject(ref)->id);
    SetScriptLongVar(index, count);
    return count;
}

// @early-stop register allocation: the status, invert and predicate values
// occupy a different register permutation; the control-flow edges agree.
RVA(0x00032660, 0x39)
void OpIfStatusPositive(i16 invert) {
    i16 target = ReadBranchTarget();
    b32 matches = false;
    if ((g_statusCondition > 0 && !invert) || (g_statusCondition == INFLICT_NONE && invert)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

// @early-stop register allocation: retail holds the active state in dx and
// the predicate in ecx; this build swaps them. Explicit state locals are flat.
RVA(0x000326a0, 0x3b)
void OpIfInBattle(void) {
    i16 target = ReadBranchTarget();
    i16 invert = ReadScriptValue();
    b32 matches = false;
    if (ScriptBooleanMatches(g_fieldBattleActive, invert)) {
        matches = true;
    }
    ScriptJumpUnless(target, matches);
}

RVA(0x000326e0, 0x17)
void OpGetBattleOutcome(void) {
    i16 index = ReadLongVarIndex();
    SetScriptLongVar(index, g_battleOutcome);
}

RVA(0x00032700, 0xf)
void OpSetFieldOption(void) {
    ExchangeFieldOption(ReadScriptValue());
}

RVA(0x00032710, 0x23)
void OpSetFieldParams(void) {
    i16 first = ReadScriptValue();
    i16 second = ReadScriptValue();
    SetFieldParams(first, second, ReadScriptValue());
}
