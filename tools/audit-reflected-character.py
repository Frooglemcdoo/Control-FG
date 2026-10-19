"""Audit the collector's shader archive without changing game files.

Usage: python tools/audit-reflected-character.py capture.zip
Prints reproducible binding/layout evidence, not a visual-fix verdict.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import zipfile


def containers(data):
    start = 0
    while True:
        start = data.find(b'DXBC', start)
        if start < 0:
            return
        if start + 32 > len(data):
            raise ValueError('truncated DXBC header')
        size, count = struct.unpack_from('<II', data, start + 24)
        if size < 32 + 4 * count or start + size > len(data):
            raise ValueError('invalid DXBC container bounds')
        blob = data[start:start + size]
        chunks = {}
        for offset in struct.unpack_from('<' + 'I' * count, blob, 32):
            if offset + 8 > size:
                raise ValueError('invalid chunk header')
            length = struct.unpack_from('<I', blob, offset + 4)[0]
            if offset + 8 + length > size:
                raise ValueError('invalid chunk size')
            chunks[blob[offset:offset + 4]] = blob[offset + 8:offset + 8 + length]
        yield blob, chunks
        start += size


def binding_names(chunks):
    rdef = chunks.get(b'RDEF')
    if rdef is None:
        return []
    count, offset = struct.unpack_from('<II', rdef, 8)
    names = []
    for index in range(count):
        name = struct.unpack_from('<I', rdef, offset + index * 40)[0]
        names.append(rdef[name:rdef.index(0, name)].decode('ascii'))
    return names


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent.parent
    spec = importlib.util.spec_from_file_location(
        'decode', root / 'research/native-integration-audit/tools/decode_dxbc.py')
    decoder = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(decoder)
    with zipfile.ZipFile(args.capture) as archive:
        def read(name):
            matches = [p for p in archive.namelist()
                       if p.replace('\\', '/').split('/')[-1] == name]
            if len(matches) != 1:
                raise ValueError(f'expected one {name}, found {len(matches)}')
            return archive.read(matches[0])
        standard = list(containers(read('standardmaterial.obj')))
        glass = list(containers(read('rt_transparent.obj')))
        lighting = list(containers(read('rt_transparent_deferred.obj')))

    character_sha = 'cf68e7a8831c24b45c9c0fd54f03cb3c1defd0f99fe0bf68f0771333d888cb66'
    selected = [blob for blob, _ in standard
                if hashlib.sha256(blob).hexdigest() == character_sha]
    if not selected:
        raise ValueError('audited primary-character variant missing')
    glass_sha = hashlib.sha256(glass[0][0]).hexdigest()
    light_sha = hashlib.sha256(lighting[0][0]).hexdigest()
    if glass_sha != '66ec0c29541de1f4aa59636ce6e4415de440a12315b983839d0de0c0a2bab9a8':
        raise ValueError('unsupported glass hit library')
    if light_sha != 'b19de5e0b8a216076b6800790b100b91caa83814abdad3ad276736ba72b5e762':
        raise ValueError('unsupported glass lighting shader')
    with tempfile.TemporaryDirectory() as tmp:
        shader = Path(tmp) / 'character.dxbc'
        shader.write_bytes(selected[0])
        primary = decoder.decode(shader)
        shader.write_bytes(lighting[0][0])
        reflected = decoder.decode(shader)

    wanted = {590, 600, 787, 795, 805, 813, 2028, 2036, 2054,
              2955, 2963, 2971}
    evidence = [i for i in primary['instructions'] if i['dword'] in wanted]
    instance = next(c for c in primary['constant_buffers']
                    if c['name'] == 'g_sbInstanceObjectData')
    rig_index = next(m for m in instance['variables'][0]['type']['members']
                     if m['name'] == 'g_iCharacterLightRigIndex')
    primary_names = {b['name'] for b in primary['bindings']}
    reflected_names = {b['name'] for b in reflected['bindings']}
    required = {'g_bCharacterLightRigs', 'g_sbInstanceObjectData', 'character_lighting'}
    if not required <= primary_names or required & reflected_names:
        raise ValueError('binding contract differs from audited shader pair')
    print(json.dumps({
        'status': 'audit_only_no_rendering_change',
        'archive_sha256': hashlib.sha256(args.capture.read_bytes()).hexdigest(),
        'standard_container_count': len(standard),
        'character_rig_variant_count': sum('g_bCharacterLightRigs' in binding_names(c)
                                           for _, c in standard),
        'primary_shader_sha256': character_sha,
        'identical_primary_variant_count': len(selected),
        'glass_hit_sha256': glass_sha,
        'glass_lighting_sha256': light_sha,
        'primary_only_bindings': sorted(required),
        'instance_stride_bytes': instance['size'],
        'rig_index_member': rig_index,
        'rig_address_instruction_evidence': evidence,
        'unresolved': ['reflected hit to native render-instance mapping',
                       'same-view lighting resource lifetime',
                       'character-specific BRDF and material inputs',
                       'in-game visual validation'],
    }, indent=2))


if __name__ == '__main__':
    main()
