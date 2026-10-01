# Todo ledgers

[Reconstruction cleanup](reconstruction-cleanup.md) maps the KF1 and Gruntz
review methods to Giten's live worklists and records unresolved source models.

## Source typing

[C source cast census](cast-census.md) records the missing target-C written-site
audit and gives pointer and scalar examples for the next type-model review.
[Deferred constant identities](constants-handoff.md) records values with
behavior-based names whose authored meanings still need evidence.
[Shared control-flow joins](goto-review.md) inventories the remaining written
`goto` sites and the retail evidence needed for a structured rewrite.
[Offset-derived field names](offset-fields.md) lists primitive fields still
named by storage offset and the evidence needed to identify them.

## Project helper reuse

[Project helper reuse](project-helper-reuse.md) records repeated Giten helper
expansions that require source-origin or instruction evidence before adoption.

## Vendor macros

[Vendor macro recovery](vendor-macros.md) tracks plausible SDK/CRT macro
sites that remain expanded, their evidence or semantic blockers, and the
next matching step. Remove an entry when all its supported sites are
recovered; retain a semantic exclusion while it explains an apparent match
that would change retail behavior.

## Data referents

- [ ] Clear the data debt and re-enable strict `data_matching` using the
  [build-system procedure](../build-system.md#re-enabling-data-matching). While
  matching is relaxed, 100% can hide a wrong data target or member addend even
  when the instruction bytes match. `giten verify data-identity` checks paired
  sites, but does not cover every relocation layout; audit ordered data
  referents before treating an exact score as a full referent match.

## Rule exceptions

A 100% match is kept even when its source breaks a project rule (a gate, a
source-modeling rule). `rule-exceptions.tsv` records every such function so the
deviation can be revisited deliberately later.

| column | meaning |
| --- | --- |
| `rva` | retail address |
| `function` | mangled name |
| `rule` | the rule or gate the source breaks |
| `deviation` | what the source does instead |
| `note` | why it was kept and what a rule-clean form would need |

Add the row in the same commit that lands the 100% source, together with any
allow entry the gate needs. Remove the row when a rule-clean spelling also
reaches 100%.

## Syntactic recovery

`syntactic-recovery.tsv` is written by `giten match`. A row is a function an
edit left at the same CUR while its new source hash lowered MAX from
`lost_max` (the peak stays in HIST). The matching loop never works these; a
separate fuzzy syntactic recovery pass looks for a spelling of the current
source that regains `lost_max`, and removes the row when it does.
