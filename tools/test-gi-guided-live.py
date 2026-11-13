"""Compare actual emitted IR with independent vectorized sampling formulas."""
from pathlib import Path
import sys,re,struct,json
import numpy as np
from gi_live_sampling import transform as live
from gi_single_producer import owned_raygen
from gi_guided_live import transform

source=Path(sys.argv[1]).read_text();owned=owned_raygen(live(source,0),0);guided=transform(owned)
lines=re.findall(r'^  (%G434_\d+) = (.*)$',guided,re.M)
assert lines
rng=np.random.default_rng(434);N=100000
normal=rng.normal(size=(N,3)).astype('f');normal/=np.linalg.norm(normal,axis=1)[:,None]
origin=(rng.normal(size=(N,3))+[-82,25,111]).astype('f')
# Include bound center, close exterior, behind-normal and far-field cases.
origin[:100]=[-82.0759007,25.003847,110.55301]
x=rng.integers(0,3840,N,dtype='uint32');y=rng.integers(0,2160,N,dtype='uint32');frame=rng.integers(0,100000,N,dtype='uint32')
old=rng.normal(size=(N,3)).astype('f');old/=np.linalg.norm(old,axis=1)[:,None]
count=np.ones(N,dtype='uint32');count[-1000:]=4
values={'%GI307SeedA':x*np.uint32(73856093)^y*np.uint32(19349663),'%GI307FrameSeed':frame*np.uint32(83492791),'%226':count}
for name,a in zip(['%255','%256','%257'],origin.T):values[name]=a
for name,a in zip(['%FMad61','%FMad59','%FMad57'],normal.T):values[name]=a
for name,a in zip(['%GISingleDir0','%GISingleDir1','%GISingleDir2'],old.T):values[name]=a

def arg(a):
    a=a.strip()
    if a.startswith('%'):return values[a]
    if a.startswith('0x'):return np.float32(struct.unpack('>d',bytes.fromhex(a[2:]))[0])
    return np.uint32(a)
for name,expr in lines:
    if expr.startswith('call float'):
        opcode,a=re.search(r'i32 (\d+), float ([^)]+)',expr).groups();a=arg(a)
        result={12:np.cos,13:np.sin,24:np.sqrt,25:lambda v:1/np.sqrt(v)}[int(opcode)](a)
    elif expr.startswith('select'):
        c,a,b=re.fullmatch(r'select i1 ([^,]+), (?:float|i32) ([^,]+), (?:float|i32) (.+)',expr).groups();result=np.where(arg(c),arg(a),arg(b))
    elif expr.startswith('fcmp'):
        op,a,b=re.fullmatch(r'fcmp (\w+) float ([^,]+), (.+)',expr).groups();a,b=arg(a),arg(b);result={'oge':np.greater_equal,'ogt':np.greater,'olt':np.less,'ole':np.less_equal}[op](a,b)
    elif expr.startswith('icmp'):
        a,b=re.fullmatch(r'icmp eq i32 ([^,]+), (.+)',expr).groups();result=arg(a)==arg(b)
    elif expr.startswith('uitofp'):
        result=arg(expr.split()[2]).astype('f')
    else:
        op,typ,a,b=re.fullmatch(r'(\w+) (float|i32) ([^,]+), (.+)',expr).groups();a,b=arg(a),arg(b)
        result={'fadd':np.add,'fsub':np.subtract,'fmul':np.multiply,'fdiv':np.divide,'xor':np.bitwise_xor,'lshr':np.right_shift,'mul':np.multiply}[op](a,b)
    values[name]=result
trace=re.search(r'^  call void @dx.op.traceRay.struct.HitData\((.+)\)',guided,re.M).group(1)
dirs=re.findall(r'float (%G434_\d+)',trace)
assert len(dirs)==3
actual=np.stack([values[k] for k in dirs],axis=1)
mask=values[lines[-1][0]]
assert np.all(mask[count!=1]==1)
assert np.array_equal(mask[count==1],(np.sum(normal[count==1]*actual[count==1],axis=1)>0).astype('uint32'))
seed=values['%GI307SeedA']^values['%GI307FrameSeed']
def unit(salt):
    v=seed^np.uint32(salt);v=(v^(v>>16))*np.uint32(2146121005);v=(v^(v>>15))*np.uint32(2221713035);v^=v>>16
    return (v>>8).astype('f')*np.float32(1/16777216)
u,v=unit(1757159915),unit(48610963)

def local(axis,z,angle):
    sign=np.where(axis[:,2]>=0,1.,-1.).astype('f');a=-1/(sign+axis[:,2]);b=axis[:,0]*axis[:,1]*a
    t=np.stack([1+sign*axis[:,0]**2*a,sign*b,-sign*axis[:,0]],axis=1)
    bt=np.stack([b,sign+axis[:,1]**2*a,-axis[:,1]],axis=1)
    radius=np.sqrt(np.maximum(0,1-z*z))
    return axis*z[:,None]+radius[:,None]*(np.cos(angle)[:,None]*t+np.sin(angle)[:,None]*bt)
def cone(center,radius):
    delta=np.asarray(center,dtype='f')-origin;d2=np.sum(delta*delta,axis=1);axis=delta/np.sqrt(np.maximum(d2,1e-20))[:,None]
    axis=np.where((d2>1e-20)[:,None],axis,np.array([0,0,1],dtype='f'))
    r=np.float32(radius);sin2=np.minimum(1,r*r/np.maximum(d2,1e-20));width=np.where(d2<=r*r,2,np.maximum(1e-6,sin2/(1+np.sqrt(np.maximum(0,1-sin2)))))
    return axis,width
axes=[];widths=[]
for c,r in [([-82.0759007,25.003847,110.55301],.160592319),([-82.0279381,25.345521,110.818513],.136421514)]:
    a,w=cone(c,r);axes.append(a);widths.append(w)
choose=u>=.05;axis=np.where(choose[:,None],axes[1],axes[0]);width=np.where(choose,widths[1],widths[0]);remap=u/np.float32(.05)-choose.astype('f')
expectedGuide=local(axis,1-v*width,np.float32(2*np.pi)*remap)
norm=normal/np.sqrt(np.sum(normal*normal,axis=1))[:,None]
base=local(norm,np.sqrt(v),np.float32(2*np.pi)*((u-np.float32(.1))/np.float32(.9)))
expected=np.where((u<.1)[:,None],expectedGuide,base);expected[count!=1]=old[count!=1]
error=np.max(np.abs(actual-expected),axis=1)
assert error.max()<2e-5,float(error.max())
assert np.array_equal(actual[count!=1],old[count!=1])
assert np.isfinite(actual).all()
assert np.max(np.abs(np.linalg.norm(actual,axis=1)-1))<2e-5
print(json.dumps({'samples':N,'max_direction_error':float(error.max()),'multi_ray_fallback_exact':True,'actual_emitted_ir_checked':True}))
