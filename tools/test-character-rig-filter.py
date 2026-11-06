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
   elif name=='round_ni':v=np.floor(v[0])
   elif name=='div':v=np.divide(v[0][:len(mask)],v[1][:len(mask)])
   elif name=='mad':v=v[0][:len(mask)]*v[1][:len(mask)]+v[2][:len(mask)]
   elif name=='mul':v=v[0][:len(mask)]*v[1][:len(mask)]
   elif name=='add':v=v[0][:len(mask)]+v[1][:len(mask)]
   else:raise AssertionError(name)
  target=regs.setdefault(r,np.zeros(4))
  # DXBC source lanes align with destination mask, including scalar swizzles.
  for c in mask:target['xyzw'.index(c)]=v['xyzw'.index(c)] if len(v)==4 else v[list(mask).index(c)]
 return regs

def cubic_reference(grid,uv):
 pos=np.asarray(uv)*np.asarray(grid.shape[:3])-.5
 base=np.floor(pos).astype(int);f=pos-base
 ws=np.array([(1-f)**3,3*f**3-6*f**2+4,-3*f**3+3*f**2+3*f+1,f**3])/6
 result=np.zeros(4)
 ws=np.array([np.convolve(ws[:,axis],[.25,.5,.25]) for axis in range(3)]).T
 for t in itertools.product(range(6),repeat=3):
  at=tuple(min(max(base[a]+t[a]-2,0),grid.shape[a]-1) for a in range(3))
  result+=grid[at]*math.prod(ws[t[a],a] for a in range(3))
 return result

def blocks(family,name):
 original=decode(root/'build'/('character-'+family+'.dxbc'))
 changed=decode(root/'build'/('character-'+family+'-'+name+'.dxbc'))
 old=original['instructions'];new=changed['instructions']
 n=next(n for n,i in enumerate(old) if i['name']=='sample_l' and 't[8][8].' in ' '.join(i['operands']))
 assert old[n+1]['name']=='sample_l'
 semantic=lambda i:(i['name'],i['operands'])
 before=[semantic(i) for i in old[:n] if i['name']!='dcl_temps']
 assert before==[semantic(i) for i in new[:n] if i['name']!='dcl_temps']
 tail=[semantic(i) for i in old[n+2:]]
 start=len(new)-len(tail);assert tail==[semantic(i) for i in new[start:]]
 block=new[n:start];assert sum(i['name']=='sample_l' for i in block)==(54 if name=='hq-cubic' else 16)
 assert original['bindings']==changed['bindings']
 return block,old[n]['operands'][0].endswith('yzw')

families=json.loads((root/'research/character-surface-r346/families.json').read_text())
rng=np.random.default_rng(352);checks=0
for family in families:
 for name in ('hq-cubic','r351-filter'):
  block,skin=blocks(family['name'],name)
  for dims in ((5,7,9),(8,4,2)):
   for kind in ('constant','random','checker','impulse'):
    a=rng.random((*dims,4))*3;b=rng.random((*dims,4))*6
    if kind=='constant':a[:]=[.1,.7,2,9];b[:]=[4,5,6,7]
    if kind=='checker':a[:]=np.indices(dims).sum(axis=0)[...,None]%2;b[:]=a*3
    if kind=='impulse':
     a[:]=0;a[tuple(n//2 for n in dims)]=[.2,1,3,5];b[:]=a*2
    grids={8:a,9:b}
    for uv in ((.5,.5,.5),(.31,.67,.43),(0,0,0),(1,1,1),(-.03,1.02,.2)):
     regs=execute(block,uv,grids)
     if name=='hq-cubic':expected=cubic_reference(a,uv)[:3];intensity=cubic_reference(b,uv)[0]
     else:
      offsets=[np.asarray(uv)+np.asarray(signs)*.5/np.asarray(dims) for signs in itertools.product((-1,1),repeat=3)]
      expected=np.mean([sample(a,p)[:3] for p in offsets],axis=0)
      intensity=np.mean([sample(b,p)[0] for p in offsets])
     actual=regs[6][1:] if skin else regs[6][:3]
     assert np.allclose(actual,expected,atol=3e-6),(family['name'],name,actual,expected)
     assert abs(regs[5][0]-intensity)<3e-6
     assert np.all(np.isfinite(actual)) and np.all(actual>=-1e-6)
     assert np.all(actual<=a[...,:3].max(axis=(0,1,2))+3e-6)
     checks+=1
# Sub-cell motion through a cell boundary: continuous values AND first derivative.
block,skin=blocks('skin-basic','hq-cubic')
grids={8:rng.random((8,8,8,4)),9:rng.random((8,8,8,4))}
for axis in range(3):
 for boundary in (2.,2.5,3.,3.5,4.):
  def out(p):
   uv=np.array([.43,.52,.61]);uv[axis]=(p+.5)/8
   return execute(block,uv,grids)[6][1:]
  e=1e-4;c=out(boundary);left=(c-out(boundary-e))/e;right=(out(boundary+e)-c)/e
  assert np.max(abs(left-right))<.001,(axis,boundary,left,right)
# Stationary stochastic input: HQ suppresses spatially uncorrelated frame noise
# without retaining history. This does NOT simulate a complete game renderer.
hq,_=blocks('skin-basic','hq-cubic');old,_=blocks('skin-basic','r351-filter')
series=[[],[]]
for frame in range(64):
 grids={8:1+rng.normal(0,.1,(8,8,8,4)),9:1+rng.normal(0,.1,(8,8,8,4))}
 for k,block in enumerate((old,hq)):series[k].append(execute(block,(.47,.53,.61),grids)[6][1:])
old_rms=np.std(series[0],axis=0).mean();hq_rms=np.std(series[1],axis=0).mean()
assert hq_rms<old_rms*.85,(old_rms,hq_rms)
print(f'PASS synthetic stationary input-noise RMS R351={old_rms:.6f} HQ={hq_rms:.6f}; no game-performance or flicker claim')
print(f'PASS {checks} emitted-filter cases: direct216-voxel weighted cubic oracle, R351 parity, constant energy, bounded impulse response, borders, sub-cell continuity and native downstream shading')
