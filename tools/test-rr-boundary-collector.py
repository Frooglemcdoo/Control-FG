import importlib.util
import json
from pathlib import Path
import tempfile
s=importlib.util.spec_from_file_location('collector',Path(__file__).with_name('collect-rr-boundary.py'))
c=importlib.util.module_from_spec(s);s.loader.exec_module(c)
with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for i in range(2):
  folder=root/f'rr-input-c1-7-{100+i}';folder.mkdir()
  resources=[dict(file=n,width=2,height=2,pixel_bytes=8,copied=True) for n in ['input-color.rgba16f','rr-output.rgba16f']]
  for r in resources:(folder/r['file']).write_bytes(bytes(32))
  data=dict(schema='control-rr-gi-r28',pid=7,capture_ordinal=1,pair_index=i,frame=100+i,previous_frame=100 if i else 0,consecutive=bool(i),output_captured=True,rr_result=1,buffers=2,resources=resources)
  (folder/'metadata.json').write_text(json.dumps(data))
 assert len(c.read_pair(root,7,1,0))==2
 assert c.read_pair(root,8,1,0) is None and c.read_pair(root,7,2,0) is None
 assert c.read_pair(root,7,1,10**12) is None
 try:c.read_pair(root,7,1,0,require_lighting=True)
 except ValueError:pass
 else:raise AssertionError('old capture accepted as lighting capture')
 for meta in root.glob('*/metadata.json'):
  extended=json.loads(meta.read_text());extended['lighting_buffers_captured']=True
  for name in ['lighting-diffuse.rgba16f','lighting-specular.rgba16f']:
   (meta.parent/name).write_bytes(bytes(32))
   extended['resources'].append(dict(file=name,width=2,height=2,pixel_bytes=8,copied=True))
  extended['buffers']=len(extended['resources']);meta.write_text(json.dumps(extended))
 assert len(c.read_pair(root,7,1,0,require_lighting=True))==2
 second=root/'rr-input-c1-7-101'/'metadata.json';data=json.loads(second.read_text())
 for field,value in [('output_captured',False),('previous_frame',99),('consecutive',False),('rr_result',0xBAD00001)]:
  changed=dict(data);changed[field]=value;second.write_text(json.dumps(changed))
  try:c.read_pair(root,7,1,0)
  except ValueError:pass
  else:raise AssertionError(field)
 second.write_text(json.dumps(data));(second.parent/'rr-output.rgba16f').write_bytes(bytes(4))
 try:c.read_pair(root,7,1,0)
 except ValueError:pass
 else:raise AssertionError('truncated image accepted')
print('PASS R151 RR collector: session/ordinal separation, fresh complete consecutive pairs, output failures and truncated images rejected')

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 assert 'No probe log' in c.capture_status(root,7)
 log=root/'probe-test-7.log';log.write_text('unrelated\n')
 assert 'unconfirmed' in c.capture_status(root,7)
 log.write_text('qpc=1 RR_INPUT_CAPTURE_C1_REQUEST accepted=1\nqpc=2 RR_INPUT_CAPTURE_R28_REJECT reason=raw_gi_read_state_unproven\n')
 assert 'raw_gi_read_state_unproven' in c.capture_status(root,7)
 attempt=root/'rr-input-c1-7-100';attempt.mkdir();(attempt/'metadata.pending').write_text('{}')
 out=root/'out';out.mkdir();c.retain_metadata(root,7,out)
 assert (out/'attempt-metadata'/attempt.name/'metadata.pending').read_text()=='{}'
print('PASS R151 helper fix: bounded capture progress, missing shortcut evidence and incomplete metadata preservation')

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for i in range(2):
  folder=root/f'rr-input-c1-9-{200+i}';folder.mkdir()
  roles=['input-color.rgba16f','native-sr-output.rgba16f','lighting-diffuse.rgba16f','lighting-specular.rgba16f']
  resources=[dict(file=n,width=2,height=2,pixel_bytes=8,copied=True) for n in roles]
  for name in roles:(folder/name).write_bytes(bytes(32))
  data=dict(schema='control-rr-gi-r28',pid=9,capture_ordinal=1,pair_index=i,frame=200+i,previous_frame=200 if i else 0,consecutive=bool(i),output_captured=True,rr_result=1,buffers=4,resources=resources,lighting_buffers_captured=True,evaluation_path='native_sr',rr_active=False)
  (folder/'metadata.json').write_text(json.dumps(data))
 assert len(c.read_pair(root,9,1,0,require_lighting=True,require_native=True))==2
 meta=folder/'metadata.json'
 for field,value in [('rr_active',True),('evaluation_path','rr')]:
  changed=dict(data);changed[field]=value;meta.write_text(json.dumps(changed))
  try:c.read_pair(root,9,1,0,require_lighting=True,require_native=True)
  except ValueError:pass
  else:raise AssertionError('RR capture accepted as native')
 meta.write_text(json.dumps(data));(folder/'lighting-specular.rgba16f').unlink()
 try:c.read_pair(root,9,1,0,require_lighting=True,require_native=True)
 except ValueError:pass
 else:raise AssertionError('missing lighting buffer accepted')
