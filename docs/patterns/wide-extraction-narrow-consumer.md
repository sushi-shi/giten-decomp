# Wide extraction followed by narrow consumption

A mask computed in a full register, followed by word-sized tests and explicit
zero extension for indexing, can describe two expression widths. Widening the
consumer alone may fix the mask while making the rest of the body less like
retail.

In a real-TU control of `BlitGlyph` under the font unit's MSVC 5.0 SP3 flags,
these source forms produced different instructions:

```c
/* Narrow destination: the mask was lowered through a byte register. */
u16 bg;
bg = attr & TEXT_ATTR_BG;

/* Wide destination: full-register mask, but also wide tests and indexing. */
i32 bg;
bg = attr & TEXT_ATTR_BG;

/* Wide extraction, followed by the narrow palette index. */
i32 background;
u16 bg;
background = attr & TEXT_ATTR_BG;
bg = background;
```

The last form retained `mov ebp,edx; and ebp,0xf` for extraction while using
`test bp,bp` and zero-extending the word for palette indexing. Its intermediate
assignment did not add a runtime copy. The complete body in
[font.cpp](../../src/Text/font.cpp) is the reproduction context; a small isolated
function need not make the same register choices.

This observation does not establish the original local names or a universal
MSVC rule. Use it when the instruction widths support both stages, and compare
all consumers. It is not a reason to add redundant temporaries to unrelated
field loads or to keep a wider type whose later operations contradict retail.
