# Game function matching walls

These functions have unresolved source or compiler-state differences. The
retail instruction order and data accesses below are the evidence to preserve
while reconstructing their source.

| Function | Current retail evidence | Open question |
| --- | --- | --- |
| `RunFieldExploration` | Retail keeps zero in ESI from the first phase test through the state switch. The source materializes zero separately in later phases. Call and branch counts and ordered referents agree. | Which source lifetime or domain operation makes that zero value live across phases? |
| `BuildViewOcclusion` | Retail loads direction into ESI before clearing the masks and uses EDI for the occlusion index. The source exchanges those registers and loads direction after the clears. Calls, branch destinations, and ordered referents agree. | Which source lifetime or helper boundary explains the direction load and register allocation? |
| `ResolveThreeSpecialRaceFusion` | Retail keeps the third slot in ESI and the staged result in EDI. The source exchanges them; call and branch counts and ordered referents agree. | Which source lifetime or classification-helper boundary accounts for the register choice? |
| `GetWorldTravelDirection` | Retail loads `y` into ECX and `x` into ESI; the current object exchanges those registers. The instruction, call, branch, and return counts agree, and this source body matched exactly in the earlier bank. | Which later translation-unit declaration changes the two parameter registers? |
| `LoadWorldTravelCandidates` | Retail reserves 0x20 stack bytes and keeps the layer in EBP; the current object reserves 0x1c bytes and reuses EBP for the score row. Instruction, call, branch, return, and ordered referent counts agree; this body matched in the earlier bank. | Which translation-unit declaration or genuine local lifetime restores the layer and row allocation? |
| `RollExceptionalWeaponAttack` | Retail keeps the target in EDI and the hit modifier in EBX; the current object exchanges those registers after `GetEquipmentHitModifier`. Instruction, call, branch, return, and ordered referent counts agree; the same body matched in the earlier bank. | Which translation-unit declaration changes the two live ranges? |
| `LoadAutomapAreas` | Splitting the first short read-error accumulation recovers retail's `test bx, bx` and the historical 89.34% match. The remaining first difference is after the level-header `fread`: retail cleans the argument stack before computing the table size, while the current object computes it first. Call, branch, return, and ordered referent counts agree. | Which authentic local lifetime or statement boundary restores that instruction schedule? |
