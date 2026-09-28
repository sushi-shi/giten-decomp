"""Enumerate original disc and PE resources; never synthesize a successful corpus."""
from __future__ import annotations
import hashlib
import json
from pathlib import Path
import struct
from giten.tool.cdfs import Image, walk
from giten.core.pe import Pe


def u16(data, offset=0):
    return struct.unpack_from('<H', data, offset)[0]


def u32(data, offset=0):
    return struct.unpack_from('<I', data, offset)[0]


def decrypt(data):
    key = (len(data) >> 8) ^ (len(data) & 255)
    result = bytearray()
    for byte in data:
        result.append(byte ^ key)
        key = byte
    return bytes(result)


def disc_files(path):
    image = Image(str(path), 2352)
    pvd = image.read(16, 1)
    if pvd[1:6] != b'CD001':
        raise ValueError('not an ISO9660 disc')
    root = pvd[156:190]
    for name, lba, size, directory in walk(image, u32(root, 2), u32(root, 10)):
        if not directory and name.startswith('DDSWIN/'):
            yield name, image.read(lba, (size + 2047) // 2048)[:size]


def pe_resources(path):
    pe = Pe(path)
    section = pe.section('.rsrc')
    base = section['va']
    data = pe.read(base, section['rsize'])
    def entries(offset, names):
        count = u16(data, offset + 12) + u16(data, offset + 14)
        for index in range(count):
            name, child = struct.unpack_from('<II', data, offset + 16 + index * 8)
            if name & 0x80000000:
                off = name & 0x7fffffff
                name = data[off + 2:off + 2 + u16(data, off) * 2].decode('utf-16le')
            if child & 0x80000000:
                yield from entries(child & 0x7fffffff, [*names, name])
            else:
                rva, size = struct.unpack_from('<II', data, child)
                yield [*names, name], pe.read(rva, size)
    yield from entries(0, [])


def jobs(disc, exe, inventory):
    """Yield (kind, argument, bytes, resource label). Inventory records exclusions."""
    retail_hash = hashlib.sha256(Path(exe).read_bytes()).hexdigest()
    found_exe = False
    for name, data in disc_files(disc):
        row = {'name': name, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        inventory.append(row)
        if name == "DDSWIN/DDS.EXE":
            found_exe = True
            if row["sha256"] != retail_hash:
                raise ValueError("disc executable differs from GITEN_RETAIL_EXE")
        if data.startswith(b'BM'):
            row['family'] = 'bitmap-chain'
            offset = frame = 0
            while offset < len(data):
                size = u32(data, offset + 2)
                if data[offset:offset + 2] != b'BM' or size < 54 or offset + size > len(data):
                    raise ValueError(f'{name}: invalid bitmap chain at {offset}')
                bmp = data[offset:offset + size]
                for fmt in (8, 555, 565):
                    # The 8-bit path always reads all 256 palette entries.
                    if fmt == 8 and u32(bmp, 10) < 1078:
                        continue
                    yield 2, fmt, bmp, f'{name}#{frame}/{fmt}'
                yield 7, 0, bmp, f'{name}#{frame}/seek-first'
                offset += size
                frame += 1
            row['frames'] = frame
            # Full-chain seek of the last frame and one past the end.
            yield 7, frame - 1, data, f'{name}/seek-last'
            yield 7, frame, data, f'{name}/seek-end'
        elif data.startswith(b'RIFF') and data[8:12] == b'MIDS':
            row['family'] = 'mids'
            offset = 12
            chunks = {}
            while offset < u32(data, 4) + 8:
                size = u32(data, offset + 4)
                chunks[data[offset:offset + 4]] = data[offset + 8:offset + 8 + size]
                offset += 8 + size + (size & 1)
            fmt = struct.unpack('<III', chunks[b'fmt '])
            row['format'] = fmt
            yield 8, 0, data, name + '/load'
            blocks = chunks[b'data']; offset = 4
            for index in range(u32(blocks)):
                size = u32(blocks, offset + 4)
                payload = blocks[offset + 8:offset + 8 + size]
                if len(payload) != size:
                    raise ValueError(f'{name}: truncated MIDS block')
                if fmt[2] & 1:
                    yield 4, fmt[1], payload, f'{name}#{index}'
                else:
                    # Exercise the alternate event grammar with the original
                    # event words and deltas, removing only the stream-id word.
                    compact = bytearray(); cursor = 0
                    while cursor < len(payload):
                        delta, stream_id, event = struct.unpack_from('<III', payload, cursor)
                        length = ((event & 0xffffff) + 3) & ~3 if event & 0x80000000 else 0
                        compact += struct.pack('<II', delta, event) + payload[cursor + 12:cursor + 12 + length]
                        cursor += 12 + length
                    yield 4, fmt[1], bytes(compact), f'{name}#{index}/removed-stream-ids'
                offset += 8 + size
            if offset != len(blocks):
                raise ValueError(f'{name}: MIDS trailing bytes')
            row['blocks'] = u32(blocks)
            if not fmt[2] & 1:
                row['note'] = 'stream ids already present; ReadBuffers uses memcpy'
        elif name.endswith('.BIN'):
            # Inspect framing without assuming every BIN is an encrypted record.
            records = []
            offset = 0
            while offset + 2 <= len(data):
                size = u16(data, offset)
                if offset + 2 + size > len(data):
                    break
                records.append(data[offset:offset + size + 2]); offset += size + 2
            if offset == len(data):
                row['family'] = 'length-prefixed-records'
                row['records'] = len(records)
                for index, record in enumerate(records):
                    for mode in range(4):
                        yield 1, mode, record, f'{name}#{index}/crypt-{mode}'
                    yield 10, 0, record, f'{name}#{index}/raw'
                    # These are original record bytes. Some consumers read them raw;
                    # the inventory does not infer encryption solely from framing.
                if name.startswith('DDSWIN/M/M') and name[10:11] != 'S' and len(records) == 1:
                    plain = decrypt(records[0][2:])
                    if len(plain) <= 0x2800 and len(plain) >= 6 and 0 < u16(plain, 4) <= 32:
                        yield 5, 0, plain, name + '/area'
                        start = u16(plain, 2)
                        end = plain.index(0, start) + 1
                        yield 11, 1, plain[start:end], name + '/area-name-tokens'
                if name == 'DDSWIN/ET/ET0001.BIN':
                    plain = decrypt(records[0][2:])
                    for index in range(u16(plain)):
                        yield 6, index, plain, f'{name}/item-{index}'
            else:
                row['family'] = 'other-game-data'
        else:
            row['family'] = 'other'
    if not found_exe:
        raise ValueError("disc has no DDSWIN/DDS.EXE")
    # Bounded malformed-event controls; these do not count as original resources.
    for index, (payload, capacity) in enumerate([(b'\0', 12), (b'\0' * 4, 12),
            (struct.pack('<II', 0, 0x80000005) + b'abcde\0\0\0', 20),
            (struct.pack('<II', 0, 0x80000005) + b'abcd', 20),
            (struct.pack('<II', 0, 0x90), 8)]):
        yield 4, capacity, payload, f'control/mids-{index}'
    yield 11, 3, bytes(b for lead in range(256) for trail in range(256)
                       for b in (lead, trail, 0)), 'control/text-all-byte-pairs'
    sound_ids = struct.unpack('<112H', Pe(exe).read(0x6a6c6, 224))
    for names, data in pe_resources(exe):
        name = 'DDS.EXE/' + '/'.join(map(str, names))
        row = {'name': name, 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
        inventory.append(row)
        if names[0] == 2:
            row['family'] = 'bitmap-resource'
            for fmt in (8, 555, 565):
                yield 3, fmt, data, f'{name}/{fmt}'
        elif names[0] == 'WAVE':
            row['family'] = 'pcm-wave'
            matches = [i for i, rid in enumerate(sound_ids) if rid == names[1]]
            row['sound_ids'] = matches
            for sound in matches:
                yield 9, sound, struct.pack('<I', names[1]) + data, f'{name}/sound-{sound}'
            if not matches:
                row['note'] = 'unreferenced resource; supplied at the resource API seam for sound 1'
                yield 9, 1, struct.pack('<I', sound_ids[1]) + data, f'{name}/unreferenced-as-sound-1'
        else:
            row['family'] = 'other-pe-resource'


def prepare(disc: Path, exe: Path, out: Path):
    out.mkdir(parents=True, exist_ok=True)
    inventory, cases = [], []
    with (out / 'jobs.bin').open('wb') as file:
        file.write(struct.pack('<II', 0x424f4a47, 0))
        for kind, arg, data, name in jobs(disc, exe, inventory):
            cases.append({'kind': kind, 'arg': arg, 'name': name, 'size': len(data)})
            file.write(struct.pack('<III', kind, arg, len(data))); file.write(data)
        file.seek(4); file.write(struct.pack('<I', len(cases)))
    (out / 'cases.json').write_text(json.dumps(cases, indent=2) + '\n')
    (out / 'inventory.json').write_text(json.dumps(inventory, indent=2) + '\n')
    print(f'{len(inventory)} resources, {len(cases)} execution jobs', flush=True)


if __name__ == '__main__':
    import sys
    prepare(*map(Path, sys.argv[1:]))
