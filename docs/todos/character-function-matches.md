# Character function matching

These three character functions still have compiler-shape differences after
their calls, referents and behavior were checked. Keep source validity and
retail side effects ahead of a higher fuzzy score.

| Retail RVA | Function | Remaining distinction |
| --- | --- | --- |
| `0x03e3c0` | `PollTextPartySlotSelection` | Calls, branches and ordered referents agree. Retail holds the text column in `edi` from `TextPlaneCellAt` through `SetTextPlaneHighlight`, and the selection mode in `ebx`; current MSVC exchanges those registers. `TextPlaneCellAt` writes 16-bit X/Y outputs. A prior higher-scoring volatile-byte output and a column-dependent sentinel were rejected because they alter that output contract or highlight behavior. The retail function has no recovered callers, but its body remains modeled. |
| `0x03e6d0` | `AddCondition` | All 87 calls, 60 branches, 32 returns, and ordered referents agree. Eight blocked exits in the poison, ice, doze and tipsy escalation cases branch to the shared return in the vampire/injury cases in retail; current MSVC selects another identical blocked-return tail. The result remains `CONDITION_ADD_BLOCKED` on each path. Loop, goto, nested-if and case-order forms already tested do not recover this tail choice. |
| `0x0413f0` | `ApplyMoonPhase` | Retail saves `esi` before its null check, restores it on the null return, and computes `character->personalFlags` only after that check. Current MSVC defers the save until the pointer is needed. Wrapping the body in a non-null guard saved `esi` early but merged two retail returns into one; widening the change counter or returning it on the null path did not help. An older source revision computed the flags pointer before the check and scored higher, but that ordering cannot justify evaluating a member address on a null character. `ApplyClockChanges` can pass an empty party slot here, so the guard is live. |

Use `giten walls diagnose <rva> --asm` to recheck the first differing edge or
register before trying a new source structure. No class or storage-layout
change is supported by these residues.
