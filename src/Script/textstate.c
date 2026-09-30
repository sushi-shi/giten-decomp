// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Script/ScriptOps.h>
#include <Script/TextState.h>
#include <Text/Font.h>

DATA(0x000911a0)
TextState g_textState;

RVA(0x000329e0, 0x52)
i16 TickTextDelay(i16 skip) {
    if (g_textState.delayOn && g_textState.delayLeft != 0) {
        if (skip && !g_textState.delaySkipDisabled && !g_textState.flag7) {
            g_textState.delayLeft = 0;
            return 0;
        }
        g_textState.delayLeft -= 2;
        if (g_textState.delayLeft < 1) {
            g_textState.delayLeft = 0;
            return 0;
        }
        return g_textState.delayLeft;
    }
    return 0;
}

RVA(0x00032a40, 0x2a)
void AdvanceTextDelay(void) {
    if (g_textState.delayOn) {
        g_textState.delayLeft = g_textState.charDelay;
    }
    if (g_textState.delayRamp) {
        g_textState.charDelay += g_textState.delayStep;
    }
}

static __inline i16 ReadTextCharDelay(void) {
    i16 previous = g_textState.charDelay;
    g_textState.charDelay = ReadScriptValue();
    return previous;
}

RVA(0x00032a70, 0x20)
i16 OpSetTextCharDelay(void) {
    return ReadTextCharDelay();
}

RVA(0x00032a90, 0x26)
i16 EnableTextDelay(void) {
    i16 prev = g_textState.delayOn;
    g_textState.delayLeft = 0;
    g_textState.delayOn = true;
    return prev;
}

RVA(0x00032ac0, 0x1d)
i16 DisableTextDelay(void) {
    i16 prev = g_textState.delayOn;
    g_textState.delayOn = false;
    g_textState.delayRamp = 0;
    return prev;
}

RVA(0x00032ae0, 0x20)
i16 OpReplaceTextCharDelay(void) {
    return ReadTextCharDelay();
}

static __inline b16 ExchangeTextDelaySkipDisabled(b16 disabled) {
    i16 previous = g_textState.delaySkipDisabled;
    g_textState.delaySkipDisabled = disabled;
    return previous;
}

RVA(0x00032b00, 0x1d)
i16 DisableTextDelaySkip(void) {
    return ExchangeTextDelaySkipDisabled(true);
}

RVA(0x00032b20, 0x1d)
i16 EnableTextDelaySkip(void) {
    return ExchangeTextDelaySkipDisabled(false);
}

RVA(0x00032b40, 0x3b)
void InitTextState(TextState* state) {
    if (!state->delayOn) {
        state->scrollEnabled = false;
        state->timedWait = false;
        state->inputWait = true;
    } else {
        state->inputWait = false;
        state->scrollEnabled = true;
        state->timedWait = true;
    }
    state->charDelay = 0;
    state->delayLeft = 0;
    state->delayStep = 0;
    state->waitFrames = 30;
    state->delayRamp = false;
    state->flag7 = false;
    state->messageHookEnabled = false;
    state->delaySkipDisabled = true;
}

RVA(0x00032b80, 0x20)
void ResetTextStateInstant(void) {
    InitTextStateFlags(&g_textState, false, false);
    InitTextState(&g_textState);
}

RVA(0x00032ba0, 0x20)
void ResetTextStateDelayed(void) {
    InitTextStateFlags(&g_textState, true, true);
    InitTextState(&g_textState);
}

RVA(0x00032bc0, 0x10)
void OpSetTextWaitFrames(void) {
    g_textState.waitFrames = ReadScriptValue();
}

RVA(0x00032bd0, 0x2a)
void SetTextTimedWait(i16 on) {
    g_textState.inputWait = on + 1;
    g_textState.timedWait = on;
}

RVA(0x00032c00, 0x1c)
void SetTextScrollMode(i16 on) {
    g_textState.scrollEnabled = on;
}

RVA(0x00032c20, 0x2e)
void PushTextDelay(i16 on) {
    g_textState.savedDelayOn = g_textState.delayOn;
    g_textState.delayOn = on;
}

RVA(0x00032c50, 0x1e)
void PopTextDelay(void) {
    g_textState.delayOn = g_textState.savedDelayOn;
}

RVA(0x00032c70, 0x20)
void SetTextPeriod(i16 window) {
    g_textState.period = GetTextPlanePageLines(window);
}

RVA(0x00032c90, 0xd)
void ReloadTextPeriod(void) {
    g_textState.periodLeft = g_textState.period;
}

RVA(0x00032ca0, 0xa)
void ClearTextPeriod(void) {
    g_textState.periodLeft = 0;
}

RVA(0x00032cb0, 0xf)
i16 IsTextMessageHookEnabled(void) {
    return g_textState.messageHookEnabled;
}

// Counts the period down; on expiry reloads it and returns -1.
RVA(0x00032cc0, 0x21)
i16 TickTextPeriod(void) {
    if (--g_textState.periodLeft <= 0) {
        ReloadTextPeriod();
        return -1;
    }
    return 0;
}
