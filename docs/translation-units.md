# Translation-unit ownership

`config/units.toml` records the reconstructed objects, not one object per
subsystem. Keep ordinary function bodies in retail RVA order. Adjacent bodies
alone do not prove a shared object: use the compiler's ordinary data sections,
initializers and emitted helper placement to distinguish joins from boundaries.
Original source filenames can remain unknown after an object's extent is known.

## Bitmap and display operations

`src/Gfx/bitmapio.cpp` owns the continuous ordinary-code span from
`ReadBitmapFile` through `ClickHotspotAt` (RVA `0x56a70` through `0x593cf`).
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
