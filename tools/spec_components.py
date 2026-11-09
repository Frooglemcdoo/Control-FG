"""R182: isolate emission or remaining shading with the original per-hit cap.

All original hit shading still executes. These are attribution diagnostics,
not optimizations. Unknown native bytecode is rejected before transformation.
"""
import base64
import hashlib
import json
from pathlib import Path
from dxbc_transform import transform_shex
from spec_parallel import SOURCE_SHA, instructions, inst, src, dst, imm

ROOT = Path(__file__).resolve().parents[1]


def component(data, emission):
    if hashlib.sha256(data).hexdigest() != SOURCE_SHA:
        raise ValueError('Unsupported native SPEC shader')
    def patch(w):
        a = instructions(w)
        assert len(a) == 984 and a[41] == [0x02000068, 43]
        assert a[957][0] & 2047 == 14 and a[960][0] & 2047 == 55
        assert a[957][3:5] == src(29, 'xyxz')
        assert a[960][-2:] == src(29, 'xyxz')
        out = []
        for i, original in enumerate(a):
            if i in (405, 406):
                continue
            if i == 943:
                out += a[405] + a[406]  # accepted R172 scheduling
            if i == 404:
                # Environment misses belong to shading, never emission.
                out += inst('mov', dst(43, 'xyz'), imm(0) if emission else src(29, 'xyzx'))
            if i == 954:
                # Save the component before native total is formed in r29.
                out += inst('mov', dst(43, 'xyz'), src(19, 'xyzx') if emission else src(14, 'xywx'))
            ins = original.copy()
            if i == 41:
                ins[1] = 44
            if i == 957:
                ins[4] = 43
            if i == 960:
                ins[-1] = 43
            out += ins
        w[:] = [w[0], len(out)+2, *out]
    return transform_shex(data, patch)


def main():
    data = base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    build = ROOT/'build'; build.mkdir(exist_ok=True)
    header = '#pragma once\n'
    report = {'source_sha256': SOURCE_SHA, 'diagnostic_only': True, 'variants': {}}
    for name, emission in [('Emission', True), ('Shading', False)]:
        result = component(data, emission)
        (build/f'spec-component-{name.lower()}.dxbc').write_bytes(result)
        header += f'inline constexpr unsigned char kSpecComponent{name}[] = {{\n'+','.join(map(str, result))+'\n};\n'
        report['variants'][name] = {'sha256': hashlib.sha256(result).hexdigest(), 'bytes': len(result)}
    (build/'spec_components.generated.h').write_text(header)
    (build/'spec-components-validation.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
