"""Conservative CFG provenance for imports cached in nonvolatile x86 registers."""
from __future__ import annotations

from collections import deque
import re

_DIR32 = 0x0006
_SAVED = {'ebx', 'esi', 'edi', 'ebp'}
_ALIASES = {'bx': 'ebx', 'bl': 'ebx', 'bh': 'ebx', 'si': 'esi',
            'di': 'edi', 'bp': 'ebp'}
_JUMP = re.compile(r'0x([0-9a-f]+)\b')


def cached_import_calls(rel: dict, asm: str) -> list[tuple[str, int]]:
    """Resolve register CALLs only when every reachable path proves one import.

    Calls preserve EBX/ESI/EDI/EBP under the supported Win32 ABIs. Unknown
    instructions discard provenance; unknown branch destinations reject the
    function. This deliberately does not guess virtual or volatile-register calls.
    """
    insns = {}
    for line in asm.splitlines():
        if ':\t' not in line:
            continue
        address, rest = line.split(':\t', 1)
        parts = rest.split('\t')
        if len(parts) < 2:
            continue
        try:
            offset = int(address.strip(), 16)
        except ValueError:
            continue
        text = parts[-1].strip().lower()
        if not text:
            continue
        mnemonic, _, operands = text.partition(' ')
        insns[offset] = (mnemonic, operands.strip())
    if not insns:
        return []
    order = sorted(insns)
    successors = {}
    for i, offset in enumerate(order):
        mnemonic, operands = insns[offset]
        after = [order[i + 1]] if i + 1 < len(order) else []
        if mnemonic.startswith('j') or mnemonic.startswith('loop'):
            target = _JUMP.fullmatch(operands)
            if target is None:
                return []
            destination = int(target.group(1), 16)
            # External direct jumps are tail calls, not another local path.
            edges = [destination] if destination in insns else []
            successors[offset] = edges + (after if mnemonic != 'jmp' else [])
        elif mnemonic.startswith('ret'):
            successors[offset] = []
        else:
            successors[offset] = after

    def transfer(offset, incoming):
        state = dict(incoming)
        mnemonic, operands = insns[offset]
        args = [a.strip() for a in operands.split(',')]
        destination = args[0] if args else ''
        register = _ALIASES.get(destination, destination)
        if mnemonic == 'mov' and destination in _SAVED:
            source = args[1] if len(args) == 2 else ''
            value = incoming.get(source)
            claim = rel.get(offset + 2)
            if (source == 'dword ptr ds:0x0'
                    and claim and claim[0].startswith('__imp_') and claim[1] == _DIR32):
                value = claim
            state.pop(destination, None)
            if value is not None:
                state[destination] = value
        elif mnemonic in {'cmp', 'test', 'push', 'nop', 'call', 'ret', 'retn'} \
                or mnemonic.startswith('j') or mnemonic.startswith('loop') \
                or mnemonic.startswith('f'):
            pass
        elif mnemonic in {'mov', 'movzx', 'movsx', 'lea', 'pop', 'xor', 'or',
                          'and', 'add', 'sub', 'adc', 'sbb', 'inc', 'dec', 'neg',
                          'not', 'shl', 'shr', 'sar', 'sal', 'rol', 'ror', 'imul'}:
            state.pop(register, None)
        elif mnemonic == 'xchg':
            for arg in args:
                state.pop(_ALIASES.get(arg, arg), None)
        else:
            state.clear()
        return state

    incoming = {order[0]: {}}
    outgoing = {}
    pending = deque([order[0]])
    while pending:
        offset = pending.popleft()
        result = transfer(offset, incoming[offset])
        if outgoing.get(offset) == result:
            continue
        outgoing[offset] = result
        for successor in successors[offset]:
            previous = incoming.get(successor)
            merged = result if previous is None else {
                reg: value for reg, value in previous.items()
                if result.get(reg) == value
            }
            if previous is None or previous != merged:
                incoming[successor] = dict(merged)
                pending.append(successor)
    return [incoming[offset][operands]
            for offset, (mnemonic, operands) in insns.items()
            if mnemonic == 'call' and operands in incoming.get(offset, {})]