print('PASS R154 native capture: no RR guides/output required; RR-active, wrong path and missing accumulation rejected')

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp)
 for i in range(2):
  folder=root/f'rr-input-c1-10-{300+i}';folder.mkdir()
  roles=['input-color.rgba16f','rr-output.rgba16f','lighting-diffuse.rgba16f','lighting-specular.rgba16f',
         'normal-roughness.rgba16f','diffuse.rgba16f','specular.rgba16f',
         'native-gbuffer1.rgba8','native-gbuffer2.rgba8','input-depth.r32f','input-motion.rg16f']
  resources=[]
  for name in roles:
   size=8 if name.endswith('rgba16f') else 4
   resources.append(dict(file=name,width=2,height=2,pixel_bytes=size,dxgi_format=10 if size==8 else 0,copied=True))
   (folder/name).write_bytes(bytes(4*size))
  data=dict(schema='control-rr-gi-r28',pid=10,capture_ordinal=1,pair_index=i,frame=300+i,
            previous_frame=300 if i else 0,consecutive=bool(i),output_captured=True,rr_result=1,
            buffers=len(resources),resources=resources,lighting_buffers_captured=True,
            evaluation_path='rr',rr_active=True,reset=0,width=2,height=2)
  (folder/'metadata.json').write_text(json.dumps(data))
 def read():return c.read_pair(root,10,1,0,require_lighting=True,require_rr=True)
 assert len(read())==2
 meta=folder/'metadata.json'
 for field,value in [('rr_active',False),('evaluation_path','native_sr'),('reset',1),('width',3)]:
  changed=dict(data);changed[field]=value;meta.write_text(json.dumps(changed))
  try:read()
  except ValueError:pass
  else:raise AssertionError('Invalid RR capture admitted: '+field)
 for missing in ['normal-roughness.rgba16f','specular.rgba16f','diffuse.rgba16f','native-gbuffer1.rgba8','input-motion.rg16f']:
  changed=dict(data);changed['resources']=[r for r in resources if r['file']!=missing];changed['buffers']=len(changed['resources'])
  meta.write_text(json.dumps(changed))
  try:read()
  except ValueError:pass
  else:raise AssertionError('Missing guide/input admitted: '+missing)
 changed=json.loads(json.dumps(data));changed['resources'][4]['dxgi_format']=2;meta.write_text(json.dumps(changed))
 try:read()
 except ValueError:pass
 else:raise AssertionError('Wrong guide format admitted')
 meta.write_text(json.dumps(data))
 assert len(read())==2
 try:c.read_pair(root,10,1,0,require_native=True,require_rr=True)
 except ValueError:pass
 else:raise AssertionError('Conflicting path requirements admitted')
print('PASS R180 RR materials: active settled RR pair, exact guide formats/extents, required native inputs, wrong paths/resets/missing guides rejected')

# R216: compare actual applied filter settings and same-frame resource identities.
from copy import deepcopy
base = dict(frame=400, resources=[dict(file='raw-gi.rgba16f',resource_identity=123)],
            gi_boundary=dict(valid=True,frame=400,applied=True,stage='clamp_only',view='normal',
                             cleanup=True,filter_build='R217',spatial_passes=0,filter_color_identity=123))
c.validate_gi_pair([(None,base)],1)
assert base['gi_filter_color_matches']==['raw-gi.rgba16f']
for field,value in [('valid',False),('frame',399),('applied',False),('stage','both'),('view','zero_gi'),('cleanup',False),('spatial_passes',6),('filter_color_identity',0)]:
 changed=deepcopy(base);changed['gi_boundary'][field]=value
 try:c.validate_gi_pair([(None,changed)],1)
 except ValueError:pass
 else:raise AssertionError(field)
