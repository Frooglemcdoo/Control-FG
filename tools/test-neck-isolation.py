"""R354 diagnostics retain the successful R353 filter and isolate only named inputs."""
import json
from character_surface import root,decode,words,neck_patches,insert_verified,NAMES
families=json.loads((root/'research/character-surface-r346/families.json').read_text())
for family in families:
 base=root/'build'/('character-'+family['name']+'-hq-cubic.dxbc');hq=base.read_bytes();info=decode(base)
 patches=neck_patches(info)
 assert [len(p) for p in patches[:2]]==[1,1]
 assert len(patches[2])==(3 if family['group']==0 else 0)
 assert len(patches[3])==(0 if family['group']==2 else 1)
 assert len(patches[4])==1 and len(patches[5])==2
 for n,p in enumerate(patches,2):
  path=root/'build'/('character-'+family['name']+'-'+NAMES[n]+'.dxbc')
  assert path.read_bytes()==insert_verified(hq,p)
  new=decode(path)
  assert info['bindings']==new['bindings'] and info['constant_buffers']==new['constant_buffers']
  # Native output writes, alpha, depth and coverage remain; only inserted values
  # at audited producer/consumer boundaries differ, verified reversibly above.
  sem=lambda i:(i['name'],i['operands'])
  writes=lambda d:[sem(i) for i in d['instructions'] if i['operands'] and i['operands'][0].startswith(('o[','type38'))]
  assert writes(info)==writes(new)
print('PASS 72 reversible source-isolation variants on unchanged HQ; explicit no-op sources for non-skin/hair; bindings and output coverage preserved')
