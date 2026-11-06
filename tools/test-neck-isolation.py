"""Execute the emitted base-buffer split before native register reuse."""
import json,re
import numpy as np
from character_surface import root,decode,base_composition_site,base_component_view,composition_site,NAMES,words
families=json.loads((root/'research/character-surface-r346/families.json').read_text())
rng=np.random.default_rng(359);checks=0
for family in families:
 base=root/'build'/('character-'+family['name']+'-hq-cubic.dxbc');hq=base.read_bytes();info=decode(base);a=base_composition_site(info)
 decl=next(i['dword'] for i in info['instructions'] if i['name']=='dcl_temps');temp=words(hq)[decl+1]
 for component in range(4):
  path=root/'build'/('character-'+family['name']+'-'+NAMES[component+2]+'.dxbc')
  assert path.read_bytes()==base_component_view(hq,info,component)
  new=decode(path);assert info['bindings']==new['bindings'] and info['constant_buffers']==new['constant_buffers']
  sem=lambda i:(i['name'],i['operands'])
  writes=lambda d:[sem(i) for i in d['instructions'] if i['operands'] and i['operands'][0].startswith(('o[','type12','type38'))]
  assert writes(info)==writes(new)
  reads=lambda d:[sem(i) for i in d['instructions'] if i['name'].startswith(('ld','sample'))]
  assert reads(info)==reads(new)
  x=new['instructions'];n=next(n for n,i in enumerate(x) if i['operands'] and i['operands'][0]==f'r[{temp}].xyz')
  size=1+(component>=2);block=x[n:n+size]
  assert sem(x[n+size])==sem(a)
  end=next(n for n,i in enumerate(x) if i['name']=='mov' and i['operands']==['r[0].xyz',f'r[{temp}].xyzw'])
  oldtail=info['instructions'][info['instructions'].index(composition_site(info)[5]):]
  assert [sem(i) for i in x[end+1:]]==[sem(i) for i in oldtail]
  for case in range(100):
   regs={i:rng.uniform(0,10,4) for i in range(100)}
   if case==0:regs={i:np.zeros(4) for i in regs}
   def value(s):
    if s.startswith('l('):return np.repeat(float(s[2:-1]),4)
    rr=int(re.search(r'r\[(\d+)\]',s)[1]);return regs[rr][['xyzw'.index(c) for c in s.split('.')[-1]]].copy()
   diffuse=value(a['operands'][1])[:3]*value(a['operands'][2])[:3]
   specular=value(a['operands'][3])[:3]
   expected=(diffuse if component%2==0 else specular)*(1 if component<2 else .25)
   alpha={i:v[3] for i,v in regs.items()}
   def execute(ins):
    d,*ss=ins['operands'];vv=[value(z) for z in ss]
    result=vv[0] if ins['name']=='mov' else vv[0]*vv[1]
    rr=int(re.search(r'r\[(\d+)\]',d)[1])
    for c in d.split('.')[-1]:regs[rr]['xyzw'.index(c)]=result['xyzw'.index(c)]
   for ins in block:execute(ins)
   # The whole native rig executes between the snapshot and output. Poison
   # all original RGB temporaries to prove no reliance on their later values.
   for rr in range(temp):regs[rr][:3]=rng.uniform(-1000,1000,3)
   execute(x[end])
   assert np.allclose(regs[0][:3],expected,rtol=1e-6)
   assert all(v[3]==alpha[i] for i,v in regs.items())
   checks+=1
print(f'PASS {checks} emitted base-buffer arithmetic cases with register poisoning; exact native fog tail, reads and output writes retained')
