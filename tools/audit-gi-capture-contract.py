"""Verify the native GI capture boundary and ray-slice contract; never patch binaries."""
import argparse
import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile

ROOT = Path(__file__).resolve().parents[1]
AUDIT = ROOT / 'research/native-integration-audit/tools'


def module(name):
    spec = importlib.util.spec_from_file_location(name, AUDIT / (name + '.py'))
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit_renderer(path):
    pe = module('pe_tools').PE(path)
    digest = hashlib.sha256(pe.data).hexdigest()
    require(digest == 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433',
            'Unreviewed renderer identity')
    checks = []

    def code(name, rva, expected):
        raw = bytes.fromhex(expected)
        require(pe.data[pe.off(rva):pe.off(rva)+len(raw)] == raw, name)
        checks.append(dict(name=name, rva=hex(rva), bytes=raw.hex()))

    def call(name, rva, target):
        code(name, rva, (b'\xe8' + struct.pack('<i', target-rva-5)).hex())

    call('GI hit resources bound as SRVs', 0x12c61c, 0xa3940)
    call('GI shading shader bound', 0x12cbd2, 0x1db750)
    code('GI compute dispatch import', 0x12cc16, 'ff 15 a4 30 4b 00')
    require(pe.imports[0x5dfcc0] == '?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z',
            'Unexpected dispatch import')
    code('Batch increment and comparison', 0x12cc1c,
         '8b 85 f0 03 00 00 ff c0 89 85 f0 03 00 00 3b 85 00 04 00 00')
    code('Batch back edge', 0x12cc37, '0f 82 69 f3 ff ff')
    require(0x12cc3d + struct.unpack('<i', bytes.fromhex('69 f3 ff ff'))[0] == 0x12bfa6,
            'Batch loop target')
    call('Hit container cleanup after all batches', 0x12cc4f, 0xa3190)
    call('GI filter is later than cleanup', 0x12cd96, 0x14c90)
    code('Cleanup zeros material container', 0xa31a7, '48 8b 11 33 ff 48 89 39')
    code('Cleanup zeros normal container', 0xa31c1, '48 8b 53 08 48 89 7b 08')
    code('Cleanup zeros position container', 0xa31db, '48 8b 53 10 48 89 7b 10')
    call('Material container passed to cleanup helper', 0xa31bb, 0xa9e00)
    call('Normal container passed to cleanup helper', 0xa31d5, 0xa9e00)
    call('Position container passed to cleanup helper', 0xa31ef, 0xa9e00)
    # Validate the ShaderTexture objects, not just nearby string literals.
    providers = []
    for name, lea, address, string_rva in [
        ('g_tMaterialId', 0xa3a1b, 0x1291888, 0x627840),
        ('g_tNormal_TexcoordX', 0xa3af0, 0x12918b0, 0x627850),
        ('g_tPosition_TexcoordY', 0xa3bc6, 0x12918d8, 0x627868),
    ]:
        code(name + ' ShaderTexture address', lea,
             (b'\x48\x8d\x2d' + struct.pack('<i', address-lea-7)).hex())
        require(pe.string(string_rva) == name, name + ' provider name')
        providers.append(dict(name=name, shader_texture_rva=hex(address)))
    return dict(sha256=digest, checks=checks, providers=providers,
                capture_callsite='0x12cc16', capture_timing='after native dispatch returns, before batch back edge',
                warning='Static call ordering and container cleanup proven; GPU resource lifetime/state at runtime still requires validation.')


def audit_shader(raw, expected, temporary):
    require(hashlib.sha256(raw).hexdigest() == expected['sha256'], 'Unreviewed GI shader identity')
    require(len(raw) == expected['bytes'], 'GI shader size mismatch')
    temporary.write_bytes(raw)
    d = module('decode_dxbc').decode(temporary)
    bindings = {b['name']: b for b in d['bindings']}
    hit = [bindings[n] for n in ('g_tMaterialId', 'g_tNormal_TexcoordX', 'g_tPosition_TexcoordY')]
    require(all(b['kind'] == 2 and b['dimension'] == 5 and b['count'] == 1 and b['space'] == 0 for b in hit),
            'Hit resources must be Texture2DArray SRVs')
    cb = next(c for c in d['constant_buffers'] if c['name'] == 'DiffuseGIConstants')
    cb_binding = bindings[cb['name']]
    require(cb['size'] == 16 and cb_binding['slot'] == 4 and cb_binding['identifier'] == 4, 'GI constants layout')
    variable = next(v for v in cb['variables'] if v['name'] == 'g_uRTDiffuseRayCount')
    require(variable['offset'] == 8 and variable['size'] == 4 and variable['flags'] & 2, 'Ray count layout/use')
    instructions = d['instructions']
    limits = [i for i in instructions if i['name'] == 'ige' and i['operands'][-1] == 'cb[4][4][0].z']
    require(len(limits) == 1, 'Expected one per-ray loop bound')
    counter = limits[0]['operands'][1]
    require(counter.endswith('.z'), 'Ray index must address array slice')
    coordinate = counter[:-2] + '.xyzw'
    loads = []
    for b in hit:
        source = f"t[{b['identifier']}][{b['slot']}]"
        matches = [i for i in instructions if i['name'] == 'ld' and i['operands'][1] == coordinate
                   and i['operands'][2].startswith(source + '.') and i['dword'] > limits[0]['dword']]
        require(len(matches) == 1, 'Ambiguous hit load for ' + b['name'])
        loads.append(dict(binding=b, instruction=matches[0]))
    increments = [i for i in instructions if i['name'] == 'iadd' and i['operands'][:2] == [counter, counter]]
    require(len(increments) == 1 and increments[0]['operands'][2] == 'l(1)', 'Ray index must increment by integer one')
    spot = bindings['g_sbDeferredLightSpotData']
    require(spot['kind'] == 5 and spot['samples'] == 320, 'Spot record stride')
    return dict(file=expected['file'], sha256=expected['sha256'], hit_loads=loads,
                ray_count=dict(binding=cb_binding, variable=variable),
                loop_bound=limits[0], loop_increment=increments[0], spot_records=spot)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('renderer', type=Path)
    p.add_argument('--output', required=True, type=Path)
    args = p.parse_args()
    result = dict(schema='control-gi-capture-contract-v1', renderer=audit_renderer(args.renderer), shaders=[])
    source = ROOT / 'research/gi-source-r267'
    manifest = json.loads((source / 'manifest.json').read_text())
    with tempfile.TemporaryDirectory() as td:
        for expected in manifest[:8]:
            raw = base64.b64decode((source / (expected['file'] + '.b64')).read_bytes(), validate=False)
            result['shaders'].append(audit_shader(raw, expected, Path(td) / 'shader.dxbc'))
    result['runtime_capture_complete'] = False
    result['requirements'] = [
        'Copy all used array slices for every observed GI batch, before container cleanup.',
        'Capture the actual variant-specific bindings and SRV slice range; do not assume fixed slots across variants.',
        'Save constants, light records and clustered indices at the same dispatch as hit textures.',
        'Maintain original resource states and retain owners through GPU completion.',
        'Reject overflow, missing batches/slices, mismatched generations and uncompleted GPU copies.',
        'Per-light intermediate operands require shader instrumentation or verified replay; textures alone do not provide them.',
    ]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print('PASS renderer call ordering, cleanup and provider addresses; all 8 GI variants use per-ray array slices. No GPU execution claim.')


if __name__ == '__main__':
    main()
