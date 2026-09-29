#ifndef GITEN_SCRIPT_SCRIPTCOMPARISONRHS_H
#define GITEN_SCRIPT_SCRIPTCOMPARISONRHS_H

#include <Enums.h>

// Whether OpJumpUnlessCompare compares against zero or a second operand.
GZ_ENUM_BEGIN(ScriptComparisonRhs)
    SCRIPT_COMPARE_WITH_ZERO = 0,
    SCRIPT_COMPARE_WITH_OPERAND = 1
GZ_ENUM_END(ScriptComparisonRhs)

#endif // GITEN_SCRIPT_SCRIPTCOMPARISONRHS_H
