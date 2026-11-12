#!/usr/bin/env python3
"""Inventory exact renderer AS writes and their provider-vtable routes.

This is an instruction/callsite audit, not proof of exclusive scene ownership.
Requires pefile and capstone. Does not modify a native DLL.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
WRITES = {
    0xa6c8c: ('buildBottomLevelAccelerationStructure', 0xa6c00),
    0x15101b: ('buildBottomLevelAccelerationStructure', 0x150fa0),
    0xa8bca: ('buildTopLevelAccelerationStructure', 0xa89c0),
    0x151060: ('compactAccelerationStructure', 0x151040),
}
PROVIDERS = {
    'deformed_geometry': {'vtable': 0x627e38, 'update': 0xa67b0, 'build': 0xa6c00, 'finalize': 0x10d40},
    'compactable_geometry': {'vtable': 0x6332f0, 'update': 0x10d40, 'build': 0x150fa0, 'finalize': 0x151040},
}

def require(ok, message):
    if not ok:
        raise ValueError(message)

def audit(data):
    p = pefile.PE(data=data)
    base = p.OPTIONAL_HEADER.ImageBase
    imports = {i.address-base: i.name.decode() for d in p.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name}
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    def instructions(begin, end):
        return list(dis.disasm(p.get_data(begin, end-begin), begin))
    def call(site, begin, name):
        i = next((i for i in instructions(begin, site+7) if i.address == site), None)
        require(i and i.mnemonic in ('call', 'jmp') and i.size in (6, 7), f'call boundary {site:x}')
        op = i.operands[0]
        require(op.type == capstone.CS_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP, f'call mode {site:x}')
        imported = imports.get(i.address+i.size+op.mem.disp, '')
        require(imported.startswith('?'+name+'@'), f'call import {site:x}')
        return {'site': hex(site), 'body': hex(begin), 'import': imported, 'bytes': i.bytes.hex()}
    writes = [call(site, begin, name) for site, (name, begin) in WRITES.items()]
    # Exhaustive byte candidates in executable sections, then exact instruction
    # boundary validation above. This does NOT cover register-indirect calls or
    # other modules. A raw byte scan alone cannot establish a callsite.
    slots = {slot for slot, name in imports.items() if any(name.startswith('?'+n+'@') for n, _ in WRITES.values())}
    found = set()
    for section in p.sections:
        if not section.Characteristics & 0x20000000:
            continue
        code = section.get_data()
        for opcode in (b'\xff\x15', b'\xff\x25'):
            pos = 0
            while True:
                pos = code.find(opcode, pos)
                if pos < 0:
                    break
                if pos+6 <= len(code):
                    site = section.VirtualAddress+pos
                    slot = site+6+struct.unpack_from('<i', code, pos+2)[0]
                    if slot in slots:
                        found.add(site-1 if pos and code[pos-1] == 0x48 else site)
                pos += 1
    require(found == set(WRITES), f'changed writer inventory: {sorted(found)}')
    for name, spec in PROVIDERS.items():
        for offset, field in ((0x28, 'update'), (0x30, 'build'), (0x38, 'finalize')):
            target = struct.unpack('<Q', p.get_data(spec['vtable']+offset, 8))[0]-base
            require(target == spec[field], f'{name} {field} vtable target')
    # Instruction windows start at proven straight-line block boundaries.
    loops = ((0xa8a63, 0xa8a6c, 0x28), (0xa8aae, 0xa8ab4, 0x30),
             (0xa8ad3, 0xa8ad9, 0x30), (0xa8b36, 0xa8b3c, 0x38))
    for begin, site, offset in loops:
        i = next((i for i in instructions(begin, site+3) if i.address == site), None)
        require(i and i.mnemonic == 'call' and i.operands[0].type == capstone.CS_OP_MEM
                and i.operands[0].mem.base == capstone.x86.X86_REG_RAX
                and i.operands[0].mem.disp == offset, f'provider loop {site:x}')
    update_dispatch = call(0xa6ba6, 0xa67b0, 'dispatch')
    return {'renderer_sha256': EXPECTED, 'writes': writes,
            'provider_tables': {n: {k: hex(v) for k, v in d.items()} for n, d in PROVIDERS.items()},
            'worker_virtual_calls': [{'site': hex(s), 'slot': hex(o)} for _, s, o in loops],
            'deformed_geometry_compute_dispatch': update_dispatch,
            'scene_lease_proven': False,
            'scope': 'Pinned renderer IAT-relative AS writes and listed provider tables/worker loops only.',
            'remaining': ['All callers of provider methods and pool reuse, including deferred jobs.',
                          'Writers of source geometry/material resources outside these AS routines.',
                          'Live retained resource acquisition and transaction integration.']}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('renderer', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    data = args.renderer.read_bytes()
    require(hashlib.sha256(data).hexdigest() == EXPECTED, 'unsupported renderer hash')
    report = audit(data)
    pe = pefile.PE(data=data)
    for site in WRITES:
        modified = bytearray(data)
        modified[pe.get_offset_from_rva(site)] ^= 1
        try:
            audit(modified)
        except ValueError:
            continue
        raise ValueError(f'modified writer accepted {site:x}')
    report['mutated_writer_rejections'] = len(WRITES)
    if args.output:
        args.output.write_text(json.dumps(report, indent=2)+'\n')
    print('PASS four native AS write sites, two provider families and worker routes; no scene lease inferred')

if __name__ == '__main__':
    main()
