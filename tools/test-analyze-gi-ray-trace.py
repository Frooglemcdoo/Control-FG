import struct
import unittest
from analyze_gi_ray_trace import analyze

def fixture():
    data=bytearray(16+3*256)
    struct.pack_into('<I',data,0,3)
    for i in range(3):
        p=16+i*256
        struct.pack_into('<4I',data,p,16,16,i if i<2 else 0xffffffff,1 if i<2 else 2)
        struct.pack_into('<I',data,p+108,42)
        struct.pack_into('<2I',data,p+144,7 if i==0 else 65535,2)
        struct.pack_into('<4f',data,p+160,1,2,.75,.75)
        if i<2:
            for off,values in [(16,[1,3,8]),(32,[2,3,4]),(48,[.5,1,2]),(64,[i+1,3*(i+1),8*(i+1)]),(80,[1,2,3,4])]:
                struct.pack_into('<'+'f'*len(values),data,p+off,*values)
        else:
            struct.pack_into('<4f',data,p+176,.75,2.25,6,1)
            struct.pack_into('<3f',data,p+192,2,6,16)
            struct.pack_into('<f',data,p+208,.75)
    return data

class RayTraceTests(unittest.TestCase):
    def test_reconstruction(self):
        result=analyze(fixture())
        self.assertEqual(result['rays'],2)
        self.assertEqual(result['miss_or_invalid_rays'],1)
        self.assertTrue(all(x==0 for x in result['errors'].values()))
    def test_runtime_accounting(self):
        data=fixture();p=16+512
        struct.pack_into('<I',data,p+236,0x278)
        for off in (224,240):struct.pack_into('<3f',data,p+off,.375,1.125,3)
        self.assertTrue(analyze(data)['runtime_accounting']['verified'])
        struct.pack_into('<f',data,p+224,0)
        with self.assertRaisesRegex(ValueError,'accounting mismatch'):analyze(data)
    def test_runtime_accounting_nonfinite(self):
        data=fixture();p=16+512
        struct.pack_into('<I',data,p+236,0x278)
        struct.pack_into('<f',data,p+224,float('nan'))
        with self.assertRaisesRegex(ValueError,'nonfinite'):analyze(data)
    def test_truncation(self):
        with self.assertRaisesRegex(ValueError,'length'):analyze(fixture()[:-1])
    def test_missing_ray(self):
        data=fixture();struct.pack_into('<I',data,0,2);data=data[:272]+data[528:]
        with self.assertRaisesRegex(ValueError,'Incomplete'):analyze(data)
    def test_duplicate(self):
        data=fixture();struct.pack_into('<I',data,16+256+8,0)
        with self.assertRaisesRegex(ValueError,'Duplicate'):analyze(data)
    def test_nan(self):
        data=fixture();struct.pack_into('<f',data,32,float('nan'))
        with self.assertRaisesRegex(ValueError,'nonfinite'):analyze(data)
    def test_wrong_output_is_measured(self):
        data=fixture();struct.pack_into('<f',data,16+512+176,1.75)
        self.assertEqual(analyze(data)['errors']['output_max_absolute'],1)

if __name__=='__main__':unittest.main()
