#!/usr/bin/env python3
"""Enumerate exact-build material/AS entry references, not an exclusivity proof.

Instruction boundaries come from x64 runtime-function ranges. Computed indirect
calls and other modules remain outside this inventory.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
TARGETS = {0xc7bc0: 'material_upload', 0xa89c0: 'scene_worker',
           0xa67b0: 'deformed_update', 0xa6c00: 'deformed_build',
           0x150fa0: 'static_build', 0x151040: 'static_finalize'}

def audit(path):
    raw = Path(path).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED:
        raise ValueError('Unsupported renderer hash')
    pe = pefile.PE(data=raw)
    base = pe.OPTIONAL_HEADER.ImageBase
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    refs = {target: {'name': name, 'direct': [], 'rip_address': [], 'data_pointers': []}
            for target, name in TARGETS.items()}
    seen = set()
    ranges = [(entry.struct.BeginAddress, entry.struct.EndAddress) for entry in pe.DIRECTORY_ENTRY_EXCEPTION]
    # This leaf thunk has no unwind record; its start is independently fixed by
    # the scene job vtable and the existing scene-window audit.
    assert pe.get_data(0xa8f70, 9) == bytes.fromhex("4883c118e947faffff")
    ranges.append((0xa8f70, 0xa8f79))
    for begin, end in ranges:
        code = pe.get_data(begin, end-begin)
        # Only decode functions containing a candidate near branch or RIP-relative
        # address displacement. Whole-function decoding establishes boundaries.
        candidates = set()
        for offset in range(max(0, len(code)-3)):
            displacement = struct.unpack_from('<i', code, offset)[0]
            if begin+offset+4+displacement in TARGETS:
                candidates.add(begin+offset)
        if not candidates:
            continue
        for ins in dis.disasm(code, begin):
            if ins.address in seen:
                continue
            seen.add(ins.address)
            if ins.mnemonic in ('call', 'jmp') and ins.operands[0].type == capstone.CS_OP_IMM:
                target = ins.operands[0].imm
                if target in refs:
                    refs[target]['direct'].append({'site': hex(ins.address), 'body': hex(begin), 'operation': ins.mnemonic})
            for operand in ins.operands:
                if operand.type == capstone.CS_OP_MEM and operand.mem.base == capstone.x86.X86_REG_RIP:
                    target = ins.address+ins.size+operand.mem.disp
                    if target in refs:
                        refs[target]['rip_address'].append({'site': hex(ins.address), 'body': hex(begin), 'operation': ins.mnemonic})
    for section in pe.sections:
        if section.Characteristics & 0x20000000:
            continue
        content = section.get_data()
        for target in refs:
            pattern = struct.pack('<Q', base+target)
            offset = content.find(pattern)
            while offset >= 0:
                refs[target]['data_pointers'].append(hex(section.VirtualAddress+offset))
                offset = content.find(pattern, offset+1)
    exports = [hex(e.address) for e in getattr(pe, 'DIRECTORY_ENTRY_EXPORT', type('Empty', (), {'symbols': []})).symbols if e.address in refs]
    assert refs[0xc7bc0]['direct'] == [{'site': '0x12491d', 'body': '0x123d60', 'operation': 'call'}]
    assert refs[0xa89c0]['direct'] == [{'site': '0xa8f74', 'body': '0xa8f70', 'operation': 'jmp'}]
    assert not exports
    return {'sha256': digest, 'entries': {hex(k): v for k,v in refs.items()},
            'exported_entries': exports,
            'limitations': ['Computed indirect calls, other leaf functions without unwind records, and writers in other modules are not excluded.',
                           'A native frame retirement epoch proves lifetime, not immutable texture contents.',
                           'Do not enable replacement solely from this inventory.']}

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('renderer')
    parser.add_argument('--out', required=True)
    args = parser.parse_args()
    result = audit(args.renderer)
    Path(args.out).write_text(json.dumps(result, indent=2)+'\n')
    print('PASS exact-build scene entrypoint inventory')
