# A shared return can forward the stored register

Signature: a path stores a constant into a global and returns that global;
retail returns it from the register the store used (`mov [g],bp` then
`mov ax,bp`), where this build materializes the constant again
(`xor ax,ax`).

In a real-TU A/B of `RunPartyCommandInput` (partyaction, the unit's C flags),
with `di`/`bp` already holding zero:

```c
/* Returns in place: the reload of g_tickElapsed folds to the constant. */
g_tickElapsed = 0;
return g_tickElapsed;          /* mov [g],bp ; xor ax,ax */

/* Leaves through the function's common `return g_tickElapsed;`. */
g_tickElapsed = 0;
break;                         /* mov [g],bp ; mov ax,bp */
```

cl 5.0 still emitted a separate `ret` for each path in the second form; the
common return block was copied into the predecessor. Its reload of the global
then became a copy of the register just stored instead of the folded
constant.

The same function showed a related tail shape for a bitfield store. Retail's
target-selection branches each begin the 14-bit `pickObject` store (load the
word, `xor` in the new value) and jump into the `and`/`xor`/store suffix of
another path. That compiled from each branch storing the field itself before
a `goto` to the label that follows the other path's store; assigning a result
variable and storing it once at the label emitted a single store after the
label instead.

Limits: this was observed in one switch-driven function whose exits were
`break` to a trailing `return` of the same global. It does not show when cl
chooses to duplicate a return block. Use it only when retail's return copies a
register that a preceding store used; a returned constant alone is not
evidence of a shared exit.
