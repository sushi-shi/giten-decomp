# Local declaration order can decide a frame local's reload

Signature: the calls, branches and registers agree except that one
frame-homed local is used as a memory operand on one side (`test [esp+N],edi`)
and loaded into a scratch register first on the other
(`mov ecx,[esp+N]` / `test ecx,edi`), often with the scratch registers of a
later block exchanged as a consequence.

With MSVC 5.0 SP3 and `/Ox /Zp1 /ML`, the order in which the locals are
*declared* decides the form, independently of the order the assignments
execute. Compile this standalone C file and change only the declarations:

```c
extern int ReadScriptValue(void);
extern void Use(int);
extern int* roster[32];
void f(void) {
    unsigned bit = 1;               /* declared before mask: test [esp+N],reg */
    unsigned mask = ReadScriptValue();
    short i;
    for (i = 0; i < 32; i++) {
        if (roster[i] && (mask & bit)) {
            Use(i);
        }
        bit <<= 1;
    }
}
```

Declaring `mask` before `bit` (`unsigned mask; unsigned bit = 1; ...
mask = ReadScriptValue();`) keeps `bit = 1` ahead of the call and emits the
reload before the test. Spelling the test (`bit & mask`, `!= 0`, a nested
`if`), the integer types, and where `bit` is initialized do not change the
form; the declaration order does.

This closed [OpRecoverRosterPool and OpCureRosterCondition](../../src/Script/scriptactor.c).
It is an observation about which of two locals competing for a register is
spilled, not a rule that every reload means a moved declaration. Confirm the
calls, constants and ordered referents first; other TU context can also move
allocation ([Translation-unit context](tu-state-probe-family-decides-reachability.md)).
