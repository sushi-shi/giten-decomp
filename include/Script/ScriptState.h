#ifndef GITEN_SCRIPT_SCRIPTSTATE_H
#define GITEN_SCRIPT_SCRIPTSTATE_H

#include <Ints.h>
#include <Script/Script.h>
#include <Script/TextState.h>

// A saved running script: the current script and the message-text state,
// stacked while another script runs.
typedef struct SavedScriptState {
    struct SavedScriptState* next;
    ScriptContext* script;
    TextState textState;
} SavedScriptState;

void SaveScriptState(void);
void RestoreScriptState(void);

#endif // GITEN_SCRIPT_SCRIPTSTATE_H
