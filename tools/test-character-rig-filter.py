"""Execute the emitted filter instructions against synthetic trilinear volumes."""
import itertools,json,math,re,sys
from pathlib import Path
import numpy as np
from character_surface import root,decode
# Test helper deliberately implements texture filtering independently of shader emission.
def sample(grid,uv):
 pos=np.asarray(uv[:3])*np.asarray(grid.shape[:3])-.5
 base=np.floor(pos).astype(int);f=pos-base;result=np.zeros(4)
 for bits in itertools.product((0,1),repeat=3):
  at=tuple(min(max(base[a]+bits[a],0),grid.shape[a]-1) for a in range(3))
  result+=grid[at]*math.prod(f[a] if bits[a] else 1-f[a] for a in range(3))
 return result

def execute(ins,uv,grids):
 regs={5:np.array([*uv,0.])}
 dims=np.array([*grids[8].shape[:3],0.])
 def value(s):
  if s.startswith('l('):
   v=[float(x) for x in s[2:-1].split(',')];return np.array(v*4 if len(v)==1 else v)
  if s.startswith('cb['):base=dims
  else:base=regs[int(re.search(r'r\[(\d+)\]',s)[1])]
  sw=s.split('.')[-1];return np.array([base['xyzw'.index(c)] for c in sw])
 for i in ins:
  name=i['name'];d,*args=i['operands'];r=int(re.search(r'r\[(\d+)\]',d)[1]);mask=d.split('.')[-1]
  if name=='sample_l':
   slot=int(re.search(r't\[8\]\[(\d+)\]',args[1])[1]);v=sample(grids[slot],value(args[0]));v=np.array([v['xyzw'.index(c)] for c in args[1].split('.')[-1]])
  else:
   v=[value(s) for s in args]
   if name in ('mov','utof'):v=v[0]
   elif name=='div':v=np.divide(v[0][:len(mask)],v[1][:len(mask)])
   elif name=='mad':v=v[0][:len(mask)]*v[1][:len(mask)]+v[2][:len(mask)]
   elif name=='mul':v=v[0][:len(mask)]*v[1][:len(mask)]
   elif name=='add':v=v[0][:len(mask)]+v[1][:len(mask)]
   else:raise AssertionError(name)
  target=regs.setdefault(r,np.zeros(4))
  # DXBC source lanes align with destination mask, including scalar swizzles.
  for c in mask:target['xyzw'.index(c)]=v['xyzw'.index(c)] if len(v)==4 else v[list(mask).index(c)]
 return regs

families=json.loads((root/'research/character-surface-r346/families.json').read_text())
rng=np.random.default_rng(351)
checks=0
for family in families:
 original=decode(root/'build'/('character-'+family['name']+'.dxbc'))
 changed=decode(root/'build'/('character-'+family['name']+'-filtered-volume.dxbc'))
 old=original['instructions'];new=changed['instructions']
 n=next(n for n,i in enumerate(old) if i['name']=='sample_l' and 't[8][8].' in ' '.join(i['operands']))
 assert old[n+1]['name']=='sample_l'
 # All instructions except the two replaced reads and increased temp count survive exactly.
 semantic=lambda i:(i['name'],i['operands'])
 before=[semantic(i) for i in old[:n] if i['name']!='dcl_temps']
 assert before==[semantic(i) for i in new[:n] if i['name']!='dcl_temps']
 tail=[semantic(i) for i in old[n+2:]]
 start=len(new)-len(tail);assert tail==[semantic(i) for i in new[start:]]
 block=new[n:start];assert sum(i['name']=='sample_l' for i in block)==16
 assert original['bindings']==changed['bindings']
 for dims in ((5,7,9),(8,4,2)):
  for kind in ('constant','random','checker'):
   a=rng.random((*dims,4))*3;b=rng.random((*dims,4))*6
   if kind=='constant':a[:]=[.1,.7,2,9];b[:]=[4,5,6,7]
   if kind=='checker':
    a[:]=np.indices(dims).sum(axis=0)[...,None]%2;b[:]=a*3
   grids={8:a,9:b}
   for uv in ((.5,.5,.5),(.31,.67,.43),(0,0,0),(1,1,1),(-.03,1.02,.2)):
    regs=execute(block,uv,grids)
    offsets=[np.asarray(uv)+np.asarray(signs)*.5/np.asarray(dims) for signs in itertools.product((-1,1),repeat=3)]
    expected=np.mean([sample(a,p)[:3] for p in offsets],axis=0)
    intensity=np.mean([sample(b,p)[0] for p in offsets])
    actual=regs[6][1:] if old[n]['operands'][0].endswith('yzw') else regs[6][:3]
    assert np.allclose(actual,expected,atol=1e-6),(family['name'],actual,expected)
    assert abs(regs[5][0]-intensity)<1e-6
    assert np.all(np.isfinite(actual))
    checks+=1
print(f'PASS {checks} emitted-filter evaluations: RGB/intensity lanes, volume sizes, constant energy, high-frequency fields, edges, native downstream shading')
