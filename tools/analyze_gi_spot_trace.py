"""Validate R274/R275 bounded records. Never label partial terms final radiance."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import struct

CAPACITY, STRIDE = 65536, 256
FIELDS = {
    'coefficients': (16, 3, 1), 'visibility': (64, 1, 2),
    'light_rgb_range': (32, 4, 4), 'distance_squared': (48, 1, 8),
    'denominator': (52, 1, 16), 'attenuation': (56, 1, 32),
    'range_attenuation': (60, 1, 64), 'cookie_rgb': (80, 3, 128),
    'diffuse_factor': (96, 1, 256), 'gated_rgb': (112, 3, 512),
    'secondary_albedo': (128, 3, 1024), 'primary_throughput': (144, 3, 2048),
    'secondary_position': (160, 3, 4096), 'secondary_normal': (176, 3, 8192),
}

def parse(data):
    if len(data) < 16:
        raise ValueError('Missing header')
    count, *reserved = struct.unpack_from('<4I', data)
    if any(reserved):
        raise ValueError('Unexpected header version/reserved fields')
    if len(data) != 16 + min(count, CAPACITY) * STRIDE:
        raise ValueError('Truncated or trailing data')
    records = []
    for offset in range(16, len(data), STRIDE):
        x, y, ray, light = struct.unpack_from('<4I', data, offset)
        mask = struct.unpack_from('<I', data, offset + 192)[0]
        if mask & ~16383 or not mask & 1:
            raise ValueError('Unknown/missing validity bits')
        if x % 32 != 16 or y % 32 != 16:
            raise ValueError('Unexpected sampling coordinates')
        record = dict(pixel=[x, y], ray=ray, light=light, validity=mask)
        for name, (pos, width, bit) in FIELDS.items():
            if mask & bit:
                values = list(struct.unpack_from('<' + 'f' * width, data, offset + pos))
                if not all(math.isfinite(v) for v in values):
                    raise ValueError(f'Nonfinite valid {name} at record {len(records)}')
                record[name] = values
        records.append(record)
    return count, records

def analyze(data):
    count, records = parse(data)
    complete = [r for r in records if r['validity'] == 16383]
    errors, denominator_errors, terms = [], [], []
    for r in complete:
        c68, c72, c76 = r['coefficients']
        d = c76 * r['distance_squared'][0] + c72 * c72
        expected = max(1 / (math.pi * max(d, .25)) - c68, 0)
        denominator_errors.append(abs(d - r['denominator'][0]))
        errors.append(abs(expected - r['attenuation'][0]))
        # This is one light's material-weighted term BEFORE final depth/count scaling.
        term = [r['diffuse_factor'][0] * g * a * t for g, a, t in
                zip(r['gated_rgb'], r['secondary_albedo'], r['primary_throughput'])]
        terms.append((max(term), r, term))
    ordered = sorted(terms, key=lambda item: item[0], reverse=True)
    total = sum(t[0] for t in terms)
    per_light = []
    for light in sorted({r['light'] for r in records}):
        group = [r for r in records if r['light'] == light]
        per_light.append(dict(light=light, records=len(group),
                              validity_counts=dict(sorted(Counter(r['validity'] for r in group).items()))))
    return dict(schema='gi-spot-trace-analysis-v1', sha256=hashlib.sha256(data).hexdigest(),
        attempted_records=count, retained_records=len(records), overflow=count > CAPACITY,
        sampled_pixels=len({tuple(r['pixel']) for r in records}),
        ray_indices=sorted({r['ray'] for r in records}),
        validity_counts=dict(sorted(Counter(r['validity'] for r in records).items())),
        per_light=per_light, full_stage_records=len(complete),
        arithmetic=dict(denominator_max_abs_error=max(denominator_errors, default=None),
                        attenuation_max_abs_error=max(errors, default=None)),
        partial_term_concentration=dict(metric='max RGB per record, before depth/ray normalization',
            top10_share=sum(t[0] for t in ordered[:10]) / total if total else None,
            distance_squared_below_one_share=sum(t[0] for t in terms if t[1]['distance_squared'][0] < 1) / total if total else None),
        brightest_partial_terms=[dict(pixel=r['pixel'], ray=r['ray'], light=r['light'],
            rgb=rgb, distance_squared=r['distance_squared'][0]) for _, r, rgb in ordered[:10]],
        limitations=['One dispatch; no temporal variance or unbiased reference measured.',
            'Grid sample; no material IDs or aligned image for hair/plant classification.',
            'Invalid stages are absent, not zero-valued observations.',
            'Partial term concentration is not final image energy or proof of faulty sampling.',
            'Spotlight records do not count rays that never entered a spotlight loop.',
            'Light indices are session-local, not identified scene objects.',
            'Header does not contain frame/dimensions/shader identity; pair with export log.'])

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('trace', type=Path)
    p.add_argument('--output', type=Path)
    args = p.parse_args()
    result = json.dumps(analyze(args.trace.read_bytes()), indent=2, allow_nan=False) + '\n'
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(result)
    else:
        print(result, end='')
