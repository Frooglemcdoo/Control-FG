"""R140: isolate contact visibility, screen occlusion, bent normals and local maps.

Hash-locked operand edits only. Keep authored rays, map visibility, material
response, resources and accumulation; only the named diagnostic input changes.
"""
import base64
import hashlib
import struct
from pathlib import Path
from dxbc_transform import transform_shex
from rt_spot_history import dest, source
from rt_spot_native_blend import native_blend, native_map_guard
from rt_spot_selection import map_priority_selection, map_visibility_probe, map_visibility_sidecar

ROOT = Path(__file__).resolve().parents[1]
LIGHT_SHA = 'a844888cdd5478605574157b323cb3376369ae9517c4954bf38bee21b664ecd7'
SELECTOR_SHA = '4319b0bd2d0ad74b670c084d92b84bd7ceda2be433ccd3e60588d49bb66ddb9c'


def fixed_edits(data, expected_sha, edits):
    """Apply hash/word-locked edits without shifting any instruction or resource."""
    if hashlib.sha256(data).hexdigest() != expected_sha:
        raise ValueError('Unsupported shadow isolation input')
    def patch(words):
        for offset, expected, replacement in edits:
            if words[offset:offset+len(expected)] != expected or len(replacement)>len(expected):
                raise ValueError('Shadow input instruction changed')
            words[offset:offset+len(expected)] = replacement + [0x0100003a]*(len(expected)-len(replacement))
    result=transform_shex(data,patch)
    shex=next(o+8 for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32) if data[o:o+4]==b'SHEX')
    allowed=set(range(4,20))
    for offset,expected,_ in edits: allowed.update(range(shex+offset*4,shex+(offset+len(expected))*4))
    assert len(data)==len(result) and all(a==b for i,(a,b) in enumerate(zip(data,result)) if i not in allowed)
    return result


def constant_move(register,mask,value):
    return [0x05000036,*dest(register,mask),0x00004001,value]


def neutralize_load(data, expected_sha, offset, register, mask, address, resource):
    expected=[0x0800002d,*dest(register,mask),*source(*address),*resource]
    return fixed_edits(data,expected_sha,[(offset,expected,constant_move(register,mask,0x3f800000))])


def remaining_isolation(light,selector):
    # Zero only bent-normal minus geometric-normal deltas. The existing blend
    # then uses the decoded geometric normal for every material case, rather
    # than substituting an arbitrary world axis or changing normal decoding.
    bent_light=[]
    for offset,register in [(2551,16),(5116,17)]:
        bent_light.append((offset,[0x08000000,*dest(register,7),0x100246,6,0x80100246,0x41,13],constant_move(register,7,0)))
    for offset,register in [(7602,12),(10023,6)]:
        bent_light.append((offset,[0x08000000,*dest(register,7),0x80100346,0x41,2,0x100246,6],constant_move(register,7,0)))
    bent_selector=[(4745,[0x08000000,*dest(10,7),0x80100346,0x41,5,0x100246,10],constant_move(10,7,0))]
    # Default already bypasses the selected spotlight map. Remaining local-map
    # paths are the producer's nonselected spots/points and selected point map.
    maps_light=[(6987,[0x09000037,*dest(4,8),0x10001a,5,0x4001,0x3f800000,0x10003a,4],constant_move(4,8,0x3f800000))]
    maps_selector=[
        (7321,[0x05000036,*dest(43,1),0x100ff6,28],constant_move(43,1,0x3f800000)),
        (12098,[0x09000037,*dest(18,8),0x10002a,21,0x4001,0x3f800000,0x10003a,18],constant_move(18,8,0x3f800000))]
    return [
        ('bent_normal_off','kBentNormalOffShader',fixed_edits(light,LIGHT_SHA,bent_light)),
        ('selector_bent_normal_off','kSelectorBentNormalOffShader',fixed_edits(selector,SELECTOR_SHA,bent_selector)),
        ('local_maps_off','kLocalMapsOffShader',fixed_edits(light,LIGHT_SHA,maps_light)),
        ('selector_local_maps_off','kSelectorLocalMapsOffShader',fixed_edits(selector,SELECTOR_SHA,maps_selector))]


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
        (out/f'rt_spot_{name}.dxbc').write_bytes(result)
        headers.append(f'static const unsigned char {symbol}[]={{\n'+','.join(f'0x{x:02x}' for x in result)+'\n};')
        print(f'PASS: {name}, one neutralized load, sha256={hashlib.sha256(result).hexdigest()}')
    isolated=remaining_isolation(light,selector)
    # Compose the exact R139 correction with the existing scalar-AO isolation.
    # Hash-lock the corrected intermediate too; never revert normal correction
    # while comparing motion. Sun visibility (t2.x), maps and RT stay active.
    bent_light=isolated[0][2]; bent_selector=isolated[1][2]
    isolated += [
        ('corrected_no_ao','kCorrectedNoAOShader',neutralize_load(bent_light,
         '55bcdf543e60dee904c656bfed355aa1f10393a8f0a950467f25a9ed5ee76a1c',338,7,3,(5,'xyww'),[0x002078d6,2,2])),
        ('selector_corrected_no_ao','kSelectorCorrectedNoAOShader',neutralize_load(bent_selector,
         '29177f00d24b7c7bc9a9563bb04de4c62f16838a198b1275342bddafc3770bc9',4436,9,3,(9,'xyzw'),[0x002078d6,2,2]))]
    # Matched full-color diagnostic: only the two selected-spot visibility
    # operands become 1. Preserve corrected normals, all other lights and maps.
    isolated.append(('corrected_open','kCorrectedOpenShader',fixed_edits(bent_light,
        '55bcdf543e60dee904c656bfed355aa1f10393a8f0a950467f25a9ed5ee76a1c',
        [(offset,source(7,'zzzz'),[0x00004001,0x3f800000]) for offset in (5989,6012)])))
    isolated.append(('selector_map_priority','kSelectorMapPriorityShader',map_priority_selection(bent_selector)))
    isolated.append(('selector_map_probe','kSelectorMapProbeShader',map_visibility_probe(bent_selector)))
    isolated.append(('selector_map_sidecar','kSelectorMapSidecarShader',map_visibility_sidecar(bent_selector)))
    isolated.append(('native_blend_legacy','kNativeBlendLegacyShader',native_blend(bent_light)))
    isolated.append(('native_blend','kNativeBlendShader',native_map_guard(bent_light)))
    for name,symbol,result in isolated:
        (out/f'rt_spot_{name}.dxbc').write_bytes(result)
        headers.append(f'static const unsigned char {symbol}[]={{\n'+','.join(f'0x{x:02x}' for x in result)+'\n};')
        print(f'PASS: {name}, fixed instruction edits, sha256={hashlib.sha256(result).hexdigest()}')
    (out/'rt_spot_shadow_isolation.h').write_text('\n'.join(headers)+'\n')

if __name__ == '__main__': generate()
