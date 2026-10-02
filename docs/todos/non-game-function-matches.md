# Non-Game function matching

These functions still have a source-to-retail difference after their current
types, calls and control flow were checked. The examples name the first useful
remaining distinction. Run `giten walls diagnose <rva> --asm` and compare the
ordered relocations before changing their source.

| Retail RVA | Function | Remaining evidence and next lever |
| --- | --- | --- |
| `0x004430` | `GetHandleSize` | Retail loads `ax` directly into the index register and then masks `eax` to 16 bits; MSVC zeroes `eax` first for the current typed `u16` member. Return casts and local spellings leave the same code. Preserve the `HandleEntry` layout unless a whole-object witness refutes it. |
| `0x002ec0` | `ResetUpperPalette` | Retail emits four dword stores for `s_paletteRefs[8..15]`, then an eight-step word-store loop for `s_paletteColors[8..15]` (one back edge, five relocations). Current MSVC merges the nonvolatile colour stores into four more dword stores (no back edge, eight relocations). Byte, word, dword, pointer and index loop forms did not recover the loop. `SetPaletteColor` matches exactly with the same typed colour writer. The retail xref model has only that writer's base reference and this reset's `+0x10` reference to colour storage; no read justifies `volatile`. Keep the ordinary array until a real observer or store-order constraint is found. |
| `0x051e40` | `RedrawTextRun` | Calls, branch count, relocations and 0xa6-byte size match. `DrawNextTextCell` expands in this function and in byte-exact `RedrawTextPlane`; both load the attribute before checking the text byte and then call `ReadTextChar` and `DrawTextCell` on the occupied arm. Retail schedules pixel-X initialization after plane lookup while holding the plane in `ebp`, and assigns the attribute/next-column registers differently. Moving pixel initialization after the guard and `count += x` reproduced the early scheduling order but gave `ebp` to pixel X, spilled the plane into `esi`, shortened the body to 0xa4 bytes and lowered the score; it was reverted. Changing pixel width had the same lifetime problem. Keep the shared helper and current `i16` cursor until a source-supported lifetime change recovers retail allocation. |
| `0x052840` | `SetTextPlaneColor` | The three selectors map to the correct glyph, dim and background masks, and `walls semdiff` found no differing constants or referents. Only the dim arm differs: retail shifts colour in `ecx`, masks the 16-bit attribute in `eax`, ORs into `ecx`, then copies it to `eax`; current MSVC masks first and ORs into `eax`. Mutating the `color` parameter as an accumulator compiled identically to the current arm. Widening the local attribute to `u32` changed the initial load and all arms, dropping the score; keep its `u16` type. The retail-only final register copy explains the two-byte size difference. |
| `0x049dd0` | `StepScreenFade` | Calls, branches and stores match. Retail uses `sub 17` and different registers for zero and the step reload; current source emits `add -17`. Alpha expression and macro spellings tested so far preserve the residue. |
| `0x048e60` | `PollMouseButtons` | Retail loads DirectInput buttons 0 and 1 as separate bytes; the current compiler merges reads into one dword load. Masking each assignment before edge tracking changed more instructions and lowered the match. A justified byte-access boundary is needed. |
| `0x047ef0` | `InitDirect3D` | Retail explicitly writes zero to the second viewport's `dvMinZ` after `ZeroMemory`; keep that assignment. Its clip X and width constants are loaded as floats on the x87 stack, while the current build stores equivalent immediates. Removing the depth assignment loses a retail store and shifts nearby x87 scheduling. |
| `0x046060` | `ProjectVector` | All arithmetic and referents are present. Retail evaluates each matrix row's y/z terms before x; equivalent source grouping canonicalizes to a different x87 order. |
| `0x050f50` | `WinMain` | Calls and branches match; retail holds `PeekMessage` in `esi`, `TranslateMessage` in `ebp`, and next tick in `edi`. Current register choices swap those roles, changing ordered IAT references. Local declaration and loop spellings tested so far compile identically. |
| `0x059180` | `ClickHotspotAt` | Retail biases its loop pointer to `Hotspot.texture` (`+0x14`), reading the rectangle at `-0x10` through `-0x4`; current MSVC biases it to `rect.left` (`+0x4`) and addresses the same fields from there. `Hotspot` is 40 bytes, and the box, NPC and enemy writers fill the rectangle and texture at those offsets. Typed candidate and selected hotspot locals, an address-only `Texture**` local, reversed coordinate assignment and swapped declaration order all compiled byte-identically to current (`hotspotclick.obj` SHA-256 `54ade9981735d991bfb7437ec4ae30929c5450590e1abae11ca27c56c707b7ae`). The clearer scoped hotspot locals are retained. The first actual difference is stack-slot allocation for `hit` at `+0x36`; the pointer basis follows at `+0x5d`. The same calls, branch and return counts, and relocations remain. Further work needs a supported lifetime/access change; the pointer offset alone is not evidence for a different aggregate layout. |
| `0x0589e0` | `DrawProjectedEffectSprite` | Retail reads the packed bitmap dimensions once as a word, then splits the bytes into `dl` and `bl`, freeing `ebx` to reload the image code later. Direct byte reads use two loads. Naming `u8` width/height components introduced an extra stack byte and changed earlier register assignment while preserving all calls and branches; that probe was removed. The `BITMAPFILEHEADER::bfReserved2` word is still the proven packed source. |
| `0x058e40` | `DrawScreenEffectSprite` | Rectangle arithmetic and calls match; retail holds the signed vertical bitmap offset in `dl`, while the current build uses `cl`. Moving the offset declaration did not alter allocation; inlining `GetEffectBitmapOffsetY` into the height expression dropped the current score and was removed. |
| `0x04bdd0` | `RenderTBox` | Retail finishes four x87 products, spills the negative X product once to a float stack slot, then copies those bits to both vertices. Current MSVC stores the product directly to vertex 1 and copies it to vertex 2. A float local assigned after multiplication was optimized away and lowered the score to 94.74; an earlier negated-X temporary raised the fuzzy score but rounded at a different point. Preserve the current arithmetic until a source expression matches retail's store order and precision. |
| `0x04c3c0` | `RenderNPC` | Calls, texture selection and integer flow match. Retail completes four x87 corner products before integer translation; current MSVC interleaves translation after two products and stores corners in a different order. Grouping X/Z by corner compiled the whole `winmain` object byte-identically and reads more clearly. Parenthesizing the negative products changed three referents and lowered the match, so the existing negation order remains. A slightly higher historical score is no longer reproduced by the current source; compare its old source hash and floating-point stores before restoring it. |
| `0x04cb30` | `RenderEnemy` | Retail loads both billboard axes twice, calculates the four far-corner products before storing, then copies far Z to near Z with integer moves. The source now expresses far-corner ownership explicitly; MSVC still emits an early X store/reload, leaving an x87 scheduling residue. |
| `0x04e090` | `DrawSceneSprites` | Retail starts `ebx` at the last slot's `frame` member, reading `group` at `-2` and X/Y at `+2/+4`; current MSVC starts `edi` at the slot base. A typed frame pointer regressed to 61.68, a safe reverse `SpriteSlot*` iterator kept the slot base and fell to 94.80, and a shared `Picture*` lookup fell to 74.60. Naming only the frame value held 95.60 but changed the compiled object. Keep the indexed form until an evidence-backed access pattern yields the frame-member basis. |
| `0x002a00` | `LatchMouseClicks` | Retail loads a 32-bit word from the position's Y half and then a 16-bit X; the current compiler loads a 16-bit Y and 32-bit X. Whole-structure copying changes more code, and widening the local Y to `i32` drops the score to 96.11. The storage layout and caller semantics should decide the access shape. |
| `0x0563b0` | `CMidiStream::SetVolume` | The all-channel MIDI loop has matching messages, branches and calls, but retail forms the message in `eax` and holds the output handle in `edx`; the current compiler swaps them. Explicit handle/message locals compiled identically. An inline wrapper for the two MIDI-send sites changed the call shape, so the direct calls remain. |

