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
if __name__=='__main__':unittest.main()
