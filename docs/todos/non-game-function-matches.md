# Non-Game function matching

These functions still have a source-to-retail difference after their current
types, calls and control flow were checked. The examples name the first useful
remaining distinction. Run `giten walls diagnose <rva> --asm` and compare the
ordered relocations before changing their source.

| Retail RVA | Function | Remaining evidence and next lever |
| --- | --- | --- |
| `0x004430` | `GetHandleSize` | Retail loads `ax` directly into the index register and then masks `eax` to 16 bits; MSVC zeroes `eax` first for the current typed `u16` member. Return casts and local spellings leave the same code. Preserve the `HandleEntry` layout unless a whole-object witness refutes it. |
| `0x002ec0` | `ResetUpperPalette` | Retail clears eight `s_paletteColors` words with an offset loop; the current nonvolatile C stores merge into four dword stores. Byte, word and dword counter/index forms did not recover that loop. A real writer or observer of individual stores is needed before marking palette storage volatile. |
| `0x051e40` | `RedrawTextRun` | Calls and branches match; retail schedules pixel-x initialization after plane lookup while holding the plane in `ebp`, and assigns the attribute/next-column registers differently. Moving initialization or changing pixel width spills the plane and loses the matching shape. Compare `DrawNextTextCell` expansion with `RedrawTextPlane` before changing the shared helper. |
| `0x052840` | `SetTextPlaneColor` | In the dim-color arm, retail shifts color into `ecx`, masks the attribute in `eax`, then ORs into `ecx`. MSVC emits the equivalent attribute-first sequence for the tested expression orders. Other arms match. |
| `0x049dd0` | `StepScreenFade` | Calls, branches and stores match. Retail uses `sub 17` and different registers for zero and the step reload; current source emits `add -17`. Alpha expression and macro spellings tested so far preserve the residue. |
| `0x048e60` | `PollMouseButtons` | Retail loads DirectInput buttons 0 and 1 as separate bytes; the current compiler merges reads into one dword load. Masking each assignment before edge tracking changed more instructions and lowered the match. A justified byte-access boundary is needed. |
| `0x047ef0` | `InitDirect3D` | Retail explicitly writes zero to the second viewport's `dvMinZ` after `ZeroMemory`; keep that assignment. Its clip X and width constants are loaded as floats on the x87 stack, while the current build stores equivalent immediates. Removing the depth assignment loses a retail store and shifts nearby x87 scheduling. |
| `0x046060` | `ProjectVector` | All arithmetic and referents are present. Retail evaluates each matrix row's y/z terms before x; equivalent source grouping canonicalizes to a different x87 order. |
| `0x050f50` | `WinMain` | Calls and branches match; retail holds `PeekMessage` in `esi`, `TranslateMessage` in `ebp`, and next tick in `edi`. Current register choices swap those roles, changing ordered IAT references. Local declaration and loop spellings tested so far compile identically. |
| `0x059180` | `ClickHotspotAt` | Retail biases its loop pointer to `Hotspot.texture`, addressing rectangle fields at negative offsets; current build biases it to `rect.left`. A typed `Hotspot*` candidate local did not change the choice. Preserve the proven `Hotspot` layout and find an ownership/access pattern that explains the pointer basis. |
| `0x0589e0` | `DrawProjectedEffectSprite` | Retail reads the packed bitmap dimensions once as a word, then splits the bytes into `dl` and `bl`, freeing `ebx` to reload the image code later. Direct byte reads use two loads, and width/height local types do not reproduce the register lifetimes. |
| `0x058e40` | `DrawScreenEffectSprite` | Rectangle arithmetic and calls match; retail holds the signed vertical bitmap offset in `dl`, while the current build uses `cl`. Moving the offset declaration did not alter allocation. |
| `0x04bdd0` | `RenderTBox` | The billboard corner products and stores differ in x87 scheduling. Storing a negated X temporary raises a fuzzy score but rounds through `float` at a different point, so it was rejected. Preserve the current arithmetic until a source expression matches retail's store order and precision. |
| `0x04c3c0` | `RenderNPC` | Calls, texture selection and integer flow match. The billboard corner products and their translated stores differ in x87 schedule. A slightly higher historical score is no longer reproduced by the current source; compare its old source hash and floating-point stores before restoring it. |
| `0x04cb30` | `RenderEnemy` | Retail loads both billboard axes twice, calculates the four far-corner products before storing, then copies far Z to near Z with integer moves. The source now expresses far-corner ownership explicitly; MSVC still emits an early X store/reload, leaving an x87 scheduling residue. |
| `0x04e090` | `DrawSceneSprites` | Retail walks pointers to sprite-slot frame words in `ebx`; current source recomputes the indexed slot. Replacing all indexed uses with a `SpriteSlot*` local changes the loop substantially, so the pointer's lifetime and access boundary need a narrower reconstruction. |
| `0x002a00` | `LatchMouseClicks` | Retail loads a 32-bit word from the position's Y half and then a 16-bit X; the current compiler loads a 16-bit Y and 32-bit X. Whole-structure copying changes more code. The storage layout and caller semantics should decide the access shape. |
| `0x0563b0` | `CMidiStream::SetVolume` | The all-channel MIDI loop has matching messages, branches and calls, but retail forms the message in `eax` and holds the output handle in `edx`; the current compiler swaps them. Explicit handle/message locals compiled identically. |

`@early-stop` comments at most sites carry the shorter local rationale. This
list keeps the cross-function evidence and the rejected probes together.
