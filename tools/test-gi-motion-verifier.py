import pathlib,tempfile,struct,subprocess,sys,json,unittest
EXE=sys.argv.pop(1) if len(sys.argv)>1 else 'validate-gi-motion.exe'
class Verify(unittest.TestCase):
 def make(self,root):
  w,h,n,frame=3,2,4,100
  header=bytearray(2048);struct.pack_into('<8I',header,1136,0x4d343834,frame,w,h,n,224,2048,1)
  data=header
  for y in range(h):
   for x in range(w):
    for r in range(n):
     a=bytearray(224);struct.pack_into('<I',a,12,frame);struct.pack_into('<4f',a,16,0,0,1,1)
     struct.pack_into('<4f',a,32,.125,.0625,.03125,1);struct.pack_into('<I',a,76,0x80000000|r);struct.pack_into('<f',a,92,1);data+=a
  (root/'trace.bin').write_bytes(data);(root/'raw.bin').write_bytes(struct.pack('<4e',.125,.0625,.03125,1)*w*h)
  return [EXE,str(root/'trace.bin'),str(root/'raw.bin'),str(w),str(h),str(n),str(frame),'0',str(w*8)]
 def test_valid(self):
  with tempfile.TemporaryDirectory() as t:
   a=self.make(pathlib.Path(t));r=subprocess.run(a,capture_output=True,text=True);self.assertEqual(r.returncode,0,r.stderr);self.assertEqual(json.loads(r.stdout)['evaluated_rays'],24)
 def test_corruption(self):
  for offset,fmt,value,key in [(2048+12,'I',99,'stamp_errors'),(2048+28,'f',.5,'weight_errors'),(2048+76,'I',0x80000002,'ray_index_errors'),(2048+32,'f',1.,'gi_mismatched_channels')]:
   with self.subTest(key=key),tempfile.TemporaryDirectory() as t:
    root=pathlib.Path(t);a=self.make(root);data=bytearray((root/'trace.bin').read_bytes());struct.pack_into('<'+fmt,data,offset,value);(root/'trace.bin').write_bytes(data)
    r=subprocess.run(a,capture_output=True,text=True);self.assertEqual(r.returncode,1,r.stderr);self.assertGreater(json.loads(r.stdout)[key],0)
 def test_truncated(self):
  with tempfile.TemporaryDirectory() as t:
   root=pathlib.Path(t);a=self.make(root);(root/'trace.bin').write_bytes(b'x');r=subprocess.run(a,capture_output=True,text=True);self.assertEqual(r.returncode,2)
 def test_preview(self):
  with tempfile.TemporaryDirectory() as t:
   root=pathlib.Path(t);self.make(root);r=subprocess.run([EXE,'--preview',str(root/'raw.bin'),'10','3','2',str(root/'n.ppm'),str(root/'b.ppm')]);self.assertEqual(r.returncode,0);self.assertTrue((root/'n.ppm').read_bytes().startswith(b'P6\n3 2\n255\n'));self.assertNotEqual((root/'n.ppm').read_bytes(),(root/'b.ppm').read_bytes())
if __name__=='__main__':unittest.main()
