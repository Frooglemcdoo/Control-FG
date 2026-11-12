#!/usr/bin/env python3
"""Inventory direct NativeBuffer mutation imports at verified instruction boundaries.

This is a bounded static inventory, not writer-completeness or synchronization proof.
"""
import argparse
import hashlib
import json
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'


def audit(path):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise ValueError('Unsupported renderer')
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    imports = {i.address-base: i.name.decode() for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name}
    imports = {a: n for a, n in imports.items() if
               any(n.startswith('?' + f + '@NativeBufferUtil') for f in ('lock', 'unlock', 'copy', 'move'))}
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    sites = {}
    for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
        begin, end = entry.struct.BeginAddress, entry.struct.EndAddress
        for i in dis.disasm(pe.get_data(begin, end-begin), begin):
            if i.mnemonic not in ('call', 'jmp') or len(i.operands) != 1:
                continue
            op = i.operands[0]
            if op.type != capstone.CS_OP_MEM or op.mem.base != capstone.x86.X86_REG_RIP:
                continue
            slot = i.address+i.size+op.mem.disp
            if slot in imports:
                sites[i.address] = {'site': hex(i.address), 'function_begin': hex(begin),
                                    'function_end': hex(end), 'import': imports[slot], 'bytes': i.bytes.hex()}
    return {'renderer_sha256': EXPECTED, 'direct_import_sites': [sites[a] for a in sorted(sites)],
            'limits': ['Scans only decoded instructions in PE runtime-function ranges.',
                       'Register-indirect calls, functions without unwind entries, other modules and GPU shader writes are not covered.',
                       'A site does not identify its target allocation or prove execution between GI stages.',
                       'Function location outside a preparation body is not proof it executes outside the preparation gate.',
                       'Resource identity alone cannot authorize persistent byte reuse.'],
            'complete_writer_coverage': False, 'persistent_reuse_authorized': False}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('renderer', type=Path)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    result = audit(a.renderer)
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Verified {len(result['direct_import_sites'])} direct mutation-import sites; complete coverage remains unproven")
