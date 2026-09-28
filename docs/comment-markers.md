# Source markers

Address and symbol bindings use [include/rva.h](../include/rva.h) macros or
[retail provider tables](../config/README.md), never comments.
`giten verify label-style` checks the closed vocabulary of line-leading markers:

| Marker | Meaning |
| --- | --- |
| `// @early-stop` | Complete reconstruction with an evidence-bounded remaining mismatch; re-derive the live residue. |
| `// @identity-TODO` | Unproven identity; state what evidence would establish it. |
| `// @interleaver <sym>` | Proven linker-pooled member within another unit's contribution; record placement evidence. |
| `// @dead-code` | Proven zero-reference function; still requires full reconstruction. |

A marker starts its comment line; trailing text is prose. Other TODOs and
observations use plain prose, not new `@` names. Mid-line mentions are prose.
For `@dead-code`, verify no effective caller, data-slot reference or `.text`
address-taking with `giten sema xref --tree`; include the zero-reference proof.
An unreferenced thunk does not establish that the final body is dead.