both=deepcopy(base);both['gi_boundary'].update(stage='both',cleanup=False,spatial_passes=6,filter_color_identity=456)
c.validate_gi_pair([(None,both)],2)
assert both['gi_filter_color_matches']==[] # Unknown provenance remains explicit.
try:c.validate_gi_pair([(None,base),(None,both)],1)
except ValueError:pass
else:raise AssertionError('Mixed-stage pair admitted')
print('PASS R216: applied frame-matched GI observations, stage order, normal view and buffer provenance')

# R263: early copy and actual RR-input GI must describe the same evaluated frame.
seq=dict(frame=800,capture_ordinal=1,gi_sequence_build='R263',sequence_pairs=4,
         pre_gi_captured=True,pre_gi_frame=800,pre_gi_tick_ms=100,evaluation_tick_ms=101,
         projection_valid=True,motion_scale_valid=True,width=16,height=8,preset=6,lighting_mode='native_blend',
         gi_boundary=dict(valid=True,frame=800,applied=True,stage='raw_bypass',view='normal',cleanup=False,
                          filter_build='R263',spatial_passes=0,spatial_observed=0,filter_color_identity=123),
         resources=[dict(file=name,dxgi_format=10,pixel_bytes=8,width=16,height=8,resource_identity=123)
                    for name in ['pre-filter-gi.rgba16f','gi-at-rr-input.rgba16f']])
second=deepcopy(seq);second['frame']=second['pre_gi_frame']=second['gi_boundary']['frame']=801
pair=[(None,seq),(None,second)];c.validate_gi_sequence_pair(pair,1)
for key,value in [('pre_gi_frame',799),('pre_gi_captured',False),('pre_gi_tick_ms',102),('gi_sequence_build','R262'),('motion_scale_valid',False),('projection_valid',False)]:
 bad=deepcopy(pair);bad[0][1][key]=value
 try:c.validate_gi_sequence_pair(bad,1)
 except ValueError:pass
 else:raise AssertionError(key)
for key,value in [('stage','both_steady_spatial'),('applied',False),('spatial_observed',1),('filter_color_identity',999)]:
 bad=deepcopy(pair);bad[0][1]['gi_boundary'][key]=value
 try:c.validate_gi_sequence_pair(bad,1)
 except ValueError:pass
 else:raise AssertionError(key)
print('PASS R263 stationary sequence: same-frame early GI, verified bypass, actual RR GI identity, projection/motion metadata and pair configuration')

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);local=root/'local';diagnostics=local/'Control-FG'/'Diagnostics';diagnostics.mkdir(parents=True)
 probe=root/'probe';probe.mkdir();out=root/'out';out.mkdir()
 names=['GI-Writer-R305-7-1.json','GI-Writer-R305-7-1-bindings.bin','GI-Writer-R305-7-1-table.bin']
 for name in names:(diagnostics/name).write_bytes(b'whole-scene-writer')
 (diagnostics/'GI-Writer-R305-8-1.json').write_bytes(b'other-session')
 c.collect_logs(probe,local,7,out)
 assert {p.name for p in (out/'logs').iterdir()}==set(names)
 assert all((out/'logs'/name).read_bytes()==b'whole-scene-writer' for name in names)
print('PASS R305 collector includes complete writer triplets and excludes other process IDs')

with tempfile.TemporaryDirectory() as tmp:
 root=Path(tmp);name='GI-Writer-R305-7-1';metadata=root/(name+'.json')
 m=dict(schema='gi-native-writer-r305',pid=7,count=2,binding_stride=64,record_stride=96,rows_per_geometry=2,
        active_rows='even',cpu_inputs_only=True,gpu_completion_proven=False,allocation_ownership_proven=False,
        binding_bytes=128,table_bytes=384,hit_bytes=384,serial=1,engine_frame=100)
 metadata.write_text(json.dumps(m));bindings=bytearray(128);bindings[68]=1
 records=bytearray(384);records[0]=1;records[192]=2
 (root/(name+'-bindings.bin')).write_bytes(bindings);table=root/(name+'-table.bin');table.write_bytes(records)
 report=c.validate_writer_files(root,7);assert report['complete_tables']==1 and report['tables'][0]['alpha']==1
 table.write_bytes(records[:-1]);assert c.validate_writer_files(root,7)['complete_tables']==0
 records[96]=1;table.write_bytes(records);assert c.validate_writer_files(root,7)['complete_tables']==0
 records[96]=0;records[224]=99;table.write_bytes(records);assert c.validate_writer_files(root,7)['complete_tables']==0
print('PASS R305 complete-table validation: sizes, alpha classification, reserved rows, roots and partial export rejection')
