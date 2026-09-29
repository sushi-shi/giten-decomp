#ifndef GITEN_SCRIPT_SCRIPTTEXT_H
#define GITEN_SCRIPT_SCRIPTTEXT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Ui/MenuBox.h>

// Script text capture and the formatted text line.

// A node of the script's window stack.
typedef struct ScriptWindowNode {
    struct ScriptWindowNode* next;
    struct ScriptWindowNode* prev;
    i16 window;
} ScriptWindowNode;

struct MenuBox;
void RunScriptMenuHandler(struct MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

b16 CaptureTextChar(u16 ch);
void SetCapturedText(const char* text);

// scriptvars' runner for a text script.
void CallTextScript(const char* text);

// scriptflow/scriptctx's script call and run, for the script menu handler.
// Codegen constraint: declared here; including <Script/Script.h> in
// scripttext.c perturbs its data-reader loops (TU state).
void CallScript(i16 file, i16 entry);
void RunCurrentScript(void);

void SetTextCapture(i16 on);

void ClearTextBuffers(void);

void ClearCapturedText(void);

void OpFormatNumber(void);

void OpFormatCapturedText(void);

i16 StackScriptWindow(i16 window);
void CloseScriptWindows(void);
i16 OpenScriptWindow(u16 kind, i16 arg);
i16 TopScriptWindow(void);

static __inline i16 GetScriptWindowOrDefault(i16 window) {
    i16 top = TopScriptWindow();
    if (top < 0) {
        top = window;
    }
    return top;
}

void OpStackMessageWindow(void);
void OpPushScriptWindow(void);
void PopScriptWindow(void);

#endif // GITEN_SCRIPT_SCRIPTTEXT_H
