#ifndef GITEN_SCRIPT_TEXTTOKEN_H
#define GITEN_SCRIPT_TEXTTOKEN_H

#include <rva.h>

#include <Ints.h>

char* GetTextToken(i16 kind, i16 byId, i16 id);

RVA_DECL(0x00036920)
char* ReadTextToken(void);

#endif // GITEN_SCRIPT_TEXTTOKEN_H
