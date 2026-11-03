#!/usr/bin/env python3
"""Hash-pinned static GI setup route; no process access or DLL modification."""
import argparse
import hashlib
import json
import runpy
import struct
from pathlib import Path

PE = runpy.run_path(str(Path(__file__).with_name('audit-gi-native-lifecycle.py')))['PE']
D3D_SHA = 'cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
RENDERER_SHA = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
CALLS = {
    0x12c4f3: (0x5df4a0, '?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z'),
    0x12c553: (0x5df4a8, '?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z'),
    0x12c562: (0x5df4b0, '?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z'),
    0x12c577: (0x5df4b8, '?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z'),
    0x12c592: (0x5df4b8, '?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z'),
    0x12c5d9: (0x5df4c0, '?endPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHHHPEBURaytracingGeometryBinding@2@H@Z'),
    0x12c5eb: (0x5df4c8, '?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z'),
}


def audit(data):
    pe = PE(data)
    def offset(rva, size=1):
        found = [raw+rva-start for start, length, raw, _ in pe.sections
                 if start <= rva and rva+size <= start+length]
        if len(found) != 1:
            raise ValueError('unmapped RVA')
        return found[0]
    def string(rva):
        out = bytearray()
        for i in range(512):
            b = data[offset(rva+i)]
            if not b:
                return out.decode('ascii')
            out.append(b)
        raise ValueError('unterminated name')
    header = struct.unpack_from('<I', data, 60)[0]
    imports = struct.unpack_from('<I', data, header+24+112+8)[0]
    names = {}
    for i in range(4096):
        ilt, _, _, name, iat = struct.unpack_from('<IIIII', data, offset(imports+i*20, 20))
        if not name:
            break
        dll = string(name)
        for j in range(65536):
            thunk = struct.unpack_from('<Q', data, offset((ilt or iat)+j*8, 8))[0]
            if not thunk:
                break
            if not thunk >> 63:
                names[iat+j*8] = (dll, string(thunk+2))
        else:
            raise ValueError('import limit')
    else:
        raise ValueError('descriptor limit')
    records = []
    for site, (iat, name) in CALLS.items():
        raw = pe.read(site, 6)
        if raw[:2] != b'\xff\x15' or site+6+struct.unpack_from('<i', raw, 2)[0] != iat:
            raise ValueError(f'call mismatch {site:x}')
        if names.get(iat) != ('d3d_rmdwin10_f.dll', name):
            raise ValueError(f'import mismatch {iat:x}')
        records.append(dict(site=hex(site), iat=hex(iat), symbol=name, bytes=raw.hex()))
    # Byte checks plus RIP targets establish the named arguments, not just nearby strings.
    strings = []
    for site, prefix, target, expected in [
        (0x12b9e8, '4c8d35', 0x630358, 'closestHitMain'),
        (0x12c54c, '488d0d', 0x630338, 'rayGenMain'),
        (0x12c559, '488d15', 0x630328, 'missMain'),
        (0x12c582, '4c8d0d', 0x630348, 'anyHitAlphaTest'),
    ]:
        raw = pe.read(site, 7)
        if raw[:3].hex() != prefix or site+7+struct.unpack_from('<i', raw, 3)[0] != target or string(target) != expected:
            raise ValueError(f'string argument mismatch {site:x}')
        strings.append(dict(site=hex(site), target=hex(target), value=expected))
    fixed = {0x12c4e5:'41bc02000000418bd4418d4c24ff', 0x12c560:'33c9',
             0x12c568:'4c897424204533c94533c033d233c9',
             0x12c57d:'4c89742420', 0x12c589:'4533c0418d500133c9'}
    for site, expected in fixed.items():
        if pe.read(site, len(bytes.fromhex(expected))).hex() != expected:
            raise ValueError(f'argument setup mismatch {site:x}')
    return dict(calls=records, shader_arguments=strings, setup_arguments=[1, 2], miss_slot=0,
                hit_groups=[dict(ray_type=0, geometry_type=0, any_hit=None, closest_hit='closestHitMain'),
                            dict(ray_type=0, geometry_type=1, any_hit='anyHitAlphaTest', closest_hit='closestHitMain')])


WRITER_BYTES = {
    # EBX is active ray count; reserve 2-rays additional records per binding.
    0x33eec: '8b9de0010000',
    0x33f2c: 'b8020000002bc38945d0',
    # Identifier index = ray + 2 * binding.type, table at state + 0x20.
    0x33f51: '8b40ec8d04414898',
    0x33f63: '488b55e8480f444cc220',
    # Active rows advance by state stride after 32 identifier + 56 root bytes.
    0x341d2: '8b881002000083e9204863d14883ea38',
    0x341f0: 'ffc1894c24343b8de0010000',
    # Reserved rows skip stride - 32; no local-root bytes admitted for them.
    0x34347: '8b881002000083e9204863d1',
    0x34361: 'ffc9894c243485c90f8fb0feffff',
    0x34378: '4883c040',
}


def audit_writer(data):
    pe = PE(data)
    for site, expected in WRITER_BYTES.items():
        if pe.read(site, len(bytes.fromhex(expected))).hex() != expected:
            raise ValueError(f'writer instruction mismatch {site:x}')
    return dict(active_rays=1, reserved_rows_per_binding=2,
                identifier_index='ray + 2 * binding.type',
                admitted_local_root_bytes=56, binding_stride=64,
                checks=[dict(site=hex(k), bytes=v) for k, v in WRITER_BYTES.items()])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('renderer', type=Path)
    p.add_argument('--output', type=Path)
    p.add_argument('--d3d', type=Path, required=True)
    args = p.parse_args()
    data = args.renderer.read_bytes()
    if hashlib.sha256(data).hexdigest() != RENDERER_SHA:
        raise ValueError('unsupported renderer')
    result = audit(data)
    writer = args.d3d.read_bytes()
    if hashlib.sha256(writer).hexdigest() != D3D_SHA:
        raise ValueError('unsupported d3d')
    result['writer'] = audit_writer(writer)
    result['writer']['sha256'] = D3D_SHA
    writer_pe = PE(writer)
    for site in WRITER_BYTES:
        changed = bytearray(writer);changed[writer_pe.offset(site, 1)] ^= 1
        try:
            audit_writer(changed)
        except ValueError:
            pass
        else:
            raise RuntimeError('changed writer accepted')
    result['writer']['mutation_rejections'] = len(WRITER_BYTES)
    rejected = 0
    pe = PE(data)
    for site in CALLS:
        changed = bytearray(data);changed[pe.offset(site, 6)+2] ^= 1
        try:
            audit(changed)
        except ValueError:
            rejected += 1
        else:
            raise RuntimeError('changed call accepted')
    result.update(schema=1, renderer_sha256=RENDERER_SHA, mutation_rejections=rejected,
                  scope='Static GI setup route only; no live writer ticket, allocation, shader identifier or dispatch lifetime proof')
    if args.output:
        args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print('PASS GI setup: 7 imported callsites, 4 shader arguments, setup(1,2), two hit groups; writer two-row spacing verified; 16 mutations rejected')


if __name__ == '__main__':
    main()
