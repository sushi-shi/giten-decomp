# A fill loop can become an inline memset

cl 5.0 `/Ox` recognizes a counted loop that stores one constant into
consecutive elements of an array and replaces that store with an inline
`memset`: `rep stosd` for larger spans, or a few dword stores for short ones.
When the loop also does other work, the constant fill is split out and the
remaining statements stay in a loop. A zero fill of eight words therefore
emits four dword stores, not a word-store loop.

Signature: a `rep stos`/dword-store fill followed by a strided loop, where the
residual loop's induction register is initialized *before* the fill:

```asm
or   eax, -1
mov  edi, OFFSET order
mov  edx, OFFSET slots+2      ; residual loop cursor, set up first
rep  stosd
L:  mov word ptr [edx], -1
    add edx, 8
    cmp edx, OFFSET slots+0x102
    jl  L
```

In `UnplaceAllSprites` this came from one loop:

```c
for (slot = 0; slot < SPRITE_SLOT_COUNT; slot++) {
    g_spriteOrder[slot] = SPRITE_UNPLACED;
    SetSpriteSlotFrame(GetSpriteSlot(slot), SPRITE_UNPLACED);
}
```

Writing the fill as an explicit `memset` ahead of the loop, or as a separate
loop, emits the same fill but initializes the cursor after `rep stosd`, in a
different register. The counter's type also matters: an `i16` counter turns
the residual loop into a `dec`-counted loop.

Limits: a fill that stays a loop is not proof that the source had no fill
loop. A descending loop, or an index narrowed through an `i16`/`u8`
parameter, kept the loop in the controls tried. Conversely, an inline
`memset` in the output can be a source `memset` or a distributed loop; prefer
the loop only when the schedule or other evidence distinguishes them.
