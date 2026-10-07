# Signed word widened through an unsigned mask

MSVC 5.0 SP3 can distinguish a signed mask from an unsigned mask even when
both expressions return the same low 16 bits. Check the literal's type as
well as its value when matching a word load followed by `and eax, 0xffff`.

The real `handle` translation unit provides a controlled example. With
`HandleEntry::size` declared `i16`, this expression in `GetHandleSize`
returns a `u32`:

```c
return GetHandleEntry(handle)->size & 0xFFFFu;
```

It emits an EAX index load, a word load into AX using that index, and the
full-register mask. Removing only the `u` suffix instead emits an ECX index
load, clears EAX, and loads the word into AX. The unsigned mask converts the
promoted signed member to unsigned before the AND; the signed mask leaves
that expression signed until the return conversion.

Reproduce the comparison in [handle.c](../../src/Mem/handle.c), retaining the
signed member in [Handle.h](../../include/Mem/Handle.h), with
`giten match handle` under the configured compiler profile. The two real-TU controls
leave the other handle function bodies and ordered relocations unchanged.
An unsigned member does not select the same sequence merely by adding this
mask.

This is a type-sensitive instruction-selection observation, not a universal
register-allocation rule or proof that every word followed by AND has signed
storage. Verify the writer, member width, return type and complete TU before
changing the declaration.
