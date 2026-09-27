#ifndef GITEN_UTIL_TEXT_H
#define GITEN_UTIL_TEXT_H

#include <Ints.h>

#include <string.h>

// Appends one single-byte or packed double-byte character to a text buffer.
static __inline void AppendTextChar(char* text, u16 ch) {
    i16 length = strlen(text);
    if (ch > 0xff) {
        text[length] = ch >> 8;
        text[length + 1] = ch;
        text[length + 2] = '\0';
    } else {
        text[length] = ch;
        text[length + 1] = '\0';
    }
}

#endif // GITEN_UTIL_TEXT_H
