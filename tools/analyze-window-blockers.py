"""R198: classify native-map blockers missed by the normal RT reference ray."""
import argparse
from collections import Counter
import importlib.util
import json
from pathlib import Path
spec=importlib.util.spec_from_file_location('map_probe',Path(__file__).with_name('analyze-map-visibility-probe.py'))
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)

def classify(record):
    if record['hit']:return 'native_hit'
    pair=record.get('eligibility')
    if not pair:raise ValueError('Missing R198 eligibility probes; R197 captures cannot answer this comparison')
    if pair['mask1_opaque']['hit']:return 'same_mask_geometry_hit'
    if pair['mask255_opaque']['hit']:return 'wider_mask_only_hit'
    return 'both_geometry_queries_miss'

def analyze(text):
    capture=probe.decoder.decode_log(text)
    counts=Counter();targets=[];all_misses=Counter()
    for r in capture['records']:
        if not r['valid']:continue
        if not r['hit']:all_misses[classify(r)]+=1
        item=probe.sample(r)
        if item['status']!='valid':raise ValueError('Invalid matched map payload')
        if item['map_visibility']<.1 and item['rt_visibility']>.9:
            result=classify(r);counts[result]+=1
            targets.append(dict(record=r,classification=result))
    return dict(schema='control-window-blockers-r198',capture_id=capture['capture_id'],counts=dict(counts),all_reference_misses=dict(all_misses),records=targets,
        limitations='Geometry-only probes force triangles opaque and skip procedural primitives. Same-mask hits implicate native material/opacity handling, not a specific material bug. Wider-mask-only hits establish a mask-excluded blocker. Both misses do not prove culling: mask-zero instances, absent triangles, different map casters, or endpoint/path differences remain possible. Do not use force-opaque or mask255 as a gameplay fix.')

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    result=analyze(a.log.read_text(encoding='utf-8-sig'));a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'}))
