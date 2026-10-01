# Function macro literals

The AST constants audit sees numbers at compiled expansion sites but does not
reliably retain the spelling location inside a function-like macro's
replacement list. For example, `ApplyWeaponPowerConditions` in
`include/Game/Attack.h` writes two power multipliers of `2` and a `while (0)`
scaffold; these are absent from the header's AST constant rows. The
`FinalizeAttackDamage` macro in the same header writes `2`, `-20`, `20`,
`100`, `0` and `0x7fffffff` in its damage logic. Its macro body likewise
needs a source-level review, including uses that the current compile database
does not expand.

`giten verify constants --macro-list [FILTER]` scans function-like `#define`
replacement lists under `src/` and `include/`. It prints written number
spellings with their file, line, column and macro name and writes
`build/gen/macro_literals.tsv`. A normal `giten verify constants` run writes
the same report alongside its AST reports. The scanner includes unexpanded
and conditionally inactive definitions, excludes comments and quoted text,
and fails if a source file changes during the scan. `--macro-max N` can
bound the site count for a focused cleanup.

The macro report is a worklist, not a semantic keep ledger. It includes
structural numbers such as `do { ... } while (0)` and numbers that may need
domain names. They have not been individually reviewed, so macro sites do
not count toward `config/constants.tsv`'s zero-open floor and `--gate` does
not reject them unless `--macro-max` is supplied. Object-like `#define`
bodies are outside this source census; their identifiers are already named,
but any embedded numeric expressions still need a separate review. The
scanner does not infer the expanded expression's type or claim that a
matching value in another macro has the same meaning.

Review each macro at its use sites and restore proven domain names or keep a
documented structural literal. Then add a committed macro decision ledger
and a gate for newly open macro sites. Preserve separate identities for
unrelated macros that happen to spell the same value.
