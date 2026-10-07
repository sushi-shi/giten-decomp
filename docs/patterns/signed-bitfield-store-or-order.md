# Signed byte bitfields and store operand order

MSVC 5.0 can select different operand ownership for a one-bit assignment
depending on the destination field's signedness. An unsigned destination
keeps the extracted source bits as the OR's left operand; a signed destination
keeps the masked old destination byte there. The left operand receives the
result on x86, so the choice also affects load scheduling and register use.

The distinguishing signature is two adjacent copies from one unsigned
source byte: the first ends with `or old_byte, source_bits`, while the second
ends with `or source_bits, old_byte`. The masks and stored bits agree.

The real-TU control is [InitObjectFromRecord](../../src/Game/fieldobj.c).
Change only `PickFlags.recordFlagA` from `i8 : 1` to `u8 : 1`, retaining the
unsigned `recordFlagB` and `itemSkill` fields and the serialized
`ObjectRecordFlags` type. Compile the unchanged function with the `fieldobj`
profile from `config/units.toml` and compare both flag-copy blocks. The
signed first field reproduces retail's destination-byte-first OR; the
unsigned control reverses that ownership. The second copy remains unchanged.

This establishes representation from the compiler signature. It does not
establish a semantic name or a scalar consumer's value domain: the first two
pick fields have no identified scalar readers. Their identities remain TODO.
The measured case uses one-bit fields in a byte allocation unit with an
unsigned bitfield source; wider sources can select a different merge idiom.
