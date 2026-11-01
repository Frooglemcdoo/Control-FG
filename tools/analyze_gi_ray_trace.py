"""Validate complete R277 ray sets against their same-dispatch final GI result."""
import argparse
from collections import defaultdict
import hashlib
import json
import math
from pathlib import Path
import struct

CAPACITY, STRIDE = 65536, 256


def analyze(data):
    if len(data) < 16:
        raise ValueError('Missing header')
    count, *reserved = struct.unpack_from('<4I', data)
    if any(reserved) or len(data) != 16+min(count,CAPACITY)*STRIDE:
        raise ValueError('Unknown header or malformed length')
    if count > CAPACITY:
        raise ValueError('Overflow: cannot certify complete per-ray sets')
    groups = defaultdict(lambda: dict(rays={}, summary=None))
    nonfinite = 0
    for offset in range(16,len(data),STRIDE):
        x,y,ray,kind = struct.unpack_from('<4I',data,offset)
        if x%32 != 16 or y%32 != 16 or kind not in (1,2):
            raise ValueError('Unknown record kind or sampling coordinate')
        def fs(at,n=3): return list(struct.unpack_from('<'+'f'*n,data,offset+at))
        material, rays = struct.unpack_from('<2I',data,offset+144)
        primary = struct.unpack_from('<I',data,offset+108)[0]
        if not 1 <= rays <= 64:
            raise ValueError('Invalid configured ray count')
        record = dict(ray_count=rays,primary_material=primary,fade_constants=fs(160,4))
        if kind == 1:
            if ray >= rays or ray in groups[x,y]['rays']:
                raise ValueError('Duplicate or out-of-range ray')
            record.update(material=material,contribution=fs(16),secondary_radiance=fs(32),
                          primary_throughput=fs(48),accumulation=fs(64),primary_position_depth=fs(80,4),
                          primary_normal=fs(96),secondary_position=fs(112),secondary_normal=fs(128))
            groups[x,y]['rays'][ray] = record
        else:
            if ray != 0xffffffff or groups[x,y]['summary'] is not None:
                raise ValueError('Duplicate or malformed summary')
            record.update(output=fs(176,4),accumulation=fs(192),scale=fs(208,1))
            groups[x,y]['summary'] = record
        nonfinite += sum(not math.isfinite(v) for value in record.values() if isinstance(value,list) for v in value)
    if not groups:
        raise ValueError('No sampled pixels')
    if nonfinite:
        raise ValueError(f'{nonfinite} nonfinite valid components')
    peak_sum = peak_output = peak_scale = 0.
    misses = 0
    brightest = []
    normalized_terms = []
    dominant_pixels = 0
    luminance = lambda rgb: sum(a*b for a,b in zip(rgb,(.2126,.7152,.0722)))
    for pixel, group in sorted(groups.items()):
        summary = group['summary']
        if summary is None or set(group['rays']) != set(range(summary['ray_count'])):
            raise ValueError(f'Incomplete ray set at {pixel}')
        total = [0.,0.,0.]
        pixel_terms = []
        for index,ray in sorted(group['rays'].items()):
            if any(ray[k] != summary[k] for k in ('ray_count','primary_material','fade_constants')):
                raise ValueError('Inconsistent per-pixel constants')
            total = [a+b for a,b in zip(total,ray['contribution'])]
            peak_sum = max(peak_sum,*(abs(a-b) for a,b in zip(total,ray['accumulation'])))
            if ray['material'] >= 65534:
                misses += 1
                if any(ray['secondary_normal']):
                    raise ValueError('Miss/invalid record contains a surface normal')
            brightest.append(dict(pixel=list(pixel),ray=index,primary_material=ray['primary_material'],
                                  secondary_material=ray['material'],contribution=ray['contribution']))
            term = luminance(ray['contribution'])*summary['scale'][0]/summary['ray_count']
            pixel_terms.append(term)
            normalized_terms.append((term,ray['material']))
        if sum(pixel_terms)>1e-8 and max(pixel_terms)>0.9*sum(pixel_terms):
            dominant_pixels += 1
        peak_sum = max(peak_sum,*(abs(a-b) for a,b in zip(total,summary['accumulation'])))
        reconstructed = [a*summary['scale'][0]/summary['ray_count'] for a in summary['accumulation']]
        peak_output = max(peak_output,*(abs(a-b) for a,b in zip(reconstructed,summary['output'])))
        x,y,z,w = summary['fade_constants']
        if y == 0:
            raise ValueError('Zero depth-fade denominator')
        depth = group['rays'][0]['primary_position_depth'][3]
        t = min(max((depth-(x-y))/(2*y),0),1)
        expected_scale = z+(w-z)*t*t*(3-2*t)
        peak_scale = max(peak_scale,abs(expected_scale-summary['scale'][0]))
    brightest.sort(key=lambda r:max(r['contribution']),reverse=True)
    energy = sum(v for v,m in normalized_terms)
    ordered = sorted((v for v,m in normalized_terms),reverse=True)
    return dict(schema='gi-ray-trace-analysis-r277',sha256=hashlib.sha256(data).hexdigest(),
                records=count,pixels=len(groups),rays=sum(len(g['rays']) for g in groups.values()),
                miss_or_invalid_rays=misses,complete_ray_sets=True,
                sampled_grid_concentration=dict(
                    metric='luminance after recorded depth scale and ray-count normalization',
                    luminance_sum=energy,
                    miss_fallback_share=sum(v for v,m in normalized_terms if m==65535)/energy if energy else None,
                    top10_ray_share=sum(ordered[:10])/energy if energy else None,
                    pixels_one_ray_over_90_percent=dominant_pixels),
                errors=dict(accumulation_max_absolute=peak_sum,output_max_absolute=peak_output,
                            depth_scale_max_absolute=peak_scale),brightest_rays=brightest[:20],
                limits=['One dispatch; not a temporal variance or converged reference measurement.',
                        'Output is the native float32 register before UAV format conversion.',
                        'Separate product versus native fused accumulation can differ by rounding.',
                        'Material IDs do not identify named scene objects.',
                        'Concentration describes this sampled grid, not full-image energy or temporal variance.',
                        'Misses contribute native fallback lighting; exclude from surface reuse, not from GI output.',
                        'Consumer geometry does not establish the ray producer PDF.'])


if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('trace',type=Path)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    a.output.write_text(json.dumps(analyze(a.trace.read_bytes()),indent=2,allow_nan=False)+'\n')
