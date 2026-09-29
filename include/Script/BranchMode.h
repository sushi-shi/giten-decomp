#ifndef GITEN_SCRIPT_BRANCHMODE_H
#define GITEN_SCRIPT_BRANCHMODE_H

#include <EnumDomain.h>

// Whether a script transfer replaces the running script or returns to it.
GZ_ENUM_BEGIN_SPLIT(ScriptBranchMode, i16)
    SCRIPT_BRANCH_JUMP = 0,
    SCRIPT_BRANCH_CALL = 1
GZ_ENUM_END_SPLIT(ScriptBranchMode)

// Whether a conditional jump takes the test's result as is or inverted.
GZ_ENUM_BEGIN_SPLIT(ScriptTestPolarity, i16)
    SCRIPT_TEST_NORMAL = 0,
    SCRIPT_TEST_INVERTED = 1
GZ_ENUM_END_SPLIT(ScriptTestPolarity)

#endif // GITEN_SCRIPT_BRANCHMODE_H
