// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/FusionScreen.h>
#include <Game/GameState.h>
#include <Game/Party.h>
#include <Game/PartyCommand.h>
#include <Game/SkillUse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/LongVar.h>
#include <Script/ObjectRef.h>
#include <Script/Script.h>
#include <Script/ScriptOperand.h>
#include <Script/ScriptOps.h>
#include <Script/ScriptText.h>
#include <Script/ScriptVars.h>
#include <Script/ScriptVm.h>
#include <Script/TextState.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>

#include <mbctype.h>
#include <mbstring.h>
#include <stddef.h>
#include <string.h>

DATA(0x00091120)
char g_numberUnit[64];

DATA(0x00091160)
ScriptContext* g_curScript;

// Negative script object ids -1, -2, ... name slots 0, 1, ...
RVA(0x000387f0, 0xa)
i16 ObjectSlotOfId(i16 id) {
    id = SCRIPT_REF_SLOT_BASE - id;
    return id;
}

RVA(0x00038800, 0x12)
i16 GetScriptActorId(void) {
    Character* actor = GetScriptActor();
    if (actor == NULL) {
        return -1;
    }
    return actor->id;
}

RVA(0x00038820, 0xb7)
i16 ResolveObjectId(i16 ref) {
    i16 slot = ObjectSlotOfId(ref);
    if (slot == 16) {
        return GetPartyRosterId(FindFavouredMember());
    }
    if (slot == 17) {
        return GetScriptActorId();
    }
    if (slot == 18) {
        return GetRosterId(RosterSlotOfId(GetScriptActorId()));
    }
    if (slot == 19) {
        return GetScriptActorId();
    }
    if (slot == 20) {
        return GetCombatant(g_actorId)->id;
    }
    if (slot == 21) {
        return GetCombatant(g_targetId)->id;
    }
    if (slot == 22 || slot == 23) {
        return LoadFusionResultCharacter(GetCharacter(-1))->id;
    }
    return GetCharacterId(slot);
}

RVA(0x000388e0, 0x17)
i16 ResolveObjectRosterSlot(i16 ref) {
    return RosterSlotOfId(ResolveObjectId(ref));
}

RVA(0x00038900, 0x138)
void OpConvertCharacterRef(void) {
    i16 index = ReadLongVarIndex();
    GZ_ENUM_STORAGE(CharacterRefConversion, i16)
    kind = ReadScriptValue();
    i16 value = GetScriptLongVar(index);
    i16 slot;
    switch (kind) {
        case ROSTER_TO_PARTY:
            value = FindPartySlot(value);
            break;
        case PARTY_TO_ROSTER:
            value = GetPartySlot(value);
            break;
        case ROSTER_TO_OBJECT_REF:
            slot = FindCharacter(GetRosterId(value));
            value = slot >= 0 ? ScriptObjectRefFromSlot(slot) : -15;
            break;
        case OBJECT_REF_TO_ROSTER:
            value = ResolveObjectRosterSlot(value);
            break;
        case PARTY_TO_OBJECT_REF:
            slot = FindCharacter(GetPartyRosterId(value));
            value = slot >= 0 ? ScriptObjectRefFromSlot(slot) : -15;
            break;
        case OBJECT_REF_TO_PARTY:
            value = FindPartyPositionOfId(ResolveObjectId(value));
            break;
        case ROSTER_TO_CHARACTER_ID:
            value = GetRosterId(value);
            break;
        case CHARACTER_ID_TO_ROSTER:
            value = FindRosterSlotById(value);
            break;
    }
    SetScriptLongVar(index, value);
}

RVA(0x00038a40, 0x9)
Character* GetScriptActor(void) {
    return g_curScript->actor;
}

