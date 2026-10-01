# Common-code recovery worklist

The source contains 222 `inline` declaration lines and 1,224 `#define`
declaration lines under `include/` and `src/` (including conditional
variants and constant macros). Those counts are a search boundary, not a
count of debt. Repeated operations should be shared only when their owner,
evaluation order, and compiled call shape support the same boundary. The
separate [vendor macro worklist](vendor-macros.md) records SDK and CRT
spellings.

## Recovered project operations

| Operation | Consumers |
| --- | --- |
| `GetEffectFrameEnd` | Both effect-script steppers in `src/Gfx/motion.c` traverse a variable-sized command using the same frame-end expression. |
| `FinalizeAttackDamage` | Item, skill, weapon, and gun damage apply resistance, moon scaling, random percentage, then clamp in the same order. |
| `ApplyFacingDamageBonus` | The same four damage paths apply the same behind/side floating-point multipliers. |
| `ShowPendingLevelUpMessage` | Field exploration and both encounter handlers mark pending rewards, format the next level-up message, and show it for the same lifetime. |

## Remaining source readings

| Family and examples | Evidence needed before extraction |
| --- | --- |
| Navigation-pad pixel reads in `LayerAtPoint` and `PadButtonAtPoint` (`src/Text/font.cpp`) | Both lock the layer's DirectDraw surface, read a 16-bit pixel and unlock it. A focused `ReadLayerPixel` inline trial changed the first consumer at its entry and shortened the TU's `.text` by 112 bytes; the trial was removed. Recover the compiler-compatible local lifetime/call boundary before sharing it. |
| Panel hide/clear in `RunMoveCommand`, three `HandleInput` paths (`src/Platform/winmain.cpp`), and `ErasePictureSurface` (`src/Text/font.cpp`) | All set panel visibility false and color-fill the surface. `HideScreenLayer(SCREEN_LAYER_PANEL)` already implements this operation out of line, but substituting a call changes the consumer's call sequence. Establish whether an inline or macro spelling preserves the observed calls before replacing five sites. |
| DirectDraw surface descriptions in `CreateGlyphSurface`, `CreateTextPlane`, `CreateScreenLayer` (`src/Text/font.cpp`) and `LoadPictureFile` (`src/Gfx/bitmapio.cpp`) | All initialize `DDSURFACEDESC` with caps, width, height and sometimes pixel format. Surface type, target storage, error cleanup, and selected pixel format differ. Separate the common description initializer from the caller-specific creation policy; avoid the four `InitDirect3D` swapped-argument `memset` calls documented in the vendor worklist. |
| Lock descriptions in `LayerAtPoint`, `PadButtonAtPoint`, `CopySurfaceSquare` (`src/Gfx/surfacecopy.cpp`) and `InitDirect3D` (`src/Platform/d3dapp.cpp`) | The shared `DDSD_CAPS`/`DDSCAPS_SYSTEMMEMORY` fields precede distinct lock targets and readback rules. The navigation-pad trial above shows a straightforward inline is not yet codegen-compatible. |
| Weapon/gun hit and exceptional rolls in `RollWeaponHit`, `RollGunHit`, `RollExceptionalWeaponAttack`, `RollExceptionalAttack` | Existing `GetExceptionalAttackLuck`, `GetExceptionalAttackBase`, and `ApplyAttackAccuracyConditions` capture proven suboperations. Nearby code still differs in weapon range, critical result, or caller policy; compare full ordered RNG calls and result assignments before combining a larger block. |
| DirectX teardown in `ReleaseDirectX` (`src/Platform/d3dapp.cpp`) and `ReleaseGraphics` (`src/Platform/winmain.cpp`) | The COM releases overlap, but graphics teardown also releases pictures/layers and MIDI state, and the render-target/room-object order differs. `ReleaseComObject` already owns the common COM release operation. Do not merge the larger sequences without order evidence. |

Exact duplicate windows were used to find leads; a shared three-line window
alone does not establish a helper. The inline and macro declaration counts
include established SDK wrappers, domain accessors, enum machinery, and
constants that have no repeated operation to extract.
