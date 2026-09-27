// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Script/EventFlags.h>
#include <Script/LongVar.h>
#include <Script/Script.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptVars.h>
#include <Util/BitSet.h>

#include <stddef.h>
#include <stdio.h>

DATA(0x000813a0)
FlagBank g_eventFlags[16];

RVA(0x000391b0, 0x1b)
void SetFlagBank(i16 bank) {
    i16 i;
    for (i = 0; i < 8; i++) {
        g_eventFlags[bank].words[i] = 0xffffffff;
    }
}

RVA(0x000391d0, 0x1a)
void ClearFlagBank(i16 bank) {
    i16 i;
    for (i = 0; i < 8; i++) {
        g_eventFlags[bank].words[i] = 0;
    }
}

// Bank 14 is the script actor's own flag bank when there is an actor.
RVA(0x000391f0, 0x52)
i32 ChangeEventFlag(u16 bank, u16 index, i16 op) {
    if (bank == 14 && g_curScript->actor != NULL) {
        return ChangeCharacterFlag(g_curScript->actor, index, op);
    }
    return ChangeBit(g_eventFlags[bank].bits, index, op);
}

RVA(0x00039250, 0x15)
i32 ClearEventFlag(u16 bank, u16 index) {
    return ChangeEventFlag(bank, index, 0);
}

// Sets every flag, then clears banks 4, 14 and 15 and flag 0.
RVA(0x00039270, 0x3f)
void ResetEventFlags(void) {
    i16 bank;
    for (bank = 0; bank < 16; bank++) {
        SetFlagBank(bank);
    }
    ClearFlagBank(4);
    ClearFlagBank(14);
    ClearFlagBank(15);
    ClearEventFlag(0, 0);
}

// Flag 0 of bank 0 always reads clear.
RVA(0x000392b0, 0x51)
i32 TestEventFlag(u16 bank, u16 index) {
    if (bank == 0 && index == 0) {
        return 0;
    }
    if (bank == 14 && g_curScript->actor != NULL) {
        return TestCharacterFlag(g_curScript->actor, index);
    }
    return TestBit(g_eventFlags[bank].bits, index);
}

RVA(0x00039310, 0x13)
i32 IsEventFlagSet(u16 bank, u16 index) {
    return TestEventFlag(bank, index);
}

RVA(0x00039330, 0x18)
i32 ModifyEventFlag(u16 bank, u16 index, i16 op) {
    return ChangeEventFlag(bank, index, op);
}

RVA(0x00039350, 0x15)
i32 SetEventFlag(u16 bank, u16 index) {
    return ChangeEventFlag(bank, index, 1);
}

RVA(0x00039370, 0x15)
i32 ToggleEventFlag(u16 bank, u16 index) {
    return ChangeEventFlag(bank, index, -1);
}

RVA(0x00039390, 0x6)
u32 GetFlagSettings(void) {
    return g_eventFlags[15].sys.packed;
}

RVA(0x000393a0, 0x1d)
void SetFlagSettings(u32 packed) {
    g_eventFlags[15].sys.packed =
        (g_eventFlags[15].sys.packed & 0x3fc0ffff) | (packed & ~0x3fc0ffff);
}

RVA(0x000393c0, 0x1a)
void SetFlagTag(u8* tag) {
    i16 i;
    for (i = 0; i < 5; i++) {
        g_eventFlags[15].sys.tag[i] = *tag++;
    }
}

// Reads a flag operand (bank byte with a negate bit, then the index); returns
// -1 when negated.
RVA(0x000393e0, 0x33)
i16 ReadFlagOperand(u16* bank, u16* index) {
    u16 first = ReadScriptByte();
    *index = ReadScriptByte();
    *bank = first & 0x7f;
    return (first & 0x80) ? -1 : 0;
}

// @identity-TODO: flag-condition words (bank 0..0x7e in bits 0-6, a negate
// bit 7, the index in the high byte; bank 0x7f with index 0xff is always set).
// The two evaluations differ only in what they return for a set, un-negated
// flag; which callers want which is unrecovered.
RVA(0x00039420, 0x52)
i16 CheckFlagWord(u16* cond) {
    i16 set;
    if ((*cond & 0x7f) == 0x7f && (*cond & 0xff00) == 0xff00) {
        set = 1;
    } else {
        set = TestEventFlag(*cond & 0x7f, *cond >> 8);
    }
    if ((set == 0 && !(*cond & 0x80)) || (set != 0 && (*cond & 0x80))) {
        return 1;
    }
    return set;
}

