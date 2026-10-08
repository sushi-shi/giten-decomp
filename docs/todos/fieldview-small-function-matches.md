# Fieldview and small function differences

These are concrete differences in the normalized retail/current instruction
pairs. Recheck them with `giten walls diagnose <rva> --asm` after changing a
source owner or translation-unit composition.

| Retail RVA | Function | Remaining distinction |
| --- | --- | --- |
| `0x02e1a0` | `FindSkillAffiliation` | The only normalized difference is one SIB byte: retail encodes the affiliation byte load with the character pointer as base and index in `ecx`; current MSVC swaps the two address operands. Calls, edges, values and ordered referents agree. A parent/child compile of the condition-name recovery proves that the added condition enumerators and named age-array bound change this instruction without changing the function fingerprint. The later `DataFileKind` include does not cause it. Preserve the supported condition names and Core model; the open lever is an authentic TU or helper boundary, not a different affiliation expression. |
| `0x002c70` | `ClearMaskSeam` | Its two mask-loop byte loads and `lea` instructions encode the commutative base/index operands in the opposite order. All instruction counts, branches, and referents agree; the existing bank reached 100 with the same source behavior. |
| `0x032c20` | `PushTextDelay` | Retail clears the saved-delay and active-delay bits together with `and eax,0xfffffdfb`, then applies the previous bit and the new low input bit. Current MSVC emits an equivalent XOR/mask sequence. The two typed bitfield assignments are unchanged from the historical exact source; introducing a saved-delay temporary or a `TextState*` owner local added loads and lowered the match. Nearby boolean-typing edits changed this translation unit's compiler state, but did not alter this function's behavior. |

The SIB-only rows have banked exact matches, so a later
translation-unit change may recover them without a local source edit.
