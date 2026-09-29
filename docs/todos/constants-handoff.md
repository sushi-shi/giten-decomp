# Constants work list: handoff

Goal: drive the open count of `giten verify constants` to 0. Each open numeric
literal either becomes a name (an enum or define backed by evidence, `NULL`,
or `true`/`false`) or gets a row in `config/constants.tsv` saying why it stays
numeric. The floor in that file only goes down.

## State

- Branch `match/constants` (PR #28). Last commit: `3106043e`.
- The last pushed floor is 1032. The commits after it are local.
- The open count is about 807, or about 523 once the uncommitted work below is
  committed.
- `src/Game/fieldobj.c` is done. `src/Game/treasurebox.c` is nearly done.
- After that, only `src/Game/partyaction.c` (about 518) remains.

### Uncommitted when stopped

Built, 0 REGRESS:

- `AutomapPhase` in `include/Game/Automap.h`, used by `RunAutomapState`.
- `ApplyRowCheck` takes a `BitChangeMode` op. `include/Ui/Panel.h` includes
  `Util/BitChangeMode.h`.
- About 99 treasurebox rows, inserted after the last fieldobj row. The stale
  `ApplyTraining 18` row is removed.

Not built yet:

- `ReleasePanel(Panel*, b16 freePanel)` in `Panel.h` and `clock.c`, with the
  callers in treasurebox, fusion, scripttext and menubox changed to `true`.
- That change makes two rows stale; delete them:
  - `src/Ui/menubox.c DestroyMenuBox 1 … ReleasePanel`
  - `src/Script/scripttext.c DestroyScriptPanel 1 … ReleasePanel`

Still missing:

- A row for treasurebox `RunAutomapState` `RunFieldPanelRow(7, 0, 0, 0)`:
  field panel row 7, with no op or flag changes.

Then run the batch workflow below.

## Batch workflow

1. `export GITEN_DIR=$PWD PYTHONPATH=$PWD/scripts`
2. Edit, then run `giten build` twice.
3. `giten verify status` must show 0 REGRESS and 0 RESET. DIPs in unedited
   functions are fine.
4. `giten verify constants --gate` must show no stale rows and no un-rowed
   compiler-proven replacements.
5. Run clang-format on the changed lines. New files need a full clang-format.
6. Run `git checkout -- README.md docs/todos/syntactic-recovery.tsv` and
   commit.
7. Run `giten verify constants --update-floor`, commit as
   `constants: lower floor`, and push.
8. Rebank from time to time with `giten verify bank --baseline-only`, committed
   as `match: rebank …`. This keeps TU dips in edited functions reported as DIP.

Tools:

- `giten verify constants --list partyaction` lists the open items.
  - The list is also in `build/gen/constants_open.tsv`, with columns file,
    line, owner, spelling, group, detail.
- `-v` with `build/gen/bare_constants.tsv` (column 10) shows the
  compiler-proven replacements.
  - Apply them by hand: `--fix` also rewrites the ones kept on purpose.

## Pitfalls

- Rows are fnmatch globs and the first match wins.
  - Put specific rows before broad ones; otherwise the specific row goes stale
    and the gate fails.
  - A broad row can also hide literals that already have names. For example,
    the `OpenDataFile` row hid `DataTableId` literals.
- Including `Game/AreaNpc.h` or `Game/SceneHotspot.h` in the fieldobj or
  FieldHud TUs clashes on `GrbToRgb`. Use small split headers instead, such as
  `Game/RoomRegion.h` and `Game/SceneHotspotKind.h`.
- `i < sizeof(x)` makes the compare unsigned. Prefer an `int` count define.
- The include-sorting helper can reorder existing includes. Check with
  `git diff | grep '^[-+]#include'`.
- A game-state phase gets a per-state enum (`XxxPhase` with `XXX_PHASE_*`),
  like `WorldMapPhase` and `MenuStateStep`.
  - Sub-state step functions return `SUBSTATE_RUNNING` or `SUBSTATE_FINISHED`.

## Evidence sources

### The disc image

`giten.codecs.corpus.disc_files(<DDSWIN.BIN>)` reads the image. It was last at
`/tmp/giten-codec-parity/build/codecs/DDSWIN.BIN`.

Map area names:

- File: `M/Mxxxx.BIN`.
- Decode: `n = u16(data, 0)`, then `r = decrypt(data[2:2 + n])`.
- The name is at `r[u16(r, 2):]`, in cp932.
- Areas 0x09 and 0x85 both carry the name 新宿都庁.

### Event-flag names (ET0018)

`ET0018` is the developers' event-flag name table.

Format, after `decrypt`:

- A 4-byte header.
- Then 29-byte entries: a big-endian `bank * 256 + bit`, a 26-byte cp932 name,
  and a zero byte.

Coverage:

- Banks 0 and 1 hold the scenario flags. The names run on through bank 1 bits
  0-28.
- Bank 2 holds the owned maps and programs.
- Bank 4 holds the boxes.
- Bank 8 bit 0 is "no enemies".
- Bank 9's names are only the Shinjuku base's.
- Bank 14 holds the actor flags. Bits 11-16 contradict the code, so they are
  not used.

Polarity:

- `ResetEventFlags` sets every bank except 4, 14 and 15. A new level sets
  bank 8.
- A scenario or owned flag means "happened" or "held" when it is **clear**.
- Check each use before writing a comment.

## Left

### partyaction.c

Handle it as fieldobj and treasurebox were.

Name the phase enums, boolean parameters (`b16`), list terminators, and counts
taken from arrays.

Reuse these names where they fit:

- `MoveCommand`
- `ViewDirection`
- `Attitude`
- `ActorFlag`
- `OwnedFlag`
- `FIELD_OBJECT_*`
- `SUBSTATE_*`
- `HANDLE_NONE`
- `CHARACTER_ID_NONE`
- `PARTY_ROW_SIZE`

Also handle `SetMenuItems(menu, 9, actor, 8, …)`, the actor command menu.

Row the rest per function, with `@identity-TODO` where the meaning is unproven.

### Deferred

Kept as rows until there is evidence:

- Action-result, resistance and battle-tally codes. These need a decoder for
  message-script files 0xdd and 0xdf.
- Skill kinds 2, 5, 6 and 7.
- Object record ids 0xce, 0x22 and 0x117. The record names are undecoded.
- Actor flags 0x20, 0x21, 0x22 and 0x3f.
- Bank 7 flags 0xfd-0xff.
- The identity of area 0x85.
- Item kinds 5 and 6.
