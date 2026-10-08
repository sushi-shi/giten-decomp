"""Attribute pinned global initialization graphs to their emitted COFF helpers.

The global DATA receiver, typed constructor/destructor, CRT registration and
atexit edges establish identity. Instruction bytes and body hashes do not:
after attribution, the usual comparison must expose an incorrect body.
Local-static callbacks remain the responsibility of static_dtors.
"""

from __future__ import annotations

import re
import struct
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from giten.manifest import units as manifest_units
from giten.delink.coffx import Obj
from giten.delink.static_dtors import argument_site
from giten.sema.gaps import xc_bounds
from giten.tool.objdump import disassemble

_HELPER = re.compile(r"^_?\$E[0-9]+$")
_INSTRUCTION = re.compile(r"^\s*([0-9a-f]+):\s*((?:[0-9a-f]{2}\s+)+)\s*\t")
_EXECUTE = 0x20000000


@dataclass
class Node:
    code: bytes
    refs: list[tuple[int, int, object, int]]
    root: bool = False


def _owner_name(symbol: str, owner: str) -> bool:
    return symbol in (owner, "_" + owner) or symbol.startswith("?" + owner + "@@")


def _instructions(code: bytes) -> dict[int, bytes]:
    return {int(m[1], 16): bytes.fromhex(m[2])
            for line in disassemble(code).splitlines()
            if (m := _INSTRUCTION.match(line))}


def _body(obj: Obj, name: str) -> tuple[bytes, dict]:
    entries = [(value, section) for index, value, section in obj.iter_symbols()
               if section > 0 and obj.sym_name(index) == name]
    if len(entries) != 1:
        raise ValueError(f"generated helper {name!r} has {len(entries)} definitions")
    start, section = entries[0]
    if not obj.section_table[section - 1]["characteristics"] & _EXECUTE:
        raise ValueError(f"generated helper {name!r} is not executable")
    following = [off for off, _, _ in obj.section_members(section) if off > start]
    payload = obj.section_payload(section)
    end = min(following, default=len(payload))
    code = payload[start:end].rstrip(b"\x90\xcc")
    refs = {off - start: ref for off, ref in obj.typed_relocations(section).items()
            if start <= off < start + len(code)}
    return code, refs


def _source_nodes(obj: Obj) -> dict[str, Node]:
    names = {obj.sym_name(i) for i, _, section in obj.iter_symbols()
             if section > 0 and _HELPER.fullmatch(obj.sym_name(i))}
    roots = set()
    for section in obj.section_table:
        if section["name"] != ".CRT$XCU":
            continue
        for off, (name, typ) in obj.typed_relocations(section["index"]).items():
            if (typ != 6 or off % 4 or off < 0
                    or off + 4 > len(obj.section_payload(section["index"]))):
                raise ValueError("unproved source CRT initializer pointer")
            if struct.unpack_from("<I", obj.section_payload(section["index"]), off)[0]:
                raise ValueError("source CRT initializer has a nonzero addend")
            if name not in names:
                raise ValueError(f"source CRT initializer {name!r} is not a defined helper")
            if name in roots:
                raise ValueError(f"duplicate source CRT initializer {name!r}")
            roots.add(name)
    nodes = {}
    for name in names:
        code, refs = _body(obj, name)
        if any(typ not in (6, 0x14) or off < 0 or off + 4 > len(code)
               for off, (_, typ) in refs.items()):
            raise ValueError(f"generated helper {name!r} has an invalid relocation operand")
        nodes[name] = Node(code, [(off, typ, target, struct.unpack_from("<I", code, off)[0])
                                 for off, (target, typ) in sorted(refs.items())], name in roots)
    return nodes


def _retail_nodes(pins, image, roots: set[int]) -> dict[int, Node]:
    nodes = {}
    for pin in pins:
        code = image.pe.read(pin.rva, pin.size)
        if code is None or len(code) != pin.size:
            raise ValueError(f"unreadable pinned body at 0x{pin.rva:x}")
        refs = []
        instructions = _instructions(code)
        for off, raw in instructions.items():
            if len(raw) == 5 and raw[0] in (0xE8, 0xE9):
                target = pin.rva + off + 5 + struct.unpack_from("<i", raw, 1)[0]
                refs.append((off + 1, 0x14, target, 0))
        for site in image.reloc_sites:
            if pin.rva <= site < pin.rva + pin.size:
                off = site - pin.rva
                if off + 4 > len(code):
                    raise ValueError(f"pinned body at 0x{pin.rva:x} has a truncated operand")
                target = struct.unpack_from("<I", code, off)[0] - image.image_base
                refs.append((off, 6, target, 0))
        nodes[pin.rva] = Node(code, sorted(refs), pin.rva in roots)
    return nodes


