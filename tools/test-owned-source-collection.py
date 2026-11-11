import importlib.util,json,pathlib,struct,tempfile,unittest,os
ROOT=pathlib.Path(__file__).resolve().parents[1]
def mod(name,file):
 s=importlib.util.spec_from_file_location(name,ROOT/'tools'/file);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
c=mod('collection','collect-rr-boundary.py');t=mod('trace_test','test-gi-owned-source-trace.py')
class Collection(unittest.TestCase):
 def test_launcher_keeps_single_producer(self):
  text=(ROOT/'tools/diagnostics/Start-GI-Owned-Source.cmd').read_text()
  self.assertIn('set "CONTROLTOH_GI_SINGLE=1"',text)
  self.assertNotIn('set "CONTROLTOH_GI_EVALUATOR=1"',text)
  self.assertIn('-OwnedSource -CollectExisting -RequireRR',text)

 def test_source_join_and_bad_metadata(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=pathlib.Path(tmp);folder=root/'Control-FG'/'Diagnostics';folder.mkdir(parents=True)
   path=folder/'GI-Owned-Source-R424-100.json';path.with_suffix('.bin').write_bytes(t.Trace().fixture())
   m=dict(schema='OGI1',pid=100,engine_frame=123,present=123,width=8,height=8,roi=[2,3,1,1],records=3,capacity=6,overflow=False,evaluator='owned',source_frames=1)
   path.write_text(json.dumps(m));result=c.validate_owned_source(root,100,123,root)
   self.assertTrue(result['same_frame_verified']);self.assertFalse(result['consecutive_source_pair'])
   for key,value in [('overflow',True),('engine_frame',124),('roi',[2,3,2,1]),('roi',[0,0,1,1]),('evaluator','native'),('records',4)]:
    bad=dict(m);bad[key]=value;path.write_text(json.dumps(bad))
    with self.subTest(key=key),self.assertRaises(ValueError):c.validate_owned_source(root,100,123,root)
if __name__=='__main__':unittest.main()
