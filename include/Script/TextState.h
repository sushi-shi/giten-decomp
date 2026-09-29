#ifndef GITEN_SCRIPT_TEXTSTATE_H
#define GITEN_SCRIPT_TEXTSTATE_H

#include <rva.h>

#include <Ints.h>

// @identity-TODO: the message-text state the script text loop hands to the
// glyph writer with every printable character (and that script calls save and
// restore as one 16-byte block). The per-character delay fields are recovered
// from their use; the roles of the flagN bits and of `spare` (+0x0e) are not.
typedef struct TextState {
    i16 flag0 : 1;
    i16 flag1 : 1;
    i16 delayOn : 1;
    i16 delayRamp : 1;
    i16 scrollEnabled : 1;
    i16 timedWait : 1;
    i16 inputWait : 1;
    i16 flag7 : 1;
    i16 delaySkipDisabled : 1;
    i16 savedDelayOn : 1;
    i16 messageHookEnabled : 1;
    i16 charDelay;
    i16 delayLeft;
    i16 delayStep;
    i16 waitFrames;
    i16 period;
    i16 periodLeft;
    i16 spare; // @identity-TODO: never read on its own; only the 16-byte block copies carry it
} TextState;

static __inline void InitTextStateFlags(TextState* state, b16 style, b16 delayOn) {
    state->flag0 = true;
    state->flag1 = style;
    state->delayOn = delayOn;
}

extern TextState g_textState;

i16 TickTextDelay(i16 skip);
void AdvanceTextDelay(void);
void ResetTextStateInstant(void);
void ResetTextStateDelayed(void);

RVA_DECL(0x00032a70)
i16 OpSetTextCharDelay(void);

i16 EnableTextDelay(void);

i16 DisableTextDelay(void);

// @identity-TODO: body is byte-identical to OpSetTextCharDelay (0x32a70); why the VM has two
// opcodes (505/508) for it is unrecovered
RVA_DECL(0x00032ae0)
i16 OpReplaceTextCharDelay(void);

// Controls whether a skip request may cancel the per-character delay.
i16 DisableTextDelaySkip(void);

i16 EnableTextDelaySkip(void);

// Sets the frame duration used by script-text waits.
RVA_DECL(0x00032bc0)
void OpSetTextWaitFrames(void);

// Selects timer waits when on, otherwise input waits.
void SetTextTimedWait(i16 on);

// Scrolls the window on text advancement when on, otherwise clears it.
void SetTextScrollMode(i16 on);

RVA_DECL(0x00032c70)
void SetTextPeriod(i16 window);

void ReloadTextPeriod(void);

void ClearTextPeriod(void);

void PushTextDelay(i16 on);
void PopTextDelay(void);
i16 TickTextPeriod(void);
i16 IsTextMessageHookEnabled(void);

#endif // GITEN_SCRIPT_TEXTSTATE_H
