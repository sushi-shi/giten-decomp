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
| Input bit positions | `include/Platform/Joystick.h` gives `JOY_LEFT` bit value `0x04`, while `JOY_SIDESTEP` shifts the button position `4` by `JOY_BUTTON_SHIFT` to reach a different bit in the same packed input word. Equal source digits do not identify the same action. `MOUSE_UP` in `include/Input/Mouse.h` is a single button's nibble state; `MOUSE_BUTTONS_NONE` is the whole polled button bit set. Both are zero but have different owners. |
| Vertex and projection indexing | `SetQuadColor`, `SetQuadSpecular`, `TranslateBillboard`, and `ProjectBillboardRect` in `include/Platform/Scene3D.h` address four ordered vertices and their projected corners with numeric indices. No semantic vertex enum has been recovered. |
| Text and UI encodings | `IsTwoByteTextChar` in `include/Text/TextPlane.h` uses `0x100` to distinguish encoded wide characters; `SaveCurrentTextAttr` sets a one-bit state. `SKIP_TEXT_PLANE` in `include/Platform/Scene3D.h` makes a bit from a dynamic plane index. These values are thresholds, flags, and shifts rather than a closed enum. |
| Text colour selectors | `TEXT_COLOR_GLYPH`, `TEXT_COLOR_DIM`, and `TEXT_COLOR_BG` in `include/Text/Font.h` are selectors 0..2 for `SetTextPlaneColor` and `OpSetWindowColor`. `TextColorIndex` instead gives actual palette colours such as black=0 and dark red=1. The equal values index different quantities. |
| Script arithmetic | `AdjustActorSpoilAmount`, `RollFixedContestValue`, and `RollRelativeContestValue` in `src/Script/scriptactor.c` contain authored roll increments and clamps. The Script source is under a separate cleanup lane; no enum meaning follows from equal values elsewhere. |
| Structural zero and one | `ReleaseComObject`, `ReleaseTextureSurfaces`, `DrawNextTextCell`, `InitEmptyWordList`, and other `do { ... } while (0)` macros use zero as a null/reset/statement-wrapper value. `ReadImageBytes` increments byte pointers by one. These are mechanical values. |

The number 80 also crosses unrelated representations: `GetPanelTextCell`
uses `TEXT_PLANE_MAX_COLS` as a row stride, `GAUGE_WIDTH` in
`src/Text/font.cpp` counts drawn pixels, and `GetSceneSpriteOffsetX` mirrors
around pixel coordinate 79. The common number does not merge text columns,
gauge width, and sprite placement into one domain.

`src/Platform/winmain.cpp` also has the `SetDistanceLight` macro's float
coefficients and address annotations. Its values describe lighting arithmetic
and referents, not enum members.

Most object-like report rows are declarations of their own constants rather
than opportunities to substitute another name. `include/Giten/Resource.h`
contains resource IDs; equal numeric IDs in unrelated Game, sound, or
rendering domains do not identify shared types. Other object-like definitions
are geometry (`MARK_SIZE`, `PAD_SIZE`, `PARTY_PANEL_WIDTH`), counts
(`OBJECT_TEXTURE_COUNT`, `ENEMY_PICTURE_COUNT`), packed masks
(`MOUSE_STATE_MASK`, `FLAG_BANK_MASK`), and timing (`FRAME_INTERVAL`,
`INPUT_DELAY_FRAMES`). They retain their separate owners. The clickable
hotspot bottom edge did have a proved owner: `HOTSPOT_VIEW_BOTTOM` now names
`VIEW_HEIGHT` because the hotspot rejection check bounds the same 328-pixel
3D view.

Shared all-ones spellings also stay separate. `PALETTE_ENTRY_NONE=0xff`
marks an image palette entry that retains no hardware slot, while
`SCRIPT_BLOCK_OBJECT=0xff` selects a field object's script block.
`SCRIPT_PANEL_NO_JUMP=0xffff` is a panel row's absent script jump, and
`TEXT_PLANE_FREE=0xffff` marks a free plane kind. Neither pair shares a
producer or a consumer.
