# Vendor macro recovery

This is a matching worklist for plausible SDK/CRT spellings that remain
expanded. Library evidence and the pinned header families are described in
[retail libraries and SDK helpers](../vendor-libraries.md). A compatible
expansion is a hypothesis about original source, not proof of authorship.

## Compiled clamp candidates

[`vendor-clamps.tsv`](vendor-clamps.tsv) lists the remaining single-branch
clamps by RVA, function, source file, candidate expression, evidence, and
next step. Every integer entry was compiled with both operand orders in
its real TU. Floating entries were compiled with the bound first, preserving
the original selection when the comparison is unordered. The classification
describes that macro candidate against retail, not a new diagnosis of the
retained source.

These rewrites do not yet preserve the existing match. Check the recorded
earliest divergence before register experiments: a conditional store can
become an unconditional store, a comparison can reverse, and a conditional
expression can promote a narrow operand. Several functions were already
below exact before the macro experiment. `CalcMaxHp` improved in the tested
state but did not preserve its banked exact match. Keep the current bodies
until these differences have an evidence-backed resolution.

Reproduce a row with `giten walls priors <rva>` and
`giten walls diagnose <rva> --asm`, then `giten match <unit>`. Inspect both
the instruction stream and ordered relocations; compare with the saved
pre-edit state as well as MAX. Changing a header can perturb an unchanged
function. For a project inline helper, check every caller too.

## Other macro candidates

| Site | Candidate and remaining work |
| --- | --- |
| `RenderEnemy` in `src/Platform/winmain.cpp` (`0x04cb30`) | Two `D3DRGB(light, light, light)` sites: specular lighting and distance shading. Direct substitution and a shared packed-color temporary preserve the conversion/call count but change allocation and lose the existing match. Compare the color expression's lifetime and OR/shift schedule; the other supported lighting sites use `D3DRGB`. |
| `ScrollTextPlaneText` in `src/Text/font.cpp` (`0x053190`) | `min(p->cols, 80)` and the reversed operand order both lose the existing match. Inspect the comparison, narrow `cols` temporary, and register lifetime across the four moves. |
| `ScrollTextPlaneSurface` in `src/Text/font.cpp` (`0x0532b0`) | `max(p->headerRows - 1, 0)` and the reversed operand order do not retain exact output. The original tests `headerRows < 1` before subtraction. Recover the promoted type and branch/scope shape before retaining the macro. |
| `StepWorldMapTravel` in `src/Game/worldtravel.c` (`0x011750`) | Both orders of `max(abs(step.x), abs(step.y))` lose the exact match. Check signed coordinate promotion and the lifetimes of both `abs` results. |
| `DrawStairs` in `src/Platform/winmain.cpp` (`0x04da30`) | Four `D3DVAL` conversions of `x0`, `x1`, `z0`, `z1` do not retain exact output despite the same value conversion. Inspect register allocation and temporary lifetimes around the vertex assignments. |
| `BuildRoomGeometry` in `src/Platform/winmain.cpp` (`0x04f780`) | Four `min` candidates for `countX` and `countY` lose exact output when tested separately from its retained `D3DVAL` conversions. Inspect the narrow counts and the width/height boundary branches; the direct repeated-operand ternaries remain. |
| `DrawProjectedEffectSprite` in `src/Gfx/effectdraw.cpp` (`0x0589e0`) | Two `D3DVAL` conversions of destination width/height reduce the current match. These are separate from the clamp candidates in the TSV; inspect FP schedule and the conversion/division boundary. |
| DirectMusic GUID blocks | Match the complete GUID blocks against era `DXGUID.LIB` objects or prerelease headers before assigning an SDK revision or an `INITGUID` owner. Retail matches the pinned `IID_IDirectMusicBuffer` and `IID_IDirectMusicObject`, but not its `IID_IDirectMusic` or `IID_IDirectMusicPerformance`. Library availability, a matching GUID, and runtime interface use are separate claims. |

## Source-reviewed boundaries

The following are deliberately not blanket macro substitutions. They need
additional evidence or a different expression, rather than a codegen search
over an incorrect replacement.

