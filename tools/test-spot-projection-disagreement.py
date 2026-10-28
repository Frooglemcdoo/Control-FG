"""Analytic perspective fixtures with known apex and plane geometry."""
import copy
import importlib.util
from pathlib import Path
import numpy as np
spec=importlib.util.spec_from_file_location('analysis',Path(__file__).with_name('analyze-spot-projection-disagreement.py'))
a=importlib.util.module_from_spec(spec);spec.loader.exec_module(a)
r=dict(light_context=dict(view_to_world=np.eye(4).tolist(),cone_view=[0,0,0],emitter_view=[2,0,0],direction_view=[0,0,1],near_distance=1),
    spot_record=dict(projection_rows=[[1,0,0,0],[0,1,0,0],[0,0,1,1],[0,0,-1,0]]),
    origin=[0,0,4],direction=[0,0,-1],hit=True,hit_position=[0,0,2],hit_distance=2)
x=a.geometry(r)
assert x['apex_cone_residual']==0 and x['apex_emitter_distance']==2
assert x['angle_degrees']==0 and x['hit_plane_margin']==1 and x['path_separation_at_hit_depth']==0
# A world translation must preserve distances, angles, and the clip-side test.
s=copy.deepcopy(r);s['light_context']['view_to_world'][3][:3]=[10,-20,30]
s['origin']=[10,-20,34];s['hit_position']=[10,-20,32]
assert a.geometry(s)==x
# Rotate the reference direction and put its hit on that ray: equal-depth
# separation must be one, independent of the hit's distance from the origin.
s=copy.deepcopy(r);s['direction']=[1/np.sqrt(5),0,-2/np.sqrt(5)];s['hit_position']=[1,0,2];s['hit_distance']=np.sqrt(5)
y=a.geometry(s)
assert abs(y['angle_degrees']-26.565051177)<1e-7 and abs(y['path_separation_at_hit_depth']-1)<1e-12
s['hit_position']=[1,0,.5];assert a.geometry(s)['hit_plane_margin']==-.5
for matrix in [np.zeros((4,4)).tolist(),np.full((4,4),float('nan')).tolist()]:
 s=copy.deepcopy(r);s['spot_record']['projection_rows']=matrix
 try:a.geometry(s)
 except ValueError:pass
 else:raise AssertionError('Invalid projection accepted')
s=copy.deepcopy(r);s['hit']=False
assert 'hit_plane_margin' not in a.geometry(s)
print('PASS projection geometry: known apex, translation, angle, depth intersection, clipping sign, misses and invalid matrices')
