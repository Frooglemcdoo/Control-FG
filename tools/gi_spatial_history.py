"""R259: isolate the second history blend inside native GI spatial filtering."""
import base64
import hashlib
import struct
from pathlib import Path
from dxbc_transform import transform_shex

HASHES = (
    '40df59b42a5d30db4f2ad7f84bfcf02e3b743de6664a05aab9d8917f1fd58d4c',
    '80af0f6bec196ab7d0816589246b19ffbaf92b07cb10aecdfa9da4acb0c66cb3',
)

def generate(data, bounded=False):
    if hashlib.sha256(data).hexdigest() not in HASHES:
        raise ValueError('Unknown native GI spatial shader')
    def patch(w):
        # Final-pass temporal coefficient only. All spatial taps, geometry
        # rejection, native bindings and output ABI remain byte-identical.
        assert w[1956:1963] == [0x07000038, 0x00100082, 0,
                               0x0010003a, 0, 0x00004001, 0x3f333333]
        if not bounded:
            w[1962] = 0
            return
        # Bound the history sample against the final current spatial result,
        # not an earlier noisy input. Preserve native depth/motion confidence.
        assert w[91] == 13
        w[91] = 15
        def dst(r): return [0x00100072, r]
        def src(r): return [0x00100e46, r]
        def imm(v): return [0x4001, struct.unpack('<I', struct.pack('<f', v))[0]]
        def ins(op, *args):
            body = sum(args, [])
            return [((len(body)+1) << 24) | op] + body
        code = ins(50, dst(13), src(1), imm(.75), imm(-.001))
        code += ins(52, dst(13), src(13), imm(0))
        code += ins(50, dst(14), src(1), imm(1.25), imm(.001))
        code += ins(52, dst(3), src(3), src(13))
        code += ins(51, dst(3), src(3), src(14))
        w[2121:2121] = code
    return transform_shex(data, patch)

def write_header(root):
    out = root / 'build/gi_spatial_history.generated.h'
    out.parent.mkdir(exist_ok=True)
    text = '#pragma once\n'
    for i in range(2):
        source = base64.b64decode((root / f'research/gi-temporal-r208/diffuse-spatial-{i}.dxbc.b64').read_bytes())
        for name, data in [(f'kGINativeSpatial{i}', source), (f'kGISingleHistorySpatial{i}', generate(source)), (f'kGIBoundedHistorySpatial{i}', generate(source, bounded=True))]:
            text += 'inline constexpr unsigned char '+name+'[]={\n'+','.join(map(str,data))+'\n};\n'
    out.write_text(text)

if __name__ == '__main__':
    write_header(Path(__file__).resolve().parents[1])
