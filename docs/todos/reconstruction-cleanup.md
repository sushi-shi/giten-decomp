# Reconstruction cleanup

The [King's Field reconstruction debt checklist](https://github.com/sushi-shi/kings-field-decomp/blob/5a3469b3d81babb022661a173c2a86404fdf6bbd/README.md#reconstruction-debt)
and [Gruntz cleanliness policy](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/cleanliness-metrics.md)
provide review methods. Their counts and source names do not describe Giten.
Here, an equal value, repeated expression, cast, union, `goto`, or padding byte
is a lead; the retail instructions, object layout, producers and consumers
decide whether a cleanup is correct. Keep exact matches and record necessary
exceptions in [rule-exceptions.tsv](rule-exceptions.tsv).

The live worklists are `giten verify board`, `giten verify constants`,
`giten verify c-casts`, `giten verify enum-reuse`, and `giten verify casts`.
The board's measured rows
and floors are in `scripts/giten/verify/board.py` and `config/cleanliness/`;
the enum decisions are in `config/reviews/enum-reuse.tsv`. Reports under
`build/gen/` are derived and are not another handwritten ledger. Regenerate
the compile database with `python3 -m giten.graph.compdb` before a Clang
census if the unit manifest changed.

## Open source models

The [typed-boundary ledger](typed-boundaries.md) gives the remaining cast,
layout and owner-seam examples at their source locations.

| Boundary | Current evidence and next step |
| --- | --- |
| Shared actor and character layout | `include/Game/FieldSight.h` has a partial `FieldActor` view, and `FieldObject` repeats a `Character` prefix beginning at `kind`. Callers such as `GetCombatantCoord`, `KnockBack`, `SpawnActorGroup`, and script actor operations convert between these layouts. The exact-match exceptions identify the affected functions. Recover a single typed common prefix from whole-object copies, offsets, callers, and storage before replacing the casts; a whole `Character` would overlap the field object's script pointer. |
| Unread field-object bytes | `FieldObject.unknownAfterTraining[3]` lies between training points and drop chance. There is no interpreted source read. Keep the identity TODO until a retail reader or record-format witness establishes its role; changing the name to `pad` would merely hide the uncertainty. |
| Address-derived member names | `giten verify board --offset-fields` lists 14 declaration sites, including `FieldObject.byte083`, `byte096`, `word098`, `word21d/21f/221`, `Character.byte069`, and the area-map `byte24/25/3e/3f`. [The field ledger](offset-fields.md) records stores and copies. Trace retail readers and complete record layouts before renaming them. |
| Cross-layer function prototypes | `winmain.cpp` calls `RedrawFieldAt` and `SetMouseState` through function-pointer conversions, and passes unsigned map-size locals to `GetMapSize(i16*, i16*)`. The Windows and C declarations disagree at these seams. Recover the original per-unit declaration and argument width before removing a conversion; the current call bytes alone do not prove the authored prototype. |
| Packed data conversions | `bitmapio.cpp` treats bitmap palette entries as DWORDs, and `d3dapp.cpp` passes float bit patterns as DWORD light state values. These are real representation boundaries until the SDK parameter and retail load widths support a typed replacement. `GetBitmapPixels` and `GetNextBitmap` use variable BMP record lengths, not fixed-member offset views. |
| Heterogeneous texture argument | `OpenTextureBitmap(Texture*, const char* name, b32 fromFile)` takes a file path when `fromFile` is true and a borrowed `BmpFile*` otherwise; callers in `layertexture.cpp` and `objecttexture.c` pass bitmap data through the string-typed parameter. Retyping the C++ function changes mangling. Recover a typed wrapper or the original API boundary before removing the casts. |
| Conditional data claims | `giten.verify.placement.short_data_claims()` still reports `g_worldTravelTerrainFlags` (16 claimed bytes in a 72-byte row) and `s_messageWindow` (2 in 16). The apparent tail of `s_shotRise` was a separately referenced Shift-JIS string and now has its own data record. [The extent review](data-extent-claims.md) records the bytes and xrefs for the two remaining rows; do not pad a declaration merely to fill a row. |
| Declaration ownership | `giten.verify.placement.misplaced_declarations()` is the live list. The scene-refresh, screen-layer, palette, handle-table, party-step, and script declarations have moved to their owners. The remaining 30 rows include 28 text-plane attribute declarations in `TextPlaneAttr.h` whose caller signatures disagree with `font.cpp`, plus the `GrbToRgb` and magic-defense prototype mismatches. Resolve their source and ABI widths before moving them. |
| Resource ID compares | The board's unnamed-domain comparisons are the paired sound-ID mappings in `MapEffectSoundId` and `MapSoundEffectId`. Both recognize IDs 11, 19, 23, 30, 32, 59, 62, 63, 64, 80, 83, 86, 99, 103, and 106; their treatment of 59 differs. The numbers are resource identifiers without proven names. A resource table or script label must establish names; numeric aliases would not improve the model. |
| Relative directions and wall words | `RelativeDirection` returns observer-relative 0..3 except when the observer faces north, so making its whole return domain `ViewDirection` would merge two frames. `RotateByDirection` also accepts arithmetic quarter turns. `WallStops` accepts both a WallKind and a full rotated cell word before masking. `ObjectRecordId`'s unloaded-layer `-1` sentinel is likewise outside its record ID domain. Recover the conversion boundaries before annotating those raw parameters. |
| Partly known display modes | `RenderMode`, `BlankRenderStep`, and `CellPaletteMode` now carry the renderer and palette paths. Several render-mode names still have `@identity-TODO` because they come from handler behavior rather than original symbols. Resolve those names through handler and caller evidence; keep the mode values typed. |
| Screen-layer identities | `ScreenLayerSlot` now types layer creation and ordering, including the no-hit sentinel. `s_layerOrder` in `winmain.cpp` constructs party-panel slots 9..13 from `SCREEN_LAYER_FIRST_PANEL`; the individual panel identities have no source-backed names yet. Keep the typed arithmetic boundary until their consumers establish each role. |
| State-local phases and steps | `GameState.phase` and `GameState.step`, plus `GetGamePhase` and `GetGameStep`, multiplex different domains according to `GameState.state`. In `statestack.c`, status, level-up, and world-map handlers each interpret phase and step differently. `SetGameState`'s old-state result and `g_rosterReturnState` are `GameStateId`; its phase and step fields stay raw until their state-specific domains can be carried without conflating them. |
| Packed-list and map extents | Variable-length records such as `Panel.rows[1]`, `ShotFile.bytes`, and `MotionFile.bytes` have storage and serialized-size constraints. [The layout and buffer review](layout-and-buffer-boundaries.md) lists the named spans and fixed-buffer callers with their current evidence. Use allocation sizes, whole-record reads, and retail data bounds to decide a stronger type or capacity; a C array bound alone is not proof of the original declaration. |

## Open review inventories

- [C written casts](cast-census.md): `giten verify c-casts` records target-C
  spelling locations and observed source/target types for all configured C units.
- [Union views](union-views.md): review the packed, variant and serialized
  overlays from whole-object use before deleting an alternate view.
- [Shared `goto` joins](goto-review.md): every written site has a function and
  next review step. The `SortRoster`, decoder and shot-loop comments include
  compiler-shape evidence; structured spelling still needs a retail compare.
- [Helper reuse](project-helper-reuse.md), [common code](common-code-review.md),
  and [vendor macros](vendor-macros.md):
  restore a helper only where its load source and timing match, including
  across calls that might change the record.
- [Deferred constant identities](constants-handoff.md): review the complete
  producer/consumer domain before replacing a numeric value. `config/constants.tsv`
  records AST-exposed spellings with their reasons. [The separate macro-literal
  census](macro-literals.md) finds tokens in function-like replacement lists;
  for example, `FinalizeAttackDamage` in `include/Game/Attack.h` has damage-scale
  and clamp literals absent from `bare_constants.tsv`. The macro literals lack
  semantic dispositions, so an empty AST open list is not full coverage.
- [Compiler warnings](compiler-warnings.md): the MSVC 5.0 census covers all 82
  configured translation units and records remaining source warnings by family
  with examples; source corrections need byte and ABI checks.

The project unions are intentional overlapping representations until a usage
audit proves one view unused: examples include `ItemStack`'s packed word and
fields, `ScriptScratchValue`'s byte/word writes, `MapCell` variants, and the
`Character` roster/field tail. Likewise, a search of the current source finds
no manual `va_start`/`va_arg` cursor or owner-from-member-pointer recovery.
These categories stay in the review scope if new source introduces a site.
