# Emitted stores need not follow source order

The optimizer can move independent loads and stores. Transcribing a retail
store sequence into C++ and recompiling need not reproduce that sequence.

The [recorded ActionOptionsMenuBar::Init A/B](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/patterns/emitted-store-order-is-not-the-source-order.md)
compares two six-member assignment orders. Assigning in member declaration
order reproduced a different, retail store order.

Use data flow to identify each stored value, especially stores interleaved with
the next call's argument setup. Check helper expansion and real aggregate-copy
boundaries before permuting individual stores. Compare from the first
divergence, since changed uses can alter earlier allocation.

Declaration order is a source hypothesis, not a compiler fixed-point rule.
Nor does one sequence emitted unchanged prove that it was the original source.
Aliasing, volatile access, calls, construction order, and dependencies constrain
which reorderings are semantically valid. Matching counts or store offsets alone
do not prove equal values, source identity, or correctness.

## Independent loop advances

A loop can have the same emitted loads, stores, branches and instruction order
while a terminal update uses a different temporary register. Check the source
sequence between the body's final update and the loop's increment clause:

```cpp
for (k = 0; k < 4; k++) {
    // Write mesh->vertices[mesh->vertexCount].
    mesh->vertexCount++;
}
```

When the two advances are independent, this alternative changes their source
evaluation order without changing which vertex the body writes:

```cpp
for (k = 0; k < 4; k++, mesh->vertexCount++) {
    // Write mesh->vertices[mesh->vertexCount].
}
```

In the MSVC 5.0 `/Ox /Ob0 /Zp1 /GX` A/B for `BuildRoomMesh`, the emitted
load/increment/store of `vertexCount` changed from EBP to EBX; the rest of the
normalized body was identical. The same edit in `BuildQuadMesh` changed its
terminal temporary allocation. The source control is reproducible from
`git show 841f4e0 -- src/Platform/winmain.cpp` with the ordinary matching build.

This is a bounded allocation observation, not evidence that cursor advances
always belong in the increment clause. A `continue` skips the body's trailing
update but executes the increment clause, and aliases or dependencies between
the counters can make the reordering observable. Audit those cases before using
this control. Prefix versus postfix spelling alone does not express this
change in source evaluation order.
