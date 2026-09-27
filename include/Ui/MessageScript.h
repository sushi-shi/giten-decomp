#ifndef GITEN_UI_MESSAGESCRIPT_H
#define GITEN_UI_MESSAGESCRIPT_H

#include <rva.h>

#include <Ints.h>

// Runs script `script` entry `entry` in the message window (holding the
// script), then starts its close timer. Declared apart from Ui/Message.h: there
// the prototype reorders two static loads in field.c.
RVA_DECL(0x00002800)
void RunMessageScript(i16 script, i16 entry, i16 ticks);
void RunMessageTextScript(i16 script, i16 entry, i16 ticks);

#endif // GITEN_UI_MESSAGESCRIPT_H
