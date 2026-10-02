# Scene function matching

| Retail RVA | Function | Remaining evidence and next lever |
| --- | --- | --- |
| `0x045930` | `DrawSceneSprite` | Retail and current code have the same eight calls, 15 branches, three returns and 20 ordered relocations. Retail loads the object pointer into `ecx` after the null guards and retains it through the visible-sprite path; current MSVC uses `eax` and later reloads the pointer, leaving one extra `mov` and a two-byte size difference (0x1b9 versus 0x1b7). The sprite layout, argument order and visibility calls agree. Reversing the private `IsSceneObjectVisible` parameter order and moving X/Y into a C89-safe inner scope compiled byte-identically; both probes were restored. The retail function has no known caller, so its entry parameters cannot be retyped from an observed call. |