def _labels(nodes, owner, owner_type: str, functions, data):
    """Relocation-backed roles and helper edges; no instruction equality gate."""
    labels, edges = {}, {}
    for key, node in nodes.items():
        instructions = _instructions(node.code)
        events, links = [], []
        named_calls = []
        receiver = False
        for off, typ, target, addend in node.refs:
            if addend:
                raise ValueError("initializer relocation has a nonzero addend")
            if typ == 0x14:
                raw = instructions.get(off - 1)
                if not raw or len(raw) != 5 or raw[0] not in (0xE8, 0xE9):
                    raise ValueError("unproved generated direct transfer")
                role = "call" if raw[0] == 0xE8 else "jump"
            elif typ == 6:
                raw = instructions.get(off - 1)
                if not raw or len(raw) != 5 or raw[0] not in (0xB9, 0x68):
                    raise ValueError("unproved generated address operand")
                role = "receiver" if raw[0] == 0xB9 else "callback"
            else:
                raise ValueError(f"unsupported initializer relocation type {typ}")
            if target in nodes:
                if role == "receiver":
                    raise ValueError("generated helper used as an object receiver")
                if role == "callback":
                    calls = [site for site, t, callee, _ in node.refs
                             if t == 0x14 and "_atexit" in functions.get(callee, ())]
                    if len(calls) != 1 or argument_site(node.code, calls[0] - 1) != off:
                        raise ValueError("unproved generated atexit callback")
                links.append((len(events), role, target))
                events.append((role, "helper"))
            elif target == owner and target in data:
                if role != "receiver":
                    raise ValueError("global owner is not the constructor receiver")
                receiver = True
                events.append((role, "owner"))
            elif target in functions:
                if typ != 0x14:
                    raise ValueError("unproved external initializer function pointer")
                names = functions[target]
                typed = {n for n in names if n.startswith(("??0" + owner_type + "@@",
                                                          "??1" + owner_type + "@@"))}
                if typed:
                    # The typed alias proves this receiver's constructor identity,
                    # even when the retail linker folded several empty constructors.
                    kinds = {"ctor" if n.startswith("??0") else "dtor" for n in typed}
                    if len(kinds) != 1:
                        raise ValueError("ambiguous typed initializer callee")
                    callee = next(iter(kinds))
                elif "_atexit" in names:
                    callee = "atexit"
                else:
                    raise ValueError("initializer calls an unproved typed callee")
                named_calls.append(callee)
                events.append((role, callee))
            else:
                raise ValueError("initializer references an unproved owner or helper")
        if receiver != any(c in ("ctor", "dtor") for c in named_calls):
            raise ValueError("initializer typed callee lacks its owner receiver")
        if receiver and (len(named_calls) != 1 or any(link[1] == "callback" for link in links)):
            raise ValueError("unproved mixed constructor/registration operation")
        labels[key] = (node.root, tuple(events))
        edges[key] = links
    return labels, edges


def _colors(labels, edges, rounds):
    colors = labels
    for _ in range(rounds):
        colors = {key: (labels[key], tuple((off, role, colors[target])
                                          for off, role, target in edges[key]))
                  for key in labels}
    return colors


def _pair(source, target, source_labels, source_edges, target_labels, target_edges):
    rounds = len(source) + len(target)
    src = _colors(source_labels, source_edges, rounds)
    dst = _colors(target_labels, target_edges, rounds)
    source_by_color, target_by_color = defaultdict(list), defaultdict(list)
    for key, color in src.items():
        source_by_color[color].append(key)
    for key, color in dst.items():
        target_by_color[color].append(key)
    if source_by_color.keys() != target_by_color.keys():
        raise ValueError("source/retail initializer registration graphs differ")
    if any(len(v) != 1 for v in source_by_color.values()) or any(
            len(v) != 1 for v in target_by_color.values()):
        raise ValueError("ambiguous initializer owner/role graph")
    mapping = {target_by_color[color][0]: names[0]
               for color, names in source_by_color.items()}
    for retail_key, source_key in mapping.items():
        translated = [(off, role, mapping[child]) for off, role, child in target_edges[retail_key]]
        if translated != source_edges[source_key]:
            raise ValueError("initializer helper edge correspondence is incomplete")
    return mapping


