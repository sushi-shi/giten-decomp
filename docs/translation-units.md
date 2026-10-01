# Translation-unit ownership

`config/units.toml` records the reconstructed objects, not one object per
subsystem. Keep ordinary function bodies in retail RVA order. Adjacent bodies
alone do not prove a shared object: use the compiler's ordinary data sections,
initializers and emitted helper placement to distinguish joins from boundaries.
Original source filenames can remain unknown after an object's extent is known.

## Bitmap and display operations

`src/Gfx/bitmapio.cpp` owns the continuous ordinary-code span from
`ReadBitmapFile` through `ClickHotspotAt` (RVA span `[0x56a70, 0x593cf)`).
This includes textures, world-map hit tests, automap coordinates, hotspots,
sprites, effects and surface copies.

The ordinary `.data` contribution starts at RVA `0x6dbe8`. Its first definitions
are `s_textureDiffuse`, `s_worldMapRegions`, `s_cursorPreviewRect`,
`s_effectLateralOffsets` and `s_effectHeightOffsets`. The last three precede
`ReadBitmapFile`'s `"rb"` literal even though their users follow the bitmap
loaders in code. Separate objects in code order cannot produce that data order.
Their definitions and every intervening ordinary function therefore belong to
the same object.

`s_wallTextureNames` is a function-local static in `LoadWallTextures`. VC5 emits
it after the earlier functions' literals, followed by its own string literals.
A file-scope definition instead joins the globals before `"rb"` and displaces
the preview/effect data. The function-local definition reproduces the table at
RVA `0x6ddc8`, without padding or section directives. The NPC bitmap name and
hotspot diagnostic continue the same literal run. The combined object retains
the C entry-point linkage and the `/Ox /Zp1 /ML` profile; its source is C++.

To reproduce the ownership check, run `giten build` and inspect
`build/gen/delink_data_manifest.tsv`. For each `bitmapio.c` row in ordinary
`data`, subtract `section_offset` from `rva`: every enrolled row must give
`0x6dbe8`. Check the compiled section's bytes and ordered relocations as well;
the constant difference alone does not prove the payloads. `giten build verify`
checks code order, data ownership, identities, pointer referents and coverage.

## Script variables and sprite opcodes

`src/Script/scriptvars.c` owns `OpLoadSprite` and `OpPlaceSprite` followed by
the variable, choice, timer and script-file operations. The hook defaults
`s_messageHookFile` and `s_messageHookEntry` at RVA `0x69828` precede the sprite
opcodes' diagnostic strings at `0x69830..0x69ecf`; the script-variable
functions' literals follow at `0x69ed0`. Those file-scope hook defaults are
shared by several functions, so moving them into a function cannot explain
separate objects. Their shared `.data` contribution starts at `0x69828`.

The explicitly zero-initialized definitions start with `g_formattedNumber`
at `0x815a0`, continue through the script-variable state, and end with
`g_shopKind` at `0x816a0`. Define that state before the sprite functions while
keeping the functions themselves in RVA order. Separate sprite and variable
objects would reverse their BSS contributions relative to retail.

## Party actions and attacks

`src/Game/partyaction.c` continues through the weapon, skill and item attack
functions, ending at RVA `0xb6c9`. The functions share floating-point operands
at the same retail addresses in `0x64298..0x642f7`. With the project's VC5 C
profile these operands are local symbols in ordinary `.rdata`, not COMDATs;
separate objects cannot share them through COMDAT selection.

Compiling the complete span produces the twelve doubles in precisely retail
order and reproduces the entire 96-byte `.rdata` contribution. Keep this check
on the raw object section: a per-function comparison can accept copies of one
retail constant in several reconstructed objects and thus miss a false split.

## Range, list and field-view helpers

`src/Game/fieldview.c` starts with the range/text helpers, continues through
the linked-list helpers, and then defines the view-occlusion and wall helpers.
Their ordinary function RVA span is `[0xb810, 0xd3e5)`.

The wall-stop table precedes the range helpers' floating-point constants in
ordinary `.rdata`, reversing the order of their code. BSS independently has
the same reversal: `s_drawTable` and `g_viewCells` precede `g_filteredText`,
which is only used by `FilterTextMarks`. Separate objects would order both
sections by their code contributions. The combined object reproduces the
48-byte `.rdata` and 248-byte `.data` contributions exactly; its three BSS
definitions lie at offsets `0`, `0x200` and `0x238` from retail RVA `0x78540`.
The list helpers lie between the two code spans and share their owner.

## Initializer and helper boundaries

The identity-matrix initializer at RVA `0x45d90` opens `d3dapp`'s code, and its
matrix/vector helper copies follow the sound functions at `0x48f90..0x492aa`.
The joystick functions follow that helper band. The camera, view/projection
matrix and IME initializers at `0x49530..0x49609` then open `winmain`.
Preserve those initializer owners when considering adjacent joins.

Do not treat a shared empty constructor's selected address as an ordinary TU
extent. For example, the default D3D vertex/vector constructor copies selected
at RVA `0x568c0` are shared compiler-generated code, whereas the initializers
and ordinary functions above identify their owning contributions.
