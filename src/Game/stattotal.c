// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/Stats.h>

RVA(0x0003dee0, 0x3b)
void UpdateStatTotals(StatBlock* stats) {
    i16 i;
    i16 sum;
    for (i = 0; i < 11; i++) {
        sum = stats->base[i] + stats->bonus[i] + stats->equipment[i] + stats->modifiers[i];
        stats->total[i] = ClampTo100(sum / 2);
    }
}