RVA(0x00038a50, 0x134)
Character* ResolveScriptObject(i16 id) {
    if (id == SCRIPT_REF_FAVOURED_MEMBER) {
        return GetPartyCharacter(FindFavouredMember());
    }
    if (id == SCRIPT_REF_ACTOR) {
        return GetScriptActor();
    }
    if (id == SCRIPT_REF_ACTOR_BY_ID) {
        return GetCharacterById(GetScriptActorId());
    }
    if (id == SCRIPT_REF_ACTOR_ALIAS) {
        return GetScriptActor();
    }
    if (id == SCRIPT_REF_BATTLE_ACTOR) {
        return GetCombatant(g_actorId);
    }
    if (id == SCRIPT_REF_BATTLE_TARGET) {
        return GetCombatant(g_targetId);
    }
    if (id == SCRIPT_REF_FUSION_RESULT || id == SCRIPT_REF_FUSION_RESULT_ALIAS) {
        return LoadFusionResultCharacter(GetCharacter(-1));
    }
    if (id < 0) {
        return AsCharacter(GetCharacter(ObjectSlotOfId(id)));
    }
    if (id >= SCRIPT_REF_CHARACTER_BASE) {
        Character* character = GetCharacter(14);
        LoadCharacterCore(id - SCRIPT_REF_CHARACTER_BASE, character);
        return character;
    }
    if (id >= SCRIPT_REF_ROSTER_BASE) {
        return GetRosterCharacter(id - SCRIPT_REF_ROSTER_BASE);
    }
    if (id >= SCRIPT_REF_PARTY_BASE) {
        return GetPartyCharacter(id - SCRIPT_REF_PARTY_BASE);
    }
    return GetFieldActor(id);
}

RVA(0x00038b90, 0xa6)
MapCoord ResolveScriptObjectCoord(i16 id) {
    MapCoord point;
    point.x = 0;
    point.y = 0;
    if (id >= SCRIPT_REF_PARTY_BASE || id == -18 || (id < 0 && id >= -16) || id == -22
        || id == -23) {
        return GetMapCoord();
    }
    if (id == -17 || id == -19) {
        FieldActor* actor = (FieldActor*)g_curScript->actor;
        if (actor != NULL) {
            point.x = actor->pos.x;
            point.y = actor->pos.y;
        }
    } else if (id == -20) {
        return GetCombatantCoord(g_actorId);
    } else if (id == -21) {
        return GetCombatantCoord(g_targetId);
    }
    return point;
}

// Reads an object reference operand: a kind byte, then a value (kinds 1..3
// offset it into the 3000/1000/2000 id ranges).
RVA(0x00038c40, 0x44)
i32 ReadObjectRef(void) {
    i16 kind = ReadScriptByte();
    i32 value = ReadScriptValue();
    switch (kind) {
        case OBJECT_REF_DIRECT:
            break;
        case OBJECT_REF_CHARACTER_ID:
            return value + SCRIPT_REF_CHARACTER_BASE;
        case OBJECT_REF_PARTY_SLOT:
            return value + SCRIPT_REF_PARTY_BASE;
        case OBJECT_REF_ROSTER_SLOT:
            return value + SCRIPT_REF_ROSTER_BASE;
    }
    return value;
}

RVA(0x00038c90, 0xf)
Character* ReadScriptObject(void) {
    return ResolveScriptObject(ReadObjectRef());
}

RVA(0x00038ca0, 0x20)
i16 ReadObjectId(void) {
    i32 id = ReadObjectRef();
    Character* object = ResolveScriptObject(id);
    if (object == NULL) {
        return id;
    }
    return object->id;
}

RVA(0x00038cc0, 0x48)
i32 GetObjectMacca(i16 ref) {
    Character* character;
    if (ref == -24) {
        strcpy(g_numberUnit, "\203}\203b\203J");
        return g_rolledMacca;
    }
    character = ResolveScriptObject(ref);
    if (character != NULL) {
        return character->macca;
    }
    return 0;
}

RVA(0x00038d10, 0x48)
i32 GetObjectMagnetite(i16 ref) {
    Character* character;
    if (ref == -24) {
        strcpy(g_numberUnit, "\202l\202`\202f");
        return g_rolledMagnetite;
    }
    character = ResolveScriptObject(ref);
    if (character != NULL) {
        return character->magnetite;
    }
    return 0;
}

