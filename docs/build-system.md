# The matching build system (manifest -> ninja -> objdiff)

The matching loop is a **native incremental ninja build** generated from a
single manifest. `giten build` is the one entry point; every edge in the graph
is also a verb of its own (see [`docs/tooling-map.md`](tooling-map.md)).

```
config/units.toml  (per-TU manifest: unit, source, flags profile)
        |  giten configure          (giten.graph.emit)
        v
build/build.ninja
        |  ninja
        +-- cl        src/<unit>.cpp -> build/objdiff/base/<unit>.obj   (wine cl 5.0)
        +-- compdb    units.toml     -> build/clangd/compile_commands.json
        +-- labels    source + base obj -> build/gen/claims/<unit>.tsv
        +-- model     claims x censuses/providers -> build/gen/bindings.tsv
        +-- delink    bindings -> synth PDB -> vostok-delinker
        |                       -> build/objdiff/target-new/<unit>.c.obj
        +-- normalize base + target objs -> the disposable comparison copies
        +-- project   the delinked directory -> compare-new/objdiff.json
        +-- report    objdiff-cli -> build/objdiff/compare-new/report.json
        +-- verify_fp the per-function source-fingerprint cache
        +-- verify_check  the MAX gate + the fast and normal gate tiers
                          (only for the `verify` target)
        v
per-unit + roll-up match % (`giten verify status`)
```

Phony aliases stop the graph early: `ninja -f build/build.ninja base` (objects
only), `claims`, `target`, `compare`, `verify`. `all` (everything except the
gates) is the default; `verify` adds the gates.
`candidate` is phase 2 and is never in `all`.

Everything runs inside `nix develop` — the one dev shell (`.#build` is a kept
alias of it), which exports `MSVC_DIR`, `DXSDK_DIR`, `WINEPREFIX`, and `ninja`
on PATH. The Wine prefix is created by `giten init`, which the shell runs on
entry (idempotent).

## Quick start

```sh
nix develop                     # the dev shell: analysis tools + MSVC 5.0 under Wine
giten init                     # once: the build wine prefix
giten match <unit|source>...   # fast loop: only these units, no gates
giten build                    # cl -> labels -> model -> delink -> compare
giten build verify             # + MAX gate and gate tiers (merge preparation)
giten match                    # the full build, then the summary for the CHANGED units
```

Pass ninja arguments straight through: `giten build -j8 -v`, or name a phony
target: `giten build base`.

## Incrementality: what re-runs after an edit

Every producer writes **if-changed** and its edge carries `restat`, so an edit
that does not change a downstream input stops the cascade there.

- **Pure code edit** (no label change): `cl` recompiles one object and `labels`
  re-extracts one fragment. The fragment is byte-identical, `restat` stops
  `model`, and the delink and every target object are untouched. Net work: one
  base obj, one normalize pass, a fresh `report.json`.
- **Label change** (add/rename a function, move an `RVA()`): the fragment
  changes → `model` rewrites `bindings.tsv` → `delink` re-runs → that unit's
  `<unit>.c.obj` updates.
- **Comparator re-pin**: the driver detects the resolved `objdiff-cli` path
  changing and updates `build/gen/comparator.id`. Only the report and its
  consumers depend on this identity; compiled and delinked objects are reused.

Two edges declare a **stamp** rather than their real outputs, because neither
set can be enumerated at configure time: `delink` writes one object per unit
that has a claim (declaring all ~311 would re-run the whole delink on every
build), and `normalize` writes a variable pair of copies per unit. Both drivers
are keyed on content upstream, so a stamp only moves when something real did.
`giten build --force-delink` drops the delink stamp when you want it anyway.

`giten build` records its wall clock in `build/gen/build_times.tsv`
(gitignored, per-worktree).

## The manifest: `config/units.toml` (single source of truth)

Per **translation unit**. Every `[[unit]]` names a `[flags]` profile
explicitly; a profile is the FULL flag set, there are no per-unit bolt-ons.

```toml
[build]
compiler = "msvc5.0"
platform  = "win32"

[flags]
c        = ["/nologo", "/c", "/O2", "/ML"]                   # the game's C
cpp      = ["/nologo", "/c", "/O2", "/ML", "/GX"]            # the platform layer's C++
cpp-noeh = ["/nologo", "/c", "/O2", "/ML"]                   # C++ without EH frames

[[unit]]
unit   = "range"                    # stem; obj is <unit>.obj, target <unit>.c.obj
source = "src/Util/range.c"
flags  = "c"
```

