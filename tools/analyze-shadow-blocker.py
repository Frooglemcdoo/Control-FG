"""Classify reference hit locations against the accepted authored clip plane.

This is offline evidence, not a replacement visibility shader. The captured ray
is one full-length reference segment, not the four-ray filtered shadow result.
"""
import argparse
from collections import Counter
import importlib.util
import json
import math
from pathlib import Path
from rt_spot_authored_clip import clip_distance, ENDPOINT_SCALE

spec=importlib.util.spec_from_file_location('transition',Path(__file__).with_name('analyze-light-transition.py'))
transition=importlib.util.module_from_spec(spec);spec.loader.exec_module(transition)


def classify(record):
    context=record['light_context']
    cone=transition.transform(context,'cone_view',True)
    axis=transition.transform(context,'direction_view',False)
    emitter=transition.transform(context,'emitter_view',True)
    # The native extractor stores length * ENDPOINT_SCALE. Recover that same
    # sampled endpoint, which need not coincide with the nominal light center.
    origin=record['origin'];full=record['t_max']
    endpoint=[o+d*full/ENDPOINT_SCALE for o,d in zip(origin,record['direction'])]
    near=context['near_distance']
    if not math.isfinite(near) or near<0 or sum(x*x for x in axis)<=0:
        return dict(classification='invalid_clip_context')
    clipped=clip_distance(origin,endpoint,cone,axis,near)
    tolerance=1e-4*max(1,full)
    status='miss'
    if record['hit']:
        gap=clipped-record['hit_distance']
        status='within_clip' if gap>tolerance else 'past_clip' if gap < -tolerance else 'clip_boundary_uncertain'
    return dict(classification=status,full_t_max=full,authored_t_max=clipped,
                authored_endpoint=[o+d*clipped for o,d in zip(origin,record['direction'])],
                sampled_endpoint=endpoint,nominal_emitter=emitter,cone=cone,axis=axis,near_distance=near,
                clip_shortened=clipped<full-tolerance,
                filtered_spot_visibility=record.get('composition',{}).get('visibility',[None])[0])


def analyze(capture):
    counts=Counter();groups={};samples=[]
    for record in capture['records']:
        if not record['valid']:continue
        if 'light_context' not in record or 'composition' not in record:
            raise ValueError('Blocker analysis requires light context and composition visibility')
        item=classify(record);counts[item['classification']]+=1
        sample=dict(record);sample['authored_clip_analysis']=item;samples.append(sample)
        if record['hit']:
            key=(record['light_id'],record['instance_index'],record['instance_id'],record['geometry_index'])
            group=groups.setdefault(key,dict(light_id=key[0],instance_index=key[1],instance_id=key[2],geometry_index=key[3],samples=0,classifications=Counter(),primitives=set()))
            group['samples']+=1;group['classifications'][item['classification']]+=1;group['primitives'].add(record['primitive_index'])
    for group in groups.values():group['primitives']=sorted(group['primitives'])
    return dict(schema='control-shadow-blocker-r159',capture_id=capture['capture_id'],extent=capture['extent'],
                limitations='Reference center segment with offline clip classification; not four-ray filtered visibility. IDs scoped to this capture. Boundary tolerance is diagnostic only.',
                counts=dict(counts),blockers=sorted(groups.values(),key=lambda g:-g['samples']),records=samples)


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('log',type=Path);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();text=args.log.read_text(encoding='utf-8-sig')
    if 'capture_kind=stationary_shadow_blocker' in text:
        raise ValueError('Use lighting.log, not manifest.txt')
    capture=transition.decoder.decode_log(text)
    result=analyze(capture);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result['counts']))

if __name__=='__main__':main()
