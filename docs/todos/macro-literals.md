# Macro replacement literals

The AST constants audit follows compiled expansions but does not reliably
retain the spelling location inside a macro's replacement list.
For example, the function-like `ApplyWeaponPowerConditions` in
`include/Game/Attack.h` writes two power multipliers of `2` and a `while (0)`
scaffold; these are absent from the header's AST constant rows. The
`FinalizeAttackDamage` macro in the same header writes `2`, `-20`, `20`,
`100`, `0` and `0x7fffffff` in its damage logic. Its macro body likewise
needs a source-level review, including uses that the current compile database
does not expand. Object-like expressions also contain source-written values:
`WORLD_TRAVEL_SPAN` in `src/Game/worldtravel.c` uses `2` and `1` around its
named radius, while `FRAME_PERIOD` in `src/Platform/winmain.cpp` uses `1000`
and `1` around its named refresh rate.

`giten verify constants --macro-list [FILTER]` scans both function-like and
object-like `#define` replacement lists under `src/` and `include/`. It
prints written number spellings with their file, line, column, macro name and
kind, and writes `build/gen/macro_literals.tsv`. A normal
`giten verify constants` run writes the same report alongside its AST reports.
The scanner includes unexpanded
and conditionally inactive definitions, excludes comments and quoted text,
and fails if a source file changes during the scan.

The macro report is a worklist, not a semantic keep ledger. It includes
structural numbers such as `do { ... } while (0)` and named object-like
definitions whose values may already be justified. Most have not been
individually reviewed, so macro sites do not count toward
`config/constants.tsv`'s zero-open floor and `--gate` does not reject them.
The scanner does not infer the expanded expression's type or claim that a
matching value in another macro has the same meaning.

## Reviewed macro families

The function-like macros in `include/Platform/Scene3D.h` retain their numeric
spellings. `DrawScreenQuad` and `DrawLitQuad` pass the four-vertex count to
`DrawPrimitive`. `SetQuadColor`, `SetQuadSpecular`, `TranslateBillboard` and
`ProjectBillboardRect` select fixed vertices by index `0` through `3`.
`SKIP_TEXT_PLANE` uses `1u` as the bit seed; `GetEffectBitmapOffsetY` shifts
the packed bitmap offset by one byte (`8` bits). `InitEffectBlitFx` and the
other statement macros use `while (0)` as a single-statement scaffold. These
are layout and bit-operation values, not names of additional enum members.

The function-like macros in `src/Script/scriptactor.c` retain their written
formulas pending script evidence. `AdjustActorSpoilAmount` tests adjustment
modes `1`, `2` and `3`, then sets the amount to `1`, adds `5`, or computes
`amount / 8 + 1`; it resets the mode to `0`. The mode's authored names need a
producer or script operand witness. `RollFixedContestValue` passes the
level-specific ranges `5..15`, `12..22` and `20..40` to `RandomAverage`.
`RollRelativeContestValue` passes `-20..20`, `0..30` and `10..40` to
`RandomPercent`. These are contest formulas, and their equal numbers do not
establish a shared enum. Their `while (0)` sites are statement scaffolds.

Review the remaining macros at their use sites and restore proven domain
names or keep documented structural literals. Then add a committed macro
decision ledger and a gate for newly open macro sites. Preserve separate
identities for unrelated macros that happen to spell the same value.
