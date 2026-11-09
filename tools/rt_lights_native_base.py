"""Build non-spotlight replay stages for atomic per-receiver replacement.

Replay the producer into private diffuse/specular/selection UAVs, then replay
selected point lighting into those private diffuse/specular UAVs using the
ORIGINAL native selection texture. The real game outputs remain the fallback.
These shaders must never directly replace either original native dispatch.
"""
from pathlib import Path
import argparse
import base64
import hashlib
import json
from dxbc_transform import transform_shex
from rt_lights_native_extract import emit, dst, literal, opnames

PRODUCER = 'a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f'
SELECTED = '6af3f7403a4e33ef1c478c386de8d0f6534ba9b852a75a9302c9b760c926b621'


def replay(data, producer):
    expected = PRODUCER if producer else SELECTED
    if hashlib.sha256(data).hexdigest() != expected:
        raise ValueError('Unverified native replay input')

    def edit(words):
        out = words[:2]
        offset = 2
        found = False
        while offset < len(words):
            length = (words[offset] >> 24) & 127
            if not length:
                raise ValueError('Unexpected custom data in native shader')
            instruction = words[offset:offset+length]
            if producer and offset == 5685:
                if instruction[0] & 2047 != opnames.index('loop'):
                    raise ValueError('Spotlight loop moved')
                # All loop-carried accumulators are initialized before the loop.
                # Exit before any spot selection, shadow, BRDF or energy update.
                instruction += emit('break')
                found = True
            elif not producer and offset == 362:
                if instruction[0] & 2047 != opnames.index('ine'):
                    raise ValueError('Selected-light identity guard moved')
                # Preserve the point guard in r5.w and original selected point ID.
                instruction += emit('mov', dst(5, 'z'), literal(0))
                found = True
            out += instruction
            offset += length
        if not found:
            raise ValueError('Replay edit point absent')
        words[:] = out
    return transform_shex(data, edit)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('producer', type=Path)
    parser.add_argument('selected', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    manifest = {'purpose': 'private non-spotlight base replay; never replace native outputs',
                'native_producer_sha256': PRODUCER, 'native_selected_sha256': SELECTED, 'outputs': {}}
    for source, producer, name in ((args.producer, True, 'rt_lights_native_base.dxbc'),
                                   (args.selected, False, 'rt_lights_native_point.dxbc')):
        data = replay(source.read_bytes(), producer)
        (args.output / (name + '.b64')).write_text(base64.b64encode(data).decode() + '\n')
        manifest['outputs'][name] = {'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}
    (args.output / 'base-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps(manifest, indent=2))


if __name__ == '__main__':
    main()
