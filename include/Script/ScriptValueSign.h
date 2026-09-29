#ifndef GITEN_SCRIPT_SCRIPTVALUESIGN_H
#define GITEN_SCRIPT_SCRIPTVALUESIGN_H

#include <EnumDomain.h>

// Whether a familiarity or level-gap opcode applies its value as read or negated.
GZ_ENUM_BEGIN_SPLIT(ScriptValueSign, i16)
    SCRIPT_VALUE_AS_READ = 0,
    SCRIPT_VALUE_NEGATED = 1
GZ_ENUM_END_SPLIT(ScriptValueSign)

#endif // GITEN_SCRIPT_SCRIPTVALUESIGN_H
