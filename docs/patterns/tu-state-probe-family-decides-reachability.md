# Translation-unit context can change an unchanged function

Recorded VC5 experiments changed unrelated declarations while leaving a target
body unchanged and obtained different allocation or scheduling. Function text
alone is therefore not a complete description of compiler input.

The [historical IL experiment](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/tu-state-probe-family-decides-reachability.md#quantified-2026-08-13-the-input-mechanism-is-c1xx-symbol-handle-renumbering-and-each-probe-kind-has-a-measured-stride)
used /d1il<prefix> to capture the ex/gl/in/sy streams and /d2il<prefix> to replay
them. Its reported replay reproduced the changed output. This locates an input
difference at the frontend/backend boundary; it does not prove the downstream
decision is made by the frontend. See the separate
[backend investigation](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/relevations/cl5-globalopt-has-a-511-handle-phase.md).

For a controlled comparison, keep compiler, flags, headers, and target source
fixed; retain both objects and compare ordered referents as well as instructions.
IL byte differences need interpretation, including source-line records.

A flat sweep means those probes did not move that input. Neither uniform nor
mixed probes prove a function unreachable, a TU permanently insensitive, or a
particular backend cause. Historical handle strides are measurements of those
probes, not portable compiler constants.

Do not retain unused declarations, includes, or fake locals to select an output.
Use the bounded [permuter workflow](../permuter.md), not an unbounded hunt for
a favorable score. Source correctness and the current MAX policy still govern
what is kept.

## Declaration count in this project

A giten measurement (lane 1, `font.c`): with the target body and every other
declaration fixed, adding k unused prototypes before `GetTextPlanePageLines`
(0x453010) gave the retail register assignment for k = 1, 3, 4, 5 and an
esi/edi swap for k = 2. The effect is not monotonic in the count, so it says
nothing about which declarations the original TU held.

It surfaces whenever a shared header gains or loses prototypes: an unrelated
unit that includes the header dips (a DIP, its own source unchanged), and an
unedited function can load `a + b` in the other order (field.c's
`s_fieldParamThird + s_fieldParamFirst`, which `data-identity` counts as an
operand-order swap, not a finding).  The declarations in
scope were the original TU's, which an incomplete TU does not reproduce, so
the board counts declarations placed away from their definition.

## Enumerators and includes count; macros do not

Enum work measured the same input (units compiled with the target bodies
fixed, compared after masking `$S`/`$SG`/`$T`/`$L` ordinals):

- One added enumerator in `<Game/AreaMap.h>` changed eight units;
  `RelativeFacing` fell to 79.67 and `LoadFieldMemory` to 87.03.
- One added `#include` of a header whose only content is its guard changed
  twelve units; adding `<EnumDomain.h>` to `<Mem/Handle.h>` changed only
  `vramaccess.obj`, the one unit that had not already read it.
- Three added `#define`s changed nothing, nor did macros added to a header
  every affected unit already read.

Spelling a macro-backed storage type (`GZ_ENUM_STORAGE`, `GZ_ENUM_PARAM`)
or naming an existing constant at a literal is therefore object-neutral. A
new enum, enumerator or include in a shared header renumbers every unit that
reads it, and its exact matches have to be checked before it is kept.

## Declarations after the function

In a C++ TU the context includes what follows the function. With
`bitmapio.cpp` otherwise fixed, appending one unused prototype, typedef or
initialized global after its last function reordered the hoisted
channel-global loads in `LoadBitmapToSurface16` and `CopyResourceBitmap16`,
which come hundreds of lines earlier. The same three appended lines left every
function of two C units (`scriptvars.c`, `treasurebox.c`) unchanged. When a C
unit becomes C++ (a fold into a C++ object), its bodies therefore become
sensitive to the whole TU, including code and declarations after them.
