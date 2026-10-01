# Remaining macro literals outside Game

`giten verify constants --macro-list` reports numeric spellings in function-like
and object-like macro definitions. The following replacement-list families in
Platform, Gfx, Text, Sound, Ui, Util, and Script still need domain or ownership
evidence before a semantic replacement.

| Family | Examples and current evidence |
| --- | --- |
| Serialized packing | `PackMidiVolumeMessage` in `include/Sound/MidiStream.h` uses MIDI status `0xb0`, controller `7`, and byte shifts. `TEXT_ATTR` in `include/Text/TextAttr.h` shifts foreground and dim nibbles by 8 and 4. `GetEffectBitmapOffsetY` in `include/Platform/Scene3D.h` reads the high byte of BMP metadata. These are format encodings; no existing project enum names their byte positions. |
| Resource geometry | `GetSceneSpriteOffsetX` in `include/Gfx/Blit.h` reflects around x=79, `GetTextGlyphWidth` chooses 8 or 16 pixels, and `GetResourceBitmapLastRow` subtracts one row. The values are dimensions and coordinate arithmetic, not enum values. |
| Palette and bitmap formats | `ConvertBitmapPalette` in `include/Gfx/Bitmap.h` iterates 256 BMP colours; the hardware palette's `PALETTE_SIZE` is 16 and is a different array. `BMP_PALETTE_ENTRY` shifts `PC_RESERVED` into the high byte. The two palette sizes must stay distinct. |
| Numeric predicates and masks | `IsPalettizedSurface` in `include/Gfx/DDraw.h` compares bit depth with 16. `IsScreenFadeIn` in `include/Platform/ScreenFade.h` tests bit 0 of fade modes; substituting the enum member `SCREEN_FADE_FROM_BLACK` would falsely present a bit mask as one fade mode. `GetPixelMask` in `include/Util/PixelMask.h` reduces a pixel index modulo eight. |
| Vertex and projection indexing | `SetQuadColor`, `SetQuadSpecular`, `TranslateBillboard`, and `ProjectBillboardRect` in `include/Platform/Scene3D.h` address four ordered vertices and their projected corners with numeric indices. No semantic vertex enum has been recovered. |
| Text and UI encodings | `IsTwoByteTextChar` in `include/Text/TextPlane.h` uses `0x100` to distinguish encoded wide characters; `SaveCurrentTextAttr` sets a one-bit state. `SKIP_TEXT_PLANE` in `include/Platform/Scene3D.h` makes a bit from a dynamic plane index. These values are thresholds, flags, and shifts rather than a closed enum. |
| Script arithmetic | `AdjustActorSpoilAmount`, `RollFixedContestValue`, and `RollRelativeContestValue` in `src/Script/scriptactor.c` contain authored roll increments and clamps. The Script source is under a separate cleanup lane; no enum meaning follows from equal values elsewhere. |
| Structural zero and one | `ReleaseComObject`, `ReleaseTextureSurfaces`, `DrawNextTextCell`, `InitEmptyWordList`, and other `do { ... } while (0)` macros use zero as a null/reset/statement-wrapper value. `ReadImageBytes` increments byte pointers by one. These are mechanical values. |

`src/Platform/winmain.cpp` also has the `SetDistanceLight` macro's float
coefficients and address annotations. Its values describe lighting arithmetic
and referents, not enum members.
