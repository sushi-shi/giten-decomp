#ifndef GITEN_SCRIPT_SCRIPTVM_H
#define GITEN_SCRIPT_SCRIPTVM_H

#include <Ints.h>

// The opcode the script VM is executing (an extended opcode as 0x100..0x3ff).
extern u16 g_scriptOpcode;

u16 GetScriptOpcode(void);

i16 ExecScriptOpcode(i16 window, u16 op);

#endif // GITEN_SCRIPT_SCRIPTVM_H
