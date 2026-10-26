"""R172: sink two immutable material SRV loads; no shading instruction changes."""
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
        if len(a) != 984 or any((a[i][0] & 2047) != 167 for i in (405, 406)):
            raise ValueError('Unexpected material load layout')
        # Keep exactly the original instructions, moving only the two loads.
        # Both sites have the same enclosing IF/LOOP stack. T4 is a read-only
        # material SRV; r5.w is unchanged; no crossed instruction touches the
        # destination components. Independently checked by the audit test.
        b = a[:405] + a[407:943] + a[405:407] + a[943:]
        words[:] = [words[0], words[1], *sum(b, [])]
    return transform_shex(data, patch)


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--source', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--header', type=Path)
    args = p.parse_args()
    result = optimize(base64.b64decode(args.source.read_text()))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    if args.header:
        args.header.parent.mkdir(parents=True, exist_ok=True)
        args.header.write_text('inline constexpr unsigned char kSpecSerialLifetimesShader[] = {\n' +
                               ','.join(map(str, result)) + '\n};\n')
    print(json.dumps({'source_sha256': SOURCE_SHA,
                      'output_sha256': hashlib.sha256(result).hexdigest(),
                      'moved_loads': 2, 'crossed_instructions': 536,
                      'status': 'generated_not_gpu_validated'}))
