#ifndef GITEN_SCRIPT_SCRIPTVM_H
#define GITEN_SCRIPT_SCRIPTVM_H

#include <EnumDomain.h>
#include <Ints.h>
#include <Script/Opcode.h>
#include <Script/ScriptStatus.h>

// The opcode the script VM is executing (an extended opcode as 0x100..0x3ff).
extern GZ_ENUM_STORAGE(ScriptOpcode, u16) g_scriptOpcode;

GZ_ENUM_RETURN(ScriptOpcode, u16) GetScriptOpcode(void);

GZ_ENUM_RETURN(ScriptStatus, i16) ExecScriptOpcode(i16 window, GZ_ENUM_PARAM(ScriptOpcode, u16) op);

#endif // GITEN_SCRIPT_SCRIPTVM_H
