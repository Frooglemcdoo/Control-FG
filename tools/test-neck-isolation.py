"""Execute R357's emitted composition arithmetic, including native aliasing."""
import json,re
import numpy as np
from character_surface import root,decode,composition_site,composition_view,NAMES,words
families=json.loads((root/'research/character-surface-r346/families.json').read_text())
rng=np.random.default_rng(357);checks=0
for family in families:
 base=root/'build'/('character-'+family['name']+'-hq-cubic.dxbc');hq=base.read_bytes();info=decode(base);a=composition_site(info)
 for component in range(4):
  path=root/'build'/('character-'+family['name']+'-'+NAMES[component+2]+'.dxbc')
  assert path.read_bytes()==composition_view(hq,info,component)
  new=decode(path);assert info['bindings']==new['bindings'] and info['constant_buffers']==new['constant_buffers']
  sem=lambda i:(i['name'],i['operands'])
  writes=lambda d:[sem(i) for i in d['instructions'] if i['operands'] and i['operands'][0].startswith(('o[','type12','type38'))]
  assert writes(info)==writes(new)
  reads=lambda d:[sem(i) for i in d['instructions'] if i['name'].startswith(('ld','sample'))]
  assert reads(info)==reads(new)
  x=new['instructions'];n=next(n for n,i in enumerate(x) if i['name']=='ld' and i['operands'][-1]=='t[2][2].ywxz')
  block=x[n+3:n+7];assert [i['name'] for i in block[1:]]==['mad','add','mov']
  # The complete original atmosphere and participating-media tail remains exact.
  oldtail=info['instructions'][info['instructions'].index(a[5]):]
  assert [sem(i) for i in x[n+7:]]==[sem(i) for i in oldtail]
  for case in range(100):
   regs={i:rng.uniform(0,10,4) for i in range(100)}
   if case==0:regs={i:np.zeros(4) for i in regs}
   def value(s):
    if s.startswith('l('):return np.repeat(float(s[2:-1]),4)
    rr=int(re.search(r'r\[(\d+)\]',s)[1]);return regs[rr][['xyzw'.index(c) for c in s.split('.')[-1]]].copy()
   diffuse=value(a[3]['operands'][1])[:3]*value(a[3]['operands'][2])[:3]
   specular=value(a[3]['operands'][3])[:3]
   baseLight=value(a[4]['operands'][2])[:3]
   material=value(a[3]['operands'][2])[:3]*.25
   expected=[baseLight,diffuse,specular,material][component]
   alpha={i:v[3] for i,v in regs.items()}
   for ins in block:
    d,*ss=ins['operands'];vv=[value(z) for z in ss]
    result=vv[0] if ins['name']=='mov' else vv[0]*vv[1] if ins['name']=='mul' else vv[0]+vv[1] if ins['name']=='add' else vv[0]*vv[1]+vv[2]
    rr=int(re.search(r'r\[(\d+)\]',d)[1])
    for c in d.split('.')[-1]:regs[rr]['xyzw'.index(c)]=result['xyzw'.index(c)]
   assert np.allclose(regs[0][:3],expected,rtol=1e-6)
   assert all(v[3]==alpha[i] for i,v in regs.items())
   checks+=1
print(f'PASS {checks} emitted composition cases including hair register aliasing; exact native fog tail, resource reads, alpha/depth/coverage retained')
