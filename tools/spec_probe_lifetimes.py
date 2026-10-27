"""R177: retain R172 and sink six probe-basis values past the nested lookup."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
from dxbc_transform import transform_shex
from spec_parallel import instructions, SOURCE_SHA


def optimize(data):
    if hashlib.sha256(data).hexdigest() != SOURCE_SHA:
        raise ValueError('Unsupported native SPEC shader')
    def patch(words):
        a = instructions(words)
        if len(a) != 984 or any((a[i][0] & 2047) != 167 for i in (201,202,405,406)):
            raise ValueError('Unexpected load layout')
        # Original instruction indices; no instruction/operand bytes change.
        order = [i for i in range(984) if i not in (201,202,405,406)]
        order[order.index(289):order.index(289)] = [201,202]
        order[order.index(943):order.index(943)] = [405,406]
        words[:] = [words[0], words[1], *sum((a[i] for i in order), [])]
    return transform_shex(data, patch)

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--header',type=Path,required=True)
    args=p.parse_args()
    result=optimize(base64.b64decode(args.source.read_text()))
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_bytes(result)
    args.header.write_text('inline constexpr unsigned char kSpecSerialLifetimesShader[] = {\n'+','.join(map(str,result))+'\n};\n')
    print(json.dumps(dict(source_sha256=SOURCE_SHA,output_sha256=hashlib.sha256(result).hexdigest(),
                         material_loads_moved=2,probe_loads_moved=2,probe_components=6,
                         probe_crossed_instructions=86,status='generated_not_gpu_validated')))
