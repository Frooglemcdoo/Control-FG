import importlib.util,pathlib,struct,unittest
ROOT=pathlib.Path(__file__).resolve().parents[1]
def module(name,file):
    spec=importlib.util.spec_from_file_location(name,ROOT/'tools'/file)
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
m=module('tracegen','generate-gi-owned-source-trace.py');a=module('traceparse','analyze-gi-owned-source.py')
class Trace(unittest.TestCase):
    def test_preserves_sources(self):
        inputs=[(ROOT/'src/shaders'/name).read_text() for name in m.PINS]
        s,e=m.generate(*inputs)
        self.assertEqual(m.erase(s),inputs[0])
        self.assertEqual(m.erase(e).replace('"gi_owned_source_secondary.generated.hlsl"','"gi_evaluator_secondary.generated.hlsl"'),inputs[1])
        with self.assertRaises(ValueError):m.generate(inputs[0]+' ',inputs[1])
    def fixture(self):
        header=struct.pack('<4I',3,a.MAGIC,256,123);rows=[]
        for i in range(3):
            r=bytearray(256);struct.pack_into('<4I',r,0,2,3,i,1 if i<2 else 2)
            struct.pack_into('<I',r,204,123)
            struct.pack_into('<4f',r,16,1,2,3,4)
            if i<2:
                struct.pack_into('<I',r,76,2);struct.pack_into('<I',r,156,2)
                struct.pack_into('<3f',r,224,i+1,2*(i+1),3*(i+1))
            else:
                struct.pack_into('<f',r,60,1);struct.pack_into('<3f',r,224,2,4,6)
            rows.append(r)
        return bytearray(header+b''.join(rows))
    def test_complete(self):self.assertEqual(a.analyze(self.fixture())['valid_pixels'],1)
    def test_corruption_rejected(self):
        mutations=[(4,0),(8,128),(16+204,7),(16+156,3),(16+76,0),(16+4*4,0x7fc00000),
                   (16+256+8,0),(16+512+12,1),(16+512+16,0x7fc00000),(16+240,9)]
        for offset,value in mutations:
            b=self.fixture();struct.pack_into('<I',b,offset,value)
            with self.subTest(offset=offset),self.assertRaises(ValueError):a.analyze(b)
        for b in [self.fixture()[:-1],self.fixture()+b'x',b'',struct.pack('<4I',0,a.MAGIC,256,1)]:
            with self.assertRaises(ValueError):a.analyze(b)
if __name__=='__main__':unittest.main()
