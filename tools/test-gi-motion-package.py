import importlib.util,pathlib,tempfile,json,struct,unittest
root=pathlib.Path(__file__).parent
spec=importlib.util.spec_from_file_location('old',root/'test-gi-scene-package.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old)
m=old.m
class Motion(unittest.TestCase):
 def fixture(self,root):
  old.Scene().fixture(root)
  import shutil
  # Start from one complete scene record and replicate a consecutive sequence.
  source=root/'rr-input-c1-7-100';detail=root/'gi-scene-detail-7-100'
  for frame in (101,200,201):shutil.rmtree(root/f'rr-input-c1-7-{frame}');shutil.rmtree(root/f'gi-scene-detail-7-{frame}')
  for index in range(16):
   frame=100+index;r=root/f'rr-input-c1-7-{frame}';d=root/f'gi-scene-detail-7-{frame}'
   if index:shutil.copytree(source,r);shutil.copytree(detail,d)
   metadata=json.loads((r/'metadata.json').read_text());metadata.update(frame=frame,motion_sequence=True,motion_index=index,motion_frames=16,capture_ordinal=1,pair_index=index,previous_frame=frame-1 if index else 0,projection_valid=True,motion_scale_valid=True,reset=0,pre_gi_frame=frame)
   names=['normal-roughness.rgba16f','specular.rgba16f','diffuse.rgba16f','raw-albedo.rgba16f','input-depth.r32f','native-gbuffer1.rgba8','native-gbuffer2.rgba8','gi-at-rr-input.rgba16f','pre-filter-gi.rgba16f','native-reconstruction-map.r32u','input-motion.rg16f','input-color.rgba16f','rr-output.rgba16f']
   metadata['resources']=[]
   for name in names:(r/name).write_bytes(bytes(32));metadata['resources'].append(dict(file=name,copied=True,width=2,height=2,pixel_bytes=8))
   (r/'metadata.json').write_text(json.dumps(metadata));h=json.loads((r/'gi-hit-slices.json').read_text());h['frame']=frame;(r/'gi-hit-slices.json').write_text(json.dumps(h))
   a=json.loads((d/'metadata.json').read_text());a['frame']=frame;a['guides']['selection'].update(sampleFrame=frame,sourceFrame=frame-4);a['motion']=dict(file='motion-rays.bin',bytes=2048+2*2*4*224,header_bytes=2048,ray_stride=224,rays=4,full_screen=True,all_rays=True,float_precision=32)
   (d/'motion-rays.bin').write_bytes(bytes(a['motion']['bytes']));(d/'metadata.json').write_text(json.dumps(a))
 def test_complete(self):
  with tempfile.TemporaryDirectory() as t:
   r=pathlib.Path(t);self.fixture(r);records,issues,_=m.inspect(r,7,motion=True);self.assertEqual(len(records),16);self.assertEqual(issues,[])
 def test_missing_lane_gap_and_precision(self):
  with tempfile.TemporaryDirectory() as t:
   r=pathlib.Path(t);self.fixture(r)
   p=r/'rr-input-c1-7-105/metadata.json';a=json.loads(p.read_text());a['previous_frame']=101;a['resources']=[x for x in a['resources'] if not x['file'].startswith('input-motion')];p.write_text(json.dumps(a))
   p=r/'gi-scene-detail-7-110/metadata.json';a=json.loads(p.read_text());a['motion']['float_precision']=16;p.write_text(json.dumps(a))
   (r/'gi-scene-detail-7-115/motion-rays.bin').write_bytes(bytes(10))
   _,issues,_=m.inspect(r,7,motion=True);self.assertTrue(any('continuity' in x for x in issues));self.assertTrue(any('lane missing' in x for x in issues));self.assertTrue(any('full-precision' in x for x in issues));self.assertTrue(any('truncated' in x for x in issues))
if __name__=='__main__':unittest.main()
