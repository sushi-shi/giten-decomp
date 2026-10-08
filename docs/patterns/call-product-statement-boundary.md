# A call product can need a separate assignment

Signature: the calls and branches agree, but allocation differs around a
comparison containing an integer product and a function call.

With MSVC 5.0 SP3 and `/Ox /Zp1 /ML`, these forms can emit different code even
when `roll` is an existing `i32` local and the product already has type `i32`:

```c
if (attack >= defense * RandomAverage(0, 7, 0)) {
```

```c
roll = defense * RandomAverage(0, 7, 0);
if (attack >= roll) {
```

The separate assignment reproduces the retail allocation in
[RollGunHit](../../src/Game/partyaction.c) and
[RollWeaponHit](../../src/Game/partyaction.c). To reproduce the observation,
compile the full `partyaction` TU with `giten match`, combine either final assignment and
comparison, and compare again with all headers and other source fixed. The
instruction changes extend earlier than the edited expression; the named
value does not require a new declaration or a stack store.

This is a bounded compiler observation, not a rule that every call product
needs a local. It does not establish the original variable name or explain
which optimization caused the allocation change. Check arithmetic widths,
evaluation order, and ordered referents before applying the same spelling
elsewhere.
