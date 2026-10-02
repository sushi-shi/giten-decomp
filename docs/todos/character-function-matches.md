# Character function matching

These character functions still have compiler-shape differences after
their calls, referents and behavior were checked. Keep source validity and
retail side effects ahead of a higher fuzzy score.

| Retail RVA | Function | Remaining distinction |
| --- | --- | --- |
| `0x03e3c0` | `PollTextPartySlotSelection` | Fresh current compilation is 96.2585% with 150 instructions; the retained MAX is 99.4558%, and retail has 149 instructions. All 15 calls, 12 branches, four returns, and 36 ordered referents agree. At `+0xd1`, retail puts the word-start column returned by `TextPlaneCellAt` in `ebx` and later holds selection mode in `edi`; current MSVC puts the word-start column in `edi` and mode in `ebx`. The separate output cell column is written through an `i16*` but unused; the output row drives the slot index and highlight. Naming these distinct outputs and the return compiled to the same current instructions. A prior higher-scoring volatile-byte output and a column-dependent sentinel were rejected because they alter the output contract or highlight behavior. No retail callers have been recovered. |
| `0x03e6d0` | `AddCondition` | All 87 calls, 60 branches, 32 returns, and ordered referents agree. Eight blocked exits in the poison, ice, doze and tipsy escalation cases branch to the shared return in the vampire/injury cases in retail; current MSVC selects another identical blocked-return tail. The result remains `CONDITION_ADD_BLOCKED` on each path. Loop, goto, nested-if and case-order forms already tested do not recover this tail choice. |
| `0x0413f0` | `ApplyMoonPhase` | Retail saves `esi` before its null check, restores it on the null return, and computes `character->personalFlags` only after that check. Current MSVC defers the save until the pointer is needed. Wrapping the body in a non-null guard saved `esi` early but merged two retail returns into one; widening the change counter or returning it on the null path did not help. An older source revision computed the flags pointer before the check and scored higher, but that ordering cannot justify evaluating a member address on a null character. `ApplyClockChanges` can pass an empty party slot here, so the guard is live. |
| `0x010db0` | `AlignmentConflicts` (`fieldobj`) | All six calls, seven branches, four returns and six ordered relocations match. Retail saves the first alignment-class result in `esi` before loading the second argument into `ax`; current MSVC loads that argument into `cx` before saving the result. The B-class path similarly exchanges the argument and sum scratch registers. Using separate A/B class locals preserved the 0x8e-byte, 55-instruction shape but lowered the score from 84.64 to 84.45, so the probe was restored. `GetAlignmentClassA/B` read the proven signed byte members at `+0x7b/+0x7a`; no type or branch change is supported by this residue. |

Use `giten walls diagnose <rva> --asm` to recheck the first differing edge or
register before trying a new source structure. No class or storage-layout
change is supported by these residues.
