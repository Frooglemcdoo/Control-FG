"""R141: reject spotlight history inconsistent with current-pixel visibility.

This is a signal agreement test, not previous-light identity validation. Keep
native depth/motion/reset handling, point history and spatial softness intact.
"""
import base64
import hashlib
import struct
from pathlib import Path
from dxbc_transform import transform_shex
from rt_spot_history import dest, source

ROOT = Path(__file__).resolve().parents[1]
BASELINE_SHA = '0c30a3cb345a5ccb7f5a10d639b611118f683d007ec4c4944453187c62928e91'
# Visibility is normalized [0,1]. This candidate threshold is intentionally
# centralized and must be validated in motion and for stationary shadow noise.
MAX_HISTORY_DIFFERENCE = 0.125
BLEND_OFFSET = 848
NATIVE_BLEND = [0x08000000,0x001000f2,2,0x80100ae6,0x41,1,0x00100e46,2]


def guard_instructions():
    tolerance = struct.unpack('<I',struct.pack('<f',MAX_HISTORY_DIFFERENCE))[0]
    return [
        # r3.x = clamped history - current spotlight visibility.
        0x08000000,*dest(3,1),*source(2,'xxxx'),0x80100aa6,0x41,1,
        # Ordered comparison rejects NaN as well as excessive disagreement.
        0x0800001d,*dest(3,1),0x00004001,tolerance,0x80100006,0x81,3,
        # x/z/w all represent spotlight; y remains native point history.
        0x09000037,*dest(2,13),*source(3,'xxxx'),*source(2,'xxxx'),*source(1,'zzzz')]


def guarded_history(data):
    if hashlib.sha256(data).hexdigest()!=BASELINE_SHA:
        raise ValueError('Unsupported spotlight history shader')
    def transform(words):
        if words[BLEND_OFFSET:BLEND_OFFSET+8]!=NATIVE_BLEND:
            raise ValueError('Native history blend changed')
        words[BLEND_OFFSET:BLEND_OFFSET]=guard_instructions()
    return transform_shex(data,transform)


def generate():
    original=base64.b64decode((ROOT/'assets/rt-spot/rt_spot_history.dxbc.b64').read_text())
    shader=guarded_history(original)
    out=ROOT/'build';out.mkdir(exist_ok=True)
    (out/'rt_spot_guarded_history.dxbc').write_bytes(shader)
    (out/'rt_spot_history_guard.h').write_text('#pragma once\nstatic const unsigned char kGuardedHistoryShader[]={\n'+','.join(f'0x{x:02x}' for x in shader)+'\n};\n')
    print('PASS: guarded spotlight history',hashlib.sha256(shader).hexdigest())

if __name__=='__main__':generate()
