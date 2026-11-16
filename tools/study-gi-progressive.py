"""Synthetic angular integration study, not a game-quality or performance claim.
Reports RMS against exact integrals over sliding history windows.
"""
import numpy as np,sys,json


def h(x):
 x=np.asarray(x,dtype=np.uint32);x=x^(x>>16);x=x*np.uint32(0x7feb352d);x=x^(x>>15);x=x*np.uint32(0x846ca68b);return x^(x>>16)
P=4096;F=64
pix=np.arange(P,dtype=np.uint32)[:,None,None];frame=np.arange(F,dtype=np.uint32)[None,:,None]
seedA=(pix*73856093)^np.uint32(714*19349663&0xffffffff);batch=np.uint32(2*2654435761&0xffffffff)
def old(n):
 ray=np.arange(n,dtype=np.uint32)[None,None,:];seed=seedA^(frame*np.uint32(83492791))^batch^(ray*np.uint32(1597334677))
 u=(h(seed^np.uint32(0x68bc21eb))>>8).astype(np.float32)*2**-24;v=(h(seed^np.uint32(0x02e5be93))>>8).astype(np.float32)*2**-24
 key=seedA^batch^((frame>>2)*np.uint32(2654435761));mix=(key^(key>>16))*np.uint32(2146121005)
 cols=2;rows=n//2;x=ray%cols;y=ray//cols
 xb=((mix>>12)+x*73+y*67)%rows;yb=((mix>>23)^y^x)%cols
 return (x.astype(np.float32)+(xb.astype(np.float32)+u)/np.float32(rows))/np.float32(cols),(y.astype(np.float32)+(yb.astype(np.float32)+v)/np.float32(cols))/np.float32(rows)

def sobol(n):
 index=frame*n+np.arange(n,dtype=np.uint32)[None,None,:];gray=index^(index>>1);a=np.zeros_like(index);b=np.zeros_like(index);v=np.uint32(1<<31)
 for bit in range(32):
  on=((gray>>bit)&1)!=0;a^=np.where(on,np.uint32(1<<(31-bit)),np.uint32(0));b^=np.where(on,v,np.uint32(0));v^=v>>np.uint32(1)
 def scramble(v,seed):
  out=np.zeros((P,F,n),dtype=np.uint32)
  prefix=np.zeros_like(v)
  for bit in range(31,7,-1):
   flip=h(seed^prefix^np.uint32((bit*0x9e3779b9)&0xffffffff))&1
   out|=(((v>>bit)&1)^flip)<<bit
   prefix|=v&np.uint32(1<<bit)
  return out
 a=scramble(a,h(seedA^batch^np.uint32(0x68bc21eb)));b=scramble(b,h(seedA^batch^np.uint32(0x02e5be93)))
 return (a>>8).astype(np.float32)*2**-24,(b>>8).astype(np.float32)*2**-24
cases={
 'broad':(lambda u,v:(np.cos(u*2*np.pi)+1)*np.sqrt(v),2/3),
 'small_patch':(lambda u,v:((u>.37)&(u<.47)&(v>.61)&(v<.71)).astype(float),.01),
 'thin_band':(lambda u,v:((u>.41)&(u<.44)).astype(float),.03),
 'diagonal':(lambda u,v:(u+v>1).astype(float),.5),
 'occlusion':(lambda u,v:((u<.27)|(v<.42)).astype(float),.27+.42-.27*.42)}
result={}
for name,fn in [('old4',lambda:old(4)),('old10',lambda:old(10)),('sobol4',lambda:sobol(4))]:
 u,v=fn();out={}
 for cn,(func,mean) in cases.items():
  estimates=func(u,v).mean(axis=2);out[cn]={}
  for w in [1,4,8,16]:
   e=np.lib.stride_tricks.sliding_window_view(estimates,w,axis=1).mean(axis=2);out[cn][w]=float(np.sqrt(np.mean((e-mean)**2)))
 result[name]=out
print(json.dumps(result,indent=2))

