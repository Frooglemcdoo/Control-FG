"""Interpret emitted R268 instructions against an independent PCF reference."""
import math,struct,tempfile
from pathlib import Path
import numpy as np
from gi_spot_pcf import ROOT,generate,base64,decoder

def run(a,texture,position,depth,rect):
 regs=np.zeros((a['temp']+7,4),dtype=np.float32)
 sample=a['sample'];compare=a['compare']
 sw=(sample[3]>>4)&255
 regs[sample[4],sw&3]=position[0];regs[sample[4],(sw>>2)&3]=position[1]
 regs[compare[6],(compare[5]>>4)&3]=depth
 regs[a['atlas']]=rect
 words=a['code'];offset=0
 def arg(pos):
  t=words[pos];pos+=1;modifier=0
  if t>>31:modifier=(words[pos]>>6)&255;pos+=1
  kind=(t>>12)&255;dim=(t>>20)&3;indices=words[pos:pos+dim];pos+=dim
  if kind==4:
   n=1 if t&3==1 else 4
   v=np.array(words[pos:pos+n],dtype=np.uint32).view(np.float32);pos+=n
   if n==1:v=np.repeat(v,4)
  elif kind==0:
   swizzle=(t>>4)&255
   if ((t>>2)&3)==2:v=np.repeat(regs[indices[0],(t>>4)&3],4)
   else:v=regs[indices[0],[(swizzle>>(i*2))&3 for i in range(4)]]
  else:v=np.zeros(4,dtype=np.float32)
  if modifier==1:v=-v
  return v,pos,t,indices
 while offset<len(words):
  token=words[offset];length=token>>24;name=decoder.opnames[token&2047]
  dest=words[offset+1];reg=words[offset+2];mask=(dest>>4)&15
  args=[];p=offset+3
  while p<offset+length:
   value,p,_,_=arg(p);args.append(value)
  assert p==offset+length
  if name=='resinfo':v=np.array([texture.shape[1],texture.shape[0],0,1],dtype=np.float32)
  elif name=='rcp':v=1/args[0]
  elif name=='mad':v=args[0]*args[1]+args[2]
  elif name=='mul':v=args[0]*args[1]
  elif name=='add':v=args[0]+args[1]
  elif name=='frc':v=args[0]-np.floor(args[0])
  elif name=='round_ni':v=np.floor(args[0])
  elif name=='min':v=np.minimum(args[0],args[1])
  elif name=='max':v=np.maximum(args[0],args[1])
  elif name=='ge':v=np.where(args[0]>=args[1],np.uint32(0xffffffff),np.uint32(0)).view(np.float32)
  elif name=='and':v=(args[0].view(np.uint32)&args[1].view(np.uint32)).view(np.float32)
  elif name=='sample_l':
   x,y=args[0][:2]*[texture.shape[1],texture.shape[0]]-.5
   ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy
   def get(dx,dy):return texture[min(max(iy+dy,0),texture.shape[0]-1),min(max(ix+dx,0),texture.shape[1]-1),0]
   value=(get(0,0)*(1-fx)+get(1,0)*fx)*(1-fy)+(get(0,1)*(1-fx)+get(1,1)*fx)*fy
   v=np.repeat(np.float32(value),4)
  else:raise AssertionError(name)
  for c in range(4):
   if mask&(1<<c):regs[reg,c]=v[c]
  offset+=length
 component=((sample[1]>>4)&15).bit_length()-1
 return float(regs[sample[2],component])

def reference(texture,uv,depth,rect):
 h,w=texture.shape[:2];x,y=np.array(uv)*[w,h]-.5
 ix,iy=math.floor(x),math.floor(y);fx,fy=x-ix,y-iy
 left,bottom=np.rint(np.array(rect[:2])*[w,h]).astype(int)
 right,top=np.rint((np.array(rect[:2])+rect[2:])*[w,h]).astype(int)-1
 def tap(dx,dy):return float(texture[min(max(iy+dy,bottom),top),min(max(ix+dx,left),right),0]>=depth)
 return ((1-fx)*tap(0,0)+fx*tap(1,0))*(1-fy)+((1-fx)*tap(0,1)+fx*tap(1,1))*fy
rng=np.random.default_rng(268)
with tempfile.TemporaryDirectory() as tmp:
 for variant in range(8):
  raw=base64.b64decode((ROOT/f'research/gi-source-r267/gi-{variant}.dxbc.b64').read_bytes())
  _,a=generate(raw,Path(tmp)/'shader.dxbc')
  for n in range(200):
   tex=rng.random((16,16,4),dtype=np.float32)
   left,bottom=2,3;size=1 if n%10==0 else 8
   rect=np.array([left/16,bottom/16,size/16,size/16],dtype=np.float32)
   uv=np.array([(left+rng.random()*size)/16,(bottom+rng.random()*size)/16],dtype=np.float32)
   depth=np.float32(rng.random())
   if n%10==1:depth=np.float32(-1)
   if n%10==2:depth=np.float32(2)
   expected=reference(tex,uv,depth,rect);actual=run(a,tex,uv,depth,rect)
   assert abs(actual-expected)<2e-5,(variant,n,actual,expected)
   assert 0<=actual<=1
  tex=np.zeros((16,16,4),dtype=np.float32);tex[:,8:,0]=1
  rect=np.array([0,0,1,1],dtype=np.float32)
  values=[run(a,tex,(u,.5),.5,rect) for u in (7.5/16,7.75/16,8/16,8.25/16,8.5/16)]
  assert np.allclose(values,[0,.25,.5,.75,1]),values
  print(f'PASS variant {variant}: emitted-token execution, 200 random/border/thin-tile/lit/shadowed cases; continuous edge ramp')
