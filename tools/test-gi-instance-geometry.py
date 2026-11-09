import struct
import unittest
from gi_instance_geometry import Snapshot,decode_instance,decode_triangle

IDENTITY=(1,0,0,0,0,1,0,0,0,0,1,0)
class Decoder(unittest.TestCase):
 def fixture(self):
  instance=decode_instance(struct.pack('<12fIIQ',*IDENTITY,0xab123456,0x89123456,4096))
  return dict(vertices=Snapshot(7,100,struct.pack('<9f',0,0,0,1,0,0,0,1,0)),indices=Snapshot(7,200,struct.pack('<3H',0,1,2)),vertex_address=100,index_address=200,vertex_count=3,index_count=3,stride=12,vertex_format=6,index_format=57,primitive=0,instance=instance,generation=7)
 def test_bits(self):
  i=self.fixture()['instance'];self.assertEqual((i.instance_id,i.mask,i.hit_group,i.flags,i.blas),(0x123456,0xab,0x123456,0x89,4096))
 def test_triangle(self):
  points,n=decode_triangle(**self.fixture());self.assertEqual(points,((0,0,0),(1,0,0),(0,1,0)));self.assertEqual(n,(0,0,1))
 def test_transform_order_winding(self):
  f=self.fixture();f['geometry_transform']=(1,0,0,2,0,1,0,0,0,0,1,0)
  f['instance']=decode_instance(struct.pack('<12fIIQ',-2,0,0,10,0,3,0,0,0,0,1,0,1<<24,0,4096))
  p,n=decode_triangle(**f);self.assertEqual(p,((6,0,0),(4,0,0),(6,3,0)));self.assertEqual(n,(0,0,-1))
 def test_rejections(self):
  for change in [dict(generation=8),dict(primitive=1),dict(index_count=4),dict(vertex_count=2),dict(stride=16),dict(index_format=43),dict(vertex_format=2),dict(vertex_address=99),dict(indices=Snapshot(7,200,b'')),dict(vertices=Snapshot(7,100,bytes(36))),dict(vertex_address=2**64)]:
   with self.subTest(change=change),self.assertRaises(ValueError):decode_triangle(**(self.fixture()|change))
 def test_uint32_padded(self):
  f=self.fixture();data=bytearray(65539*16)
  for i,p in enumerate(((0,0,0),(1,0,0),(0,1,0))):struct.pack_into('<3f',data,(65536+i)*16,*p)
  f.update(vertices=Snapshot(7,100,bytes(data)),indices=Snapshot(7,200,struct.pack('<3I',65536,65537,65538)),vertex_count=65539,index_format=42,stride=16)
  self.assertEqual(decode_triangle(**f)[1],(0,0,1))
  for change in [dict(index_address=202),dict(indices=Snapshot(7,200,bytes(11))),dict(vertex_count=65538)]:
   with self.assertRaises(ValueError):decode_triangle(**(f|change))
 def test_instance_rejections(self):
  for data in [bytes(63),bytes(64),struct.pack('<12fIIQ',*IDENTITY,0,0,4097),struct.pack('<12fIIQ',float('nan'),*IDENTITY[1:],0,0,4096)]:
   with self.assertRaises(ValueError):decode_instance(data)
 def test_mask_zero(self):
  f=self.fixture();f['instance']=decode_instance(struct.pack('<12fIIQ',*IDENTITY,0,0,4096))
  with self.assertRaises(ValueError):decode_triangle(**f)
if __name__=='__main__':unittest.main()