RVA(0x00038d60, 0x10)
ScriptContext* SetCurrentScript(ScriptContext* script) {
    ScriptContext* prev = g_curScript;
    g_curScript = script;
    return prev;
}

// Makes `script` current and starts it at `entry` of script file `file`.
RVA(0x00038d70, 0x20)
void StartScript(i16 file, i16 entry, ScriptContext* script) {
    SetCurrentScript(script);
    GotoScript(file, entry);
}

// Makes `script` current and starts it at `entry` of the loaded code block
// `code`, passing `arg` and `entry` as the call arguments.
RVA(0x00038d90, 0x34)
void StartScriptInCode(u32 code, i16 arg, i16 entry, ScriptContext* script) {
    i16 pc;
    SetCurrentScript(script);
    pc = ScriptEntryPc(code, entry);
    ScriptJumpWithArgs(code, pc, arg, entry);
}

// Unwinds the script's calls and closes its windows.
RVA(0x00038dd0, 0x12)
void EndScript(ScriptContext* script) {
    UnwindCallFrames(script);
    CloseScriptWindows();
}

RVA(0x00038df0, 0x28)
ScriptContext* NewScriptContext(i16 mode, Character* actor) {
    ScriptContext* script = AllocCleared(1, sizeof(ScriptContext));
    script->mode = mode;
    script->actor = actor;
    script->callStack = 0;
    ClearScriptPosition(script);
    return script;
}

RVA(0x00038e20, 0x19)
ScriptContext* FreeScriptContext(ScriptContext* script) {
    EndScript(script);
    return FreeBlock(script);
}

RVA(0x00038e40, 0x6)
ScriptContext* GetCurrentScript(void) {
    return g_curScript;
}

// Reads the byte at `*pos` of a code block and advances `*pos`; 0 without a
// block.
RVA(0x00038e50, 0x2d)
u8 ReadCodeByte(i32 code, u16* pos) {
    u8* data;
    if (code == 0) {
        return 0;
    }
    data = HandleReadPtr(code);
    return data[(*pos)++];
}

RVA(0x00038e80, 0x33)
u16 ReadCodeWord(i32 code, u16* pos) {
    u16 word = ReadCodeByte(code, pos);
    word += ReadCodeByte(code, pos) << 8;
    return word;
}

RVA(0x00038ec0, 0x38)
u32 ReadCodeLong(i32 code, u16* pos) {
    u32 value = ReadCodeWord(code, pos);
    value += (u32)ReadCodeWord(code, pos) << 16;
    return value;
}

// Reads one (possibly double-byte Shift-JIS) character.
RVA(0x00038f00, 0x4a)
u16 ReadCodeChar(i32 code, u16* pos) {
    u16 lo;
    u16 c = ReadCodeByte(code, pos);
    if (!_ismbblead(c)) {
        return c;
    }
    lo = ReadCodeByte(code, pos);
    return (c << 8) + lo;
}

// The next character of the current script, returning from calls at the end
// of a block; 0 when the script ends.
RVA(0x00038f50, 0x47)
u16 ReadScriptChar(void) {
    u16 c;
    if (g_curScript == NULL) {
        return 0;
    }
    c = ReadCodeChar(g_curScript->codeBase, &g_curScript->pc);
    while (c == 0) {
        if (ReturnFromCall() != 0) {
            return 0;
        }
        c = ReadCodeChar(g_curScript->codeBase, &g_curScript->pc);
    }
    return c;
}

RVA(0x00038fa0, 0x16)
u8 ReadScriptByte(void) {
    return ReadCodeByte(g_curScript->codeBase, &g_curScript->pc);
}

RVA(0x00038fc0, 0x16)
u16 ReadScriptWord(void) {
    return ReadCodeWord(g_curScript->codeBase, &g_curScript->pc);
}

