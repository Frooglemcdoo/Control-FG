"""Execute emitted R360 instructions against an independent image-space oracle."""
import json,struct,re
from pathlib import Path
import numpy as np
from character_surface import root,words,decode
from character_diffuse import opnames

def execute(code,regs,position,resources,depthslot,samples):
 regs=regs.copy();pc=0;stack=[];reads=[]
 def arg(p):
  token=code[p];p+=1;mod=0
  if token>>31:mod=(code[p]>>6)&255;p+=1
  kind=(token>>12)&255;dim=(token>>20)&3;idx=code[p:p+dim];p+=dim
  if kind==4:
   n=1 if token&3==1 else 4;v=np.array(code[p:p+n],np.uint32).view(np.float32);p+=n
   if n==1:v=np.repeat(v,4)
  else:
   if kind==0:v=regs[idx[0]]
   elif kind==1:v=np.array([*position,0,1],np.float32)
   elif kind==8:
    assert idx==[depthslot,depthslot,2];v=np.array([0,samples,0,0],np.uint32).view(np.float32)
   elif kind==7:return idx,p
   else:raise AssertionError(kind)
   sw=(token>>4)&255;v=v[[(sw>>(2*c))&3 for c in range(4)]].copy()
  if mod==1:v=-v
  elif mod==2:v=abs(v)
  return v,p
 with np.errstate(all='ignore'):
  while pc<len(code):
   token=code[pc];length=token>>24;name=opnames[token&2047]
   active=all(stack)
   if name=='if':
    v,_=arg(pc+1);stack.append(active and bool(v.view(np.uint32)[0]));pc+=length;continue
   if name=='endif':stack.pop();pc+=length;continue
   if not active:pc+=length;continue
   dest=code[pc+1];rr=code[pc+2];mask=(dest>>4)&15;p=pc+3;args=[]
   while p<pc+length:v,p=arg(p);args.append(v)
   assert p==pc+length
   if name=='mov':v=args[0]
   elif name=='mad':v=args[0]*args[1]+args[2]
   elif name=='mul':v=args[0]*args[1]
   elif name=='add':v=args[0]+args[1]
   elif name=='div':v=args[0]/args[1]
   elif name=='max':v=np.maximum(args[0],args[1])
   elif name in ('dp2','dp3'):
    n=int(name[-1]);v=np.repeat(np.sum(args[0][:n]*args[1][:n],dtype=np.float32),4)
   elif name in ('ftoi','ftou'):v=args[0].astype(np.int32 if name=='ftoi' else np.uint32).view(np.float32)
   elif name=='utof':v=args[0].view(np.uint32).astype(np.float32)
   elif name in ('and','or','ishl','ushr','iadd','ieq','uge','ult'):
    a,b=[x.view(np.uint32) for x in args]
    v={'and':lambda:a&b,'or':lambda:a|b,'ishl':lambda:a<<(b&31),'ushr':lambda:a>>(b&31),'iadd':lambda:a+b,'ieq':lambda:np.where(a==b,np.uint32(0xffffffff),np.uint32(0)),'uge':lambda:np.where(a>=b,np.uint32(0xffffffff),np.uint32(0)),'ult':lambda:np.where(a<b,np.uint32(0xffffffff),np.uint32(0))}[name]().view(np.float32)
   elif name=='lt':v=np.where(args[0]<args[1],np.uint32(0xffffffff),np.uint32(0)).view(np.float32)
   elif name=='resinfo':
    tex=resources[args[1][1]];v=np.array([tex.shape[1],tex.shape[0],1,1],np.uint32).view(np.float32)
   elif name=='ld':
    slot=args[1][1];tex=resources[slot];xx,yy=args[0].view(np.int32)[:2];assert 0<=xx<tex.shape[1] and 0<=yy<tex.shape[0],('OOB',slot,xx,yy)
    reads.append((slot,xx,yy));v=tex[yy,xx].copy()
   else:raise AssertionError(name)
   if token&(1<<13):v=np.clip(v,0,1)
   for c in range(4):
    if mask&(1<<c):regs[rr,c]=v[c]
   pc+=length
 assert not stack
 return regs,reads

def normal(g):
 r,gg,a=np.floor(g[[0,1,3]].astype(float)*255+.5).astype(int)
 q=1.3*(2*np.array([(r*8+((a>>1)&7))/2047,(gg*16+(a>>4))/4095])-1)
 s=np.dot(q,q);return np.array([2*q[0],2*q[1],s-1])/(s+1),a&1

