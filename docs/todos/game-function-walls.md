# Game function matching walls

These functions have unresolved source or compiler-state differences. The
retail instruction order and data accesses below are the evidence to preserve
while reconstructing their source.

| Function | Current retail evidence | Open question |
| --- | --- | --- |
| `RunFieldExploration` | Retail keeps zero in ESI from the first phase test through the state switch. The source materializes zero separately in later phases. Call and branch counts and ordered referents agree. | Which source lifetime or domain operation makes that zero value live across phases? |
| `BuildViewOcclusion` | Retail loads direction into ESI before clearing the masks and uses EDI for the occlusion index. The source exchanges those registers and loads direction after the clears. Calls, branch destinations, and ordered referents agree. | Which source lifetime or helper boundary explains the direction load and register allocation? |
| `ResolveThreeSpecialRaceFusion` | Retail keeps the third slot in ESI and the staged result in EDI. The source exchanges them; call and branch counts and ordered referents agree. | Which source lifetime or classification-helper boundary accounts for the register choice? |