| Sites | Why the apparent macro is not sufficient; next step |
| --- | --- |
| `CMidiStream::FreeBuffers` | `GlobalFreePtr` in the pinned `WINDOWSX.H` evaluates `GlobalHandle` twice. Retail caches one handle, unlocks it, then frees it. Preserve the one-call sequence unless another era definition is demonstrated. Other matching allocation/free sequences already use `GlobalAllocPtr` / `GlobalFreePtr`. |
| Four description clears in `InitDirect3D` | `memset(&desc, sizeof(desc), 0)` has zero length. `ZeroMemory(&desc, sizeof(desc))` would introduce writes absent from retail. Any source recovery must retain the swapped arguments' behavior. |
| `DD_OK`, `D3D_OK`, `DS_OK`, and WinMM result comparisons | `FAILED` / `SUCCEEDED` test a signed HRESULT range, not an exact success code. WinMM returns `MMRESULT`, not HRESULT. Change a site only if the retail branch tests the signed range. |
| `ClampInt`, `ClampShort`, `ClampUShort` | The `else if` gives the lower-bound arm priority even when `lo > hi`. A nested `min(max(...))` changes that case. Establish a range precondition or retain the authored helper; callers already use it. |
| `TrainingThreshold` | The middle arm computes `level - 1`, while the outer arms yield `0` and `99`. A simple clamp of `level` or `level - 1` changes endpoints. Preserve this piecewise rule. |
| Fixed-bound, multi-arm clamps in `DrawHotspotMarks`, `RenderPanelMode`, `ReleasePartyPanel`, `PlaceDraggedLayer`, `ApplyItemDamageRatio`, `ScaleDamageByEquipment`, `ScaleLevelGap`, and `WeightWorldTravelCandidates` | Source-review leads, not compiled macro recoveries. Preserve arm priority, overflow behavior, and boundary inclusivity before testing a nested expression. Drag limits also depend on the layer's dimensions. Diagnose the entire branch family rather than converting one arm into an unconditional assignment. |
| Packed game/script words, effect bitmap offsets, and Shift-JIS words | `LOWORD` / `HIWORD` and `LOBYTE` / `HIBYTE` impose unsigned widths. Effect offsets are signed bytes and game fields include nibbles/bitfields. Retain domain helpers until the complete value's representation agrees with an SDK expansion. |
| MIDI volume messages and stream-event alignment | The project packs a MIDI status/channel/controller message and aligns variable-length event payloads. `MEVT_EVENTPARM` is already used where applicable; `mmioFOURCC` is a different byte layout. Establish an actual SDK macro for the remaining operation before replacing it. |
| Generic `memset`, `memcpy`, and `memmove` in game data, pixel buffers, and MIDI payloads | `ZeroMemory`, `CopyMemory`, `MoveMemory`, and `FillMemory` can expand to these calls, but a CRT call alone cannot identify the historical wrapper. SDK record clears use `ZeroMemory`; extend the family only with ownership or source-lineage evidence. |
| Manual `RECT` operations and `WinMainProc` | Win32 rectangle helpers are imported functions; MFC rectangle methods require an MFC owner. Message-cracking macros require compatible handler boundaries and parameter extraction. Neither an MFC class nor those extra calls/handlers are established here. |
| Scalar Direct3D multiplication/division | `D3DMultiply` is ordinary multiplication, while `D3DDivide` explicitly promotes both operands to double and rounds the result to float. Check x87 precision and rounding boundaries; ordinary arithmetic alone does not identify either spelling. |

The CRT stream and character-table sweep found existing `_fileno` and
`_ismbblead` uses, with no remaining raw `FILE`/ctype internals to convert.
Likewise, the C COM callers use SDK dispatch macros; no raw `lpVtbl` dispatch
remains. MFC/ATL macro recovery has no identified library or class owner in
this target. Reopen those families only when new evidence establishes one.

Source-hash resets from retained recoveries belong in
[`syntactic-recovery.tsv`](syntactic-recovery.tsv), not the deferred-candidate
table. Resolve those with the retained macro as the base.