Every profile is **recovered from the retail bytes, not chosen** - see
[`docs/compiler-flags.md`](compiler-flags.md) for the evidence. `/O2` forces
function-level COMDAT packaging (no `/Gy` needed); `/GF` is off because retail
literals live in writable `.data`; `/ML` because the static CRT is LIBC.LIB.
Per-unit rationale (why a TU exists, was split, or absorbed another) lives in
[`https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/tu-partition-brief.md`](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/tu-partition-brief.md); the compile-flag evidence
is in [`docs/compiler-flags.md`](compiler-flags.md).

## The `cl` rule (the wine compiler bridge)

```ninja
rule cl
  command = $py -m giten.tool.cl --out $out --src $in -- $cflags
```

`giten.tool.cl` is the Linux→Wine bridge. For each TU it resolves `CL.EXE`
under `$MSVC_DIR/bin` (case-insensitively), keeps a persistent `wineserver -p`
alive so `ninja -j` does not pay a cold wineserver start per object, translates
the paths with `winepath -w`, runs `wine cl.exe <flags> /Fo<obj.w> <src.w>`, and
treats **"the `.obj` exists"** as the success signal — Wine spews unrelated
driver noise and can return a non-cl exit code. Objects are written **with the
COFF timestamp stabilised and only if the content changed**, which is what makes
`giten match`'s "which units moved?" question answerable by hash.

## Labels → claims → the Model

`giten labels` (`giten.retail_labels.source`) reads one `src/<unit>.cpp` and
emits `build/gen/claims/<unit>.tsv`. The macros come from **LLVM IR**
(`@llvm.global.annotations` pairs the mangled symbol DIRECTLY with the
annotation — no positional join) plus the clang AST for `extern` declarations
IR drops, and every emitted name is authorized against the unit's own base
object: a name cl did not emit is DROPPED and reported, never claimed.

The extraction front end reads the VC5 source dialect, not modern C++
portability policy. Its shared clang flags demote two constructs VC5 accepts
but clang otherwise rejects: taking the address of a temporary and using a
signed `switch` with an unsigned `0x80000000`-range SDK HRESULT case macro.
The latter keeps `case DDERR_*`/`DIERR_*`/`DSERR_*` source intact instead of
requiring a cast around every label.

`giten model` is the one join. Claims (from `src/`) and provider tables (from
`config/retail/`) are resolved against the base censuses by channel precedence:

```
src > src_compgen > src_dyninit > src_data_compgen > functions_zlib/data_zlib
    > data_vtables > data_compgen > data_static_libs > functions_static_libs
    > src_decl
```

The winner per rva is the binding; the losers are recorded as aliases. The
result is `build/gen/bindings.tsv` plus `build/gen/violations.tsv`, and
**violations must be 0** — an unadmitted claim, a size crossing, a duplicate
data name, or a keyword-spelled `RVA_DYNINIT` owner all land there. This is why
there is no separate channel/size/uniqueness audit any more: the invariant is
structural.

## The target (delink) half

`giten delink` (`giten.delink.run`):

