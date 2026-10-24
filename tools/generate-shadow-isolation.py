"""R137: isolate RT contact visibility from direct-light screen occlusion.

Hash-locked operand edits only. Keep authored rays, map visibility, material
response, bent normals, resources and accumulated lighting unchanged.
"""
import base64
import hashlib
import struct
from pathlib import Path
from dxbc_transform import transform_shex
from rt_spot_history import dest, source

ROOT = Path(__file__).resolve().parents[1]
LIGHT_SHA = 'a844888cdd5478605574157b323cb3376369ae9517c4954bf38bee21b664ecd7'
SELECTOR_SHA = '4319b0bd2d0ad74b670c084d92b84bd7ceda2be433ccd3e60588d49bb66ddb9c'


def neutralize_load(data, expected_sha, offset, register, mask, address, resource):
    if hashlib.sha256(data).hexdigest() != expected_sha:
        raise ValueError('Unsupported shadow isolation input')
    expected = [0x0800002d, *dest(register, mask), *source(*address), *resource]
    replacement = [0x05000036, *dest(register, mask), 0x00004001, 0x3f800000,
                   0x0100003a, 0x0100003a, 0x0100003a]
    def patch(words):
        if words[offset:offset+8] != expected:
            raise ValueError('Shadow input instruction changed')
        words[offset:offset+8] = replacement
    return transform_shex(data, patch)


def generate():
    light = base64.b64decode((ROOT/'assets/rt-spot/rt_spot_light.dxbc.b64').read_text())
    selector = base64.b64decode((ROOT/'assets/rt-spot/rt_spot_selection.dxbc.b64').read_text())
    # t15.xy -> r7.zw: selected spot and point contact visibility.
    # t2.yw -> r7.xy / r9.xy: screen-space diffuse/specular occlusion.
    variants = [
        ('contact_off', 'kContactOffShader', light, LIGHT_SHA, 346, 7, 12, (5,'xyww'), [0x002074e6,15,15]),
        ('screen_occlusion_off', 'kScreenOcclusionOffShader', light, LIGHT_SHA, 338, 7, 3, (5,'xyww'), [0x002078d6,2,2]),
        ('selector_occlusion_off', 'kSelectorOcclusionOffShader', selector, SELECTOR_SHA, 4436, 9, 3, (9,'xyzw'), [0x002078d6,2,2]),
    ]
    out = ROOT/'build'; out.mkdir(exist_ok=True)
    headers = ['#pragma once']
    for name, symbol, data, sha, offset, register, mask, address, resource in variants:
        result = neutralize_load(data,sha,offset,register,mask,address,resource)
        # Container checksum plus exactly one eight-DWORD instruction may differ.
        shex = next(o+8 for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32) if data[o:o+4]==b'SHEX')
        allowed = set(range(4,20)) | set(range(shex+offset*4,shex+(offset+8)*4))
        assert len(data)==len(result) and all(a==b for i,(a,b) in enumerate(zip(data,result)) if i not in allowed)
        (out/f'rt_spot_{name}.dxbc').write_bytes(result)
        headers.append(f'static const unsigned char {symbol}[]={{\n'+','.join(f'0x{x:02x}' for x in result)+'\n};')
        print(f'PASS: {name}, one neutralized load, sha256={hashlib.sha256(result).hexdigest()}')
    (out/'rt_spot_shadow_isolation.h').write_text('\n'.join(headers)+'\n')

if __name__ == '__main__': generate()
