# Enum reuse review

`giten verify enum-reuse` evaluates every project enum member and groups
declarations by integer value. Equal numbers are candidates for review, not
evidence that their enum domains are interchangeable.

The command combines a lexical inventory of `include/` and `src/` with libclang
evaluation in every translation unit from the existing compile database. It
refuses to report success if a source member is not evaluated or an evaluated
member is missing from the lexical inventory. It writes derived reports to
ignored `build/gen/`:

- `enum_reuse.tsv` lists evaluated members and their source contexts.
- `enum_value_collisions.tsv` groups declarations by value and joins bare
  function literals at the same value.
- `enum_domain_pairs.tsv` lists pairs with overlapping value sets.
- `enum_role_pairs.tsv` narrows the leads to pairs where at least two equal
  values have equal member-name suffixes after each enum's common prefix is
  removed. This name match is a search aid, not a merge decision; the full pair
  report remains the coverage view.

Use `--value 10` to inspect one value, `--duplicates` for values declared in
multiple domains, `--json` for the full evaluated census, and `--no-report`
to avoid writing derived files. The command reads the existing
`build/clangd/compile_commands.json`; it does not compile the game.

## Decisions

`config/reviews/enum-reuse.tsv` snapshots each starting enum block and all its
evaluated `name=value` members. Each row receives one decision:

- `retain`: the source enum keeps its domain because its values select a
  distinct quantity, table, operation, representation, or state machine.
- `canonical`: this enum owns values reused by another reviewed block.
- `reuse`: members move to the canonical enum; `member_reuse` maps every moved
  original member to its new owner and name.
- `pending`: the pair's producers, consumers, and encodings still need review.

For a candidate pair, follow both value paths through their fields, callers,
and tables. Check the entire member sets, not just the equal values. Direct
transport of the same quantity supports reuse; a conversion between distinct
representations or two unrelated zero-based tables supports retention.
Preserve per-state phase domains when the same numbers drive different state
machines. A shared sentinel alone does not make the positive payloads one
type.

For example, the item and skill status pages both use substeps 3 and 4 to
show a description and wait for dismissal, after sharing menu substeps 0..2.
`StatusPageDescriptionStep` owns that common suffix. `EquipPart` and
`EquipSlotIndex` instead keep separate types: the same numeric positions name
different equipment parts, and `GetEquipSlot` maps between their orders.
The display and quit choices likewise share the same `GetGameStep` open/poll
protocol through `SystemMenuChoiceStep`; their different selection actions stay
in their separate handlers.
Equal-value grouping also misses reuse when two member sets are disjoint:
human IDs and named object-record IDs both reach `Character.id`, so
`ObjectRecordId` owns both sets. Row state and policy bits both occupy
`PanelRow.flags`, so `PanelFlags` owns both sets.
`BattleProtectionResult` and the final attack-resistance codes share negative
reflection values, but zero means blocked in the former and immune in the
latter, so their complete result domains cannot be merged.

The ledger check requires every starting member to have a current home with
the same evaluated value. New, removed, or changed members require a new
review decision. Pending rows keep the standalone review command nonzero until
the semantic audit is complete.