1. `giten.delink.pdb_synth` builds `build/pdb/giten_named.pdb` from the Model
   — function records with the claim-resolved extent and a synthetic
   `c:\proj\<unit>.c` source file so the delinker emits one `<unit>.c.obj` per
   TU, plus data records for every relocation-target address renamed to the
   claimed source names, cl's own `??_C@` string-pool spellings, uniquely
   paired C `$SG` literals (with RVA-unique manifest names when separate TUs
   reuse an ordinal), and the proven `__imp_` IAT decorations. An identity
   is always PROVIDED, never invented: a
   target no name reaches keeps a fence spelling that states the verdict —
   `DAT_<va>` when only library bands reference it, `UNPROVISIONED_<va>` (which
   the delinker refuses to emit) when a game band does. The surviving
   `UNPROVISIONED_` targets are the derived data-debt worklist,
   `build/gen/data_debt.tsv`, written on every delink in both modes; with
   `data_matching = false` they are then spelled `DAT_` so the delinker emits
   them (see [Data matching](#data-matching)).
2. `giten.delink.data_manifest` writes the data + section manifests, including
   the `class=common` re-proof: every COFF COMMON row in
   `config/retail/data_compgen.tsv` must be emitted by some base obj, and the
   owner is the earliest-arriving module in link order among those that emit it.
3. `vostok-delinker` runs over `build/exe/DDS.EXE` (the stable retail copy)
   into `build/delink/named/`;
4. the in-scope `<unit>.c.obj` are collected into `build/objdiff/target-new/`.
   The address-bucketed `seg_NNNN.cpp.obj` for the un-named `.text` remainder
   stay behind and are never collected.

### The EH funclet band

cl 5.0 compiles every `/GX` function that owns a destructible object into two
pieces: the body, and a small EXECUTE COMDAT (`.text$x`) holding that function's
**unwind funclets** (`mov ecx,[ebp-X] ; jmp <dtor>`, one per unwind state)
followed by its **registration stub** (`mov eax,<FuncInfo> ; jmp
__CxxFrameHandler`), which the prologue pushes to build its
`EXCEPTION_REGISTRATION`. The retail linker packed every one of those COMDATs
into one contiguous band at the end of `.text` (RVA `0x1d7d00`..`0x1e3b55`).

No unit's contribution covered that band, so each prologue's `push` decomposed
as an **undefined `FUN_005exxxx` plus a nonzero addend**: the delinked object
set did not close over EH, and objdiff could only name-match the reference —
the funclet bytes were never compared.

`giten.delink.eh_band` derives each group from **retail data alone**: it scans
a claimed function's body for a `push imm32` landing on a `b8 …/e9 …` stub,
reads the `FuncInfo` that stub loads (magic `0x19930520`) and walks its unwind
map (and try-block map) for the funclet addresses. `pdb_synth` adds one record
per funclet plus one for the stub, attributed to the OWNING unit, superseding
the census's finer per-funclet `eh` rows so the delinker never sees overlapping
records. Naming mirrors cl's own labels — anything coarser is truncated at the
base's next `$L` label and compares against the wrong extent:

    __ehunwind$<owner>$<n>   the n-th unwind funclet, n in ADDRESS order (== state order)
    __ehreg$<owner>          the registration stub
    __ehfuncinfo$<owner>     the 32-byte `_s_FuncInfo` record the stub loads
    __ehunwindmap$<owner>    the `8 * maxState` unwind map that follows it

Both data extents are PROVEN out of the record rather than assumed: the blob
enrolls only when its own `pUnwindMap` word points at `funcinfo + 32` and its
try-block / ip-to-state maps are empty. `giten.compare.canonicalize` renames
the base's compiler-numbered `$L<n>` labels to the same names (the owner is the
function containing the `push`, identical on both sides), so the two sides
co-name without either reading the other.

Result: 750 groups / 2284 unwind funclets / 30,672 B carved, **zero** funclet
pushes left on an undefined `FUN_`, every push decomposing as `__ehreg$<owner>+0`,
and the funclet bytes genuinely compared. These symbols are scored but excluded
from the reconstruction-target denominator (`giten.verify.universe` classifies
the whole band `eh`).

Getting the stub's `mov eax,<FuncInfo>` right needed a DELINKER fix rather than
a manifest workaround (`nix/patches/vostok-data-hypothesis-must-contain.patch`).
`data_manifest::hypothesis_owner_and_addend_for_rva` ranked enrolled definitions
by `(!contains, distance, …)` but returned the best one even when NOTHING
contained the rva, with an unbounded addend — and both callers consult it BEFORE
the `--recover-data-relocs-from-pdb` fallback, so the guess beat the
exact-address PDB symbol. Measured: **1,020 of 21,730** enrolled-symbol data
relocations decomposed past their symbol's end, across 185 objects —
`??_R4CGruntVoice@@6B@ + 0x10800` into a 0x14 B RTTI locator (all 750 stubs),
`_inflate_mask + 0x3db4` into 0x44 B (164 sites), and negative addends where the
nearest enrolled datum sits AFTER the target. Requiring containment takes that
to **0**.

Strict containment deliberately excludes one-past pointers: the byte at
`datum_rva + sizeof(datum)` is not part of the datum. When retail proves that an
individual relocation encoded such an expression, record its exact function,
target, relocation-field RVA, owner and addend in
`config/retail/reloc_referents.tsv`. The delinker validates the owner/addend
equation, site membership and occurrence count before using it, and the build
graph makes the manifest an input to re-delinking.

## Normalization and pairing (objdiff)

Between delink and report, the `normalize` edge
(`giten.compare.normalize` → `giten.compare.canonicalize`) rewrites the
compiler-private data names (`$SG`/`$T`/`name$S<n>`), resolves COFF weak
externals to their default (cl's `??_E<C>` vector-deleting-dtor slot →
`??_G<C>`), materializes COFF COMMONs into `.bss` exactly as the linker would,
and rewrites same-function jump-table `DIR32` labels — into a
**content-addressed, disposable comparison copy** under
`build/objdiff/normalized/{base,target}/`.

Explicit source COMDAT aliases at one retail RVA are normalized to the Model's
primary name only after every claimed emitted body equals the complete retail
body. This proof currently accepts relocation-free bodies with alignment padding
only; a mismatching claim stops normalization. It does not infer aliases from
identical code. Function references are retargeted without merging definitions
or changing section bytes, and the symbol sidecars record the RVA and proof.
Each object's cache also records the proven alias-map digest.

The transform is **matching-neutral**: the real `base/` and `delink/` objects
are untouched, and a fail-closed reparse proves that only symbol names and
authorized jump-table and proven function-alias relocation fields moved while
every other resolved offset is identical. So normalization can only sharpen objdiff, never inflate a false
match. See [`docs/data-attribution.md`](data-attribution.md). The one
score-relaxing step, data relocation relaxation, is separate and runs only with
`data_matching = false` (next section).

## Data matching

`config/compare.toml` holds one committed switch, read through
`giten.core.data_matching`:

```toml
[compare]
data_matching = true
```

It is a separate file, not a `config/units.toml` key, because every `cl` edge
depends on the manifest: flipping the switch must not recompile every TU. The
delink, normalize and `verify_check` edges depend on it.

| | `true` (strict) | `false` (relaxed) |
| --- | --- | --- |
| compare | relocation targets scored by name and addend | `giten.compare.normalize.relax_data_relocations` runs after canonicalization on both copies: every code-section relocation whose target is not a function (COFF type `0x20`, or defined in a code section) and not an `__imp_` IAT slot or EH-band record is retargeted to one appended undefined `$data`, and its inline DIR32 addend is zeroed. A fail-closed reparse proves only those fields moved. Instruction bytes, immediates, call targets, function pointers, jump tables, EH records and data-section relocations stay strict. |
| delink | a game-referenced datum with no provided identity is an `UNPROVISIONED_` fence the delinker refuses | the same fences are spelled `DAT_`; the worklist is still `build/gen/data_debt.tsv` |
| placeholder `extern` | `undefined-closure` FAILS on every data extern a base obj references that no live base obj defines with a Model claim (and no provider table, `.LIB` or IAT supplies) | `undefined-closure` lists the same rows as advisory debt |
| data gates | `data-tu-order`, `data-coverage`, `data-relocs` referent rows fail | the same findings print with a count as `ADVISORY` and pass; `data-relocs`' unscored-unit and orphan-payload rows still fail |
| `data-access` | fails | fails: its gated categories are instruction-level facts about claims that exist |
| `data-identity` | fails | fails: an identity conflict is a bug the relaxation hides, not debt |

objdiff's own `functionRelocDiffs` cannot express the relaxed mode: its relaxed
settings drop function call identity too, and two undefined externals always
compare by name. Hence one shared sink symbol on both sides.

A unit may reference a global it does not own through a plain `extern` in the
owner module's header while data matching is off. No marker is needed: the
object closure identifies it (undefined in every base obj, or defined without a
claim).

The relaxation hides WHICH datum a reference names, so `data-identity`
(`giten.verify.data_identity`, normal tier) reads it before relaxation, in both
modes. Every function whose relocation layout lines up with its delinked target
(count, offsets, types, and each target on the same side of
`normalize.is_relaxed`) pairs its relaxed relocations positionally, each only
when the opcode bytes ahead of the operand agree too. A pair maps the base
symbol and addend to the retail rva the operand holds; less the addend, that is
the object base. It fails on one retail base reached through two source symbols
(pooled identical literals are one), one source symbol reaching two retail
bases, and a Model claim its own references contradict. The derived map is
`build/gen/data_identity.tsv` (`symbol`, `rva`, `sites`, `units`, `status`:
`claimed`, `provided`, `placeholder`, `literal`, `unclaimed`).

`giten build` ends with one line in both modes:

```
[giten build] data debt (data_matching = false): N unprovisioned identit(ies) (build/gen/data_debt.tsv), M placeholder extern(s) (`giten verify undefined-closure --list`)
```

### Scores and the mode

`config/match_baseline.tsv` records the mode it was banked under
(`# [data_matching]`; a ledger without it was banked strict). Scores from the
two modes do not compare: `giten verify check` fails and `giten verify bank`
refuses while the ledger's mode differs from `config/compare.toml`.
`giten verify bank --rebase-data-matching` banks the current build, sets MAX and
HIST of every scored row to its CUR, drops `absent` rows, and records the new
mode, so a mode change never shows up as lost matches.

### Re-enabling data matching

1. While still relaxed, run `giten verify data-identity` and keep
   `build/gen/data_identity.tsv`: its `placeholder` rows state each placeholder
   extern's retail rva (a strict build stops in the delinker until the debt is
   gone). An extern no paired function reaches has no row; prove its address
   from the retail references.
   Then set `data_matching = true` in `config/compare.toml` and `giten build`.
