"""Compile the exact frozen GI runtime bundle; no downloaded shader binaries."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--dxc', required=True)
    parser.add_argument('--output', default=str(ROOT / 'build'))
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    evaluator = output / 'gi-frozen-evaluator.hlsl'
    subprocess.run([sys.executable, str(ROOT / 'tools/gi_frozen_evaluator.py'), '--output', str(evaluator)], check=True)
    stages = [('Raygen', ROOT / 'src/shaders/gi_frozen_raygen.hlsl', 'lib_6_3', None),
              ('Evaluator', evaluator, 'cs_6_0', 'main'),
              ('Constants', ROOT / 'src/shaders/gi_ray_constant_snapshot.hlsl', 'cs_6_0', 'main')]
    stages += [(name, ROOT / 'src/shaders/gi_frozen_guide_build.hlsl', 'cs_6_0', name)
               for name in ('Clear', 'Claim', 'Accumulate', 'Freeze')]
    arrays, manifest = [], {}
    for name, source, target, entry in stages:
        binary = output / ('gi-frozen-' + name.lower() + '.dxil')
        command = [args.dxc, '-T', target, '-HV', '2018', '-O3', '-WX',
                   '-I', str(ROOT / 'src/shaders'), str(source), '-Fo', str(binary)]
        if entry:
            command += ['-E', entry]
        subprocess.run(command, check=True)
        data = binary.read_bytes()
        if len(data) < 32 or data[:4] != b'DXBC':
            raise ValueError(f'Invalid compiled container: {name}')
        arrays.append('inline constexpr unsigned char k' + name + '[]={' + ','.join(map(str, data)) + '};')
        manifest[name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    header = '#pragma once\nnamespace gi_frozen_shader {\n' + '\n'.join(arrays) + '\n}\n'
    (output / 'gi_frozen.generated.h').write_text(header)
    (output / 'gi-frozen-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('Compiled and embedded seven frozen GI stages; runtime activation remains separately gated.')


if __name__ == '__main__':
    main()
