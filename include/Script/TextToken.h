#ifndef GITEN_SCRIPT_TEXTTOKEN_H
#define GITEN_SCRIPT_TEXTTOKEN_H

#include <rva.h>

#include <Ints.h>

// @identity-TODO: sixteen 4-byte (two-character) names in .bss that only the
// text-token name tables point at (12 for `sign`, 4 for `affiliation`); who
// fills them is unrecovered. Placeholder extern until its owner defines it.
extern char g_shortNames[16][4];

char* GetTextToken(i16 kind, i16 byId, i16 id);

RVA_DECL(0x00036920)
char* ReadTextToken(void);

#endif // GITEN_SCRIPT_TEXTTOKEN_H
