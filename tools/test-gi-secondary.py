import importlib.util,pathlib,re,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('secondary',ROOT/'tools/generate-gi-secondary.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
class Secondary(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.reference=(ROOT/'src/shaders/gi_evaluator_reference.generated.hlsl').read_text()
        cls.text=m.generate(cls.reference)
    def test_reproducible(self):
        self.assertEqual(self.text,(ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text())
    def test_every_instruction_preserved(self):
        before=re.findall(r'// native dword (\d+)',self.reference)
        after=re.findall(r'// native dword (\d+)',self.text)
        self.assertEqual(sorted(before),sorted(after));self.assertEqual(len(after),674)
        old={re.search(r'// native dword (\d+)',x)[1]:x for x in self.reference.splitlines() if '// native dword ' in x}
        new={re.search(r'// native dword (\d+)',x)[1]:x for x in self.text.splitlines() if '// native dword ' in x}
        for key in old:
            if key in ('269','995','1003','1051'):continue
            self.assertEqual(re.sub(r'\br(\d+)\b',r'state.r\1',old[key]),new[key])
    def test_secondary_gets_explicit_sample(self):
        block=self.text.split('void GIEvaluateSecondary(')[1].split('bool GIInitializeReceiver(')[0]
        for name in ('g_tMaterialId.Load','g_tPosition_TexcoordY.Load','g_tNormal_TexcoordX.Load'):
            self.assertNotIn(name,block)
        self.assertIn('state.r7.w=sample.material;',block)
        self.assertIn('state.r11=sample.positionUV;',block)
        self.assertIn('state.r13=sample.normalUV;',block)
        # Direction remains recomputed from target receiver origin and secondary point.
        self.assertIn('asfloat(state.r1.xyzx)',block)
        self.assertIn('// native dword 1059',block)
        self.assertNotIn('// native dword 5029',block)
    def test_normalization_and_outputs_stay_in_resolve(self):
        kernel=self.text.split('#ifndef GI_SECONDARY_LIBRARY_ONLY')[1]
        for at in (5029,5136,5143,5150,5184,5191):self.assertIn('// native dword '+str(at),kernel)
        self.assertIn('GIInitializeReceiver(dispatchId.xy,state)',kernel)
    def test_changed_translation_rejected(self):
        with self.assertRaises(ValueError):m.generate(self.reference.replace('uint4 r29=0u','uint4 r29=1u'))
if __name__=='__main__':unittest.main()
