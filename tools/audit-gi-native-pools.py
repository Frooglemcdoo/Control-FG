#!/usr/bin/env python3
"""Build-locked static/skinned RT pool evidence, not a mutable-scene lease."""
import argparse, hashlib, json
from pathlib import Path
import capstone
import pefile
EXPECTED='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
FREE=[0x151720,0x1518a0,0x151a20,0xa7220,0xa7400,0xa7580]
ALLOC=[0x1504c0,0x150e21,0xa6380]
def audit(path):
    raw=Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest()!=EXPECTED:
        raise ValueError('Unsupported renderer; do not reuse offsets')
    pe=pefile.PE(data=raw)
    dis=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);dis.detail=True
    imports={i.address-pe.OPTIONAL_HEADER.ImageBase:(i.name or b'').decode()
             for g in pe.DIRECTORY_ENTRY_IMPORT for i in g.imports}
    functions=[]
    for lo in FREE+ALLOC:
        e=next(e.struct for e in pe.DIRECTORY_ENTRY_EXCEPTION if e.struct.BeginAddress==lo)
        code=pe.get_data(lo,e.EndAddress-lo);seq=list(dis.disasm(code,lo))
        if seq[-1].address+seq[-1].size!=e.EndAddress:
            raise ValueError(f'Incomplete decode at {lo:#x}')
        calls=[]
        for ins in seq:
            if ins.mnemonic!='call': continue
            op=ins.operands[0]
            target=ins.op_str
            if op.type==capstone.x86.X86_OP_IMM: target=hex(op.imm)
            elif op.type==capstone.x86.X86_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP:
                target=imports.get(ins.address+ins.size+op.mem.disp,ins.op_str)
            calls.append({'rva':hex(ins.address),'target':target})
        if lo in FREE:
            # Every recovered free path decrements the chunk reference count,
            # and its immediate cleanup calls native deferred resource release.
            if not any(i.mnemonic=='sub' and i.op_str=='dword ptr [rdi + 0x1c], 1' for i in seq):
                raise ValueError('Chunk reference-count decrement changed')
            if not any('?releaseResources@NativeBuffer@d3d@@' in c['target'] for c in calls):
                raise ValueError('Native release path missing')
        functions.append({'start':hex(lo),'end':hex(e.EndAddress),
                          'sha256':hashlib.sha256(code).hexdigest(),'calls':calls,
                          'code_hex':code.hex()})
    # Exact observed cursor/refcount sequences in static and skinned allocation.
    windows=[(0x150973,0x150982),(0x150a00,0x150a0f),
             (0x150f26,0x150f35),(0xa64b7,0xa64c9)]
    return {'renderer_sha256':EXPECTED,'functions':functions,
            'cursor_increment_windows':[{'start':hex(a),'end':hex(b),
                                          'bytes':pe.get_data(a,b-a).hex()} for a,b in windows],
            'observed_layout':{'native_buffer':'chunk+8','gpu_address':'chunk+0x10',
                               'allocation_cursor':'chunk+0x18','reference_count':'chunk+0x1c'},
            'interpretation':'The listed allocation paths advance a chunk cursor. Listed free paths decrement chunk references and release the whole native buffer at zero, synchronously or through a job. They do not insert a freed individual range into a free list.',
            'scene_lease_proven':False,
            'limitations':['Only these recovered provider/pool paths, not every renderer allocation',
                           'No guarantee of immutable vertex/material/AS contents from allocation lifetime',
                           'Native retirement still requires the consumer submission inside its frame boundary']}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('renderer');p.add_argument('--output',required=True)
    a=p.parse_args();Path(a.output).write_text(json.dumps(audit(a.renderer),indent=2)+'\n')
    print('PASS six native chunk-release paths and three allocation functions; scene lease NOT certified')
