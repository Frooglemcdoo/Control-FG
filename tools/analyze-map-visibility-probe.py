"""Compare selected native map visibility with accepted filtered RT visibility.

Only for R163 MAP PROBE captures. Diffuse-before RGB carries diagnostic data;
none of the resulting diffuse values or screenshots represents normal lighting.
"""
import argparse
from collections import Counter
import importlib.util
import json
import math
from pathlib import Path
import re

spec=importlib.util.spec_from_file_location('decoder',Path(__file__).with_name('decode-spot-hit-capture.py'))
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)


def sample(record):
    if not record['valid']:return {'status':'invalid_ray'}
    sidecar='native_map' in record
    rgb=(record['native_map'] if sidecar else record.get('composition',{}).get('diffuse_before',[]))[:3]
    if len(rgb)!=3 or not all(math.isfinite(x) for x in rgb):return {'status':'missing_payload'}
    visibility,complement,encoded_id=rgb
    light=record['light_id']
    # R11G11B10 can encode ID+1 exactly through 64 in the blue lane.
    # Reject larger indices rather than trusting an ambiguous rounded identity.
    if (not sidecar and light>=64) or light>=65535:return {'status':'id_precision_limit'}
    if abs(encoded_id-(light+1))>1e-4:return {'status':'id_mismatch'}
    if not 0<=visibility<=1 or not 0<=complement<=1 or abs(visibility+complement-1)>0.012:
        return {'status':'payload_changed'}
    rt=record['composition']['visibility'][0]
    if not math.isfinite(rt) or not 0<=rt<=1:return {'status':'invalid_rt_visibility'}
    return dict(status='valid',map_visibility=visibility,rt_visibility=rt,
                map_minus_rt=visibility-rt,pixel=record['pixel'],light_id=light)


def analyze(text):
    starts=re.findall(r'blocker_trace_start mode=(\S+)',text)
    sidecar=len(starts)==1 and starts[0] in ('native_pcf_blend','native_map_guard','native_pcf_blend_r199')
    if not sidecar and starts!=['selected_map_probe_rr_off']:
        raise ValueError('Requires exactly one Native Blend or MAP PROBE blocker trace')
    capture=decoder.decode_log(text)
    if sidecar and any(r['valid'] and 'native_map' not in r for r in capture['records']):
        raise ValueError('Native Blend capture lacks private map payload')
    counts=Counter();records=[]
    for record in capture['records']:
        item=sample(record);counts[item['status']]+=1
        if item['status']=='valid':
            item['light_context']=record['light_context']
            item['hit_position']=record.get('hit_position')
            records.append(item)
    if not records:raise ValueError('No validated map visibility payloads')
    return dict(schema='control-selected-map-sidecar-r196' if sidecar else 'control-selected-map-probe-r163',capture_id=capture['capture_id'],extent=capture['extent'],composition=starts[0],map_guard=starts[0]=='native_map_guard',
        counts=dict(counts),map_lit_rt_blocked=sum(r['map_visibility']>.9 and r['rt_visibility']<.1 for r in records),
        map_blocked_rt_lit=sum(r['map_visibility']<.1 and r['rt_visibility']>.9 for r in records),records=records,
        limitations=('Float32 private native-map output; displayed Native Blend lighting retained. ' if sidecar else 'Legacy diagnostic diffuse payload; blue-lane IDs >=64 rejected. ')+ 'Native map samples use native bias/filtering; RT is the accepted filtered result. Agreement does not prove the shadow is physically correct; disagreement does not alone distinguish caster, bias or filtering.')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('log',type=Path);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();result=analyze(a.log.read_text(encoding='utf-8-sig'))
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'}))

if __name__=='__main__':main()
