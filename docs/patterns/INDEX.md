# MSVC 5.0 pattern reference

Short observations, not a wall database. [Scope and admission rules](README.md).

## Compiler output

- [Template inline eligibility](vc5-template-members-inline-without-inline-keyword.md) — an unmarked template member can expand under /Ob1.
- [Explicit-only template arguments](vc5-explicit-only-template-arguments-collapse.md) — `F<X>()` with `X` absent from the parameters collapses instantiations within a TU.
- [Mixed inline and out-of-line calls](inline-budget-emits-ool-comdat.md) — inspect each call site; symbol presence is not an expansion census.
- [EH frames and lifetimes](eh-frame-presence-is-a-source-fact.md) — unwind records are evidence, not an object counter.
- [Local-static guards](function-local-static-dynamic-init-guard.md) — recognize dynamic initialization without inventing flag globals.
- [Scopes and stack slots](switch-arm-locals-overlay-only-when-scoped.md) — sibling scopes can change stack reuse.
- [Store scheduling](emitted-store-order-is-not-the-source-order.md) — emitted order need not be source order.
- [Call arguments](call-argument-evaluated-before-pushes-means-a-temporary.md) — an inner call evaluated before the other pushes went through a local.
- [Call products](call-product-statement-boundary.md) — assigning a product before comparing can change allocation across the function.
- [Translation-unit context](tu-state-probe-family-decides-reachability.md) — unchanged function text can emit different code.
- [C globals and COMMONs](c-bss-globals-and-commons.md) — a C global inside an object's `.bss` run was zero-initialized; bare ones are linked after all `.bss`.
- [Object boundaries from data](object-membership-from-data-layout.md) — interleaved statics, data order and literal runs show which units form one object.
- [Signed remainder](signed-modulo-pow2-abs-restore.md) — sign correction around a power-of-two mask.
- [Extraction and consumer widths](wide-extraction-narrow-consumer.md) — a wide mask can feed a narrow index without a runtime copy.
