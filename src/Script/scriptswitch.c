// @identity-TODO: the owning TU is unproven; this unit holds the switch
// opcodes' contiguous retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Alignment.h>
#include <Input/Mouse.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptCmd.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptSwitchEncoding.h>
#include <Util/Range.h>

RVA(0x00032740, 0x79)
void SwitchOnValue(u8 value, i16 call, i16 exactMatch) {
    i16 entry;
    // The parameter's word is reused as the selected target after its mode is read.
    i16 selected = ReadScriptSwitch(value, &exactMatch, &entry, exactMatch);
    if (selected > SCRIPT_SWITCH_END) {
        if (call) {
            PushCallFrame(g_curScript, 0);
        }
        ScriptJump(exactMatch);
    } else if (selected != SCRIPT_SWITCH_END) {
        if (!call) {
            GotoScript(exactMatch, entry);
        } else {
            CallScript(exactMatch, entry);
        }
    }
}

RVA(0x000327c0, 0xfe)
i16 ReadScriptSwitch(u8 value, i16* target, i16* entry, i16 exactMatch) {
    i16 selected = 255;
    u8 localJump = 0;
    u8 key;
    if (exactMatch) {
        for (key = ReadScriptByte(); key != 255; key = ReadScriptByte()) {
            if (value != key) {
                ReadScriptByte();
                ReadScriptWord();
            } else {
                localJump = ReadScriptByte();
                if (localJump) {
                    *target = ReadJumpTarget();
                } else {
                    ReadScriptBytePair(target, entry);
                }
                selected = key;
            }
        }
        if (localJump) {
            selected |= SCRIPT_SWITCH_LOCAL_JUMP;
        }
        return selected;
    } else {
        for (key = ReadScriptByte(); key != SCRIPT_SWITCH_END; key = ReadScriptByte()) {
            if (value <= key && selected == SCRIPT_SWITCH_END) {
                localJump = ReadScriptByte();
                if (localJump) {
                    *target = ReadJumpTarget();
                } else {
                    ReadScriptBytePair(target, entry);
                }
                selected = key;
            } else {
                ReadScriptByte();
                ReadScriptWord();
            }
        }
    }
    if (localJump) {
        selected |= SCRIPT_SWITCH_LOCAL_JUMP;
    }
    return selected;
}

RVA(0x000328c0, 0x1f)
void OpSwitchOnRandom(i16 call) {
    SwitchOnValue(RandomAverage(1, 100, 0), call, 0);
}

RVA(0x000328e0, 0x17)
void OpSwitchOnSelection(i16 call) {
    SwitchOnValue(g_hoveredObjectId, call, 1);
}

RVA(0x00032900, 0x2f)
void OpSwitchOnAlignmentA(i16 call) {
    i16 alignment = AlignmentClass(GetObjectAlignmentLevelB(ReadObjectRef()));
    alignment = 1 - alignment;
    SwitchOnValue(alignment, call, 0);
}

RVA(0x00032930, 0x2f)
void OpSwitchOnAlignmentB(i16 call) {
    i16 alignment = AlignmentClass(GetObjectAlignmentLevelA(ReadObjectRef()));
    alignment = 1 - alignment;
    SwitchOnValue(alignment, call, 0);
}

RVA(0x00032960, 0x16)
void OpSwitchOnRange(i16 call) {
    SwitchOnValue(ReadScriptValue(), call, 0);
}

RVA(0x00032980, 0x20)
void OpSwitchOnActorAttrA(i16 call) {
    SwitchOnValue(g_curScript->actor->attitude, call, 1);
}

RVA(0x000329a0, 0x20)
void OpSwitchOnActorAttrB(i16 call) {
    SwitchOnValue(g_curScript->actor->fieldState, call, 1);
}

RVA(0x000329c0, 0x16)
void OpSwitchOnValue(i16 call) {
    SwitchOnValue(ReadScriptValue(), call, 1);
}
