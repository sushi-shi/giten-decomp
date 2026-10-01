# Deferred constant identities

`config/constants.tsv` is the review ledger for numeric spellings exposed by
the translation-unit AST in `src/` and `include/`. Its floor is zero: each
AST-exposed literal is named or has a narrowly matched reason to stay numeric.
`giten verify constants --list [FILTER]` shows any newly open AST sites, and
`--gate` detects stale or unreviewed rows. Numeric spellings in macro
definitions have a [separate source census](macro-literals.md); they do
not yet participate in this floor.
The generated `build/clangd/compile_commands.json` must list the current
translation units; `python3 -m giten.graph.compdb` refreshes it after a unit
is renamed or removed. `giten configure` only rewrites the Ninja manifest.
A failed parse is an incomplete census, not a zero-open result.

The cases below have behavior-based names or numeric review rows, but their
authored identities remain open. A naming change needs code, record or string
evidence for the complete value domain; equal numbers alone do not join
unrelated domains.

## Skill and battle values

- **Battle result word and tally slots:** The combat resolver identifies
  ordinary action outcomes and the result-word tags 0x50, 0x70 and 0x80 for
  HP, MP and experience draining. Tally slot 5 is set by Tetrakarn and
  Counterattack and reflects Sword-attribute attacks. Slot 6 reflects
  Physical-attribute attacks in `CheckBattleProtection`, but no present
  ET0004 skill has the effect code that would set it through
  `UseBattleTallySkill`. MS00DF's barrier-cleared message is generic and
  cannot distinguish an authored name for slot 6. Relate the remaining tally
  effects to both their script messages and record producers before naming.
- **Skill kinds 9, 10 and 14:** Neither Windows nor PC-98 ET0004 contains a
  record with these kinds. Kinds 9 and 10 reach the same battle-tally clearing
  handler; kind 14 takes the field-effect call and success path shared by
  kinds 12 and 13. These actions support behavior labels, not distinct
  authored category names. A producer outside the current skill table or a
  surviving label is needed.
- **Target areas 1, 3..7 and 17:** `CollectTargets` gives code 2 the line
  path, 6 and 7 the visible-grid path, and 0xff the weapon-hit special case.
  Areas 1, 3, 4, 5 and 17 use the default cell collector. ET0004 mixes
  close attacks, broad attacks and remedies within these values. ET0001
  assigns area 4 to sixteen items with the same target flags, count and
  range; those fields do not distinguish its meaning. The PC-98 dispatcher
  has the same collection split. Distinct selection semantics require caller
  or record evidence beyond geometry.
- **Attack attribute 10:** It bypasses the resistance array with a fixed
  value of 50. No current ET0004 skill or ET0001 attack item selects it;
  its in-world identity remains unproven.
- **Field-effect 0x1a:** It invokes `SpawnActorGroup`, like Sabatoma's 0x19,
  but no current ET0004 record in either version selects 0x1a. Its authored
  skill identity needs a producer or label. Code 0x23 directly reports
  `FIELD_EFFECT_DONE` without changing state; its behavior is known but its
  authored purpose is not.

## Map and object values

- **Spring-link cell codes 0x85..0x87:** All three share an automap spring
  mark and dispatch MS003A through different entries. Each script entry
  sets selectors consumed by later branches, so the spring icon and one
  Hagenti encounter do not establish separate meanings for the three
  codes. M0081 places them in adjacent cells, confirming that the map data
  distinguishes them. The PC-98 data keeps the distinction but supplies no
  separate icon or label.
- **Object-list cell codes 0x8b, 0x8c and 0x8f:** Windows maps contain 97,
  12 and 9 records respectively. The object-cell queries recognize 0x8d and
  0x8e instead; `CheckCellEvent` ignores the kind-10 result for object
  records. Their separate map roles need another consumer or data witness.
  Cell kinds 10 and 13 have table entries but no event branch that explains
  their authored purpose. `CELL_INERT` records the observed no-op behavior.
- **Trap cell codes 0x68..0x6b:** Neither Windows nor PC-98 area maps contain
  exit records for them. The code establishes the range but not individual
  identities. Codes 0x6c..0x6e have record-mask evidence and are already
  named by their alignment behavior.
- **Wall kinds 6 and 12:** Kind 6 blocks movement but draws no wall; kind 12
  draws a wall but permits passage. Their behavior names are supported by
  `GetWallAt`, `BuildRoomGeometry` and the wall-stop table. Their in-world
  roles and the distinction between door-like wall-stop classes 1 and 2
  remain open.

## Item and saved-state values

- **Item kinds 5 and 6:** ET0001 has fifteen kind-5 items spanning charms,
  incense, a shield and scenario objects, and one kind-6 Necronomicon. The
  PC-98 records keep the same assignments. `DecodeItemRecord` gives kind 5
  targeting and message fields but not kind 6; both currently reach no-effect
  item handlers. One Guardian Set script side effect does not explain the
  whole kind. The authored categories need a common operation or label that
  covers every record.
- **Event-flag banks 3, 5, 6, 10 and 11:** Source readers identify several
  other banks, and ET0018 names flags only in banks 0, 1, 2, 4, 8, 9 and 14.
  Preserve these bank numbers as numeric selectors until a writer, reader or
  script label establishes their roles. A clear flag can mean the event has
  happened, so inspect polarity at each use.
- **Moon phases:** The saved byte cycles through 28 phases. Phases 0 and 14
  are new and full moon; phase 15 triggers `ClearMoonFlags`. Other phase
  numbers have no separately established names. The count and tick duration
  are extents, not additional moon-state members.
- **Human sign names:** ET0000 supplies seven title names and four blood-type
  labels, but its adjacent twelve sign-name buffers are empty and never
  written. Their individual sign identities cannot be inferred from the
  table index alone.
