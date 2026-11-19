import importlib.util,json,pathlib,tempfile,unittest,zipfile,hashlib
spec=importlib.util.spec_from_file_location('scene',pathlib.Path(__file__).parent/'diagnostics/collect-gi-scene.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Scene(unittest.TestCase):
 def fixture(self,root):
  for ordinal,first in [(1,100),(2,200)]:
   for i in range(2):
    frame=first+i;p=root/f'rr-input-c1-7-{frame}';p.mkdir()
    data=dict(scene_capture_build='R477',scene_full_resolution=True,rr_active=True,rr_result=1,output_captured=True,export_stride=1,pre_gi_captured=True,pre_gi_frame=frame,frame=frame,pid=7,width=2,height=2,capture_ordinal=ordinal,pair_index=i,previous_frame=first if i else 0,resources=[dict(file='rr.rgba16f',copied=True,width=2,height=2,pixel_bytes=8)])
    (p/'metadata.json').write_text(json.dumps(data));(p/'rr.rgba16f').write_bytes(bytes(range(32)))
    hit=dict(full_screen=True,raw_gi_captured=True,detailed_frame_matched=1,frame=frame,gpu_fence_completed=True,all_batches_recorded=True,recorded_batches=1,file='gi-hit-slices.bin',bytes=64,raw_gi=dict(width=2,height=2,offset=32,bytes=32),rays_per_batch=4,batches=[dict(region_width=2,region_height=2,region_count=1,captured_slices=4,resources=[dict(offset=0,slice_region_bytes=8)])])
    (p/'gi-hit-slices.json').write_text(json.dumps(hit));(p/'gi-hit-slices.bin').write_bytes(bytes(range(64)))
    d=root/f'gi-scene-detail-7-{frame}';d.mkdir();(d/'metadata.json').write_text(json.dumps(dict(frame=frame,gpu_completed=True,width=2,height=2,file='details.bin',bytes=768,red_offset=0,columns=2,rows=2,red_stride=192)));(d/'details.bin').write_bytes(b'a'*768)
 def test_round_trip_and_bounds(self):
  with tempfile.TemporaryDirectory() as t:
   root=pathlib.Path(t);self.fixture(root);records,issues,files=m.inspect(root,7);self.assertEqual(len(records),4);self.assertEqual(issues,[])
   manifest={};out=m.package(root,files,root/'out',manifest,chunk_bytes=100,part_bytes=250)
   for f in manifest['files']:
    data=b''
    for c in f['chunks']:
     with zipfile.ZipFile(out/f'scene-part-{c["part"]:03d}.zip') as z:chunk=z.read(c['entry'])
     self.assertEqual(hashlib.sha256(chunk).hexdigest(),c['sha256']);self.assertEqual(c['offset'],len(data));data+=chunk
    self.assertEqual(data,(root/f['path']).read_bytes());self.assertEqual(hashlib.sha256(data).hexdigest(),f['sha256'])
   for p in manifest['parts']:
    with zipfile.ZipFile(out/p['file']) as z:self.assertLessEqual(sum(i.file_size for i in z.infolist()),250)
 def test_missing_output_detail_and_frame_rejected(self):
  with tempfile.TemporaryDirectory() as t:
   root=pathlib.Path(t);self.fixture(root);p=root/'rr-input-c1-7-101/metadata.json';d=json.loads(p.read_text());d['output_captured']=False;d['previous_frame']=99;p.write_text(json.dumps(d));(root/'gi-scene-detail-7-100/metadata.json').unlink()
   _,issues,_=m.inspect(root,7);self.assertTrue(any('RR output' in x for x in issues));self.assertTrue(any('detailed-grid' in x for x in issues));self.assertTrue(any('pair 1' in x for x in issues))
if __name__=='__main__':unittest.main()
