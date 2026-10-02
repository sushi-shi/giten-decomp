# Fieldview and small function differences

These are concrete differences in the normalized retail/current instruction
pairs. Recheck them with `giten walls diagnose <rva> --asm` after changing a
source owner or translation-unit composition.

| Retail RVA | Function | Remaining distinction |
| --- | --- | --- |
| `0x00cdc0` | `GetMouseWorldCell` | Staging the mouse position and block offset in a `MapCoord origin` matches every calculation through the final coordinate store. MSVC duplicates the return block (two returns versus retail's one). Updating the returned cell directly gives one return but changes the earlier mouse-coordinate register allocation and division schedule. Copying `origin` through separate member stores or replacing it with scalar `x`/`y` locals both lowered the match to 84.89%; aggregate initialization and a post-branch copy also lowered it. Keep the staged coordinate ownership while looking for a source form with one shared exit. |
| `0x00d2a0` | `GetFacingBit` | Retail loads the facing word into `cx` before shifting; current MSVC loads only its low byte into `cl`. The facing storage is a 16-bit direction and the shift observes only `cl`. Narrow and wide local/cast spellings tried so far retain the byte load. |
| `0x045930` | `DrawSceneSprite` | Retail holds the object pointer in `ecx` on the nonzero-mode path, while current MSVC takes `eax` and reloads the pointer before its second visibility check. The resulting add uses the one-byte-shorter `eax` encoding and changes the later flag/offset register allocation. Its 8 calls, 15 branches, 3 returns, and 20 ordered referents agree. A redundant pointer alias compiled to the same state; simplifying the proven `TestFieldObjectFlag(...) != true` check to logical negation changed the code and scored lower. |
| `0x02e1a0` | `FindSkillAffiliation` | The only normalized difference is one SIB byte: retail encodes the affiliation byte load with the character pointer as base and index in `ecx`; current MSVC swaps the two address operands. Calls, edges, and values agree. The function body remained unchanged from the initial exact bank through the first recorded 99.52% dip. That interval added the evidence-backed `DataFileKind` include to its translation unit and changed shared headers, so the next probe belongs at TU state rather than the affiliation expression. |
| `0x002c70` | `ClearMaskSeam` | Its two mask-loop byte loads and `lea` instructions encode the commutative base/index operands in the opposite order. All instruction counts, branches, and referents agree; the existing bank reached 100 with the same source behavior. |
| `0x032c20` | `PushTextDelay` | Retail clears the saved-delay and active-delay bits together with `and eax,0xfffffdfb`, then applies the previous bit and the new low input bit. Current MSVC emits an equivalent XOR/mask sequence. The two typed bitfield assignments are unchanged from the historical exact source; introducing a saved-delay temporary or a `TextState*` owner local added loads and lowered the match. Nearby boolean-typing edits changed this translation unit's compiler state, but did not alter this function's behavior. |

The SIB-only rows have banked exact matches, so a later
translation-unit change may recover them without a local source edit.
