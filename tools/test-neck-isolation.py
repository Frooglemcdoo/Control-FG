"""R355 separates exact diffuse/specular inputs without changing gameplay shaders."""
import json,re
import numpy as np
from character_surface import root,decode,buffer_site,buffer_off,buffer_view,NAMES
families=json.loads((root/'research/character-surface-r346/families.json').read_text())
checks=0
for family in families:
 base=root/'build'/('character-'+family['name']+'-hq-cubic.dxbc');hq=base.read_bytes();info=decode(base)
 for index,name in enumerate(('g_tLightBufferDiffuse','g_tLightBufferSpecular')):
  r,offset=buffer_site(info,name)
  for view in (False,True):
   n=index+(4 if view else 2)
   path=root/'build'/('character-'+family['name']+'-'+NAMES[n]+'.dxbc')
   expected=(buffer_view if view else buffer_off)(hq,info,name)
   assert path.read_bytes()==expected
   new=decode(path)
   assert info['bindings']==new['bindings'] and info['constant_buffers']==new['constant_buffers']
   sem=lambda i:(i['name'],i['operands'])
   writes=lambda d:[sem(i) for i in d['instructions'] if i['operands'] and i['operands'][0].startswith(('o[','type12','type38'))]
   oldwrites=writes(info);newwrites=writes(new)
   assert oldwrites==(newwrites[:-1] if view else newwrites)
   if view:
    block=new['instructions'][-6:-1]
    assert [i['name'] for i in block]==['max','mul','add','div','mov']
    assert block[-1]['operands'][0]=='o[0].xyz'
    temp=int(re.search(r'r\[(\d+)\]',block[0]['operands'][0])[1])
    # Execute the actual emitted display arithmetic independently of emission.
    for v in ((0,0,0),(-1,.01,2),(.001,1,100),(10000,.25,.5)):
     regs={temp:np.array([*v,0.])};out=None
     def value(s):
      if s.startswith('l('):return np.repeat(float(s[2:-1]),4)
      rr=int(re.search(r'r\[(\d+)\]',s)[1]);return regs[rr][['xyzw'.index(c) for c in s.split('.')[-1]]]
     for ins in block:
      d,*ss=ins['operands'];vv=[value(x) for x in ss];op=ins['name']
      a=vv[0] if op=='mov' else np.maximum(*vv) if op=='max' else vv[0]*vv[1] if op=='mul' else vv[0]+vv[1] if op=='add' else np.divide(vv[0],vv[1],out=np.zeros(4),where=vv[1]!=0)
      if d=='o[0].xyz':out=a[:3];continue
      rr=int(re.search(r'r\[(\d+)\]',d)[1]);target=regs.setdefault(rr,np.zeros(4))
      for c in d.split('.')[-1]:target['xyzw'.index(c)]=a['xyzw'.index(c)]
     x=4*np.maximum(v,0);assert np.allclose(out,x/(1+x),rtol=1e-6)
     assert np.all((out>=0)&(out<1));checks+=1
print(f'PASS 48 reversible buffer diagnostics; {checks} emitted display evaluations; native alpha/depth/coverage and bindings preserved')
