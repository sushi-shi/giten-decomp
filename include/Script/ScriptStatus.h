#ifndef GITEN_SCRIPT_SCRIPTSTATUS_H
#define GITEN_SCRIPT_SCRIPTSTATUS_H

#include <EnumDomain.h>

// What running one script opcode or character tells the script loop: go on,
// stop the script, defer a character that did not fit, or yield until the next frame.
GZ_ENUM_BEGIN_SPLIT(ScriptStatus, i16)
    SCRIPT_YIELD = -3,
    SCRIPT_DEFERRED_CHAR = -2,
    SCRIPT_END = -1,
    SCRIPT_CONTINUE = 0
GZ_ENUM_END_SPLIT(ScriptStatus)

#endif // GITEN_SCRIPT_SCRIPTSTATUS_H