`@early-stop` comments at most sites carry the shorter local rationale. This
list keeps the cross-function evidence and the rejected probes together.

## Storage and owner checks

- `HandleEntry` is an eight-byte table entry: `SetHandleEntry` writes flags at
  `+0`, size at `+2`, and pointer at `+4`; `GetHandleSize` reads `+2`, while
  `HandlePtr` reads `+4`. `NewHandle`, resize and clear all use that writer.
  The low score of `GetHandleSize` does not indicate a missing wider size
  field. Retail xrefs to the table agree with these member offsets.
- `MousePosition` has 16-bit X, Y and button fields at `+0`, `+2` and `+4`.
  `SetMouseState` writes each field, and direct readers across Game, Script
  and Platform use those offsets. Retail's 32-bit load beginning at Y in
  `LatchMouseClicks` uses only its low word; it does not establish a 32-bit Y
  member. A whole-struct copy changed the rest of the function substantially.
- Retail references `s_paletteColors` only at its base from `SetPaletteColor`
  and at `+0x10` from `ResetUpperPalette`, both writers. `s_paletteRefs` has
  the expected retain, release and reset references. No retail read supports
  adding `volatile` to force separate palette stores. The current ordinary
  stores preserve the observed final values.
- `CMidiStream` has one virtual destructor slot. Constructors initialize
  `m_channelVolumes` to null; `Play` and `Replay` retain the caller's pointer,
  and `OnMessage` forwards it to replay and volume setting. The only
  reconstructed external `Play` call passes null. The all-channel loop's
  `eax`/`edx` swap is not evidence for a copied array or different stream
  handle type.

