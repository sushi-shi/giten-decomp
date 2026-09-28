"""Normalize explicitly pinned, byte-proven folded function identities.

Sharing bytes alone never establishes an alias. The Model must first bind both
source COMDAT names to the same retail RVA. Every claimed emitted body must
then equal that complete retail body, without relocations or non-padding tail.
"""
from __future__ import annotations

import hashlib
import json
import struct
from pathlib import Path

from giten.compare import canonicalize as canon


def _body(obj: canon.CoffObject, name: str, size: int) -> bytes | None:
    hits = []
    for section, ranges in canon._function_ranges(obj).items():
        for start, end, symbol in ranges:
            if symbol.name != name:
                continue
            raw = obj.section_bytes(obj.sections[section - 1])[start:end]
            if len(raw) < size or any(byte not in (0x90, 0xCC) for byte in raw[size:]):
                return None
            if any(r.section == section and start <= r.site < end
                   for r in obj.relocations):
                return None
            hits.append(raw[:size])
    return hits[0] if len(hits) == 1 else None


def prove(model, base_dir: Path, img, *, overrides=None,
          _base_objects=None) -> dict[str, tuple[str, int]]:
    """{alias: (Model primary name, RVA)}, only for verified source COMDATs."""
    aliases = {}
    objects = {}
    override_paths = set((overrides or {}).values())

    def object_at(path):
        if path not in objects:
            payload = path.read_bytes()
            cached = (_base_objects or {}).get(path)
            if path in override_paths or cached is None or cached.data != payload:
                cached = canon.CoffObject(payload)
            objects[path] = cached
            if _base_objects is not None and path not in override_paths:
                _base_objects[path] = cached
        return objects[path]

    for binding in model.functions:
        if binding.channel not in ('src', 'src_compgen') or not binding.name:
            continue
        claims = [binding] + [a for a in binding.aliases
                              if a.channel in ('src', 'src_compgen') and a.name]
        if len({c.name for c in claims}) < 2:
            continue
        expected = img.read(binding.rva, binding.size)
        if (not expected or len(expected) != binding.size
                or img.relocs_in(binding.rva, binding.rva + binding.size)):
            raise ValueError(f'folded function 0x{binding.rva:x} has no relocation-free retail body')
        for claim in claims:
            if claim.size != binding.size:
                raise ValueError(f'folded function {claim.name} has a conflicting extent')
            path = (overrides or {}).get(claim.unit, base_dir / f'{claim.unit}.obj')
            if _body(object_at(path), claim.name, binding.size) != expected:
                raise ValueError(f'folded function {claim.name} in {claim.unit} '
                                 f'does not equal retail 0x{binding.rva:x}')
        for path in (overrides or {}).values():
            obj = object_at(path)
            defined = {s.name for s in obj.symbols.values() if s.section > 0}
            for claim in claims:
                if claim.name in defined and _body(obj, claim.name, binding.size) != expected:
                    raise ValueError(f'folded function {claim.name} in candidate {path} '
                                     f'does not equal retail 0x{binding.rva:x}')
        for claim in claims:
            if claim.name == binding.name:
                continue
            value = (binding.name, binding.rva)
            if claim.name in aliases and aliases[claim.name] != value:
                raise ValueError(f'conflicting folded function identity for {claim.name}')
            aliases[claim.name] = value
    return aliases


def prepare(base_dir: Path):
    """Snapshot identities once; re-prove bodies for each disposable TU object."""
    from giten.model import resolve
    from giten.sema.image import retail
    model, img = resolve(), retail()
    base_objects = {}

    def aliases(overrides=None):
        return prove(model, base_dir, img, overrides=overrides,
                     _base_objects=base_objects)

    return aliases


def load(base_dir: Path) -> dict[str, tuple[str, int]]:
    return prepare(base_dir)()


def digest(aliases) -> str:
    return hashlib.sha256(json.dumps(aliases, sort_keys=True).encode()).hexdigest()


def rewrite(payload: bytes, aliases) -> tuple[bytes, tuple]:
    """Retarget aliases without merging sections or changing instruction bytes."""
    if not aliases:
        return payload, ()
    obj = canon.CoffObject(payload)
    names = {}
    for symbol in obj.symbols.values():
        names.setdefault(symbol.name, []).append(symbol.index)
    renames, redirect, rows = {}, {}, []
    for alias, (primary, rva) in sorted(aliases.items()):
        sources = names.get(alias, [])
        if not sources:
            continue
        targets = names.get(primary, [])
        if len(targets) > 1:
            raise ValueError(f'ambiguous folded function primary {primary}')
        if targets:
            target = targets[0]
        else:
            target = sources[0]
            renames[target] = primary
            names[primary] = [target]
        for index in sources:
            redirect[index] = target
        rows.append(canon.CanonicalRow(
            alias, primary, 'function-alias', 'text', 0, 0, 0, 0, 0,
            f'{rva:08x}', 'same-rva-relocation-free-body-equals-retail', ''))
    result = bytearray(canon._rewrite_names(obj, renames))
    for relocation in obj.relocations:
        target = redirect.get(relocation.symbol_index)
        if target is not None:
            struct.pack_into('<I', result, relocation.offset + 4, target)
    data = bytes(result)
    canon._assert_only_canonical_changes(obj, data, renames, (), redirect)
    return data, tuple(rows)