RVA(0x00038fe0, 0x16)
u32 ReadScriptLong(void) {
    return ReadCodeLong(g_curScript->codeBase, &g_curScript->pc);
}

// The next character for `window`: one the window deferred, else the
// script's next.
RVA(0x00039000, 0x18)
u16 NextScriptChar(i16 window) {
    u16 c = TakeWindowDeferredChar(window);
    if (!c) {
        c = ReadScriptChar();
    }
    return c;
}

#ifdef GITEN_COMPAT
// Whether ReadCodeChar's character `ch` is text rather than an opcode: a
// single byte by _ismbcprint, whose tables the CRT builds from the code page
// alone; a double-byte character (ReadCodeChar pairs only a lead byte) when
// its trail byte is a Shift-JIS trail byte.
static b32 IsScriptTextChar(u16 ch) {
    i32 trail;

    if (ch <= 0xff) {
        return _ismbcprint(ch) != 0;
    }
    trail = ch & 0xff;
    return trail >= 0x40 && trail <= 0xfc && trail != 0x7f;
}
#endif

// Runs one character: an opcode for a non-printable one, else the character
// is captured or written to `window`. Negative when the script stops.
RVA(0x00039020, 0x6d)
i16 StepScript(i16 window, u16 ch) {
    i16 result;
#ifdef GITEN_COMPAT
    // @bug For a double-byte character _ismbcprint asks GetStringTypeA for its
    // C1 type bits, which differ by Windows version: on Windows XP some
    // characters the game prints (the long vowel mark ー, 0x815b) have none of
    // the printable bits. Such a character goes to ExecScriptOpcode, which has
    // no case for it, so it is dropped from the text. The test here does not
    // ask the system.
    if (!IsScriptTextChar(ch)) {
#else
    if (!_ismbcprint(ch)) {
#endif
        return ExecScriptOpcode(window, ch);
    }
    if (!CaptureTextChar(ch)) {
        result = PutTextChar(window, ch, &g_textState, g_inChoices);
        if (result == -1) {
            return result;
        }
        AdvanceTextDelay();
        return result;
    }
    return 0;
}

// Runs script file `file` from `entry` in a fresh context until it stops.
RVA(0x00039090, 0x54)
i16 RunScript(i16 file, i16 entry, i16 window) {
    i16 result;
    StartScript(file, entry, NewScriptContext(0, NULL));
    ResetTextStateInstant();
    do {
        result = StepScript(window, NextScriptChar(window));
    } while (result >= 0);
    EndScript(GetCurrentScript());
    return window;
}

// Runs the current script in its newest window until it stops.
RVA(0x000390f0, 0x22)
void RunCurrentScript(void) {
    i16 window;
    do {
        window = TopScriptWindow();
    } while (StepScript(window, NextScriptChar(window)) >= 0);
}

// Runs the current script until a step does something (its newest window,
// else `window`); returns that step's result.
RVA(0x00039120, 0x2b)
i16 RunScriptStep(i16 window) {
    i16 result;
    do {
        i16 top = TopScriptWindow();
        if (top >= 0) {
            window = top;
        }
        result = StepScript(window, NextScriptChar(window));
    } while (result == 0);
    return result;
}

// Runs a script step unless the text delay is still counting down.
RVA(0x00039150, 0x21)
i16 TickScript(i16 window) {
    if (TickTextDelay(0)) {
        return 0;
    }
    return RunScriptStep(window);
}

// After a step that deferred its character (-2), takes the deferred character
// back from the newest window (else `window`). `caller` names the calling
// flow (a debug label; both callers pass one, e.g. "enemy action flow").
RVA(0x00039180, 0x24)
u16 RetakeDeferredChar(i16 window, i16 result, const char* caller) {
    i16 top;
    if (result != -2) {
        return 0;
    }
    top = GetScriptWindowOrDefault(window);
    return TakeWindowDeferredChar(top);
}
