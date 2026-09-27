#ifndef GITEN_TEXT_WINDOWTEXT_H
#define GITEN_TEXT_WINDOWTEXT_H

#include <rva.h>

// @identity-TODO: the effect of style (TextState.flag1) is unproven.
// noKinsoku bypasses punctuation-dependent line-breaking slack.
void PrintWindowText(i16 window, const char* text, u16 attr, i16 style, i16 noKinsoku);

#endif // GITEN_TEXT_WINDOWTEXT_H