def provision(model, names_map: dict, base_dir: Path, image) -> dict[int, tuple[str, str, int]]:
    """Return current emitted names; any unprovisioned global pin is fatal."""
    pins = [b for b in model.functions if b.channel == "src_dyninit" and b.rva not in names_map]
    by_owner = defaultdict(list)
    for pin in pins:
        owners = {c.name for c in pin.aliases if c.channel == "src_dyninit"}
        if len(owners) != 1:
            raise ValueError(f"global initializer pin 0x{pin.rva:x}: ambiguous owner")
        by_owner[pin.unit, next(iter(owners))].append(pin)
    lo, hi = xc_bounds(image.pe, model)
    table = image.pe.read(lo, hi - lo)
    entries = [struct.unpack_from("<I", table, off)[0] - image.image_base
               for off in range(0, len(table), 4) if struct.unpack_from("<I", table, off)[0]]
    if len(entries) != len(set(entries)):
        raise ValueError("global initializers: duplicate retail CRT registration")
    roots = set(entries)
    function_names = {b.rva: {b.name, *(c.name for c in b.aliases)}
                      for b in model.functions if b.name}
    data_names = {b.rva: {b.name, *(c.name for c in b.aliases)}
                  for b in model.data if b.name}
    source_functions = {}
    for rva, names in function_names.items():
        for name in names:
            if name in source_functions and source_functions[name] != names:
                raise ValueError(f"ambiguous initializer callee {name!r}")
            source_functions[name] = names
    result, used_names, objects = {}, set(), {}
    for (unit, owner_name), owner_pins in by_owner.items():
        context = f"global initializer {unit}/{owner_name} pins " + ",".join(
            f"0x{b.rva:x}" for b in owner_pins)
        try:
            owners = [b for b in model.data if b.unit == unit and b.channel
                      and any(_owner_name(n, owner_name) for n in data_names.get(b.rva, ()))]
            if len(owners) != 1:
                raise ValueError("missing or ambiguous global DATA identity")
            owner = owners[0]
            type_match = re.search(r"@@3[UV]([^@]+)@@", owner.name)
            if type_match is None:
                raise ValueError("global DATA has no typed C++ owner identity")
            owner_type = type_match[1]
            if unit not in objects:
                objects[unit] = _source_nodes(Obj(base_dir / f"{unit}.obj"))
            all_source = objects[unit]
            source_owner_names = data_names[owner.rva]
            selected = {key for key, node in all_source.items()
                        if any(target in source_owner_names for _, _, target, _ in node.refs)}
            # Connected helper closure includes the CRT entry and atexit wrapper.
            while True:
                expanded = selected | {key for key, node in all_source.items()
                                       if any(target in selected for _, _, target, _ in node.refs)}
                expanded |= {target for key in selected for _, _, target, _ in all_source[key].refs
                             if target in all_source}
                if expanded == selected:
                    break
                selected = expanded
            source = {key: all_source[key] for key in selected}
            target = _retail_nodes(owner_pins, image, roots)
            if len(source) != len(target) or not source:
                raise ValueError("source/retail global helper counts differ")
            source_data = {name for names in data_names.values() for name in names}
            # Normalize owner aliases to the one resolved source data symbol.
            source_owner = next((n for n in source_owner_names if any(
                t == n for node in source.values() for _, _, t, _ in node.refs)), None)
            if source_owner is None or sum(any(t == n for node in source.values()
                for _, _, t, _ in node.refs) for n in source_owner_names) != 1:
                raise ValueError("source global receiver identity is ambiguous")
            sl, se = _labels(source, source_owner, owner_type, source_functions, source_data)
            tl, te = _labels(target, owner.rva, owner_type, function_names, set(data_names))
            mapping = _pair(source, target, sl, se, tl, te)
            if not any(node.root for node in source.values()) or not any(
                    node.root for node in target.values()):
                raise ValueError("global graph has no proved CRT initializer root")
            for pin in owner_pins:
                name = mapping[pin.rva]
                if (unit, name) in used_names:
                    raise ValueError("source helper attributed to multiple globals")
                used_names.add((unit, name))
                result[pin.rva] = (name, unit, pin.size)
        except (ValueError, OSError) as error:
            raise ValueError(f"{context}: {error}") from error
    missing_roots = roots - names_map.keys() - result.keys()
    if missing_roots:
        raise ValueError("global initializers: unprovisioned retail CRT roots " +
                         ", ".join(f"0x{rva:x}" for rva in sorted(missing_roots)))
    # Census all emitted registration sections, including units whose pins were
    # accidentally omitted. An absent whole owner graph must not escape checks.
    attributed = {(unit, name) for name, unit, _ in (*names_map.values(), *result.values())}
    for unit in sorted({row["unit"] for row in manifest_units()}):
        path = base_dir / f"{unit}.obj"
        nodes = objects.get(unit)
        if nodes is None:
            nodes = _source_nodes(Obj(path))
        missing = [name for name, node in nodes.items()
                   if node.root and (unit, name) not in attributed]
        if missing:
            raise ValueError(f"global initializers: unprovisioned source CRT roots in {unit}: " +
                             ", ".join(sorted(missing)))
    return result
