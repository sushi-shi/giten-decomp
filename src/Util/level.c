// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Util/Level.h>

RVA(0x0003dec0, 0x14)
i32 ClampLevel(i16 level) {
    return level < 99 ? level : 99;
}
