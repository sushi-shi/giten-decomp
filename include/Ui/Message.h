#ifndef GITEN_UI_MESSAGE_H
#define GITEN_UI_MESSAGE_H

#include <rva.h>

#include <Ints.h>

void PushTextWindowState(const char* text);
b16 RunTextWindowState(void);

void SetMessageLifetime(i16 ticks);
// SetMessageHold with MESSAGE_HOLD_QUERY only reads the hold.
#define MESSAGE_HOLD_QUERY (-1)

i16 SetMessageHold(i16 hold);
i16 RefreshMessageWindow(void);
i16 StartMessageTimer(i16 ticks, b16 hold);
i16 OpenMessageText(void);
void TickMessageWindow(void);
b16 FinishMessageScene(void);

// The message window, created on first use (a kind-15 text plane).
RVA_DECL(0x000025c0)
i16 OpenMessageWindow(void);

RVA_DECL(0x000026f0)
void ShowMessage(const char* text, i16 ticks);

RVA_DECL(0x00002720)
i16 CloseMessageWindow(void);

// @identity-TODO: that 0x3af40(scene, entry, window) starts a scene/script (state 5, handler
// 0x3afe0) is inferred; decode 0x3af40/0x3b340 and the state-5 handler to confirm.
RVA_DECL(0x00002780)
void RunMessageScene(i16 scene, i16 entry, i16 ticks);

#endif // GITEN_UI_MESSAGE_H
