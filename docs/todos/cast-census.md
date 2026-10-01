# C source cast census

The C++ cleanliness board counts selected C++ cast forms, and
`giten verify casts` reviews `reinterpret_cast` seams in `.cpp` and headers.
Neither command inventories written C casts in the 68 `.c` translation units.
Those casts are valid C syntax, so the C++ zero-cast rule is not an appropriate
gate for them. A source-wide written-site census would make type and ownership
cleanup measurable, as KF1's [target-C cast audit](https://github.com/sushi-shi/kings-field-decomp/blob/master/docs/cast-audit.md)
does.

## Open work

- Parse every C unit from `config/units.toml` with the generated target compile
  database. Attribute each `CSTYLE_CAST_EXPR` to its spelling location so
  casts in headers and macros count once even when expanded by many units.
  Fail on parse errors and report any project header the unit set misses.
- Record source and target types, enclosing function, and every unit context.
  Separate pointer representation boundaries from scalar narrowing and enum
  conversions. Publish a complete site list and a non-increasing count; do not
  treat a reduced count alone as evidence of a correct source model.
- Review repeated pointer casts by their owner and consumers. For example,
  `src/Game/clock.c` casts area-map bytes to `AreaLevel*`, `u8*` and `u16*`
  while walking serialized offsets; `src/Script/scripttext.c` casts a handle
  payload to `u32*`, `i16*` and `char*` for different script operations.
  These may be real decoding boundaries or missing typed accessors; the cast
  spelling alone cannot decide.
- Review scalar casts with the retail access width and arithmetic: the
  `src/Game/partyaction.c` switches cast `GetGamePhase()` and `GetGameStep()`
  to `u16`, while `src/Game/fieldview.c` narrows a direction before indexing
  `s_wallStops`. Keep narrowing where the instructions or encoded domain
  require it.

The existing `build/clangd/compile_commands.json` lists C translation units,
but a complete cast count has not been established or banked. The C++ cast
ledger's 51 reviewed casts do not cover this work.
