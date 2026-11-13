"""R434 fixed-scene guide: replace direction only in audited owned variant 0.
Original origin, range, count, traversal, hit tables and loop stay intact.
The one-ray route is the only guided route; all others preserve
owned directions. Below-hemisphere proposals are traced but weighted zero.
"""
import struct
from gi_single_producer import edit_export, replace_once

def f(x):
    x=struct.unpack('<f',struct.pack('<f',float(x)))[0]
    return '0x'+struct.pack('>d',x).hex().upper()

class IR:
    def __init__(self):self.lines=[];self.n=0
    def op(self,expr):
        self.n+=1;n='%G434_'+str(self.n);self.lines.append('  '+n+' = '+expr);return n
    def binary(self,op,a,b):return self.op(op+' float '+a+', '+b)
    def add(self,a,b):return self.binary('fadd',a,b)
    def sub(self,a,b):return self.binary('fsub',a,b)
    def mul(self,a,b):return self.binary('fmul',a,b)
    def div(self,a,b):return self.binary('fdiv',a,b)
    def unary(self,opcode,a):return self.op(f'call float @dx.op.unary.f32(i32 {opcode}, float {a})')
    def compare(self,op,a,b):return self.op('fcmp '+op+' float '+a+', '+b)
    def select(self,c,a,b):return self.op('select i1 '+c+', float '+a+', float '+b)
    def dot(self,a,b):return self.add(self.add(self.mul(a[0],b[0]),self.mul(a[1],b[1])),self.mul(a[2],b[2]))
    def normalize(self,a):
        inv=self.unary(25,self.dot(a,a));return [self.mul(x,inv) for x in a]
    def maximum(self,a,b):return self.select(self.compare('ogt',a,b),a,b)
    def local(self,axis,z,angle):
        x,y,nz=axis;s=self.select(self.compare('oge',nz,f(0)),f(1),f(-1))
        a=self.div(f(-1),self.add(s,nz));b=self.mul(self.mul(x,y),a)
        t=[self.add(f(1),self.mul(self.mul(self.mul(s,x),x),a)),self.mul(s,b),self.mul(self.mul(f(-1),s),x)]
        bt=[b,self.add(s,self.mul(self.mul(y,y),a)),self.mul(f(-1),y)]
        r=self.unary(24,self.maximum(f(0),self.sub(f(1),self.mul(z,z))))
        co=self.unary(12,angle);si=self.unary(13,angle)
        return [self.add(self.mul(n,z),self.mul(r,self.add(self.mul(co,u),self.mul(si,v)))) for n,u,v in zip(axis,t,bt)]
    def cone(self,origin,center,r):
        delta=[self.sub(f(c),o) for c,o in zip(center,origin)];d2=self.dot(delta,delta)
        inv=self.unary(25,self.maximum(d2,f(1e-20)));valid=self.compare('ogt',d2,f(1e-20));axis=[self.select(valid,self.mul(x,inv),f(1 if i==2 else 0)) for i,x in enumerate(delta)]
        rr=self.mul(f(r),f(r));inside=self.compare('ole',d2,rr);sin2=self.div(rr,self.maximum(d2,rr))
        width=self.div(sin2,self.add(f(1),self.unary(24,self.maximum(f(0),self.sub(f(1),sin2)))))
        return axis,self.select(inside,f(2),self.maximum(f(1e-6),width))

def transform(source):
    def patch(body):
        ir=IR()
        seed=ir.op('xor i32 %GI307SeedA, %GI307FrameSeed')
        def unit(salt):
            v=ir.op('xor i32 '+seed+', '+str(salt))
            for shift,multiplier in ((16,2146121005),(15,2221713035),(16,None)):
                shiftv=ir.op('lshr i32 '+v+', '+str(shift));v=ir.op('xor i32 '+v+', '+shiftv)
                if multiplier:v=ir.op('mul i32 '+v+', '+str(multiplier))
            bits=ir.op('lshr i32 '+v+', 8');val=ir.op('uitofp i32 '+bits+' to float')
            return ir.mul(val,f(1/16777216))
        u=unit(1757159915);v=unit(48610963)
        origin=['%255','%256','%257'];normal=ir.normalize(['%FMad61','%FMad59','%FMad57'])
        # Exact guide from the GPU-tested replay. No temporal resource reuse.
        a,aw=ir.cone(origin,[-82.0759007,25.003847,110.55301],.160592319)
        b,bw=ir.cone(origin,[-82.0279381,25.345521,110.818513],.136421514)
        choose=ir.compare('oge',u,f(.05));axis=[ir.select(choose,y,x) for x,y in zip(a,b)]
        width=ir.select(choose,bw,aw)
        remap=ir.div(u,f(.05));remap=ir.sub(remap,ir.select(choose,f(1),f(0)))
        cone=ir.local(axis,ir.sub(f(1),ir.mul(v,width)),ir.mul(f(6.283185307179586),remap))
        base=ir.local(normal,ir.unary(24,v),ir.mul(f(6.283185307179586),ir.div(ir.sub(u,f(.1)),f(.9))))
        guided=ir.compare('olt',u,f(.1));direction=[ir.select(guided,x,y) for x,y in zip(cone,base)]
        enabled=ir.op('icmp eq i32 %226, 1')
        original=['%GISingleDir0','%GISingleDir1','%GISingleDir2']
        selected=[ir.select(enabled,x,y) for x,y in zip(direction,original)]
        needle='  call void @dx.op.traceRay.struct.HitData('
        if body.count(needle)!=1:raise ValueError('Trace count changed')
        body=body.replace(needle,'\n'.join(ir.lines)+'\n'+needle)
        return replace_once(body,', '.join('float '+d for d in original),', '.join('float '+d for d in selected))
    return edit_export(source,'rayGenMain',patch)
