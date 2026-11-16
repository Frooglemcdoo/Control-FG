"""Execute emitted integer sampling IR; verify independent reference and net coverage."""
import sys,re,struct,json,argparse,subprocess,base64
from pathlib import Path
import numpy as np
from gi_progressive_sampling import arithmetic,transform,hlsl
from gi_live_sampling import transform as live,ROOT
from gi_single_producer import owned_raygen
from gi_guided_fast import transform as guided

def hash32(x):
 x=np.asarray(x,dtype=np.uint32)
 with np.errstate(over='ignore'):
  x=x^(x>>16);x=x*np.uint32(0x7feb352d);x=x^(x>>15);x=x*np.uint32(0x846ca68b)
 return x^(x>>16)
def reference(frame,pixel,batch,ray):
 with np.errstate(over='ignore'):n=np.asarray(frame,dtype=np.uint32)*np.uint32(4)+np.asarray(ray,dtype=np.uint32)
 g=n^(n>>1);u=np.zeros_like(g);v=np.zeros_like(g);direction=1<<31
 for bit in range(32):
  mask=np.uint32(0)-((g>>bit)&1);u^=mask&np.uint32(1<<(31-bit));v^=mask&np.uint32(direction);direction^=direction>>1
 result=[]
 for value,salt in [(u,0x68bc21eb),(v,0x02e5be93)]:
  seed=hash32(pixel^batch^np.uint32(salt));out=np.zeros(np.broadcast_shapes(value.shape,seed.shape),dtype=np.uint32)
  for bit in range(31,7,-1):
   prefix=value&np.uint32(((0xffffffff<<(bit+1))&0xffffffff) if bit<31 else 0)
   flip=hash32(seed^prefix^np.uint32((bit*0x9e3779b9)&0xffffffff))&1
   out|=(((value>>bit)&1)^flip)<<bit
  result.append((out>>8).astype(np.float32)*np.float32(2**-24))
 return result

def emitted(frame,pixel,batch,ray):
 lines,out=arithmetic(frame='%frame',pixel='%pixel',batch='%batch',ray='%ray')
 vals=dict(frame=frame,pixel=pixel,batch=batch,ray=ray)
 def get(s):
  if s.startswith('%'):return vals[s[1:]]
  if s.startswith('0x'):return np.float32(struct.unpack('>d',bytes.fromhex(s[2:]))[0])
  return np.uint32(s)
 for line in lines:
  name,expr=line.strip()[1:].split(' = ')
  if expr.startswith('uitofp'):v=np.asarray(get(expr.split()[2]),dtype=np.float32)
  else:
   op,_,args=expr.split(' ',2);a,b=map(get,args.split(', '))
   with np.errstate(over='ignore'):
    v={'add':lambda:a+b,'mul':lambda:a*b,'xor':lambda:a^b,'and':lambda:a&b,'or':lambda:a|b,'lshr':lambda:a>>b,'shl':lambda:a<<b,'fmul':lambda:a*b}[op]()
  vals[name]=v
 return [vals[x[1:]] for x in out]

def verify_transform(source,variant):
 new=transform(source,variant);names=('Frc','Frc26') if variant==0 else ('Frc','Frc85')
 old='\n'.join(re.search(r'^  %'+name+r' = .*$',source,re.M)[0] for name in names)
 start=new.index('  %G445Old0 =');end=new.index('\n',new.index('  %'+names[1]+' = phi float',start))
 block=new[start:end];pred=re.search(r'\[ %G445Old0, ([^\]]+) \]',block)[1]
 restored=(new[:start]+old+new[end:]).replace(', %G445Merge ]',', '+pred+' ]')
 assert restored==source,'Unexpected change outside angular sampling/phi edges'
 assert f'%G445Four = icmp eq i32 %{226 if variant==0 else 286}, 4' in block
 print('PASS reversible four-ray-only edit; range, hit/miss, trace operands and other counts preserved:',variant)

if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--dxc');p.add_argument('--fixture',type=Path);a=p.parse_args()
 if a.fixture:
  a.fixture.write_text(hlsl()+'''RWStructuredBuffer<uint2> output:register(u0);
uint Hash(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
[numthreads(64,1,1)]void main(uint3 id:SV_DispatchThreadID){
 uint n=id.x;if(n>=4096)return;uint frame=Hash(n*31u);
 if(n<9){uint boundary[9]={0,1,2,255,256,65535,0x3fffffff,0x40000000,0xffffffff};frame=boundary[n];}
 output[n]=asuint(Progressive(frame,Hash(n>>2),1013904226u,n&3));}
''')
 else:
  n=np.arange(8192,dtype=np.uint32);frame=hash32(n*31);frame[:9]=[0,1,2,255,256,65535,0x3fffffff,0x40000000,0xffffffff]
  args=(frame,hash32(n>>2),np.uint32(1013904226),n&3)
  for x,y in zip(emitted(*args),reference(*args)):assert np.array_equal(x,y) and np.isfinite(x).all() and (x>=0).all() and (x<1).all()
  # Aligned powers-of-two windows retain one sample in each dyadic rectangle.
  index=np.arange(4096,dtype=np.uint32)
  u,v=reference(index//4,np.uint32(739191),np.uint32(1013904226),index%4)
  for m in range(2,13):
   size=1<<m
   for split in range(m+1):
    cell=(u*(1<<split)).astype(int)+(1<<split)*(v*(1<<(m-split))).astype(int)
    assert np.array_equal(np.sort(cell[:size]),np.arange(size))
  print('PASS emitted IR/reference exact equality, frame boundaries, finite coordinates, 4..4096 sample net coverage')
  if a.dxc:
   for variant in [0,1]:
    path=ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64';raw=Path('/tmp') if sys.platform!='win32' else ROOT/'build';blob=raw/f'gi-progressive-native-{variant}.dxbc';blob.write_bytes(base64.b64decode(path.read_bytes()))
    source=subprocess.check_output([a.dxc,'-dumpbin',str(blob)],text=True);owned=owned_raygen(live(source,variant),variant)
    verify_transform(owned,variant)
    if variant==0:verify_transform(guided(owned),variant)
