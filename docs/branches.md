# Generated branches

`giten branch` derives readable source from `main`. The model follows the
King's Field reconstruction: two generated branches and one maintained by hand.

```text
             main (you are here)
               |
     +---------+---------+
     |                   |
     v                   v
  source              classic
     |
     v
   port
```

| Branch | Contents | Maintained by |
| --- | --- | --- |
| `main` | Reconstruction and matching | Hand |
| `source` | Readable source; builds the retail game, or with `--fixes` the bug-fixed one | `giten branch source` |
| `classic` | Readable source of the retail game, without the bug fixes | `giten branch classic` |
| `port` | Bug fixes enabled, modernisation | Hand, seeded once by `giten branch port` |

## Exports

Both exports hold the unit sources of `config/units.toml`, the project
headers, `build.json`, `build.py`, a Nix shell with Wine and Python,
`.clang-format`, `LICENSE` and a README. They drop the matching tree's
`config/`, `docs/`, `scripts/` and tests.

In every source and header the generator:

- removes comments, except licence notices, `clang-format off/on`, and a
  non-ASCII comment on the line of a string literal, which glosses an escaped
  Shift-JIS literal;
- removes the `include/rva.h` claims (`RVA`, `RVA_DECL`, `DATA`,
  `RVA_COMPGEN`, `RVA_DYNINIT`, `DATA_MESSAGE_MAP`, `OVERRIDE`) and replaces
  `DATA_COMPGEN(rva, value)` by `value`;
- expands the enum-domain macros to MSVC 5.0's spelling: `typedef enum X {`,
  `} X;`, and the storage type for `GZ_ENUM_STORAGE`;
- formats the result with the tree's clang-format style.

`include/rva.h`, `include/Enums.h` and `include/EnumDomain.h` stay as
stand-ins holding only their include guard and includes, and every
`#include` stays. MSVC 5.0's register allocation and temporary numbering
follow the files a unit opens, not only its tokens (`include/Ints.h`), so
removing a header changes objects whose preprocessed tokens are identical.

`classic` also decides `GITEN_BUGFIX` and `GITEN_COMPAT` as undefined: it keeps
the retail arm of each conditional on them and deletes the fix and its
directives. `source` keeps those conditionals; its `build.py --fixes` defines
`GITEN_BUGFIX`, as `giten play` does. Other conditionals (`__cplusplus`) stay.
A conditional that mixes a fix flag with anything but `defined`, `!`, `&&` and
`||`, a scaffolding name that survives, or a file the export does not know
fails generation.

`build.py` compiles every unit with its `units.toml` profile under Wine with
the user's MSVC 5.0 and DirectX 6 SDK, and links with the flags and libraries
of the candidate link (`giten.graph.link`), in the same object order.

## Resources

No generated branch carries game content. The generator refuses binary files
and resource or image suffixes. `build.py` reads the resources from the
user's original `DDS.EXE` (`--exe` or `GITEN_RETAIL_EXE`) into an ignored
`build/DDS.res` at build time, as `giten link` does. A recovered resource
script can enter the exports as text; its payloads cannot.

## Commands

```sh
giten branch source --verify --publish
giten branch classic --verify --publish
```

Generation reads the committed `HEAD`; `--ref` selects another revision.
`--working-tree` previews tracked working files, staged new files included,
but cannot publish. Output goes to `build/branch/<name>`.

`--verify` builds the export with its own `build.py`: the retail game, and
for `source` also the fixed one. Every object must equal the matching
build's object for the same decision, `build/objdiff/base` or, for a unit
the play build recompiles, `build/play/obj`, apart from the COFF timestamp
and the `.file` record, which holds the source path. With `GITEN_RETAIL_EXE`
set, the linked `DDS.EXE` must also equal the matching tree's
`build/exe/DDS.candidate.EXE` (retail) or `build/play/DDS.EXE` (fixed) apart
from the PE link timestamp. The work tree's `src/`, `include/` and
`config/units.toml` must equal the exported revision.

`--publish` commits the export to the local branch `<name>`, checked out in
the worktree `build/<name>`. Each publication replaces the branch with one
root commit:

```text
classic: regenerate from <main commit, 12 digits>

Generated-By: giten branch
Source-Commit: <main commit>
```

A branch whose tip lacks the `Generated-By` line, a worktree with local
changes, or an untracked file in the way is refused. Regenerating the same
content from the same commit changes nothing. Publication never pushes:

```sh
git push --force-with-lease=source:<expected remote tip> origin source
git push --force-with-lease=classic:<expected remote tip> origin classic
```

For a first push, the expected tip is empty (`--force-with-lease=source:`).

## Port

`port` holds the fixes and the modernisation that cannot live in `main`. It
starts from `source` with the fixes enabled:

```sh
giten branch source --publish
giten branch port
```

`giten branch port` refuses when `port` exists or `source` was generated from
another commit. It adds one commit on top of `source` that decides both flags
as defined, removing the retail arms, and checks it out at `build/port`. From
then on `port` is ordinary history and is never regenerated.

To follow `main`, regenerate `source` and apply its change to `port`:

```sh
git -C build/port diff <previous source tip> source | git -C build/port apply -3
```

The previous tip is unreachable once `source` is replaced; note it before
regenerating (the reflog keeps it locally). Hunks inside fix conditionals of
`source` meet code whose conditional `port` has already removed and are
resolved by hand. A fix that must stay byte-neutral for the matching build
belongs in `main` under its flag, where `giten play`, `source` and `port` all
receive it. Changes that cannot compile under the matching build's
constraints, or that restructure code, belong in `port` only.
