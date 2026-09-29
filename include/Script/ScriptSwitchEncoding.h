#ifndef GITEN_SCRIPT_SCRIPTSWITCHENCODING_H
#define GITEN_SCRIPT_SCRIPTSWITCHENCODING_H

#include <EnumDomain.h>

// A script switch table ends at key SCRIPT_SWITCH_END; ReadScriptSwitch marks a
// target in the running script with SCRIPT_SWITCH_LOCAL_JUMP.
GZ_ENUM_CONST_BEGIN(ScriptSwitchEncoding)
    SCRIPT_SWITCH_END = 255,
    SCRIPT_SWITCH_LOCAL_JUMP = 256
GZ_ENUM_CONST_END(ScriptSwitchEncoding)

#endif // GITEN_SCRIPT_SCRIPTSWITCHENCODING_H
