# MSVC 5 compiler warnings

An audit compiled all 82 configured translation units with their
`config/units.toml` profiles and captured 392 warnings. All point into project
`src/` files, rather than MSVC, SDK, or vendored headers.
The compile driver normally discards successful compiler output, so these
diagnostics are not shown by an ordinary build.

The source-supported fixes include `OpSaveObjectConditions` passing the five
condition-bit bytes that `SetFlagTag` reads. The debug-menu's two const tables
now pass through a const `SetMenuItems` parameter and a const `MenuEntry` view.
Its handlers only read their rows; the integrated 82-unit compile/compare keeps
the score unchanged.
The flag-operation and random-bound widths in two script units remove five more
C4761 diagnostics without changing their COFF objects.
The private `OpenSystemMenu` wrapper now carries the 16-bit item-count width
of `SetMenuItems`, removing two further diagnostics with identical COFF output.
The private mouse-click helper now takes the promoted button-mask result as a
32-bit predicate, removing two more without changing its COFF object.
The remaining reviewed warning groups are:

| Warning | Count | Written site and evidence still needed |
| --- | ---: | --- |
| C4805, mixed `bool` and integer comparison | 9 | `LoadTexture` and `OpenTextureBitmap` compare their integer-backed `fromFile` parameter with `true` at five sites; `InitDirectDraw` and `WinMain` compare the Win32 `BOOL` registry field `hardwareOnly` at two; `HighlightHotspot` compares its integer `on` parameter; and `GetShownPanelCharacter` compares the screen layer's integer `visible` member. MSVC 5 compiles these as strict comparisons with 1, which the retail bytes support. Direct truthiness changes the generated functions. The registry record is not validated, so a noncanonical nonzero `hardwareOnly` can reach this check; the other callers currently pass canonical 0/1 values. Keep the boolean spelling and the warning until a stronger type or ownership model preserves the match. |
| C4133, incompatible pointer | 1 | `DecodeAreaMap` in `src/Game/clock.c` assigns `base + src->doorsOffset` (`u8*`) to `AreaLevel.doors` (`DoorCell*`). The source is a packed area-record offset; recover the record's typed cell boundary before changing this conversion. |
| C4761, integral size mismatch in argument | 382 | Explicit width transitions at calls across 36 source files. For example, `RandomPercent` in `src/Util/range.c` passes 32-bit bounds to `RandomAverage(i16, i16, i16)`; `OpCloseScriptPanel` in `src/Script/scripttext.c` passes `ReadScriptValue()` to a narrower image key; `LoadNpcPalette` in `src/Game/treasurebox.c` passes an `i16` expression to a narrower palette API. Check each caller's value range and the retail argument width before altering a declaration or inserting a narrowing conversion. See [the focused width audit](warning-width-boundaries.md). |

The C4761 sites by translation unit are listed below so the full audit scope
survives even though compiler output under `build/` is ignored. Counts are
individual diagnostics, not distinct source lines.

[Script and graphics width boundaries](warning-width-boundaries.md) and
[Game argument-width warnings](argument-width-warnings.md) record reviewed
source groups, retained conversions, and the evidence still needed for a
change. To regenerate the individual written-site worklist after source or
signature changes, run this from `nix develop`:

```sh
giten verify compiler-warnings
giten verify compiler-warnings --list C4761
```

The command compiles all 82 configured units to disposable objects without
linking or running a test suite. It publishes
`build/gen/compiler_warnings.tsv` and
`build/gen/compiler_warnings_coverage.json` only when every unit compiles and
the source snapshot remains unchanged. Each TSV row retains the unit, source
line, warning code, occurrence on that line, message and source spelling.
VC5 gives line-only locations here, so the column is blank; repeated
diagnostics on one line remain separate rows. The per-unit counts below are
a review snapshot; the generated report is the current worklist.

When one line contains multiple diagnostics, inspect the callee parameter
types and each argument expression. For example, `RandomAverage(lo + 100,
hi + 100, 0)` in `src/Util/range.c` produces two warnings on one line, one
for each 32-bit bound passed to an `i16` parameter.

| Unit | Count | Unit | Count | Unit | Count |
| --- | ---: | --- | ---: | --- | ---: |
| `character.c` | 14 | `clock.c` | 13 | `debugmenu.c` | 3 |
| `equipeffect.c` | 5 | `fieldmain.c` | 16 | `fieldobj.c` | 15 |
| `fieldscreen.c` | 14 | `fieldview.c` | 25 | `fusion.c` | 8 |
| `gameloop.c` | 1 | `itemattack.c` | 2 | `itemrecord.c` | 7 |
| `partyaction.c` | 9 | `savegame.c` | 2 | `skillattack.c` | 3 |
| `skilluse.c` | 6 | `statestack.c` | 27 | `statuspanel.c` | 43 |
| `treasurebox.c` | 35 | `worldtravel.c` | 14 | `blit.c` | 4 |
| `motion.c` | 1 | `vramaccess.c` | 2 | | |
| `vec3.c` | 3 | `handle.c` | 5 | `eventflags.c` | 9 |
| `scriptactor.c` | 46 | `scriptctx.c` | 5 | `scriptfield.c` | 3 |
| `scriptswitch.c` | 10 | `scripttext.c` | 2 | `scriptvars.c` | 16 |
| `scriptvm.c` | 3 | `windowcolor.c` | 5 | `windowtext.c` | 3 |
| `range.c` | 3 | | | | |

These warnings alone do not authorize wider parameters: changing a public
parameter can alter C++ mangling, caller extension, stack slots, or MSVC code
generation. Preserve the established ABI while recovering the actual domain
and call boundary.
