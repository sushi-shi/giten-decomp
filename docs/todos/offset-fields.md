# Offset-derived field names

`giten verify board --offset-fields` lists declarations whose names are
`byte`, `word` or `dword` followed by a hexadecimal offset. The current
lexical worklist has 14 declarations. It counts declaration sites, not every
read or write, and is non-ratcheted: some fields have no reader in the
reconstructed code. The older `address-derived identifiers` metric covers
different name shapes and therefore missed these.

| Owner | Fields | Current evidence and next question |
| --- | --- | --- |
| `AreaLevelRecord` and `AreaLevel` | `byte24`, `byte25` copied to decoded `byte3e`, `byte3f` | `DecodeAreaMap` copies the pair; no semantic reader is identified. Trace consumers or file-format labels before naming either byte. |
| `Character` and `FieldObject` | `byte069`, `byte083` | `Character.byte069` is assigned `gender`, then copied into `FieldObject.byte083` and `FieldObject.gender`. This suggests a related value but does not prove why the object stores it twice. Trace both stores and every reader. |
| `FieldObject` | `byte096`, `word098` | `fieldobj.c` writes 1 and 0x11 on a creation path; no distinct reader has yet established the roles. |
| `FieldObject` | `byte21a`, `byte21b`, `word21d`, `word21f`, `word221` | Initialization clears all five; one path later sets `word21d` to 1. Identify a reader and the complete state transitions before replacing offsets with domain names. |
| `PanelRow` | `word04` | The row reset writes zero; the claimed code has no reader. Keep the neutral name until a row operation or data record proves its role. |

The worklist recognizes only these obvious primitive declarations. Names
with other prefixes and casts that encode an offset remain outside this
metric and need source/retail review.
