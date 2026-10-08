#ifndef GITEN_SCRIPT_LONGVAR_H
#define GITEN_SCRIPT_LONGVAR_H

#include <Ints.h>
#include <Script/LongOperandMode.h>

// The long-variable script opcodes: each reads a variable index (and
// operands) from the script, works in an accumulator and writes the variable
// back. `inPlace` takes the variable's own value as the first operand instead
// of an immediate.
i16 ReadLongVarIndex(void);
i32 ReadLongOperand(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void LoadLongOperands(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
i32 StoreLongResult(void);
void LoadLongVar(void);
void StoreLongVar(void);
void OpSetLongVar(void);
void OpMulLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpDivLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpAddLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpSubLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpAndLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpOrLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpXorLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpShlLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpSarLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpPercentLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpSqrtLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
void OpModLongVar(GZ_ENUM_PARAM(LongOperandMode, i16) inPlace);
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
b16 StoreFrameLocals(void);
b16 LoadFrameLocals(void);
b16 SwapFrameLocals(void);

void ReadScriptBytePair(i16* first, i16* second);

#endif // GITEN_SCRIPT_LONGVAR_H