RVA(0x00039480, 0x5d)
i16 MatchFlagWord(u16* cond) {
    i16 set;
    if ((*cond & 0x7f) == 0x7f && (*cond & 0xff00) == 0xff00) {
        set = 1;
    } else {
        set = TestEventFlag(*cond & 0x7f, *cond >> 8);
    }
    if (set == 0 && !(*cond & 0x80)) {
        return 1;
    }
    if (set != 0 && (*cond & 0x80)) {
        return 1;
    }
    return 0;
}

// Whether flag `index` of `bank` (bits 0-6) is set, inverted by bit 7.
RVA(0x000394e0, 0x3f)
i16 MatchEventFlag(u16 bank, u16 index) {
    i16 negate = bank & 0x80;
    if (!TestEventFlag(bank & 0x7f, index)) {
        if (negate) {
            return 0;
        }
    } else if (!negate) {
        return 0;
    }
    return 1;
}

RVA(0x00039520, 0x3a)
i16 ReadAndMatchEventFlag(void) {
    u16 bank;
    u16 index;
    if (ReadFlagOperand(&bank, &index)) {
        bank |= 0x80;
    }
    return MatchEventFlag(bank, index);
}

// Script value operands pack a flag as bank | index << 8.
RVA(0x00039560, 0x2b)
void OpModifyEventFlagByValue(void) {
    i32 flag = ReadScriptValue();
    i32 op = ReadScriptValue();
    i8 bank = flag;
    flag >>= 8;
    ChangeEventFlag(bank & 0xff, flag & 0xff, op);
}

RVA(0x00039590, 0x37)
void OpTestEventFlagByValue(void) {
    i32 flag = ReadScriptValue();
    i16 dest = ReadLongVarIndex();
    i8 bank = flag;
    flag >>= 8;
    SetScriptLongVar(dest, TestEventFlag(bank & 0xff, flag & 0xff));
}

RVA(0x000395d0, 0x20)
void OpStoreScriptVar(void) {
    i16 index = ReadScriptValue();
    i32 value = ReadScriptValue();
    g_scriptVars[index] = value;
}

RVA(0x000395f0, 0x30)
void OpLoadScriptVar(void) {
    i16 index = ReadScriptValue();
    i16 dest = ReadLongVarIndex();
    SetScriptLongVar(dest, g_scriptVars[index]);
}

// Changes a flag; bank 14 addresses the referenced object's own flags.
RVA(0x00039620, 0x5e)
void OpModifyEventFlag(void) {
    Character* object = ReadScriptObject();
    u16 bank;
    u16 index;
    i32 op;
    ReadFlagOperand(&bank, &index);
    op = ReadScriptValue();
    if (bank != 14) {
        ChangeEventFlag(bank, index, op);
    } else if (object != NULL) {
        ChangeCharacterFlag(object, index, op);
    }
}

RVA(0x00039680, 0x69)
void OpTestEventFlag(void) {
    Character* object = ReadScriptObject();
    u16 bank;
    u16 index;
    i16 dest;
    i16 set;
    ReadFlagOperand(&bank, &index);
    dest = ReadLongVarIndex();
    set = 0;
    if (bank != 14) {
        set = TestEventFlag(bank, index);
    } else if (object != NULL) {
        set = TestCharacterFlag(object, index);
    }
    SetScriptLongVar(dest, set);
}

RVA(0x000396f0, 0x24)
i16 WriteEventFlags(FILE* fp) {
    return sizeof(g_eventFlags) - fwrite(g_eventFlags, 1, sizeof(g_eventFlags), fp);
}

RVA(0x00039720, 0x24)
i16 ReadEventFlags(FILE* fp) {
    return sizeof(g_eventFlags) - fread(g_eventFlags, 1, sizeof(g_eventFlags), fp);
}

RVA(0x00039750, 0x24)
i16 WriteScriptVars(FILE* fp) {
    return 0x100 - fwrite(g_scriptVars, sizeof(g_scriptVars[0]), 0x100, fp);
}

RVA(0x00039780, 0x24)
i16 ReadScriptVars(FILE* fp) {
    return 0x100 - fread(g_scriptVars, sizeof(g_scriptVars[0]), 0x100, fp);
}
