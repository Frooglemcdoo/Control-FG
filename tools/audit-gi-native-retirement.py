#!/usr/bin/env python3
"""Verify the captured build's native retirement ordering, not scene immutability."""
import argparse
import hashlib
import json
from pathlib import Path
import pefile

EXPECTED = 'cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
# Independently disassembled instruction boundaries. Keep these narrow claims
# distinct from proving every geometry suballocator and concurrent writer.
CHECKS = {
    0x239B0: '48890db1321100c3',  # setCurrentFrame -> 136C68
    0x239C0: '488b05a1321100c3',  # getCurrentFrame <- 136C68
    0x31DD5: '488b8188000000488b38488b7008',  # timeline value, fence
    0x31DE7: 'e8b4160100',  # flush before Signal
    0x31DF0: '488b4810488b014c8bc7488bd6ff5070',  # queue Signal(fence,value)
    0x31E65: '488b05bc4e1000488b888800000048ff01',  # current value++
    0x31E76: 'e84527ffff33c9e8ae29ffff',  # stamp batch; drain(false)
    0x24616: '488b050b271100488b88880000000f10010f114218',  # batch+18=value,fence
    0x249AE: '4584ed752c',  # force-drain path is separate
    0x249B3: '0f1047180f2945d7660f73d80866480f7ec14885c97415',
    0x249CA: '488b01ff5040483b45d70f92c084c00f85c8020000',  # incomplete skips batch
    0x4354E: '498b04244d8bc58bd7498bccff5050',  # ExecuteCommandLists
    0x4359C: '4c8b0585370f00',  # NativeDevice global
    0x435A3: '498b80880000000f1000498b0cd7660fd64140',  # list's retirement value
    0x24485: '488b07488bcfff5008',  # retain IUnknown before enqueue
    0x244AC: '488938488d0d6affffff48894808',  # deferred Release callback
    0x24420: '488b0148ff6010',
    # The ray path has its own SBT timeline, independent of Present retirement.
    0x35701: '498b8020020000488b80a800000048ff00498b9020020000488b82a8000000488b0883e1014c8d348949c1e6044c03f2',
    0x357C6: 'e875e2ffff4c8b0556151000498b8020020000488b80a000000048ff00',
    0x358D9: '488b4920488b01488d55a7ff9060020000',  # DispatchRays
    0x35A4B: '488b05d6121000488b8820020000488bb9a0000000498bcfe838da0000498b4f10488b014c8b07488b5708ff5070',
    0x35A99: '488b0588121000488b8820020000488bb9a8000000498bcfe8ead90000498b4f10488b014c8b07488b5708ff5070',
}

def audit(path):
    raw = Path(path).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED:
        raise ValueError('Unsupported D3D identity; do not reuse offsets')
    pe = pefile.PE(data=raw)
    for rva, expected in CHECKS.items():
        expected = bytes.fromhex(expected)
        if pe.get_data(rva, len(expected)) != expected:
            raise ValueError(f'Native retirement instruction mismatch at {rva:#x}')
    exports = {s.address: (s.name or b'').decode('ascii')
               for s in pe.DIRECTORY_ENTRY_EXPORT.symbols}
    for rva, name in [(0x31B50, 'present@DeviceUtil@d3d'),
                      (0x239B0, 'setCurrentFrame@d3d'),
                      (0x239C0, 'getCurrentFrame@d3d'),
                      (0x353E0, 'raytrace@DeviceUtilRaytracing@d3d')]:
        if name not in exports.get(rva, ''):
            raise ValueError(f'Unexpected exported identity at {rva:#x}')
    return {
        'd3d_sha256': digest,
        'verified_instruction_windows': {hex(k):v for k,v in CHECKS.items()},
        'native_device_global_rva': '0x136d28',
        'frame_number_global_rva': '0x136c68',
        'timeline_offset': '0x88',
        'timeline_fields': {'value':'0x0', 'fence':'0x8'},
        'present_order': ['flush pending native command lists',
                          'signal native fence at value N',
                          'increment timeline to N+1',
                          'stamp deferred-release batch with N+1 and fence',
                          'drain completed batches with force=false'],
        'ordinary_drain_waits_for_stored_fence_value': True,
        'force_drain_is_distinct': True,
        'native_sbt_timeline': {
            'context_offset_from_device':'0x220',
            'fence_fields':['0xa0','0xa8'],
            'slot_selection':'increment +a8 fence value, select parity, stride 0x50',
            'native_dispatch_call':'0x358e4',
            'ordinary_post_dispatch_signals':['0x35a76','0x35ac4'],
            'flush_before_each_signal':True,
            'signals_before_raytrace_returns':True,
            'same_timeline_as_present':False,
            'scene_mutation_authorized_by_these_fences': 'not established',
            'replay_requirement':'Preserve native SBT bytes before these native lifetime cuts; retaining a resource pointer or Present timeline alone is insufficient',
        },
        'scene_lease_proven': False,
        'remaining': ['Geometry/BLAS suballocation recycling and mutable contents',
                      'Transitive and concurrent writes between GI ray and shade',
                      'Actual consumer submitted before native retirement boundary'],
    }

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('d3d')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    result = audit(args.d3d)
    Path(args.output).write_text(json.dumps(result, indent=2)+'\n')
    print('PASS native Present signal/advance/stamp/drain order; scene lease NOT certified')
