import unittest
from gi_secondary_filter import patch,SOURCE
class FilterScope(unittest.TestCase):
 def test_exact_native_scope(self):
  a=SOURCE.read_text();b=patch(a);before=a.splitlines();after=b.splitlines()
  self.assertEqual(len(before),len(after));changed=[(x,y) for x,y in zip(before,after) if x!=y]
  self.assertEqual(len(changed),1);self.assertTrue(changed[0][0].endswith('// native dword 2338'))
  self.assertIn('GIMaterialTextureLod()',changed[0][1])
  # All other texture/light/projection/shadow operations must remain identical.
  self.assertEqual(a.count('SampleLevel'),b.count('SampleLevel'))
 def test_reject_missing_duplicate_or_changed_contract(self):
  a=SOURCE.read_text();line=next(x for x in a.splitlines() if x.endswith('// native dword 2338'))
  for invalid in [a.replace(line,''),a+'\n'+line,a.replace(line,line.replace('g_sLinearWrap','g_sLinearClamp'))]:
   with self.assertRaises(ValueError):patch(invalid)
if __name__=='__main__':unittest.main()
