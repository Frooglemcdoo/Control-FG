import json,tempfile,unittest
from pathlib import Path
import numpy as np
from audit_gi_signal_origin import audit
class SignalOrigin(unittest.TestCase):
 def fixture(self,p):
  m={'width':9,'height':9,'frame':1,'pre_gi_frame':1,'pre_gi_captured':True,'gi_boundary':{'valid':True,'frame':1,'stage':'raw_bypass'}};(p/'metadata.json').write_text(json.dumps(m))
  a=np.ones((9,9,4),dtype='<f2')*.01;a[4,4,:3]=1
  for n in ['pre-filter-gi.rgba16f','gi-at-rr-input.rgba16f','input-color.rgba16f']:a.tofile(p/n)
  raw=np.ones((9,9,4),dtype='<f2')*.5;raw[...,3]=-3;raw.tofile(p/'raw-albedo.rgba16f')
 def test_signal(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);self.fixture(p);r=audit(p);self.assertTrue(r['gi_rgb_exactly_preserved']);h=r['regions']['tagged_hair_interior'];self.assertEqual(h['gi_bright_candidates'],1);self.assertEqual(h['also_input_color_over_2x_median'],1)
 def test_mismatch(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);self.fixture(p);m=json.loads((p/'metadata.json').read_text());m['pre_gi_frame']=2;(p/'metadata.json').write_text(json.dumps(m))
   with self.assertRaises(ValueError):audit(p)
 def test_corruption(self):
  for corruption in ['truncated','nan']:
   with self.subTest(corruption=corruption),tempfile.TemporaryDirectory() as d:
    p=Path(d);self.fixture(p);f=p/'pre-filter-gi.rgba16f'
    if corruption=='truncated':f.write_bytes(b'')
    else:
     a=np.fromfile(f,'<f2');a[0]=float('nan');a.tofile(f)
    with self.assertRaises(ValueError):audit(p)
 def test_changed_boundary(self):
  with tempfile.TemporaryDirectory() as d:
   p=Path(d);self.fixture(p);f=p/'gi-at-rr-input.rgba16f';a=np.fromfile(f,'<f2');a[0]=.2;a.tofile(f);self.assertFalse(audit(p)['gi_rgb_exactly_preserved'])
if __name__=='__main__':unittest.main()
