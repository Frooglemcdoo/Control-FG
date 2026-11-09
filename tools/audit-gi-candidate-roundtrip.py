"""Replay R277 native throughput/radiance through local selection accounting."""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
from analyze_gi_ray_trace import analyze
from gi_candidate_contract import Ray,choices,expectation


def audit(data):
    validated=analyze(data)
    pixels=defaultdict(lambda:dict(rays=[]))
    for offset in range(16,len(data),256):
        x,y,index,kind=struct.unpack_from('<4I',data,offset)
        read=lambda at,n=3:struct.unpack_from('<'+'f'*n,data,offset+at)
        if kind==1:
            material=struct.unpack_from('<I',data,offset+144)[0]
            pixels[x,y]['rays'].append(Ray(index,material,read(48),read(32)))
        else:
            pixels[x,y].update(count=struct.unpack_from('<I',data,offset+148)[0],scale=read(208,1)[0],output=read(176))
    max_error=0.;missing_fallback_error=0.;hit_count_error=0.;all_miss=0
    for pixel,p in pixels.items():
        outcomes=choices(pixel,p['rays'],p['count'],p['scale'])
        result=expectation(pixel,outcomes)
        max_error=max(max_error,*(abs(a-b) for a,b in zip(result,p['output'])))
        fallback=outcomes[0].fallback
        missing_fallback_error=max(missing_fallback_error,*fallback)
        hit_count=sum(r.material!=65535 for r in p['rays'])
        if hit_count:
            wrong=tuple((v-f)*p['count']/hit_count+f for v,f in zip(result,fallback))
            hit_count_error=max(hit_count_error,*(abs(a-b) for a,b in zip(wrong,result)))
        else:all_miss+=1
    if max_error>1e-6:
        raise ValueError(f'Round-trip changed native energy: {max_error}')
    return dict(schema='gi-local-candidate-roundtrip-v1',trace_sha256=hashlib.sha256(data).hexdigest(),
                pixels=len(pixels),rays=validated['rays'],all_miss_pixels=all_miss,
                expectation_max_absolute_error_vs_native=max_error,
                deliberate_negative_controls=dict(drop_miss_max_absolute_error=missing_fallback_error,
                    normalize_hits_by_hit_count_max_absolute_error=hit_count_error),
                limitations=['Offline same-pixel accounting prototype, not RTXDI integration.',
                    'Enumerates expectation over local candidate selection; does not measure convergence.',
                    'Preserves source-pixel weights; cannot be used at another pixel.',
                    'No temporal history, spatial reuse, game shader change, or quality/performance claim.'])


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('trace',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();result=audit(a.trace.read_bytes());a.output.write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
