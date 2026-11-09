import base64,hashlib,json,tempfile,unittest,importlib.util,struct
from pathlib import Path
from gi_ray_trace import generate as baseline
from gi_joint_light_trace import generate
ROOT=Path(__file__).resolve().parents[1]
class Joint(unittest.TestCase):
 def test_native_identity_and_legacy(self):
  raw=base64.b64decode((ROOT/'research/gi-source-r267/gi-6.dxbc.b64').read_text())
  identity=next(x for x in json.loads((ROOT/'research/gi-source-r267/manifest.json').read_text()) if x['file']=='gi-6.dxbc')
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'probe.dxbc';a,_=baseline(raw,p,identity);plan={};b,_=baseline(raw,p,identity,instrumentation_plan=plan);self.assertEqual(a,b)
   changed,report=generate(raw,p,identity);self.assertTrue(report['native_instruction_recovery']);self.assertFalse(report['runtime_enabled']);self.assertNotEqual(changed,a)
   self.assertEqual(hashlib.sha256(changed).hexdigest(),report['sha256'])
   damaged=bytearray(raw);damaged[-1]^=1
   with self.assertRaises(AssertionError):generate(bytes(damaged),p,identity)
 def test_existing_ray_stream_is_readable(self):
  from analyze_gi_joint_trace import analyze_joint
  spec=importlib.util.spec_from_file_location('ray_fixture',ROOT/'tools/test-analyze-gi-ray-trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);data=m.fixture()
  report=analyze_joint(data);self.assertEqual(report['executed_spot_records'],0);self.assertTrue(report['native_accounting']['complete_ray_sets'])
 def test_same_ray_join_and_rejection(self):
  from analyze_gi_joint_trace import analyze_joint
  spec=importlib.util.spec_from_file_location('ray_fixture',ROOT/'tools/test-analyze-gi-ray-trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);data=m.fixture()
  spot=bytearray(256);struct.pack_into('<4I',spot,0,16,16,0,3)
  for offset,values in [(16,[.5,1,2]),(32,[2,2,2]),(48,[.5,1,2]),(64,[1,1,1])]:struct.pack_into('<3f',spot,offset,*values)
  struct.pack_into('<f',spot,140,.5);struct.pack_into('<4I',spot,144,6,2,7,42);struct.pack_into('<I',spot,172,0x281)
  data+=spot;struct.pack_into('<I',data,0,4)
  struct.pack_into('<3I',data,4,0x281,0,0)
  report=analyze_joint(data);self.assertEqual(report['executed_spot_records'],1);self.assertEqual(report['brightest_spot_terms'][0]['normalized_rgb'],[.1875,.375,.75])
  for offset,value in [(144,7),(152,65535),(156,99)]:
   bad=bytearray(data);struct.pack_into('<I',bad,len(bad)-256+offset,value)
   if offset==144:continue # Another light ID is valid, unlike mismatched material context.
   with self.assertRaises(ValueError):analyze_joint(bad)
  outside=bytearray(data);struct.pack_into('<I',outside,16,32)
  with self.assertRaisesRegex(ValueError,'outside ROI'):analyze_joint(outside)
  duplicate=bytearray(data)+spot;struct.pack_into('<I',duplicate,0,5)
  with self.assertRaisesRegex(ValueError,'duplicate'):analyze_joint(duplicate)
 def test_candidate_stages_and_missing_data(self):
  from analyze_gi_joint_trace import analyze_joint
  spec=importlib.util.spec_from_file_location('ray_fixture',ROOT/'tools/test-analyze-gi-ray-trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);data=m.fixture()
  spot=bytearray(256);struct.pack_into('<4I',spot,0,16,16,0,3)
  for offset,values in [(16,[.5,1,2]),(32,[2,2,2]),(48,[.5,1,2]),(64,[1,1,1])]:struct.pack_into('<3f',spot,offset,*values)
  struct.pack_into('<f',spot,140,.5);struct.pack_into('<4I',spot,144,6,2,7,42);struct.pack_into('<I',spot,172,0x281)
  candidate=bytearray(256);struct.pack_into('<4I',candidate,0,16,16,0,4)
  struct.pack_into('<4I',candidate,96,257,6,7,42);struct.pack_into('<2I',candidate,176,0x282,2)
  struct.pack_into('<I',candidate,76,63)
  for offset,values in [(16,[.25,.75,.5,2]),(32,[1,.5,.25,1]),(48,[.8,.5,.5,1]),(80,[.25,.75,.5,.6])]:struct.pack_into('<4f',candidate,offset,*values)
  for offset,values in [(64,[.5,.25,1]),(192,[4,8,2]),(208,[2,2,2]),(224,[2,2,2])]:struct.pack_into('<3f',candidate,offset,*values)
  rejected=[]
  for light,bits in [(7,7),(8,1)]:
   b=bytearray(256);struct.pack_into('<4I',b,0,16,16,0,4);struct.pack_into('<4I',b,96,0,light,7,42)
   struct.pack_into('<I',b,76,bits);struct.pack_into('<2I',b,176,0x282,2);rejected.append(b)
  data+=spot+candidate+b''.join(rejected);struct.pack_into('<4I',data,0,7,0x282,0,0)
  report=analyze_joint(data)
  self.assertEqual(report['candidate_counts'],dict(outside_projection=1,zero_visibility=1,executed=1,cookie_sampled=1))
  start=16+4*256
  for offset,value in [(76,1),(104,99),(176,0),(180,9)]:
   bad=bytearray(data);struct.pack_into('<I',bad,start+offset,value)
   with self.assertRaises(ValueError):analyze_joint(bad)
  for offset in (56,208,224):
   bad=bytearray(data);struct.pack_into('<f',bad,start+offset,9)
   with self.assertRaises(ValueError):analyze_joint(bad)
  missing=bytearray(data[:start]+data[start+256:]);struct.pack_into('<I',missing,0,6)
  with self.assertRaisesRegex(ValueError,'missing candidate'):analyze_joint(missing)
  duplicate=bytearray(data)+candidate;struct.pack_into('<I',duplicate,0,8)
  with self.assertRaisesRegex(ValueError,'duplicate candidate'):analyze_joint(duplicate)
  # Nonfinite rejected projection is recorded as invalid, never JSON NaN.
  bad=bytearray(data);struct.pack_into('<f',bad,len(bad)-256+16,float('inf'))
  report=analyze_joint(bad);self.assertIsNone(report['candidate_stages'][-1]['projection']);json.dumps(report,allow_nan=False)
 def test_volume_accounting_and_rejection(self):
  from analyze_gi_joint_trace import analyze_joint
  spec=importlib.util.spec_from_file_location('ray_fixture',ROOT/'tools/test-analyze-gi-ray-trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);data=m.fixture()
  records=[]
  for ray,material in [(0,7),(1,65535)]:
   summary=bytearray(256);volume=bytearray(256)
   struct.pack_into('<4I',summary,0,16,16,ray,5);struct.pack_into('<4I',volume,0,16,16,ray,6)
   for offset,values in [(64,[1,2,3,2]),(80,[.5,1,1.5]),(96,[.5,.5,.5]),(112,[1,1.5,2]),(128,[.5,1,2,2])]:struct.pack_into('<'+'f'*len(values),summary,offset,*values)
   struct.pack_into('<4I',summary,144,42,2,material,0x283);struct.pack_into('<I',summary,160,1)
   struct.pack_into('<4f',volume,48,.5,1,1.5,2);struct.pack_into('<4f',volume,64,1,2,3,2)
   struct.pack_into('<2I',volume,80,0,123);struct.pack_into('<4I',volume,96,42,2,material,0x283)
   records.extend([summary,volume])
  data+=b''.join(records);struct.pack_into('<4I',data,0,7,0x283,0,0)
  report=analyze_joint(data);self.assertEqual(report['volume_ray_count'],2);self.assertEqual(report['volume_rays'][1]['final_rgb'],[1,1.5,2])
  for relative in (64,76,80,96,112,128):
   bad=bytearray(data);struct.pack_into('<f',bad,16+3*256+relative,9)
   with self.assertRaises(ValueError):analyze_joint(bad)
  bad=bytearray(data);struct.pack_into('<f',bad,16+5*256+140,9)
  with self.assertRaisesRegex(ValueError,'fallback radiance'):analyze_joint(bad)
  bad=bytearray(data);struct.pack_into('<I',bad,16+4*256+80,1)
  with self.assertRaisesRegex(ValueError,'volume index'):analyze_joint(bad)
  bad=bytearray(data[:-256]);struct.pack_into('<I',bad,0,6)
  with self.assertRaisesRegex(ValueError,'blend accounting'):analyze_joint(bad)
  bad=bytearray(data)+records[-1];struct.pack_into('<I',bad,0,8)
  with self.assertRaisesRegex(ValueError,'duplicate'):analyze_joint(bad)
  # No contributing volume is legitimate: zero weight, ambient-only fallback.
  empty=bytearray(m.fixture());struct.pack_into('<4I',empty,0,5,0x283,0,0)
  for original in records[::2]:
   b=bytearray(original);struct.pack_into('<4f',b,64,0,0,0,0);struct.pack_into('<3f',b,80,0,0,0)
   struct.pack_into('<3f',b,96,1,1.5,2);struct.pack_into('<I',b,160,0);empty+=b
  self.assertEqual(analyze_joint(empty)['volume_ray_count'],2)
 def test_wide_point_surface_accounting(self):
  from analyze_gi_joint_trace import analyze_joint
  spec=importlib.util.spec_from_file_location('ray_fixture',ROOT/'tools/test-analyze-gi-ray-trace.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);data=m.fixture()
  # Ambient-only volume summaries match the existing two-ray oracle fixture.
  for ray,material in [(0,7),(1,65535)]:
   b=bytearray(256);struct.pack_into('<4I',b,0,16,16,ray,5)
   for o,v in [(96,[1,1.5,2]),(112,[1,1.5,2]),(128,[.5,1,2,2])]:struct.pack_into('<'+'f'*len(v),b,o,*v)
   struct.pack_into('<4I',b,144,42,2,material,0x283);data+=b
  point=bytearray(256);surface=bytearray(256)
  struct.pack_into('<4I',point,0,16,16,0,7);struct.pack_into('<4I',surface,0,16,16,0,8)
  for o,v in [(16,[1,3,8]),(32,[2,3,4]),(48,[.5,1,2]),(64,[1,1,1])]:struct.pack_into('<3f',point,o,*v)
  struct.pack_into('<f',point,80,1);struct.pack_into('<5I',point,96,9,2,7,42,0x284)
  for o,v in [(32,[2,3,4]),(48,[2,3,4]),(64,[2,3,4]),(80,[1,1,1]),(96,[.5,1,2])]:struct.pack_into('<3f',surface,o,*v)
  struct.pack_into('<4I',surface,112,2,7,42,0x284);data+=point+surface
  struct.pack_into('<4I',data,0,7,0x284,0,0)
  report=analyze_joint(data);self.assertEqual(len(report['point_records']),1);self.assertEqual(len(report['surface_records']),1)
  for record,offset in [(5,16),(5,48),(5,104),(5,112),(6,16),(6,32),(6,48),(6,64),(6,80),(6,96),(6,124)]:
   bad=bytearray(data);struct.pack_into('<f',bad,16+record*256+offset,99)
   with self.assertRaises(ValueError):analyze_joint(bad)
  for x,y,valid in [(0,0,True),(126,126,True),(48,49,True),(79,79,True),(47,49,False),(80,79,False),(127,127,False),(128,0,False)]:
   moved=bytearray(data)
   for offset in range(16,len(moved),256):struct.pack_into('<2I',moved,offset,x,y)
   if valid:self.assertEqual(len(analyze_joint(moved)['surface_records']),1)
   else:
    with self.assertRaises(ValueError):analyze_joint(moved)
if __name__=='__main__':unittest.main()
