"""Extract Control's spotlight BRDF into an independent work-atlas shader.

The source is hash locked. Native material/projector calculations stay byte exact.
Input t14 is now uint4(light index, receiver x, receiver y, enabled) per work texel.
Output u0/u1 is overwritten diffuse/specular RGB; alpha is extraction validity.
This shader is an adapter component, not a drop-in game shader replacement.
"""
from pathlib import Path
import argparse
import base64
import hashlib
import json
import struct
import sys
from dxbc_transform import transform_shex

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research/native-integration-audit/tools'))
from decode_dxbc import opnames

NATIVE_SHA = '6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621'
FIRST_INSTRUCTION = 176
POINT_BRANCH = 6025


def dst(index, mask='xyzw'):
    return [0x100002 | sum(16 << 'xyzw'.index(c) for c in mask), index]


def src(index, swizzle='xyzw'):
    swizzle = (swizzle * 4)[:4]
    return [0x100006 | sum('xyzw'.index(c) << (4 + 2*i) for i, c in enumerate(swizzle)), index]


def resource(index, swizzle='xyzw', kind=7):
    value = src(index, swizzle)
    return [(value[0] & ~0x300000) | 0x200000 | (kind << 12), index, index]


def literal(bits):
    return [0x4001, bits]


def emit(name, *operands, flags=0):
    words = [word for operand in operands for word in operand]
    return [((len(words) + 1) << 24) | opnames.index(name) | flags, *words]


def thread(swizzle='xyzw'):
    return [(src(0, swizzle)[0] & ~0x300000) | (32 << 12)]


def preamble():
    # Native uses r0..r21. Reserve r22..r25 for atlas metadata and guards.
    code = emit('mov', dst(24, 'xy'), thread('xyxx'))
    code += emit('mov', dst(24, 'zw'), literal(0))
    for slot in (0, 1):
        code += emit('store_uav_typed', resource(slot, kind=30), thread('xyyy'), literal(0))
    code += emit('ld', dst(23), src(24, 'xyww'), resource(14))
    code += emit('ine', dst(25, 'x'), src(23, 'w'), literal(1))
    code += emit('if', src(25, 'x'), flags=1 << 18)
    code += emit('ret') + emit('endif')
    code += emit('bufinfo', dst(25, 'x'), resource(13))
    code += emit('uge', dst(25, 'x'), src(23, 'x'), src(25, 'x'))
    code += emit('ieq', dst(25, 'y'), src(23, 'x'), literal(65535))
    code += emit('or', dst(25, 'x'), src(25, 'x'), src(25, 'y'))
    code += emit('if', src(25, 'x'), flags=1 << 18)
    code += emit('ret') + emit('endif')
    # Use actual GBuffer dimensions, not padded atlas or output resolution.
    code += emit('resinfo', dst(25, 'xy'), literal(0), resource(0), flags=2 << 11)
    code += emit('uge', dst(25, 'xy'), src(23, 'yzyy'), src(25, 'xyxx'))
    code += emit('or', dst(25, 'x'), src(25, 'x'), src(25, 'y'))
    code += emit('if', src(25, 'x'), flags=1 << 18)
    code += emit('ret') + emit('endif')
    code += emit('mov', dst(22, 'xy'), src(23, 'yzyy'))
    return code


def extract(data):
    if hashlib.sha256(data).hexdigest() != NATIVE_SHA:
        raise ValueError('Unsupported native spotlight shader; refusing unverified edits')

    def transform(words):
        instructions = {}
        offset = 2
        while offset < len(words):
            length = (words[offset] >> 24) & 127
            if not length:
                raise ValueError('Unexpected custom data in hash-locked input')
            instructions[offset] = words[offset:offset+length]
            offset += length
        expected = {170: 'dcl_temps', 176: 'utof', 303: 'mov', 346: 'ld',
                    354: 'ld', 1707: 'movc', 5979: 'ld_uav_typed', 5986: 'mad',
                    6002: 'ld_uav_typed', 6009: 'mad', POINT_BRANCH: 'if'}
        for offset, name in expected.items():
            if instructions[offset][0] & 2047 != opnames.index(name):
                raise ValueError(f'Native instruction changed at {offset}')
        edits = {
            170: emit('dcl_temps', [26]),
            176: preamble() + emit('utof', dst(0, 'xy'), src(22, 'xyxx')),
            303: emit('mov', dst(5, 'xy'), src(22, 'xyxx')),
            # Discard the old single-light visibility and selection entirely.
            346: [],
            354: emit('mov', dst(5, 'x'), src(23, 'x')) + emit('mov', dst(5, 'y'), literal(65535)),
            # Preserve map block's live intermediate values, but remove occlusion.
            1707: emit('mov', dst(0, 'x'), literal(0x3f800000)),
            5979: [],
            5986: emit('mov', dst(9, 'xyz'), src(0, 'xywx')) + emit('mov', dst(9, 'w'), literal(0x3f800000)),
            6002: [],
            6009: emit('mov', dst(8, 'w'), literal(0x3f800000)),
        }
        out = words[:2]
        for offset, instruction in instructions.items():
            if offset == POINT_BRANCH:
                out += emit('ret')
                break
            out += edits.get(offset, instruction)
        words[:] = out
    return transform_shex(data, transform)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('native', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    data = extract(args.native.read_bytes())
    args.output.mkdir(parents=True, exist_ok=True)
    name = 'rt_lights_native_spot.dxbc'
    (args.output / (name + '.b64')).write_text(base64.b64encode(data).decode() + '\n')
    manifest = {'native_sha256': NATIVE_SHA,
        'input_t14': 'uint4(lightIndex, receiverX, receiverY, enabled=1)',
        'outputs': {name: {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}},
        'output_u0_u1': 'float4(diffuse/specular RGB, validity)',
        'game_integration': False}
    (args.output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