## Banked exact current dips

These functions previously reached 100%, and the relevant source operations
remain equivalent to their banked versions. Their current byte differences
should be revisited through translation-unit
composition or a supported source improvement, without changing a proven
storage type merely to force a register allocation.

| Function | Current distinction |
| --- | --- |
| `NewArrayHandle` | Retail loads `count` into `eax` and `size` into `ecx`; current MSVC reverses the two before the same masks and multiply. Swapping multiplication operands in source compiles identically. |
| `GrbToRgb` | Retail copies the low input byte into `dl` and ORs red before blue; current MSVC copies the whole word into `edx` and ORs blue first. A fresh replay of the `249a6690` source and header with current tooling matched all 0x21 bytes. Adding the palette header's `ReleaseImagePalette` prototype before this function is sufficient to change the old TU's output; see the replay evidence below. Keep the proven `u32` ABI and palette types. |
| `MarkPaletteDirty` | Retail loads, ORs and stores the flag byte separately; current MSVC folds this into one memory OR. The palette flag has no asynchronous owner that would justify `volatile`. |
| `QueuePaletteUpload` | Retail tests the flag, sets the queued bit with a load/store, then clears the dirty bit with a second load/store. Current MSVC combines the two changes into one load/store. The final byte is the same; a required observer between the stores has not been found. |
| `PushTextDelay` | Retail clears the `delayOn` bit with a byte mask after merging the saved bit through XOR; current MSVC applies a wide mask earlier. The surrounding bitfield reads and stores agree. Typed local probes worsened the match, and changing the signed bitfield storage would affect its other readers. |
| `ResizeScriptEntry` | Retail updates the running size register in the shrink arm before `ResizeHandle`; current MSVC computes the same `size + delta` in a temporary. Writing `size += delta` in that arm compiled identically, so the branch and handle ownership remain as modeled. |
| `OpReadDataInt` | Retail holds the accumulated integer and descending byte pointer in the opposite registers. The shared `ReadSizedDataInt` helper also feeds the exact `OpReadRecordInt`; changing its signed-byte logic solely for this caller would lose that evidence. |
| `ReadTextChar` | Two `text[pos]` loads have their SIB base and index reversed; the decoded bytes, branches, and return paths otherwise match. The unsigned byte decoder boundary is shared by its callers. |
| `ClearTextPlaneLine` | The text-byte store has the same SIB base/index reversal as `ReadTextChar`; every other instruction matches. Keep the `TextPlaneTextRow` and `TextPlaneAttrRow` typed accessors. |

The `GrbToRgb` replay used the current compiler and retail target. The old
`vram.c` plus old `Vram.h` matched exactly. Substituting the current `Vram.h`,
or adding only its new `Palette.h` include, changed the old function to a
nonmatching register schedule with the same first divergence. An empty
included header, the old `PaletteState` definition alone, or the
`ImagePalette` layout alone left it exact. A forward declaration of
`ImagePalette` plus the
`ReleaseImagePalette` prototype changed it. The original expression and named
colour components compile to the same current function bytes; a typed
low-byte local kept the same first divergence and match score. Removing that
prototype or delaying the complete `ImagePalette` definition in the current
TU did not restore exactness. The prototype is a sufficient trigger in the
old TU, not a complete explanation
of the current state. All probes were reverted.
