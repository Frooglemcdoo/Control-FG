"""Regression checks for the complete GI evaluator translation boundary."""
import base64, pathlib, subprocess, sys, tempfile, unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
class Evaluator(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.path=pathlib.Path(cls.tmp.name)
  cls.raw=base64.b64decode((ROOT/'research/gi-source-r267/gi-6.dxbc.b64').read_bytes())
  cls.source=cls.path/'native.dxbc';cls.source.write_bytes(cls.raw)
  cls.output=cls.path/'evaluator.hlsl'
  subprocess.run([sys.executable,str(ROOT/'tools/generate-gi-evaluator.py'),str(cls.source),str(cls.output),str(cls.path/'replay.hlsl')],check=True,stdout=subprocess.DEVNULL)
  cls.text=cls.output.read_text()
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def test_checked_in_source_reproducible(self):
  self.assertEqual(self.text,(ROOT/'src/shaders/gi_evaluator_reference.generated.hlsl').read_text())
 def line(self,offset):return next(l for l in self.text.splitlines() if l.endswith('// native dword '+str(offset)))
 def test_all_executable_instructions_covered_once(self):
  import re
  expected=[int(m.group(1)) for l in (ROOT/'research/gi-source-r267/gi-6.asm').read_text().splitlines() if (m:=re.match(r'(\d+):',l)) and int(m.group(1))>=245]
  actual=[int(x) for x in re.findall(r'// native dword (\d+)',self.text)]
  self.assertEqual(actual,expected);self.assertEqual(len(actual),674)
 def test_array_slice_and_mip_are_not_confused(self):
  # DXBC 2D loads take mip from W; array loads retain Z as layer.
  self.assertIn('.Load(asint(r0.xyww).xyw)',self.line(295))
  self.assertIn('.Load(asint(r0.xyzw).xyzw)',self.line(1003))
  self.assertIn('.Load(asint(r19.xyzw).xyw)',self.line(3861))
 def test_outputs_and_normalization_preserved(self):
  self.assertIn('g_rwtHitInfo[',self.line(5184));self.assertIn('g_rwtDiffuseGIColorTarget[',self.line(5191))
  self.assertIn('asfloat(r0.xyzx)/asfloat(r0.wwww)',self.line(5150))
  self.assertIn('mad(asfloat(r3.xyzx),asfloat(r15.xyzx),asfloat(r10.xyzx))',self.line(5029))
  self.assertIn('asint(giCB4[0u].zzzz)',self.line(5143))
 def test_native_material_lookup_not_quantized(self):
  self.assertIn('g_sMaterialTextureArray[NonUniformResourceIndex(',self.text)
  self.assertNotIn('PTNativeMaterialMip',self.text)
  self.assertIn('g_tGIBrickTextures[6]',self.text)
  self.assertIn('uint d[80]',self.text);self.assertIn('uint d[40]',self.text)
 def test_replay_uses_actual_evaluator_instructions(self):
  replay=(self.path/'replay.hlsl').read_text()
  self.assertEqual(replay,(ROOT/'src/shaders/gi_evaluator_replay.generated.hlsl').read_text())
  for offset in (5029,5136,5143,5150):self.assertIn(self.line(offset),replay)
  self.assertEqual(replay.count('ReplayAccumulate(throughput, radiance,'),3)
  self.assertEqual(replay.count('ReplayNormalize(scale,'),3)
 def test_wrong_shader_rejected(self):
  b=bytearray(self.raw);b[-1]^=1;(self.path/'bad.dxbc').write_bytes(b)
  r=subprocess.run([sys.executable,str(ROOT/'tools/generate-gi-evaluator.py'),str(self.path/'bad.dxbc'),str(self.path/'bad.hlsl')],capture_output=True)
  self.assertNotEqual(r.returncode,0);self.assertFalse((self.path/'bad.hlsl').exists())
if __name__=='__main__':unittest.main()
