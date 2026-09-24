"""Compare GPU-completed FP16 tile samples; requires Python and numpy.
Usage: python tools/analyze-pixels.py PATH_TO_EXTRACTED_CAPTURE_DIRECTORY
Exact RGB matches are stronger than nearest-image scores. Static tiles can be ambiguous.
"""
from pathlib import Path
import json,sys
import numpy as np

def analyze(root):
    groups={}
    for p in sorted(Path(root).rglob('b*-f*-*.json')):
        m=json.loads(p.read_text());raw=p.with_suffix('.raw').read_bytes()
        if len(raw)!=589824 or m['format']!=10 or m['tiles']!=9 or m['tile_width']!=128 or m['tile_height']!=64 or m['row_pitch']!=1024:
            raise ValueError('Unexpected capture layout: '+str(p))
        rgb=np.frombuffer(raw,dtype='<f2').reshape(9,64,128,4)[...,:3].astype(np.float32)
        key=(str(p.parent),m['burst'],m['epoch'],m['width'],m['height'])
        groups.setdefault(key,[]).append((m,rgb))
    out=[]
    for key,items in groups.items():
        producers=[x for x in items if x[0]['role']=='producer'];bridges=[x for x in items if x[0]['role']=='bridge']
        comparisons=[]
        for bm,b in bridges:
            scores=[]
            for pm,p in producers:
                finite=np.isfinite(b)&np.isfinite(p)
                err=float(np.abs(b[finite]-p[finite]).mean()) if finite.any() else None
                scores.append({'producer_frame':pm['frame'],'offset':bm['frame']-pm['frame'],'exact_rgb':bool(np.array_equal(b,p)),'mean_abs_rgb_error':err,'finite_components':int(finite.sum())})
            exact=[x['offset'] for x in scores if x['exact_rgb']]
            comparisons.append({'bridge_frame':bm['frame'],'exact_matching_offsets':exact,'ambiguous_exact_match':len(exact)>1,'comparisons':scores})
        out.append({'folder':key[0],'burst':key[1],'epoch':key[2],'producer_count':len(producers),'bridge_count':len(bridges),'consecutive_producers':sorted(x[0]['frame'] for x in producers)==list(range(min((x[0]['frame'] for x in producers),default=0),min((x[0]['frame'] for x in producers),default=0)+len(producers))),'results':comparisons})
    return {'bursts':out,'interpretation':'Positive offset means bridge samples match an earlier producer frame. Compare healthy and failing bursts; static imagery, post-processing differences, or incomplete capture can make results inconclusive. Samples are nine tiles, not the full image.'}
if __name__=='__main__':print(json.dumps(analyze(sys.argv[1]),indent=2,allow_nan=False))
