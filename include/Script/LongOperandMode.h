#ifndef GITEN_SCRIPT_LONGOPERANDMODE_H
#define GITEN_SCRIPT_LONGOPERANDMODE_H

#include <EnumDomain.h>

// Whether a long-variable operation reads an explicit operand or works in place
// (LoadLongOperands).
GZ_ENUM_BEGIN_SPLIT(LongOperandMode, i16)
    LONG_OPERAND_EXPLICIT = 0,
    LONG_OPERAND_IN_PLACE = 1
GZ_ENUM_END_SPLIT(LongOperandMode)

#endif // GITEN_SCRIPT_LONGOPERANDMODE_H
