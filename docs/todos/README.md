# Todo ledgers

[Reconstruction cleanup](reconstruction-cleanup.md) maps the KF1 and Gruntz
review methods to Giten's live worklists and records unresolved source models.

## Source typing

[C source cast census](cast-census.md) records the target-C written-site
audit and gives pointer and scalar examples for the next type-model review.
[Enum boundaries](enum-boundaries.md) records mixed-value protocols and
unproven identities after strict syntax checks on all configured units.
[Deferred constant identities](constants-handoff.md) records values with
behavior-based names whose authored meanings still need evidence.
[Game macro literal review](game-macro-literals.md) records proven substitutions
and unresolved formula/domain values inside Game macro bodies.
[Non-Game macro literal review](non-game-macro-literals.md) records proved
size/bound names and the retained platform, graphics, text and sound values.
[Macro replacement literals](macro-literals.md) records numeric spellings in
macro replacement lists that the AST constants ledger does not cover.
[Shared control-flow joins](goto-review.md) inventories the remaining written
`goto` sites and the retail evidence needed for a structured rewrite.
[Offset-derived field names](offset-fields.md) lists primitive fields still
named by storage offset and the evidence needed to identify them.
[Compiler warnings](compiler-warnings.md) records the full MSVC 5.0 warning
census, resolved cases, and remaining source-model examples.
[Script and graphics width boundaries](warning-width-boundaries.md) and
[Game argument-width warnings](argument-width-warnings.md) record focused
caller/callee evidence for retained narrowing sites.
[Platform and graphics identities](platform-gfx-sound-identities.md) records
remaining live function TODOs and the evidence needed to close them.
[Non-Game function matching](non-game-function-matches.md) records the remaining
compiler and access-shape walls in memory, graphics, platform, text and sound.
[Character function matching](character-function-matches.md) records the three
remaining character-unit compiler walls and the rejected null-pointer shape.

## Layout and buffer boundaries

[Layout and buffer boundaries](layout-and-buffer-boundaries.md) records
constructed objects, named layout spans and fixed-buffer operations whose
current source does not yet support a cleanup.

## Declaration placement

[Declaration placement](declaration-placement.md) records the caller and owner
ABI evidence behind the remaining cross-header function declarations.

## Project helper reuse

[Project helper reuse](project-helper-reuse.md) records repeated Giten helper
expansions that require source-origin or instruction evidence before adoption.
[Common code review](common-code-review.md) records recovered and unresolved
inline or macro candidates across translation units.

## Vendor macros

[Vendor macro recovery](vendor-macros.md) tracks plausible SDK/CRT macro
sites that remain expanded, their evidence or semantic blockers, and the
next matching step. Remove an entry when all its supported sites are
recovered; retain a semantic exclusion while it explains an apparent match
that would change retail behavior.

## Data referents

[Game data band](game-data-band.md) accounts for every initialized `.data` and
zero-filled `.bss` byte, distinguishing source/model claims from unidentified
retail spans. It records the wall arrays and game-adjacent gaps without
assigning unsupported storage.
[Short data extent claims](data-extent-claims.md) records the two remaining
claims, their retail bytes and references, and the evidence needed to name
adjacent storage without inventing padding.

Strict `data_matching` is enabled in `config/compare.toml`, and the current
build reports no unprovisioned identities or placeholder externs. It compares
data targets and addends, while `giten verify data-access` checks retail reads
against declared storage. Neither comparison proves the run-time index range
of a register-indexed array read. The final `UpdateFieldHud` wall-row scan is
recorded in [layout and buffer boundaries](layout-and-buffer-boundaries.md).

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
