# Common-code recovery worklist

The source contains 222 `inline` declaration lines and 1,226 `#define`
declaration lines under `include/` and `src/` (including conditional
variants and constant macros). Those counts are a search boundary, not a
count of debt. Repeated operations should be shared only when their owner,
evaluation order, and compiled call shape support the same boundary. The
separate [vendor macro worklist](vendor-macros.md) records SDK and CRT
spellings. This worklist reviews candidate families, not every reconstructed
function. It has no function-by-function checklist reconciled with the retail
function inventory, as the King's Field common-code review does.

## Recovered project operations

| Operation | Consumers |
| --- | --- |
| `GetEffectFrameEnd` | Both effect-script steppers in `src/Gfx/motion.c` traverse a variable-sized command using the same frame-end expression. |
| `FinalizeAttackDamage` | Item, skill, weapon, and gun damage apply resistance, moon scaling, random percentage, then clamp in the same order. |
| `ApplyFacingDamageBonus` | The same four damage paths apply the same behind/side floating-point multipliers. |
| `ShowPendingLevelUpMessage` | Field exploration and both encounter handlers mark pending rewards, format the next level-up message, and show it for the same lifetime. |
| `ClearPanelLayerSurface` | Four `winmain.cpp` paths and both `font.cpp` paths color-fill the panel surface; the visibility decision stays with each caller. Both consumer objects are byte identical to their pre-extraction objects. |
| `InitOffscreenSurfaceDesc` | `CreatePicture`, `CreateTextPlane`, and `CreateScreenLayer` prepare the same DirectDraw system-memory description with caller-selected dimensions and pixel format. Their edited function bodies retain their pre-extraction bytes. |

## Remaining source readings

| Family and examples | Evidence needed before extraction |
| --- | --- |
| Navigation-pad pixel reads in `LayerAtPoint` and `PadButtonAtPoint` (`src/Text/font.cpp`) | Both lock the layer's DirectDraw surface, read a 16-bit pixel and unlock it. A focused `ReadLayerPixel` inline trial changed the first consumer at its entry and shortened the TU's `.text` by 112 bytes; the trial was removed. Recover the compiler-compatible local lifetime/call boundary before sharing it. |
| Other DirectDraw surface descriptions in `CreateGlyphSurface` (`src/Text/font.cpp`) and `LoadPictureFile` (`src/Gfx/bitmapio.cpp`) | The glyph surface omits pixel format; the file texture selects caps by device type and writes its pixel format before caps. These do not use the recovered offscreen initializer. The four `InitDirect3D` swapped-argument `memset` calls in the vendor worklist also require their existing behavior. |
| Lock descriptions in `LayerAtPoint`, `PadButtonAtPoint`, `CopySurfaceSquare` (`src/Gfx/bitmapio.cpp`) and `ReadSurfaceWord` (`src/Platform/d3dapp.cpp`) | The shared `DDSD_CAPS`/`DDSCAPS_SYSTEMMEMORY` fields precede distinct lock targets and readback rules. `ReadSurfaceWord` is already an out-of-line pixel accessor used by other callers; calling it from the navigation-pad functions would replace their in-place lock sequence. The focused inline trial above changed their code. Keep the four lock sequences pending a compatible boundary. |
| Weapon/gun hit and exceptional rolls in `RollWeaponHit`, `RollGunHit`, `RollExceptionalWeaponAttack`, `RollExceptionalAttack` | Existing `GetExceptionalAttackLuck`, `GetExceptionalAttackBase`, and `ApplyAttackAccuracyConditions` capture proven suboperations. Gun hit adds DANCE evasion before the shared roll shape; the exceptional weapon path returns `SetActionResult` while the other sets the result then returns a constant. Keep those ordered result and RNG paths separate unless a smaller compiler-compatible operation is found. |
| DirectX teardown in `ReleaseDirectX` (`src/Platform/d3dapp.cpp`) and `ReleaseGraphics` (`src/Platform/winmain.cpp`) | The COM releases overlap, but graphics teardown also releases pictures/layers and MIDI state, and the render-target/room-object order differs. `ReleaseComObject` already owns the common COM release operation. The larger sequences have different lifecycle policy and cannot share an ordered release body. |
| Fusion detail formatting in `DrawFusionCharacterDetails` and `DrawFusionPreviewCard` (`src/Game/fusion.c`) | Both format HP, MP, and alignment labels into `g_scratchBuffer` before drawing them. The text coordinates differ and the preview card draws a name first. Inspect the formatting and buffer lifetime as a possible smaller shared operation; the full drawing sequences are distinct. |
| Hotspot resets in three field render paths (`src/Platform/winmain.cpp`) | All three clear `g_hotspots` and `g_hotspotCount`. The first resets the count before clearing the array; the other two reverse that order. Check the retail instruction order and intervening effects before proposing a common reset operation. |

The first five remaining families have been read through their callers and
current helpers; the final two are new leads needing instruction comparison.
Exact duplicate windows were used to find leads, but a shared three-line
window alone does not establish a helper. The inline and macro declaration
counts include established SDK wrappers, domain accessors, enum machinery,
and constants that have no repeated operation to extract.
