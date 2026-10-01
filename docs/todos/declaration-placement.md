# Declaration placement

Run `python3 -m giten.verify.placement --list` in `nix develop` for the current
list. The census reports a prototype when its defining source file does not
include that header. It is a lead for owner recovery, not proof that two
declarations can be made identical. The [Gruntz declaration audit](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/declaration-name-audit.md)
likewise keeps declaration and definition sites separate so that each can be
checked against its own callers and compiler output.

The palette case `SetPaletteColor` formerly declared in both `AreaNpc.h` and
`Vram.h` now has its single declaration in `Gfx/Palette.h`, included by both.
This closed one of the 31 reported rows. The 30 remaining rows are below.

| Declaration outside its owner | Concrete evidence and required recovery |
| --- | --- |
| `Game/AreaNpc.h`: `GrbToRgb(u16)` | `Gfx/Vram.h` and `vram.c` declare and define `GrbToRgb(u32)`. Retail `LoadNpcPalette` loads a 16-bit palette word into `ax` and pushes `eax` without extending it. Its caller declaration must preserve that 16-bit argument conversion. Keep the caller prototype until the original header boundary or an equivalent proven ABI form is recovered; see `rule-exceptions.tsv` at `0x01f620`. |
| `Game/EquipMagicDefense.h`: `SumEquippedMagicDefenseBonus` returning `i32` | `itemrecord.c` defines an `i16` result (`mov ax, si` in retail). `RecalcDerivedStats` pushes `eax` directly for a 32-bit argument of `CalcMagicDefenseStat`, with no sign extension. Its caller declaration is therefore intentionally `i32`; see `rule-exceptions.tsv` at `0x024f40`. |
| `Text/TextPlaneAttr.h`: six getters with `i16` results | `font.cpp` defines `IsTextPlaneAttrSaved`, `GetTextPlaneCursorX`, `GetTextPlaneCursorY`, `GetTextPlaneHeaderRows`, `GetTextPlaneIndent`, and `GetTextPlaneLineStep` with `i32` results. Script callers use only or sign-extend `ax`. In particular, `OpGetWindowCursor` sign-extends `ax` after `GetTextPlaneCursorX/Y`, whereas the font definitions sign-extend into `eax`. The six `rule-exceptions.tsv` rows at `0x0526c0`, `0x0529b0`, `0x0529d0`, `0x053390`, `0x052cb0`, and `0x052ab0` record the evidence. |
| `Text/TextPlaneAttr.h`: 22 other calls | `ResetTextPlaneAttr`, `ReverseTextPlaneAttr`, `SetTextPlaneColor`, `SaveTextPlaneAttr`, `RestoreTextPlaneAttr`, `RestoreTextPlaneIndentMode`, `SetTextPlaneAttr`, `SaveTextPlaneIndentMode`, `SetTextPlaneIndentEnabled`, `SetTextPlaneIndent`, `ApplyTextPlaneIndent`, `GetActiveTextPlaneIndent`, `GetTextPlaneOrigin`, `ReverseTextRun`, `BlankTextRun`, `GetTextPlaneNormalAttr`, `GetTextPlaneAccentAttr`, `ResetTextPlaneNormalAttr`, `ResetTextPlaneAccentAttr`, `SetTextPlaneNormalAttr`, `SetTextPlaneAccentAttr`, and `ScrollTextWindowLine` have matching declarations in `Text/Font.h`, which `font.cpp` includes through `FontApi.h`. The script TUs use these alongside the six incompatible getters above. Including the entire caller header in `font.cpp`, or replacing the entire caller header with `Font.h` in those TUs, introduces conflicting declarations. These 22 can move only as part of an evidence-backed separation of the original caller and definition interfaces. |

All 28 `TextPlaneAttr.h` declarations have uses outside `font.cpp`; they are
not dead prototypes that can simply be deleted. The active callers are
`windowcolor.c`, `scriptvars.c`, and `scriptactor.c`. The first two each call
both same-signature functions and at least one getter with a different return
width. A narrow copy of the mismatched declarations elsewhere would merely
scatter the same ownership exception. An owner-interface split needs source or
instruction evidence for its boundary and then a full compile/compare.
