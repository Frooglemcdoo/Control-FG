"""Decode bounded motion traces and match light geometry, never index colors.

Matches are geometric candidates, not persistent engine identities. A light absent
from the 64x64 selected-light sample grid is not proven absent from the scene.
"""
import argparse
import importlib.util
import json
import math
import re
from pathlib import Path

spec = importlib.util.spec_from_file_location('spot_decoder', Path(__file__).with_name('decode-spot-hit-capture.py'))
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)


def transform(context, field, point):
    matrix = context['view_to_world']
    values = context[field]
    if not all(math.isfinite(x) for row in matrix for x in row) or not all(math.isfinite(x) for x in values):
        raise ValueError('Nonfinite light/camera context')
    if any(abs(matrix[i][3]) > .001 for i in range(3)) or abs(matrix[3][3]-1) > .001:
        raise ValueError('Unsupported non-affine view-to-world matrix')
    for i in range(3):
        for j in range(3):
            dot = sum(matrix[i][k]*matrix[j][k] for k in range(3))
            if abs(dot-(1 if i==j else 0)) > .005:
                raise ValueError('Non-rigid camera transform')
    # Same column components as the captured native ray construction.
    return [sum(values[i]*matrix[i][j] for i in range(3)) + (matrix[3][j] if point else 0) for j in range(3)]


def light_geometry(context):
    direction = transform(context, 'direction_view', False)
    length = math.sqrt(sum(x*x for x in direction))
    if not .99 < length < 1.01:
        raise ValueError('Invalid light direction')
    shape = [context[k] for k in ('near_distance','far_distance','far_width','radius','sin_half_cone_squared')]
    if not all(math.isfinite(x) for x in shape):
        raise ValueError('Nonfinite light shape')
    return dict(emitter=transform(context,'emitter_view',True), cone=transform(context,'cone_view',True),
                direction=[x/length for x in direction], shape=shape)


def distance(a,b):
    return math.sqrt(sum((x-y)**2 for x,y in zip(a,b)))


def compatible(a,b,tolerance):
    return (distance(a['emitter'],b['emitter'])<=tolerance and
            distance(a['cone'],b['cone'])<=tolerance and
            distance(a['direction'],b['direction'])<=.002 and
            all(abs(x-y)<=.001*max(1,abs(x),abs(y)) for x,y in zip(a['shape'],b['shape'])))


def summarize(capture, tolerance):
    groups={}
    for record in capture['records']:
        if not record['valid']: continue
        if 'light_context' not in record: raise ValueError('Missing light context')
        geometry=light_geometry(record['light_context'])
        light=record['light_id']
        if light==65535: raise ValueError('Invalid selected light in valid capture')
        if light not in groups:
            groups[light]=dict(index=light,geometry=geometry,selected_samples=0,receiver_min=list(record['origin']),receiver_max=list(record['origin']))
        group=groups[light]
        if not compatible(geometry,group['geometry'],tolerance):
            raise ValueError('One frame/index refers to inconsistent light geometry')
        group['selected_samples']+=1
        group['receiver_min']=[min(x,y) for x,y in zip(group['receiver_min'],record['origin'])]
        group['receiver_max']=[max(x,y) for x,y in zip(group['receiver_max'],record['origin'])]
    if not groups: raise ValueError('No valid sampled lights')
    return dict(capture_id=capture['capture_id'],extent=capture['extent'],lights=list(groups.values()))


def compare(previous,current,tolerance):
    result=[]
    # Require an unambiguous match in both directions. Co-located authored lights
    # can share geometry; these records do not include a persistent engine UUID.
    for light in current['lights']:
        matches=[old for old in previous['lights'] if compatible(old['geometry'],light['geometry'],tolerance)]
        if len(matches)==1:
            old=matches[0]
            reverse=[new for new in current['lights'] if compatible(old['geometry'],new['geometry'],tolerance)]
            if len(reverse)==1:
                result.append(dict(current_index=light['index'],previous_index=old['index'],
                    status='same_geometry_same_index' if light['index']==old['index'] else 'same_geometry_renumbered',
                    selected_samples_before=old['selected_samples'],selected_samples_after=light['selected_samples']))
                continue
        result.append(dict(current_index=light['index'],status='ambiguous_geometry' if matches else 'not_matched_in_previous_sample'))
    return result


def analyze(text,tolerance=.02):
    if not math.isfinite(tolerance) or tolerance<=0: raise ValueError('Invalid matching tolerance')
    starts=list(re.finditer(r'^.* transition_trace_start samples=(\d+) mode=default_no_spot_history.*$',text,re.M))
    endings=list(re.finditer(r'^.* capture_suite_end reason=complete failures=0 .*$',text,re.M))
    if len(starts)!=1 or len(endings)!=1 or starts[0].end()>=endings[0].start():
        raise ValueError('Expected one complete motion trace with zero failures')
    text=text[starts[0].end():endings[0].start()]
    if ' rt_hit_gpu_failed ' in text: raise ValueError('GPU capture failed')
    blocks=re.findall(r'^[^\n]* rt_hit_gpu_recorded [^\n]*\n.*?^[^\n]* rt_hit_gpu_export_complete [^\n]*$',text,re.M|re.S)
    requests=re.findall(r' transition_sample_requested sample=(\d+) mode=default_no_spot_history seq=(\d+) present=(\d+)',text)
    expected=int(starts[0][1])
    if expected!=6 or len(blocks)!=expected or [int(x[0]) for x in requests]!=list(range(expected)) or len({x[1] for x in requests})!=1:
        raise ValueError('Missing samples, reordered requests or changed mode sequence')
    summaries=[];seen=set();previous_present=-1
    for block,request in zip(blocks,requests):
        # Trim leading telemetry so decoder sees only this completed export.
        lines=block.splitlines();begin=next(i for i,line in enumerate(lines) if ' rt_hit_gpu_recorded ' in line)
        block='\n'.join(lines[begin:])
        present=re.search(r'captured_present=(\d+)',lines[begin])
        if not present or int(present[1])<=previous_present or int(present[1])<int(request[2]):
            raise ValueError('Stale or non-monotonic captured frame')
        previous_present=int(present[1])
        capture=decoder.decode_log(block)
        if capture['capture_id'] in seen: raise ValueError('Replayed capture')
        seen.add(capture['capture_id'])
        summary=summarize(capture,tolerance);summary['captured_present']=previous_present
        if summaries: summary['geometry_matches']=compare(summaries[-1],summary,tolerance)
        summaries.append(summary)
    return dict(schema='control-light-transition-v1',position_tolerance_engine_units=tolerance,
        limitation='Geometry matches are candidates, not persistent IDs. Missing sampled lights do not prove culling. Hit rays are unclipped reference rays, not the final displayed visibility.',captures=summaries)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path);parser.add_argument('--position-tolerance',type=float,default=.02)
    args=parser.parse_args()
    print(json.dumps(analyze(args.log.read_text(encoding='utf-8-sig'),args.position_tolerance),indent=2))

if __name__=='__main__': main()
