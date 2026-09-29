#ifndef GITEN_SCRIPT_SCRIPTVARS_H
#define GITEN_SCRIPT_SCRIPTVARS_H

#include <rva.h>

#include <EnumDomain.h>
#include <Ints.h>
#include <Script/ScriptBlock.h>
#include <Script/ScriptStatus.h>

// An entry of the loaded script-file cache. `holds` counts the nested holds
// taken while it was loaded; a purge keeps the preloaded files and the held
// run after them.
typedef struct ScriptFileEntry {
    struct ScriptFileEntry* next;
    ScriptBlock* block;
    i16 holds;
} ScriptFileEntry;

// Scratch storage whose byte and word writes preserve the remaining bytes.
typedef union ScriptScratchValue {
    u32 value;
    u16 word;
    u8 byte;
} ScriptScratchValue;

// The script long variables.
#define SCRIPT_LONG_VAR_COUNT 26

void ClearScriptLongVars(void);

// The loaded script files, oldest first.
extern ScriptFileEntry* g_scriptFiles;

ScriptFileEntry* ScriptFileListTail(ScriptFileEntry* entry);
ScriptBlock* CacheScriptFile(i16 file);
void PreloadScriptFiles(void);
ScriptEntry FindCachedScriptEntry(i16 file, i16 entry);
void LoadCachedScriptFile(i16 id, i16 file);
void HoldScriptFiles(void);
void ThawObjectsForScript(void);
i16 FindFavouredMember(void);
void ReleaseScriptFiles(void);
void PurgeScriptFiles(void);
void OpShowBackground(void);
void OpRestoreBackground(void);

u32 GetScriptLongVar(i16 index);
u32 SetScriptLongVar(i16 index, u32 value);
void SwapScriptLongVars(i16 a, i16 b);
void CopyScriptLongVar(i16 dst, i16 src);
void ClearScriptLongVar(i16 index);
void ClearScriptLongVars(void);
void SaveSystemVars(u32* vars, u32* settings);
void RestoreSystemVars(u32* vars, u32* settings);
void ClearSystemVars(void);
b32 FreeCallFrame(i32 frame);
i16 AccessScriptReg(i16 write, i16 index, i16 value);
i32 NewCallFrame(void);
i32 FreeCallFrames(i32 stack);
void TransferFrameVars(i32 frame, i16 load);

// Set while a script builds a choice list.
extern b16 g_inChoices;

// The script engine's numbered variables (saved and loaded as one block).
extern i32 g_scriptVars[256];

void SetWindowOption(i16 option);
i16 SetHold(i16 on);

b16 OpBeginChoices(i16 window);

b16 OpNextChoice(i16 window);

// Records the current choice's text bounds and advances its index.
b16 FinishScriptChoice(i16 window);

RVA_DECL(0x0003a9a0)
b16 OpEndChoices(i16 window);

b16 OpRunChoiceMenu(i16 window);

void OpGetSelectedObjectId(void);

void OpGetHoveredObjectId(void);

void OpScrollWindow(i16 window);

void OpGetWindowScrollTop(i16 window);

void OpSetWindowScrollTop(i16 window);

void OpGetWindowIndent(i16 window);

void OpSetWindowIndent(i16 window);

void OpGetWindowScrollStep(i16 window);

void OpSetWindowScrollStep(i16 window);

void OpGetWindowCursor(i16 window);

void OpSetWindowCursor(i16 window);

void ClearMessageWindow(i16 window);

// Starts scene `scene` (0xe0: the party leader's actor scene with `arg`; else
// game state 5 in phase `phase`, keeping `scene` and `arg` for it).
// @identity-TODO: what state 5 runs is unrecovered.
void StartDebugScene(i16 scene, i16 arg, i16 phase);

// Installs the script callback run on each frame; (255, 255) disables it.
void OpSetMessageHook(void);
void RunMessageHook(void);

void WaitForScriptText(i16 window);
void AdvanceScriptTextWindow(i16 window);
b16 RunActorScene(void);
b16 RunScriptScene(void);
b16 StepOnTextPeriod(i16 window);

void OpPrintRosterName(void);

RVA_DECL(0x0003ba20)
void OpPrintOperandText(void);

// @identity-TODO: the formatted-number buffer extent is unproven.
extern char g_formattedNumber[64];

void OpPrintNumber(void);

void OpStartTickCounter(void);

// @identity-TODO: Mode -1 vs 0 differ only in the stored g_tickCountOn value (TickCounter
// counts only when >0); a reader distinguishing them is unrecovered.
void OpPauseTickCounter(void);

void OpStopTickCounter(void);

void OpGetTickCounter(void);

void OpSetTickCounter(void);

void OpStartCountdown(void);

// @identity-TODO: Looks like a PC-98 memory peek/poke whose two address operands are ignored on
// Windows, one scratch dword 0x81648 standing in; the PC-98 overlay would confirm.
RVA_DECL(0x0003bd40)
void OpPeekPokeScratch(void);

// @identity-TODO: The meaning of game state 1 and of its type byte 0/1/2 is unrecovered.
GZ_ENUM_RETURN(ScriptStatus, i16) OpWaitMessage(i16 window);

i16 AccessScriptReg(i16 write, i16 index, i16 value);

void TickCounter(void);

#endif // GITEN_SCRIPT_SCRIPTVARS_H
