"""Rank spotlight candidates before map shadowing; retain mapped nonselected light.

Hash-locked to the captured direct-light/ID producer. Rays and selected-light
composition are unchanged. Material response, frustum and cookie tests remain.
"""
import hashlib
from dxbc_transform import transform_shex
from rt_spot_history import instruction, dest, source, immediate

NATIVE_SHA = 'a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f'

def multiply(dst, src, weight):
    body=dest(dst,7)+source(src)+source(weight,'xxxx')
    return [((len(body)+1)<<24)|56,*body]

def unshadowed_selection(data):
    if hashlib.sha256(data).hexdigest()!=NATIVE_SHA:
        raise ValueError('Unsupported spotlight selector')
    def transform(w):
        assert w[225]&2047==104 and w[226]==43
        assert w[5685]&2047==48 and w[7313]&2047==57
        assert w[7596]&2047==56 and w[10445]&2047==0
        assert w[10453]&2047==49
        # Extra register is private to the spotlight loop. Initialize it even
        # when the native frustum branch rejects this candidate.
        w[226]=44
        edits={
            5686:(0,instruction('mov',dest(43,1),immediate([0,0,0,0]))),
            # Preserve final static/dynamic map visibility for native accumulation.
            7313:(7,instruction('mov',dest(43,1),source(28,'wwww'))+
                instruction('mov',dest(29,8),immediate([0xffffffff]*4))),
            # Keep the native near-plane mask, omit multiplication by map visibility.
            7596:(7,[0x0100003a]*7),
            # Score this candidate's unshadowed diffuse+specular contribution.
            # Do not use the old adjacent-candidate brightness difference.
            10445:(8,instruction('mov',dest(33,1),source(13,'wwww'))),
            # Native selection/accumulation below still receives mapped lighting.
            10453:(0,multiply(36,36,43)+multiply(35,35,43)),
        }
        for at,(size,replacement) in sorted(edits.items(),reverse=True):
            w[at:at+size]=replacement
    return transform_shex(data,transform)
