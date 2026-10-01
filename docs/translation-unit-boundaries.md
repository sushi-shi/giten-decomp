# Translation-unit boundary decisions

The [ownership groups](translation-units.md) explain the current manifest.
Their outer boundaries need separate evidence. An interval that must stay
together does not identify the start or end of its enclosing object.

## Applying the ordering rule

Use the pinned compiler, current profiles and ordinary COFF contributions.
Keep function RVA order. Exclude COMDATs, selected header copies and COMMON
symbols from ordinary-section ordering arguments.

For each proposed join:

1. Identify data using retail operands and candidate relocations, including
   addends. Source annotations provide candidate identities, not proof of
   original ownership.
2. Check whether separately owned contributions would overlap or reverse
   relative to code order. An inversion rejects the split **with that
   ownership**. A separate data owner remains an alternative.
3. Compile the joined sources. For each ordinary section, independently
   addressed members must agree on `retail RVA - candidate section offset`.
   Inspect bytes and relocations as well. Compare with each original object's
   section first: an existing discrepancy is not new boundary evidence.
4. Check whether the join pools constants that have distinct retail addresses.
   Conversely, shared retail operands emitted as ordinary compiler-local
   constants support keeping their users together.
5. Assess shared private state, helper relationships, responsibilities and
   initializer placement. These guide a reconstruction; thematic similarity
   or successful compilation is not a recovered boundary.

A failed concatenation can reflect declaration order, scope or initialization
spelling. Distinguish that from a join that cannot work under the ownership
model. C/C++ suffixes, duplicate reconstructed identifiers and incompatible
recovered prototypes do not prove separate original objects.

## Required separation from floating-point pools

With the C profile, compiling these functions together emits one ordinary
eight-byte zero constant; compiling separately emits one in each object:

```c
int A(double x) { return x < 0.0; }
int B(double x) { return x > 0.0; }
```

The same mechanism holds for these functions under the C++ `cpp` profile:

```cpp
float A(float x) { return x * 160.0f; }
float B(float x) { return x * x * 160.0f; }
```

Real-source controls reproduce that behavior:

| Code regions | Retail evidence | Joined-source result | Decision |
| --- | --- | --- | --- |
| Combat / field utilities | `ComputeWeaponDamage` uses zero at `0x642b0`; `RoundToInt` uses zero at `0x64318` | Joining `partyaction`, `bitset`, and `fieldview` emits one zero pool member | At least one boundary separates those users. The bit helpers' side of that boundary is unresolved. |
| D3D application / Windows main code | `ProjectVector` uses float 160 at `0x649e8`; `TurnLeftStep` uses float 160 at `0x64a4c` | Joining `d3dapp`, `joystick`, and `winmain` emits one such pool member | Preserve a separation; initializer/helper evidence adds placement constraints. This does not decide joystick's ownership by itself. |

Reproduce the first real-source control inside `nix develop`:

```sh
mkdir -p build/tu-boundaries
cat src/Game/partyaction.c src/Util/bitset.c src/Game/fieldview.c > build/tu-boundaries/combat-utilities.c
giten tool cl --out build/tu-boundaries/combat-utilities.obj --src build/tu-boundaries/combat-utilities.c -- /nologo /c /Ox /Zp1 /ML
llvm-readobj --sections --symbols --relocations --section-data build/tu-boundaries/combat-utilities.obj
```

This constraint is on compiler-generated literal pools under these profiles.
Replacing literals with externally owned named constants is a different source
model. Identical bytes at different addresses alone do not establish a cut;
identify the actual pool operands first.

## Joins contradicted by current initialized-data layout

These adjacent joins move later file-scope state ahead of earlier data that
retail places before it. Individual ordinary sections supply consistent
anchors; the joined sections do not. Keep the owners separate unless new
scope/ownership evidence explains the difference.

| Current cut | Representative anchors | Structural assessment |
| --- | --- | --- |
| `fieldobj` / `worldtravel` | `s_approachX` at `0x685e0`, `s_shownPlace` at `0x68628` | Local object simulation and world-travel presentation have different persistent state. `s_shownPlace` is shared by multiple functions, so a function-local replacement does not explain it. |
| `worldtravel` / `fieldmain` | `s_shownPlace` at `0x68628`, `s_fieldImageCacheKey` at `0x6864c` | World-map display state and field-image caching have separate lifecycles. The cache key is shared by reset and lookup functions. |
| `fusion` / `skilluse` | `s_fusionInfoPlane` at `0x68f88`, `s_promptX` at `0x690c8` | Fusion UI and action targeting are coherent separate groups. This witness depends on scope: stored-only prompt coordinates could be function-local in another reconstruction. |
| `skilluse` / `recordcache` | `s_promptX` at `0x690c8`, `g_cachedRecordId` at `0x69108` | Skill targeting and record-cache management have distinct state. The cache ID participates in several functions and has an external interface. |
| `character` / `statusmenu` | `s_affinity` at `0x69ee0`, `s_statusMenu` at `0x6a070`; `.rdata` also conflicts | Character calculations and status-menu presentation are a sensible split. The menu object is used by several menu operations. |

