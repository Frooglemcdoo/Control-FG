"""Camera invariance, ambiguous geometry and complete-trace provenance checks."""
import copy
import importlib.util
import struct
from pathlib import Path
spec=importlib.util.spec_from_file_location('transition',Path(__file__).with_name('analyze-light-transition.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
identity=[[1,0,0,0],[0,1,0,0],[0,0,1,0],[0,0,0,1]]
base=dict(cone_view=[3,4,5],emitter_view=[3,4,6],direction_view=[0,0,1],
          near_distance=1,far_distance=10,far_width=2,radius=.5,sin_half_cone_squared=.25,view_to_world=identity)
rotated=copy.deepcopy(base)
rotated.update(cone_view=[4,7,5],emitter_view=[4,7,6],direction_view=[0,0,1],
               view_to_world=[[0,1,0,0],[-1,0,0,0],[0,0,1,0],[10,0,0,1]])
a=m.light_geometry(base);b=m.light_geometry(rotated)
assert a==b
for field,value in [('emitter_view',[float('nan'),0,0]),('direction_view',[0,0,0]),('view_to_world',[[0,0,0,0]]*4)]:
 bad=copy.deepcopy(base);bad[field]=value
 try:m.light_geometry(bad)
 except ValueError:pass
 else:raise AssertionError('Invalid geometry accepted')
def light(index,geometry):return dict(index=index,geometry=geometry,selected_samples=5)
p=dict(lights=[light(7,a)]);c=dict(lights=[light(12,b)])
assert m.compare(p,c,.02)[0]['status']=='same_geometry_renumbered'
assert m.compare(p,p,.02)[0]['status']=='same_geometry_same_index'
assert m.compare(dict(lights=[light(7,a),light(8,a)]),c,.02)[0]['status']=='ambiguous_geometry'
assert all(x['status']=='ambiguous_geometry' for x in m.compare(p,dict(lights=[light(12,a),light(13,a)]),.02))
changed=copy.deepcopy(a);changed['emitter'][0]+=1
assert m.compare(p,dict(lights=[light(12,changed)]),.02)[0]['status']=='not_matched_in_previous_sample'

def block(n):
 frame=20+n
 record=m.decoder.RECORD.pack(5,0xffffffff,0xffffffff,0xffffffff,0,0,0,0xffffffff,
                             100,20,-30,5,0,-1,0,7+n,1,1,frame,1)
 context=struct.pack('<32f',3,4,5,1,3,4,6,10,0,0,1,2,0,0,.5,.25,*sum(identity,[]))
 return (f'ms=1 transition_sample_requested sample={n} mode=default_no_spot_history seq=7 present={100+n*10}\n'
         f'ms=2 rt_hit_gpu_recorded generation={n+1} frame_tag={frame} extent=128x128 table_records=2 captured_present={101+n*10} light_context_bytes=128\n'
         f'ms=3 rt_hit_gpu_complete generation={n+1} frame_tag={frame} unknown=0 translated=1 null=1 invalid_layout=0 valid_rays=1 hits=0 misses=1 invalid_records=0 gpu_completion_confirmed=1 gpu_hit_capture=1\n'
         'ms=4 rt_hit_gpu_record index=0 bytes='+record.hex()+' light_context='+context.hex()+'\n'
         f'ms=5 rt_hit_gpu_export_complete records_examined=4096 frame_tag={frame}\n')
text='ms=0 transition_trace_start samples=6 mode=default_no_spot_history\n'+''.join(block(i) for i in range(6))+'ms=6 capture_suite_end reason=complete failures=0 restored=default_no_spot_history\n'
result=m.analyze(text);assert len(result['captures'])==6
assert all(c['geometry_matches'][0]['status']=='same_geometry_renumbered' for c in result['captures'][1:])
for bad in [text.replace(block(2),''),text.replace('failures=0','failures=1'),
            text.replace('sample=2 mode=default_no_spot_history seq=7','sample=2 mode=default_no_spot_history seq=8'),
            text.replace('captured_present=121','captured_present=101'),
            text.replace('gpu_completion_confirmed=1','gpu_completion_confirmed=0'),text.replace(block(2),block(1)),
            text.replace('ms=4 rt_hit_gpu_record index=0','ms=4 rt_hit_gpu_failed reason=test\nms=4 rt_hit_gpu_record index=0',1)]:
 try:m.analyze(bad)
 except ValueError:pass
 else:raise AssertionError('Invalid trace accepted')
print('PASS: world-space camera invariance, renumbering candidates, ambiguity, six-sample export and provenance rejection')