2. Work the debt to zero: each `undefined-closure` placeholder extern becomes a
   real definition with `DATA()` in its owner TU at that rva, and each
   `build/gen/data_debt.tsv` row gets a provided identity. The build fails in
   the delinker, and `giten build verify` fails, until both lists are empty.
3. With both at zero and the gates green, `giten verify bank
   --rebase-data-matching` and commit the ledger beside the switch.

`giten.compare.project` writes `compare-new/objdiff.json`. Symbols are
pre-named on both sides (cdecl `_<name>`), so objdiff pairs them **by symbol
name** with no `symbol_mappings` overlay, under **strict relocation scoring**
(target name/address AND the pointed-to data participate). Every manifest unit
gets a base entry; which units have a TARGET is read off the directory the
delinker wrote, never predicted — a unit with no target pairs against an empty
`dummy.obj` and lists at 0%. That distinction is load-bearing: predicting the
named set once left two data-only units pointing at the dummy while their real
target objs sat unopened, and objdiff scores an empty pairing 100.00% on every
measure with zero totals.

## Gates: `giten verify check --tier`

The graph's `verify_check` edge (`giten build verify`, run when preparing a
merge) runs the MAX gate plus the **fast** and **normal** tiers; `full` and
`link` opt in. The full roster is in
[`docs/tooling-map.md`](tooling-map.md#the-verify-slice--scores-the-max-gate-and-every-ported-gate).

| tier | question it answers | when |
| --- | --- | --- |
| **fast** | is the source text within its committed ledgers? (board, cast ledger, vtable bans, enum devices, label style, include order) | merge preparation |
| **normal** | is this change structurally safe? (name uniqueness, library overlap, TU order, data TU order, undefined closure; the data identity/placement gates are advisory while `data_matching = false`) | merge preparation |
| **full** | what reconstruction debt remains? (vtable tier, alloc-size sizeof oracle, reloc multisets, data relocs, caller/callee, the retail data-access map + the claim-side coverage census) | periodic, or to build a work plan |
| **link** | does it link, land where retail landed, and reach the same referents? | after `giten link` |

Two rules hold across all of them. A gate **returns findings and writes
nothing** (a finding is failing, or advisory: printed with its count, not
failing) — lowering a floor is always a separate manual verb (`giten verify
board --update`, `giten verify bank`). And every gate ships with its **negative
control**: `giten verify selftest` feeds each one a known violation and asserts
it FAILS, then asserts clean input passes. A gate nobody has seen fail is a
green light, not a check.

## Semantic navigation — `giten sema`

```sh
giten sema rva     0x00080850    # address dossier: winning binding, aliases, channel, match%
giten sema disasm  0x0008c750    # annotated retail i386 assembly  [--lite --blocks --switch]
giten sema dump    0x0008c750    # raw bytes + relocation targets + asm
giten sema xref    0x00080850    # callers, callees, referent sites
giten sema strings 0x00080850    # strings a function reaches; --find TEXT reverses it
giten sema vtable  0x001e8754    # a vtable's slots / who holds a function
giten sema class   CGrunt        # every vtable the class holds, slot by slot
giten sema map                   # the retail address-space map
giten sema match   cplay         # objdiff scores for a unit / function
```

sema is a **read-only consumer with four inputs and no policy of its own**: the
Model for identity, the retail image for bytes, the compare report for scores,
`config/units.toml` for the unit list. It writes nothing. Every module is also a
direct entry (`python3 -m giten.sema.xref 0x136180`), and `giten sema -` is
batch mode — newline-delimited view commands on stdin answered against ONE
loaded Model and image, so a 40-query investigation pays one parse instead of
forty. rc convention: **0 answered, 1 answered-NO, 2 error**.

**Doctrine: assembly only.** Nothing here decompiles; views annotate real
instruction bytes with Model labels, and a question the labels cannot answer is
reported as unanswered rather than guessed.

Two things sema deliberately does **not** do:

- **base-vs-target comparison.** That is the compare report's job, and
  classifying a divergence is `giten walls diagnose <fn>` — it reads the
  NORMALIZED pair (the exact evidence objdiff scored) and names the first
  divergence class: referent → inline/call-set → cfg → regalloc.
- **source navigation.** `giten lsp refs|hover|rename` is clangd-backed and
  USR-exact, so a same-named member of a different class is never touched.
  It needs `build/clangd/compile_commands.json`
  (`python3 -m giten.graph.compdb`, or just `giten build`).

clangd is a READER of this MSVC5 dialect: navigation is reliable, its
diagnostics are NOT build truth — the wine `cl` build and objdiff are.

## Formatting — the Rust-like house style

The reconstructed C++ is formatted with **clang-format** (from the Nix dev
shell) to read as close to Rust as the language allows: 4-space indent, 100-col
lines, attached braces *including on function definitions* (`int f() {`), `&`/`*`
bound to the type (`int* p`), a hanging-close (BlockIndent) wrap with function
*declaration* params one-per-line (call args and data arrays stay bin-packed, so
GUID/byte tables don't explode), and braces on every control body. The full
config — and the deliberate decompile-specific deviations — lives in the root
**`.clang-format`**.

Formatting is **whitespace-only ⇒ matching-neutral**: it never changes the COFF
bytes objdiff compares (the one parser-visible case, `> >` vs `>>` for MSVC 5.0,
is pinned by `Standard: c++03`).

**You normally never run it by hand.** A repo-tracked **pre-commit hook**
(`.githooks/pre-commit`) runs `clang-format` over staged `src/`+`include/` files
on each commit; the dev shell enables it on entry via `git config core.hooksPath
.githooks` (idempotent; shared across worktrees). Outside the Nix shell (no
`clang-format` on PATH) the hook skips with a notice rather than blocking the
commit.

Two deliberate deviations from pure rustfmt, because this is a decompile:

- **Comment text is never reflowed** (`ReflowComments: Never`) — the ASCII
  "carcass" diagrams and `// +0xNN` field-offset tables map source to
  disassembly and must not be rewrapped. Trailing comments *are* column-aligned
  (`AlignTrailingComments: Always`) so those offset columns stay tidy after the
  surrounding code is reflowed.
- **Includes are never reordered** (`SortIncludes: Never`). Include order here is
  hand-tuned and interleaved with explanatory comments.

**Vendored code is never formatted.** `vendor/` (e.g. `vendor/zlib-1.0.4/`) must
stay byte-for-byte as shipped — it is part of the matching surface. It sits
outside the `src/`+`include/` roots the hook touches, and is independently
guarded by `vendor/.clang-format` (`DisableFormat: true`), so even an editor's
format-on-save leaves it alone.

## Add a translation unit

1. add an `[[unit]]` block to `config/units.toml` (`unit`, `source`, and a
   `flags` profile);
2. `#include "rva.h"` and annotate **each** matched function with an `RVA()`
   macro directly above the definition, after the description. A real example
   from `src/Giten/SBI_RectOnly.cpp`:

   ```cpp
   // ---------------------------------------------------------------------------
   // CSBI_RectOnly::CSBI_RectOnly()
   // Inlines the CStatusBarItem base ctor (the dead m_8=0 store is elided),
   // stores its own vptr, then sets m_8 = 1.
   RVA(0x101fa0, 0x1b)   // retail .text RVA (VA = 0x400000 + rva), byte size
   CSBI_RectOnly::CSBI_RectOnly()
   {
       m_8 = 1;
   }
   ```

   The macros live in `include/rva.h` and compile to nothing under MSVC 5.0 —
   it predates `__attribute__` and C99 variadic macros, so each macro is
   FIXED-arity:

   - `RVA(addr, size)` — a matched function; `size` is its whole `.text`
     contribution, including the filler and the jump/index tables cl emits after
     the body (a code-only size leaves the table relocations unpaired);
   - `RVA_DECL(addr)` — on a function PROTOTYPE with no body in the tree yet
     (a callee declared in its owner's header): a label-only claim, channel
     `src_decl`. It names the retail body so every call relocation reaching it
     pairs by name instead of against `FUN_<va>`; it owns no body, so the
     function stays in the unowned delink bucket and remains a reconstruction
     target. Extraction reads it off the declaration with pylibclang (an
     annotation on a declaration never reaches IR), from every TU that sees
     the header. A definition's `RVA()` for the same rva supersedes it with
     the declaration recorded as an alias; a different name for the same rva
     is a model violation;
   - `DATA(addr)` — on the definition of a matched global;
   - `DATA_MESSAGE_MAP(map, entries)` — on `BEGIN_MESSAGE_MAP`, labels its two SDK-generated data objects (see [the pattern](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/mfc-message-map-real-static-data.md));
   - `RVA_COMPGEN(rva, size, mangled)` — a deterministically named
     compiler-generated function with no source body (such as a `??_G` deleting
     dtor) that cannot hold an attribute. Volatile ordinal names such as
     `_$E<n>` are FORBIDDEN here;
   - `RVA_DYNINIT(rva, size, owner)` — the `$E` dynamic-init helper, pinned at
     its OWNER (the owning datum's definition line) precisely because the `$E`
     ordinal is emission-order state, not identity;
   - `DATA_COMPGEN(rva, value)` — the last-resort use-site data pin; its rule
     and wiring are in [`docs/data-attribution.md`](data-attribution.md) §3b-iii.

   The vendored zlib C TUs keep PRISTINE source — no labels in it; their
   rva→symbol map is the static `config/retail/functions_zlib.tsv` (+
   `data_zlib.tsv`). See [`https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/zlib-matching.md`](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/zlib-matching.md).
3. `giten build`.

### Compiler-generated DATA with no source spelling

`config/retail/data_compgen.tsv` is a **manifest**, not a macro, and it covers
what neither source-side data device can reach:

| device | binds to | why it cannot reach this |
| :-- | :-- | :-- |
| `DATA(rva)` | a VarDecl in the MAIN file | a function-local static inside a **header** inline lives outside the main file, and extraction is main-file-only |
| `DATA_COMPGEN(rva, value)` | a value expression at a **use site** | a `??_B` dynamic-init guard byte has **no source spelling at all** — cl assigns it a counter (`??_B?1??Fn@@YAHXZ@51`) |

The manifest has two classes:

- **`class=common`** — the COFF COMMONs cl emits from a header inline's local
  static, plus the `??_B` guard beside them. cl emits a tentative definition
  into every TU that instantiates the inline and the linker merges the copies
  into one bss slot, so there is **no owning TU**: any source position would
  fabricate one. Only the retail address is stated; the `owner` column
  documents the emitting header inline. Everything else is re-proven every
  build — `giten delink` requires an emitting base obj for each row and errors
  on a row no base obj supplies.
- **`class=copy`** — a reviewed per-TU copy of a header static (the
  GruntDirStatics device), where the owner IS the emitting TU, decided by its
  `$E` static initializer's position. Several rows may share one rva, since each
  TU's copy folds onto the same retail byte.

**Why this is not the retired `DATA_SYMBOL`.** `DATA_SYMBOL` was a source
*declaration* that let a datum exist as a name-only pin **instead of** a real
C++ definition. Here the definition is real and the only fact stated is the
retail ADDRESS, which the compiler cannot know.

This class is invisible to every other signal — objdiff masks relocations so an
unnamed COMMON costs 0%, and it links perfectly well (`giten link` resolves
them all as `<common>`), so the link tier sees nothing either. That is why the
enrolment is a hard delink-time requirement rather than an advisory audit.

## Phase 2 — the candidate link (opt-in)

`giten link` (or `ninja candidate`) links every base `<unit>.obj` into
`build/exe/DDS.candidate.EXE` + `.map` with the genuine VC5 `link.exe`
(version **5.10.7303** — the linker that built retail DDS.EXE) under wine.
It is **out of the default target**, so a normal `giten build` is unaffected.

**There is no `/FORCE`, and it must never come back.** The tree links with zero
unresolved externals and zero duplicate symbols, so the link is an ORACLE.
`/FORCE` would re-swallow exactly the defects this phase exists to catch — an
unresolved extern (a fabricated name, a body homed nowhere) and an
LNK2005/LNK4006 duplicate. A link failure here is a FINDING: read the LNK codes
and fix the source.

Layout study uses `/OPT:NOREF /OPT:NOICF` to keep every COMDAT in the map;
retail is a flat `/INCREMENTAL:NO /FIXED` link with no `.reloc` (see
[linking](linker-flags.md)). The obj list goes
through a **response file** — VC5 `link` has a short argv limit under wine.
`link.exe` statically imports **`MSDIS100.DLL`** (the VC5 disassembler, only
used by `/dump /disasm`); `giten.tool.wine` provisions it into the prefix so
the linker loads at all.

### The library set

The objects already carry the CRT: `cl /ML` writes `-defaultlib:LIBC` +
`-defaultlib:OLDNAMES` into `.drectve`. We do **not** pass `/NODEFAULTLIB`, so
those fire exactly as they did for the devs. The Win32, WINMM and DirectX 6
import libs declare themselves nowhere and are named explicitly, `libc.lib`
first and then retail's import-descriptor order (KERNEL32, USER32, GDI32,
ADVAPI32, DDRAW, DSOUND, DINPUT, WINMM), `dxguid` last.

`giten.graph.implib` rebuilds any import lib the toolchain lacks from
**retail's own import table** (a stub DLL with matching decorated exports and
hints, re-verified against the produced archive). For DDS.EXE the toolchain has
all eight, so it builds nothing.

The link carries a real **`.rsrc`** built locally from the original EXE named
by `GITEN_RETAIL_EXE`. The graph converts its resource directory to an ignored
`build/gen/retail.res`; VC5 `link.exe` places the payloads at the candidate's
own resource RVA. The source tree contains no resource payloads or resource
download links. No RC.EXE or reconstructed resource script is needed for this
path. `giten link` depends on the supplied EXE, so changing it rebuilds the
generated `.res` and candidate.

The `.map` is the deliverable that feeds
[`https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/link-order-investigation.md`](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/link-order-investigation.md): each
function's link-assigned RVA and source object, which cross-referenced with the
retail RVAs recovers the original build order (intra-TU order = source-definition
order; cross-TU order = object link order).

## Generated vs. tracked

**Tracked:** `config/units.toml`, `config/compare.toml`, `config/retail/*` (the censuses, providers and
retail-derived evidence), `config/match_baseline.tsv`, `config/cleanliness/*`,
the `src/` sources with their label macros, `include/rva.h`, and the whole
`scripts/giten/` package.

**Generated (git-ignored):** `build/build.ninja`, `.ninja_log`/`.ninja_deps`,
and everything under `build/`.

| subdir | what it is | generated by |
|---|---|---|
| `gen/` | `claims/<unit>.tsv`, `bindings.tsv` (the serialized Model), `violations.tsv`, the delink data manifests, the data-access map + coverage artifacts, the fingerprint cache | the `labels`/`model`/`delink` edges and the verify gates |
| `objdiff/` | `base/<unit>.obj` (wine `cl`), `target-new/<unit>.c.obj` (delinked), `normalized/` (the comparison copies), `compare-new/objdiff.json` + `report.json` | the `cl`/`delink`/`normalize`/`project`/`report` edges |
| `delink/` | `named/` — raw per-symbol COFF objects straight out of vostok-delinker | the `delink` edge |
| `pdb/` | the synthesized PDB (`giten_named.pdb` + `.yaml`) | `giten.delink.pdb_synth` |
| `exe/` | `DDS.EXE` (retail + the synthesized `.reloc`; `$GITEN_EXE`), plus the candidate EXE/map/logs | the `reloc_image` edge / `giten link` |
| `clangd/` | `compile_commands.json` + the lowercase include mirrors | `giten.graph.compdb` |
| `lib/` | synthesised import libs (none needed for DDS.EXE) | `giten.graph.implib` |
| `ghidra/` | the viewer payload (`knowledge.json`) and the project | `giten ghidra export` / `build` |
| `wineprefix/` | the Wine prefix with the MSVC 5.0 toolchain registered | `giten init` |

## Current status

Run `giten verify status` for live scores. A normal build can refresh the
README's derived score block; only explicit banking writes the match ledger.
Do not maintain another score snapshot here or hand-edit between the markers.
