#!/usr/bin/env python3
"""Build-locked native scene-window evidence; never certifies an ownership lease."""
import argparse, hashlib, json
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'

def audit(path):
    raw = Path(path).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED:
        raise ValueError('Unsupported renderer identity; do not reuse offsets')
    pe = pefile.PE(data=raw)
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    imports = {item.address - pe.OPTIONAL_HEADER.ImageBase:
               (item.name or b'').decode('ascii')
               for group in pe.DIRECTORY_ENTRY_IMPORT for item in group.imports}
    lo, hi = 0x128D20, 0x12DA38
    assert any(e.struct.BeginAddress == lo and e.struct.EndAddress == hi
               for e in pe.DIRECTORY_ENTRY_EXCEPTION), 'Unexpected function extent'
    instructions = list(dis.disasm(pe.get_data(lo, hi-lo), lo))
    assert instructions[-1].address + instructions[-1].size == hi
    calls = {}
    for ins in instructions:
        if ins.mnemonic != 'call':
            continue
        op = ins.operands[0]
        if op.type == capstone.x86.X86_OP_IMM:
            calls[ins.address] = {'target_rva': hex(op.imm)}
        elif op.type == capstone.x86.X86_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP:
            slot = ins.address + ins.size + op.mem.disp
            calls[ins.address] = {'import': imports.get(slot, 'unknown'), 'slot_rva': hex(slot)}
        else:
            calls[ins.address] = {'indirect': ins.op_str}
    for address in [0x12C5EB, 0x12D342, 0x12D8BA]:
        assert '?raytrace@DeviceUtilRaytracing@d3d@@' in calls[address]['import']
    assert '?dispatch@DeviceUtil@d3d@@' in calls[0x12CC16]['import']
    # Trivial accessors are byte-verified, not inferred from neighboring methods.
    assert pe.get_data(0xA8190, 8) == bytes.fromhex('48 8b 81 80 00 00 00 c3')
    assert pe.get_data(0xA81A0, 7) == bytes.fromhex('8b 81 88 00 00 00 c3')
    for address, target in [(0x12C5AA,0xA81A0),(0x12C5C3,0xA8190),
                            (0x12D2FF,0xA81A0),(0x12D318,0xA8190),
                            (0x12D878,0xA81A0),(0x12D891,0xA8190)]:
        assert calls[address]['target_rva'] == hex(target)
    # These later ray passes are CONDITIONAL. Their presence in code alone does
    # not imply they executed in the capture or authorize delayed GI dispatch.
    interval = [{'rva': hex(a), **value} for a,value in calls.items()
                if 0x12C5F1 <= a <= 0x12CC16]
    return {'renderer_sha256': digest, 'function': [hex(lo),hex(hi)],
            'function_sha256': hashlib.sha256(pe.get_data(lo,hi-lo)).hexdigest(),
            'gi_ray_call': '0x12c5eb', 'gi_shade_call': '0x12cc16',
            'later_conditional_ray_calls': {'sun_shadow':'0x12d342','ao':'0x12d8ba'},
            'scene_binding_array_offset':'0x80', 'scene_binding_count_offset':'0x88',
            'calls_between_ray_and_shade':interval,
            'scene_lease_proven':False,
            'remaining':['Verify executed native scene scope and current frame ownership',
                         'Audit transitive/concurrent scene or material mutation',
                         'Tie descriptor allocation and resource retirement to submitted consumer']}

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('renderer')
    parser.add_argument('--output',required=True)
    args=parser.parse_args()
    result=audit(args.renderer)
    Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    print('Verified native GI interval and two CONDITIONAL later scene consumers; scene lease NOT certified.')
