// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Text/TextPlane.h>

#include <mbctype.h>

static __inline i16 ReadTextTrailByte(u16* code, const u8* text, i16 pos) {
    *code <<= 8;
    *code += text[pos++];
    return pos;
}

// Reads the character at text[pos] into *code: a Shift-JIS lead byte and a
// 0xfe/0xff escape byte both pair with the next byte. Returns the position
// after the character.
// @early-stop register residue: both text[pos] loads encode their SIB with
// base and index swapped ([pos+text] here, [text+pos] in retail); calls,
// CFG and every other byte match. Indexing spellings, parameter types,
// header TU-state sweeps and the permuter are flat.
RVA(0x00001aa0, 0x7c)
i16 ReadTextChar(u16* code, const u8* text, i16 pos) {
    *code = 0;
    if (text[pos] == 0) {
        return pos;
    }
    *code = text[pos++];
    if (*code >= 0xfe) {
        pos = ReadTextTrailByte(code, text, pos);
        return pos;
    }
    if (_ismbblead(*code)) {
        if (text[pos] == 0) {
            *code = 0x81a6;
            return pos;
        }
        pos = ReadTextTrailByte(code, text, pos);
    }
    return pos;
}
