// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Ints.h>

// Maps a signed alignment byte (-128..127) onto the 24-cell alignment chart,
// counting from the far end.
RVA(0x00042ca0, 0x2f)
i16 AlignmentChartCell(i16 value) {
    return 23 - (i16)(((value - -128.0) / 256.0) * 24.0);
}