def mapped(resources,p):
 xx,yy=p;word=int(resources[11][yy//2,xx//2,0].view(np.uint32));bits=(word>>((xx%2)*8+(yy%2)*16))&3
 return xx//2*2+(bits&1),yy//2*2+(bits>>1)

def oracle(resources,p,center,samples):
 x,y=center;c=resources[3][y,x,:3].astype(float)
 if samples>1 or not np.all(abs(c)<1e20):return c
 n,mask=normal(resources[0][y,x]);mid=(resources[1][y,x,2:]*np.float32(255)).astype(np.uint32)
 total=4*c;den=4.;h,w=resources[3].shape[:2]
 for yy in (-2,0,2):
  for xx in (-2,0,2):
   if xx==yy==0:continue
   q=(p[0]+xx,p[1]+yy)
   if not (0<=q[0]<w and 0<=q[1]<h):continue
   sx,sy=mapped(resources,q)
   if not (0<=sx<w and 0<=sy<h):continue
   nn,mm=normal(resources[0][sy,sx]);mi=(resources[1][sy,sx,2:]*np.float32(255)).astype(np.uint32)
   if mm!=mask or not np.array_equal(mi,mid):continue
   val=resources[3][sy,sx,:3].astype(float)
   if not np.all(abs(val)<1e20):continue
   weight=np.clip((np.dot(nn,n)-.95)*20,0,1)**2*(2 if xx==0 or yy==0 else 1)
   weight*=np.clip(1-np.max(abs(val-c)/(.5*np.maximum(abs(val),abs(c))+.0001)),0,1)
   total+=weight*val;den+=weight
 return total/den

rng=np.random.default_rng(360);families=json.loads((root/'research/character-surface-r346/families.json').read_text());checks=0
for family in families:
 base=root/'build'/('character-'+family['name']+'-hq-cubic.dxbc');new=root/'build'/('character-'+family['name']+'-hq-diffuse.dxbc')
 d=decode(base);fd=decode(new);w=words(base.read_bytes());fw=words(new.read_bytes())
 load=next(i for i in d['instructions'] if i['name']=='ld' and i['operands'][-1]=='t[3][3].xyzw');at=load['dword']+(w[load['dword']]>>24);size=len(fw)-len(w);code=fw[at:at+size]
 output=int(re.search(r'r\[(\d+)\]',load['operands'][0])[1]);coord=w[load['dword']+4]
 decl=next(i['dword'] for i in d['instructions'] if i['name']=='dcl_temps');temp=w[decl+1]
 depth=next(b['slot'] for b in d['bindings'] if b['name']=='common_depth_general')
 assert d['bindings']==fd['bindings'] and d['constant_buffers']==fd['constant_buffers']
 restored=fw[:at]+fw[at+size:];restored[decl+1]-=13;restored[1]=len(restored);assert restored==w
 for case in range(32):
  h=9;ww=11;tex={0:np.tile(np.array([.5,.5,.5,128/255],np.float32),(h,ww,1)),1:np.tile(np.array([.2,.3,10/255,20/255],np.float32),(h,ww,1)),3:rng.uniform(.85,1.15,(h,ww,4)).astype(np.float32),11:rng.integers(0,2**32,(5,6,4),dtype=np.uint32).view(np.float32)}
  pos=(int(rng.integers(0,ww)),int(rng.integers(0,h)));center=mapped(tex,pos)
  if center[0]>=ww or center[1]>=h:center=pos
  if case==0:tex[3][:]=3.5
  if case%8==1:tex[1][:,ww//2:,2]=11/255
  if case%8==2:tex[0][:,ww//2:,:2]=.9
  if case%8==3:tex[3][:,ww//2:,:3]*=100
  if case%8==4:tex[3][::2,::2,0]=np.nan
  if case%8==5:tex[0][:,ww//2:,3]=129/255
  if case%8==6:tex[3][center[1],center[0],:3]=np.inf
  samples=4 if case%8==7 else 1
  regs=rng.uniform(1,2,(128,4)).astype(np.float32);regs[coord]=np.array([*center,0,0],np.uint32).view(np.float32);regs[output,:3]=tex[3][center[1],center[0],:3]
  expected=oracle(tex,pos,center,samples);actual,reads=execute(code,regs,pos,tex,depth,samples)
  assert np.allclose(actual[output,:3],expected,rtol=3e-5,atol=1e-6,equal_nan=True),(family['name'],case,actual[output,:3],expected)
  for rr in range(temp):
   keep=[3] if rr==output else [0,1,2,3];assert np.array_equal(regs[rr,keep],actual[rr,keep],equal_nan=True)
  if samples>1:assert not reads
  checks+=1
print(f'PASS {checks} emitted diffuse reconstruction cases across12 families: native remap, odd borders, constant radiance, normal/material/mask and radiance edges, NaN/Inf, MSAA fallback, original-register preservation')

# Constant-surface noise test: the reference already agrees with emitted code.
tex={0:np.tile(np.array([.5,.5,.5,128/255],np.float32),(16,16,1)),1:np.tile(np.array([.2,.3,10/255,20/255],np.float32),(16,16,1)),3:rng.uniform(.9,1.1,(16,16,4)).astype(np.float32),11:np.zeros((8,8,4),np.float32)}
before=[];after=[]
for y in range(2,14):
 for x in range(2,14):
  center=mapped(tex,(x,y));before.append(tex[3][center[1],center[0],0]-1);after.append(oracle(tex,(x,y),center,1)[0]-1)
old_rms=np.sqrt(np.mean(np.square(before)));new_rms=np.sqrt(np.mean(np.square(after)))
assert new_rms<old_rms*.8,(old_rms,new_rms)
print(f'PASS synthetic diffuse-noise RMS {old_rms:.6f} -> {new_rms:.6f}; no game-quality/performance claim')

# Optional Windows fixture: transplant the SAME emitted block into a tiny
# compiled compute wrapper, then compare real GPU readbacks with this oracle.
import sys
if '--gpu-fixture' in sys.argv:
 from character_diffuse import block,cb,ins
 from character_surface import dst,src
 from character_surface import transform_shex
 from decode_dxbc import operand
 path=root/'build/diffuse-fixture-base.dxbc';base=path.read_bytes();d=decode(path);w=words(base)
 binding=next(b for b in d['bindings'] if b['name']=='Diffuse');identifier=binding['identifier']
 load=next(i for i in d['instructions'] if i['name']=='ld' and i['operands'][-1].startswith(f't[{identifier}][3].'))
 at=load['dword']+(w[load['dword']]>>24);out=int(re.search(r'r\[(\d+)\]',load['operands'][0])[1]);native=w[load['dword']:at];assert len(native)==8
 decl=next(i['dword'] for i in d['instructions'] if i['name']=='dcl_temps');temp=w[decl+1]
 save=ins('mov',dst(temp+13,15),native[3:5])
 code=block(temp,out,src(temp+13),cb(0,3,0xe4),0)
 ids={b['slot']:b['identifier'] for b in d['bindings'] if b['kind']==2}
 pc=0
 while pc<len(code):
  end=pc+(code[pc]>>24);p=pc+1
  while p<end:
   start=p;_,p=operand(code,p)
   if (code[start]>>12)&255==7:code[start+1]=ids[code[start+2]]
  assert p==end;pc=end
 def patch(v):
  v[at:at]=code;v[load['dword']:load['dword']]=save;v[decl+1]=temp+14
 (root/'build/diffuse-fixture-filtered.dxbc').write_bytes(transform_shex(base,patch))
 for case in range(8):
  g1=np.tile(np.array([.5,.5,.5,128/255],np.float32),(9,9,1));g2=np.tile(np.array([.2,.3,10/255,20/255],np.float32),(9,9,1));diffuse=rng.uniform(.9,1.1,(9,9,4)).astype(np.float32)
  diffmap=rng.integers(0,2**32,(5,5,4),dtype=np.uint32).view(np.float32);p=(4,4);samples=1
  if case==0:diffuse[:]=3.5
  if case==2:g2[:,5:,2]=11/255
  if case==3:g1[:,5:,:2]=.9
  if case==4:diffuse[:,5:,:3]=100
  if case==5:p=(0,0)
  if case==6:samples=4
  if case==7:g1[:,5:,3]=129/255
  textures={0:g1,1:g2,3:diffuse,11:diffmap};center=mapped(textures,p);expected=oracle(textures,p,center,samples)
  result=np.array([*expected,diffuse[center[1],center[0],3]],np.float32)
  const=np.zeros(64,np.float32);const.view(np.uint32)[9]=samples;const[12:14]=p
  pre=root/'build'/f'diffuse-fixture-{case}-'
  for name,value in [('g1',g1),('g2',g2),('diffuse',diffuse),('map',diffmap[:,:,0]),('cb',const),('expected',result)]:Path(str(pre)+name+'.bin').write_bytes(value.tobytes())
 print('Prepared exact emitted GPU fixture and eight oracle cases')