These results support retaining the cuts. They do not independently prove
that the first/last data-free function on each side has the correct owner.

## BSS-sensitive cuts

The current joins `debugmenu`/`blit`, `message`/`vramaccess`,
`playtime`/`screensave`, `screensave`/`savegame`, and `savegame`/`handle` also
fail ordinary-BSS placement controls. Keep these as conditional evidence:
explicit zero initializers, uninitialized statics and COMMON storage have
different emission behavior. Equivalent zero-initialization spellings can
change placement without changing runtime semantics.

For example, this C control places `second` at BSS offset 0 and `first` at 4:

```c
static int first = 0;
int* First(void) { return &first; }
static int second;
int* Second(void) { return &second; }
```

Changing only `second` to `static int second = 0;` reverses those offsets.
Therefore a BSS-only mismatch from a proposed join is weaker than a
compiler-pool conflict. Check initializer spelling before treating it as an
original object boundary.

The divisions are nevertheless reasonable: diagnostic UI versus bitmap
preparation; message state versus legacy screen-buffer helpers; play-time
counters versus refresh requests; refresh versus save-game serialization;
and serialization versus the shared handle allocator. No storage inversion
requires those joins.

## Plausible joins that remain unresolved

| Region | Assessment and current decision |
| --- | --- |
| `bitset` / `fieldview` / `vec3` | Bit helpers could extend the general-utility prefix, and vector helpers fit the spatial-utility tail. Neither has ordinary data fixing its side of a cut. Keep their ownership unresolved; the combat zero-pool constraint does not choose a side for `bitset`. |
| `statestack` / `waitstate` | Wait/fade/message handlers are close clients of the state-stack API. Separate handlers and a larger state-machine object both make sense. Calls remain out of line in the joined C control; absence of inlining supplies no boundary witness. |
| `itemrecord` / `equipeffect` | Equipment effects naturally use item records, but that consumer/helper relationship alone does not establish common ownership. Keep the cut provisional. |
| `windowcolor` / `scripttext`, and adjacent small script units | Opcode families are plausible groups, but all share the VM API. Grouping by topic would not locate original object boundaries. The sprite/variable join has ordering evidence; these additional joins do not. |
| `scriptctx` / `eventflags` / `scriptvars` | Context helpers, flag storage, and opcodes can have independent owners despite close call relationships. The tested joins add no decisive boundary evidence. |
| `gamestate` / `character` | Party/map access and character calculations have related but distinct responsibilities. The party singleton has external users; its use is not evidence of a shared TU. |
| `statusmenu` / `clickwait` / `statuspanel` | A strong semantic candidate for a larger status-screen unit. Menu state and page state also make sensible separate objects. The tiny click helper has no ownership anchor; keep both cuts provisional. |
| `nibble` / `textapi` / `menuresult` / `windowtext` | Text-facing helpers are adjacent, but nibble manipulation and selection setters are reusable primitives. No ordinary-data evidence attaches these data-free helpers to either neighboring owner. |
| `abortflag` / `videostate` | Abort/hotspot state and plane/video access can be distinct low-level interfaces. Shared platform callers do not resolve ownership. |
| `bitmapio` tail | `DrawScreenEffectSprite` shares `s_effectImage` with preceding effect operations, extending the conditional private-state span through it. `CopySurfaceSquare` is a generic DirectDraw utility; `ClickHotspotAt` combines input, hotspots and texture sampling. Both fit a broad display unit, but neither has an anchor fixing its inclusion. Retain the reconstruction with an explicitly provisional tail. |

Other surviving manifest cuts are not certified merely because these checks
found no contradiction. A data-free fragment can still belong to a neighbor.
An unconstrained internal cut is also not an instruction to split a coherent
TU into one file per helper.

## Avoid false ownership edges

An address operand can land inside a neighboring datum without using it.
Resolve the candidate symbol **and addend**, then check the instructions and
source indexing. Concrete traps in this codebase are:

- `PreviewEquipChange` indexes `s_equipCountSlots` with a negative base
  adjustment. The encoded displacement lands inside `s_switchedItems`.
- `CanFloodViewCell` addresses the previous row of `s_viewFloodMask`; its
  adjusted displacement lands inside a preceding combat string.
- `MarkReachableWorldTravelCells` addresses the previous row of
  `s_travelScores`; its displacement lands inside `s_scriptSets`.
- `CreateTextPlane` compares against the end of the text-plane array. That
  one-past address coincides with the guard used by `DrawScreenFade`.

None supplies a private-data link across the current TU boundary.
