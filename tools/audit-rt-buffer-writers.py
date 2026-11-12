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
# Names are anchored in existing feature hooks and the native scene-window audit,
# not inferred from address proximity. The scan independently discovers callsites.
CONSUMERS = {
    0xa557e: ('glass_reflections', 'src/pt_glass_runtime.h'),
    0x129652: ('opaque_reflections', 'src/opaque_reflection_runtime.h'),
    0x12c5eb: ('indirect_lighting', 'tools/audit-gi-native-route.py'),
    0x12d342: ('sun_shadow', 'tools/audit-gi-native-scene-window.py'),
    0x12d8ba: ('ambient_occlusion', 'tools/audit-gi-native-scene-window.py'),
    0x23f9c4: ('spot_shadow', 'src/rt_spot_runtime.h'),
}


def audit(path):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != EXPECTED:
        raise ValueError('Unsupported renderer')
    pe = pefile.PE(data=data)
    base = pe.OPTIONAL_HEADER.ImageBase
    imports = {i.address-base: i.name.decode() for d in pe.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name}
    mutations = {a: n for a, n in imports.items() if
               any(n.startswith('?' + f + '@NativeBufferUtil') for f in ('lock', 'unlock', 'copy', 'move'))}
    traversal = {a: n for a, n in imports.items() if n.startswith('?raytrace@DeviceUtilRaytracing@')}
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    sites = {}
    consumers = {}
    for entry in pe.DIRECTORY_ENTRY_EXCEPTION:
        begin, end = entry.struct.BeginAddress, entry.struct.EndAddress
        for i in dis.disasm(pe.get_data(begin, end-begin), begin):
            if i.mnemonic not in ('call', 'jmp') or len(i.operands) != 1:
                continue
            op = i.operands[0]
            if op.type != capstone.CS_OP_MEM or op.mem.base != capstone.x86.X86_REG_RIP:
                continue
            slot = i.address+i.size+op.mem.disp
            if slot in mutations:
                sites[i.address] = {'site': hex(i.address), 'function_begin': hex(begin),
                                    'function_end': hex(end), 'import': mutations[slot], 'bytes': i.bytes.hex()}
            if slot in traversal:
                label, source = CONSUMERS.get(i.address, ('unclassified', None))
                consumers[i.address] = {'site': hex(i.address), 'function_begin': hex(begin),
                                       'function_end': hex(end), 'import': traversal[slot],
                                       'bytes': i.bytes.hex(), 'consumer': label,
                                       'classification_source': source}
    if set(consumers) != set(CONSUMERS):
        raise ValueError('Native traversal inventory differs from reviewed consumer routes')
    return {'renderer_sha256': EXPECTED, 'direct_import_sites': [sites[a] for a in sorted(sites)],
            'native_traversal_sites': [consumers[a] for a in sorted(consumers)],
            'consumer_routing_requirement': 'Account for all six listed native traversal sites before retiring their shared scene; a passing GI-only route is insufficient.',
            'limits': ['Scans only decoded instructions in PE runtime-function ranges.',
                       'Register-indirect calls, functions without unwind entries, other modules and GPU shader writes are not covered.',
                       'A site does not identify its target allocation or prove execution between GI stages.',
                       'Function location outside a preparation body is not proof it executes outside the preparation gate.',
                       'Resource identity alone cannot authorize persistent byte reuse.',
                       'Traversal classification uses existing source audits; static callsites do not prove runtime execution or exhaustive indirect consumers.'],
            'complete_writer_coverage': False, 'persistent_reuse_authorized': False}


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('renderer', type=Path)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    result = audit(a.renderer)
    a.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Verified {len(result['direct_import_sites'])} direct mutation-import sites; complete coverage remains unproven")
