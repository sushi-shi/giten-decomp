#ifndef GITEN_SCRIPT_SCRIPT_H
#define GITEN_SCRIPT_SCRIPT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Game/GameState.h>
#include <Ints.h>
#include <Script/ScriptBlock.h>
#include <Script/ScriptStatus.h>

// @identity-TODO: a running script's context; the word at +0x0e is the script
// position that jumps and calls rewrite, relative to the code block (a memory
// handle) at +0x0a; +0x06 is the handle list of its call frames.
typedef struct ScriptContext {
    i16 mode; // @identity-TODO: set at creation (0, or 1 for an actor's script)
    Character* actor;
    i32 callStack;
    u32 codeBase;
    u16 pc;
} ScriptContext;

static __inline void ClearScriptPosition(ScriptContext* script) {
    script->codeBase = 0;
    script->pc = 0;
}

// A saved call: the list links, the return position, the system variables
// 18..25 with the bank-15 settings word, and the two call arguments.
typedef struct ScriptFrame {
    i32 next;
    i32 prev;
    u32 codeBase;
    i16 pc;
    u32 sysVars[8];
    u32 settings;
    i16 argA;
    i16 argB;
} ScriptFrame;

extern ScriptContext* g_curScript;

// The script registers: register 0 is the result flag the opcodes set.
extern i16 g_scriptRegs[16];

// @identity-TODO: two words a script call passes (saved and restored with each
// call frame); their meaning is unrecovered.
extern i16 g_scriptArgA;
extern i16 g_scriptArgB;

Character* GetScriptActor(void);
i16 GetScriptActorId(void);
i16 ObjectSlotOfId(i16 id);

static __inline i16 ScriptObjectRefFromSlot(i16 slot) {
    return -1 - slot;
}

ScriptContext* SetCurrentScript(ScriptContext* script);
ScriptContext* GetCurrentScript(void);
#define ScriptBooleanMatches(value, negate) (((value) && !(negate)) || (!(value) && (negate)))

void ScriptJump(i16 pc);
i32 ScriptJumpUnless(i16 pc, i32 condition);
b16 ScriptJumpTo(u32 codeBase, i16 pc);
i32 PushCallFrame(ScriptContext* script, i16 keepVars);
i32 TopCallFrame(ScriptContext* script);
void PopCallFrame(ScriptContext* script, i16 discard);
void UnwindCallFrames(ScriptContext* script);
void SwapTopCallFrames(ScriptContext* script);
b16 ScriptJumpWithArgs(u32 codeBase, i16 pc, i16 argA, i16 argB);
i16 ScriptEntryPc(u32 code, i16 entry);

void StartScript(i16 file, i16 entry, ScriptContext* script);
void StartScriptInCode(u32 code, i16 arg, i16 entry, ScriptContext* script);
void EndScript(ScriptContext* script);
ScriptContext* NewScriptContext(i16 mode, Character* actor);
ScriptContext* FreeScriptContext(ScriptContext* script);
u16 NextScriptChar(i16 window);
i16 StepScript(i16 window, u16 ch);
i16 RunScript(i16 file, i16 entry, i16 window);
void RunCurrentScript(void);
i16 RunScriptStep(i16 window);
i16 TickScript(i16 window);
u16 RetakeDeferredChar(i16 window, i16 result, const char* caller);

ScriptEntry ResolveScriptEntry(i16 file, i16 entry);
void GotoScript(i16 file, i16 entry);
void CallScript(i16 file, i16 entry);
void OpJumpScript(i16 call);
i16 ReadJumpTarget(void);
void OpJump(void);

GZ_ENUM_RETURN(ScriptStatus, i16) SetActorMode(GZ_ENUM_PARAM(ActorMode, i16) mode);

// Resolves a script object id (negative: party slots; 1000+/2000+/3000+:
// other ranges; -16..-23: special objects) to its record.
Character* ResolveScriptObject(i16 id);
MapCoord ResolveScriptObjectCoord(i16 id);

RVA_DECL(0x00038900)
void OpConvertCharacterRef(void);

GZ_ENUM_BEGIN_SPLIT(CharacterRefConversion, i16)
    ROSTER_TO_PARTY = 0,
    PARTY_TO_ROSTER = 1,
    ROSTER_TO_OBJECT_REF = 2,
    OBJECT_REF_TO_ROSTER = 3,
    PARTY_TO_OBJECT_REF = 4,
    OBJECT_REF_TO_PARTY = 5,
    ROSTER_TO_CHARACTER_ID = 6,
    CHARACTER_ID_TO_ROSTER = 7
GZ_ENUM_END_SPLIT(CharacterRefConversion)

i16 ResolveObjectRosterSlot(i16 ref);

u8 ReadScriptByte(void);
u16 ReadScriptWord(void);
u32 ReadScriptLong(void);
u16 ReadScriptChar(void);
i32 ReadObjectRef(void);
Character* ReadScriptObject(void);

// The id of the object an operand names (the operand itself when it names
// none).
RVA_DECL(0x00038ca0)
i16 ReadObjectId(void);

// The character id a script object reference names (the special references
// -16..-23 resolved to members or combatants).
i16 ResolveObjectId(i16 ref);

// The macca / magnetite an object carries; object -24 is the actor last
// rolled (g_rolledMacca / g_rolledMagnetite), and then names the unit.
RVA_DECL(0x00038cc0)
i32 GetObjectMacca(i16 ref);

RVA_DECL(0x00038d10)
i32 GetObjectMagnetite(i16 ref);

#endif // GITEN_SCRIPT_SCRIPT_H
