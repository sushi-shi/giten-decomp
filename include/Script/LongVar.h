#ifndef GITEN_SCRIPT_LONGVAR_H
#define GITEN_SCRIPT_LONGVAR_H

#include <Ints.h>

// The long-variable script opcodes: each reads a variable index (and
// operands) from the script, works in an accumulator and writes the variable
// back. `inPlace` takes the variable's own value as the first operand instead
// of an immediate.
i16 ReadLongVarIndex(void);
i32 ReadLongOperand(i16 inPlace);
void LoadLongOperands(i16 inPlace);
i32 StoreLongResult(void);
void LoadLongVar(void);
void StoreLongVar(void);
void OpSetLongVar(void);
void OpMulLongVar(i16 inPlace);
void OpDivLongVar(i16 inPlace);
void OpAddLongVar(i16 inPlace);
void OpSubLongVar(i16 inPlace);
void OpAndLongVar(i16 inPlace);
void OpOrLongVar(i16 inPlace);
void OpXorLongVar(i16 inPlace);
void OpShlLongVar(i16 inPlace);
void OpSarLongVar(i16 inPlace);
void OpPercentLongVar(i16 inPlace);
void OpSqrtLongVar(i16 inPlace);
void OpModLongVar(i16 inPlace);
void OpSwapLongVars(void);
void OpCopyLongVar(void);
void OpUnsetLongVar(void);
void OpZeroLongVar(void);
void OpNegLongVar(void);
void OpNotLongVar(void);
void OpIncLongVar(void);
void OpDecLongVar(void);
void OpIncLongVarBelow(void);
void OpDecLongVarAbove(void);
void OpClampLongVar(void);
void OpRollLongVar(void);
void OpRandLongVar(void);

// The frame-local opcodes: store, load or exchange the system variables with
// the newest call frame; 0 when there is no frame.
i16 StoreFrameLocals(void);
i16 LoadFrameLocals(void);
i16 SwapFrameLocals(void);

void ReadScriptBytePair(i16* first, i16* second);

#endif // GITEN_SCRIPT_LONGVAR_H
