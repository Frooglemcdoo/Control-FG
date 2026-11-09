"""Known geometry tests for hit/plane classification; never a gameplay sign-off."""
import copy,importlib.util,math
from pathlib import Path
spec=importlib.util.spec_from_file_location('blocker',Path(__file__).with_name('analyze-shadow-blocker.py'))
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
r=dict(valid=True,origin=[0,0,10],direction=[0,0,-1],t_max=10*a.ENDPOINT_SCALE,hit=True,hit_distance=5,
 light_id=7,instance_index=2,instance_id=42,geometry_index=3,primitive_index=99,
 light_context=dict(cone_view=[0,0,0],direction_view=[0,0,1],emitter_view=[0,0,0],near_distance=2,
 view_to_world=[[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]),composition=dict(visibility=[.25,1,0,0]))
x=a.classify(r);assert abs(x['authored_t_max']-8*a.ENDPOINT_SCALE)<1e-9 and x['classification']=='within_clip'
r['hit_distance']=9;assert a.classify(r)['classification']=='past_clip'
r['hit_distance']=8*a.ENDPOINT_SCALE;assert a.classify(r)['classification']=='clip_boundary_uncertain'
r['hit']=False;assert a.classify(r)['classification']=='miss'
r['hit']=True;r['hit_distance']=5
# Translating the camera/world basis must preserve distances and classification.
t=copy.deepcopy(r);t['origin']=[12,20,40];t['light_context']['view_to_world'][3]=[12,20,30,1]
y=a.classify(t);assert abs(y['authored_t_max']-x['authored_t_max'])<1e-9
# Rotating view +Z to world +X must retain the same clip distance.
t=copy.deepcopy(r);t['origin']=[10,0,0];t['direction']=[-1,0,0];t['light_context']['view_to_world']=[[0,0,-1,0],[0,1,0,0],[1,0,0,0],[0,0,0,1]]
assert abs(a.classify(t)['authored_t_max']-x['authored_t_max'])<1e-9
r['light_context']['near_distance']=-1;assert a.classify(r)['classification']=='invalid_clip_context'
r['light_context']['near_distance']=2
result=a.analyze(dict(capture_id='test',extent=[128,128],records=[r,dict(valid=False)]))
assert result['counts']=={'within_clip':1} and result['blockers'][0]['primitives']==[99]
# A filtered visibility value is retained as separate evidence, not inferred
# from this one diagnostic hit, and exact integer identities survive grouping.
assert result['records'][0]['authored_clip_analysis']['filtered_spot_visibility']==.25
print('PASS blocker classification: hits, misses, clip boundary, translated/rotated contexts, invalid parameters and identity grouping')
