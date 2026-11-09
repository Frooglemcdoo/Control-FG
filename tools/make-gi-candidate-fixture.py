"""Generate reproducible GPU inputs from a small, trace-derived JSON fixture."""
import argparse
import json
import struct
from pathlib import Path


def pack(fixture):
    words = []
    def bits(v): return struct.unpack('<I', struct.pack('<f', v))[0]
    for pixel in fixture['pixels']:
        record = [0] * 520
        record[:2] = [len(pixel['rays']), bits(pixel['scale'])]
        for i, ray in enumerate(pixel['rays']):
            record[4+i*8:12+i*8] = [*map(bits, ray['throughput']), ray['material'],
                                     *map(bits, ray['radiance']), i]
        record[516:519] = list(map(bits, pixel['expected']))
        words.extend(record)
    return words


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('fixture', type=Path)
    p.add_argument('output', type=Path)
    a = p.parse_args()
    fixture = json.loads(a.fixture.read_text())
    words = pack(fixture)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text('#pragma once\nstatic const unsigned int fixtureWords[] = {\n' +
        '\n'.join(','.join(hex(w)+'u' for w in words[i:i+16])+',' for i in range(0,len(words),16)) + '\n};\n')
