# Enum domains

A proven value domain is declared once, in a shared header, with the macros
of `include/Enums.h` and `include/EnumDomain.h`. The retail build sees
ordinary C enums and the original integer types, so records, codegen and C++
decorated names do not change.

## Declaring

| Macro | Use |
| --- | --- |
| `GZ_ENUM_BEGIN(Name)` / `GZ_ENUM_END(Name)` | A domain with no single narrow width. |
| `GZ_ENUM_BEGIN_SPLIT(Name, storage)` / `GZ_ENUM_END_SPLIT(Name)` | A domain stored at one retail width. |
| `GZ_ENUM_FLAGS_BEGIN(Name, storage)` / `GZ_ENUM_FLAGS_END(Name)` | A bit set; members combine with `\|`. |
| `GZ_ENUM_CONST_BEGIN(Name)` / `GZ_ENUM_CONST_END(Name)` | Encoding biases, masks, sentinels and counts; not a type. |

A domain needs its value set from compares, switch arms, stores and table
indices, and its names from what each value does: the handler a switch arm
runs, a string or trace the code prints, a resource the value selects. Name
the proven members only and leave the rest numeric under an
`@identity-TODO`. A range test compares against a `_FIRST`/`_LAST` or
`_BEGIN`/`_END` marker, never a member.

## Using

`GZ_ENUM_STORAGE(Name, storage)` types a field, global or table at the
domain's declared width. `GZ_ENUM_PARAM`, `GZ_ENUM_RETURN` and
`GZ_ENUM_LOCAL` type a parameter, return value or temporary whose retail
width is evidenced separately. Every declaration of a function spells its
annotations alike, and a header only names a domain that its own includes
declare.

`giten verify enum-domains` (fast tier) checks that storage widths agree
with the declaration, that storage names a declared domain rather than a
constant group, that headers hold no bare `enum` blocks, and the range-test
naming rule.

The [enum reuse review](enum-reuse.md) compares evaluated values across
domains and records which equal-value declarations actually share a type.

## Codegen constraint

cl 5.0's output depends on the declarations a unit reads. An added
enumerator or `#include` renumbers them and can move unrelated functions,
while macros and the storage annotations above do not
([measurement](patterns/tu-state-probe-family-decides-reachability.md#enumerators-and-includes-count-macros-do-not)).
Such a move in a function you did not edit keeps its MAX, so it never blocks
a proven domain. Edited functions do reset MAX to CUR: when naming constants
in bodies, rebank any function whose CUR was below MAX (a probe-banked or
TU-dipped row) under its probe.

## Constants work list

`giten verify constants` lists every numeric constant in `src/` and
`include/`; each is open until it is written as a name (an enumerator, a
named macro, `NULL`, `true`/`false`) or `config/constants.tsv` keeps it
numeric with a reason. `--list [FILTER]` prints the open ones for a file or
owner, `build/gen/constants_open.tsv` holds them all, and the committed floor
of open constants never rises.
