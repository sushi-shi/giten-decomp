#ifndef GITEN_SCRIPT_SCRIPTPANEL_H
#define GITEN_SCRIPT_SCRIPTPANEL_H

#include <Ints.h>

struct Panel;

typedef union ScriptPanelJump {
    u16 value;
    struct {
        u8 file;
        u8 entry;
    } parts;
} ScriptPanelJump;

typedef struct ScriptPanel {
    struct ScriptPanel* next;
    struct ScriptPanel* prev;
    struct Panel* panel;
    ScriptPanelJump* jumps;
    i16 image;
    i32 screenSave;
    struct {
        i16 drawn : 1;
    } flags;
} ScriptPanel;

static __inline ScriptPanelJump* GetScriptPanelJump(ScriptPanel* panel, i16 index) {
    return &panel->jumps[index];
}

void ErasePanelPictures(struct Panel* panel);
void DestroyScriptPanel(ScriptPanel* node);
void FreeScriptPanels(void);
void ResetScriptPanels(void);
void CloseScriptInterface(void);
i16 PollScriptPanels(void);
ScriptPanel* CreateScriptPanel(i16 image, i16 count, i16 x, i16 y);
void DrawScriptPanel(ScriptPanel* node);
b16 CloseScriptPanelByImage(i16 image);
void SetLastPanelRowState(i16 index, u16 flags);
b16 DrawScriptPanelByImage(i16 image);

void CloseLastScriptPanel(void);
void OpOpenScriptPanel(void);
void OpCloseScriptPanel(void);
void CloseAllScriptPanels(void);
void OpSetPanelEntryJump(void);
void DrawScriptPanels(void);
void OpSkipPanelOperands(void);
void OpSetPanelEntryValue(void);
void OpSetLastPanelFlag(void);

#endif // GITEN_SCRIPT_SCRIPTPANEL_H
