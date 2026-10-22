"""Build the hash-locked spotlight ray and composition pair from captured shaders.

Requires DXC, the small assemble-dxil helper, and DXV. Does not modify the game.
The point branch, alpha handlers and resource layout
remain intact. The selected-light compute variant removes map visibility only;
it must never be selected without a matching full-distance spotlight dispatch.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from dxbc_checksum import checksum
from rt_spot_history import bounded_history
from rt_spot_debug_views import debug_view
from rt_spot_diagnostics import raw_history, raw_spatial
from rt_spot_samples import four_spot_samples
from rt_spot_filter import SPATIAL_SHAS, bounded_penumbra

MULTI_RAY_SHA = 'd5efe2c2219257e2a7b51bbd65a7fa3b80cdb56fb7e6444223f8b4d7ee45a1a9'
SINGLE_RAY_SHA = '3c6bee7eea5b5758dcaf96b400bb7589b4af2031c17bf6ba5ce2343623ddb618'
LIGHT_SHA = '6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621'
TEMPORAL_SHA = 'feb37637481fed22a3cd4d256da613d1f61aec70fc11ff5e42aa2a585101fedb'

def checked(path, sha):
    data = path.read_bytes()
    if hashlib.sha256(data).hexdigest() != sha:
        raise ValueError(f'Unsupported input shader: {path}')
    return data

def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError('Native DXIL instruction anchor changed')
    return source.replace(before, after)

def full_distance(source, single_ray=True):
    if not single_ray:
        raise ValueError('Only the canonical R71 replacement is generated')
    return four_spot_samples(source)

def patch_instruction(data, dword, expected_opcode, words):
    data = bytearray(data)
    count = struct.unpack_from('<I', data, 28)[0]
    offsets = struct.unpack_from('<' + 'I' * count, data, 32)
    shex = next(offset + 8 for offset in offsets if data[offset:offset+4] == b'SHEX')
    start = shex + dword * 4
    token = struct.unpack_from('<I', data, start)[0]
    size = (token >> 24) & 127
    if token & 2047 != expected_opcode or len(words) > size:
        raise ValueError('Native DXBC instruction anchor changed')
    # Single-DWORD NOPs retain all subsequent instruction/chunk offsets.
    words = words + [0x0100003a] * (size - len(words))
    struct.pack_into('<' + 'I' * size, data, start, *words)
    data[4:20] = checksum(bytes(data))
    return bytes(data)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('capture', type=Path)
    ap.add_argument('output', type=Path)
    for name in ('dxc', 'assembler', 'dxv'): ap.add_argument('--' + name, required=True)
    args = ap.parse_args()
    ray_sha = SINGLE_RAY_SHA
    ray = args.capture / f'dxil_library-{ray_sha}.dxbc'
    checked(ray, ray_sha)
    light = checked(args.capture / f'compute_cs-{LIGHT_SHA}.dxbc', LIGHT_SHA)
    temporal = checked(args.capture / f'compute_cs-{TEMPORAL_SHA}.dxbc', TEMPORAL_SHA)
    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        source = subprocess.check_output([args.dxc, '-dumpbin', str(ray)], text=True)
        ll = directory / 'full-distance.ll'
        ll.write_text(full_distance(source, ray_sha == SINGLE_RAY_SHA))
        unsigned = directory / 'unsigned.dxil'
        signed = directory / 'signed.dxil'
        subprocess.run([args.assembler, str(ll), str(unsigned)], check=True)
        subprocess.run([args.dxv, str(unsigned), '-o=' + str(signed)], check=True)
        full = signed.read_bytes()
    visibility = debug_view(light)
    light_ids = debug_view(light, light_ids=True)
    # mov r0.x, 1; replaces final static/dynamic map visibility before its zero gate.
    light = patch_instruction(light, 1707, 55, [0x05000036, 0x00100012, 0, 0x00004001, 0x3f800000])
    raw_temporal = raw_history(temporal)
    history = bounded_history(temporal)
    # mov r1.xyzw, r1.zwzz; current visibility only, for history reset on transition.
    temporal = patch_instruction(temporal, 324, 50, [0x05000036, 0x001000f2, 1, 0x00100ae6, 1])
    args.output.mkdir(parents=True, exist_ok=True)
    ray_name = 'rt_spot_full.dxil'
    outputs = {'rt_spot_visibility.dxbc': visibility, 'rt_spot_light_ids.dxbc': light_ids, 'rt_spot_raw_history.dxbc': raw_temporal, ray_name: full, 'rt_spot_light.dxbc': light, 'rt_spot_reset.dxbc': temporal, 'rt_spot_history.dxbc': history}
    for index, sha in enumerate(SPATIAL_SHAS):
        native = checked(args.capture / f'compute_cs-{sha}.dxbc', sha)
        outputs[f'rt_spot_spatial{index}.dxbc'] = bounded_penumbra(native)
        outputs[f'rt_spot_raw_spatial{index}.dxbc'] = raw_spatial(native)
    manifest = {'native_multi_ray': MULTI_RAY_SHA, 'native_ray': ray_sha, 'native_light': LIGHT_SHA, 'native_temporal': TEMPORAL_SHA, 'outputs': {}}
    for name, data in outputs.items():
        (args.output / name).write_bytes(data)
        manifest['outputs'][name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))

if __name__ == '__main__': main()
