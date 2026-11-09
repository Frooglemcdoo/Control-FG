"""R174: skip discarded area-light roughness arithmetic, preserving serial SPEC."""
import argparse,base64,hashlib,json
from pathlib import Path
from spec_parallel import SOURCE_SHA,instructions,inst,src
from dxbc_transform import transform_shex

def optimize(data):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unsupported native SPEC shader')
    def patch(w):
        a=instructions(w)
        assert len(a)==984
        out=[]
        for i,ins in enumerate(a):
            if i in (405,406):continue
            if i==943:out+=a[405]+a[406] # retain R172 load scheduling
            if i==568:out+=inst('if_',src(28,'w'),flags=0x40000)
            if i==857:out+=inst('if_',src(26,'w'),flags=0x40000)
            if i in (608,897):out+=inst('endif')
            out+=ins
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--header',type=Path,required=True);args=p.parse_args()
    result=optimize(base64.b64decode(args.source.read_text()))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result)
    args.header.write_text('inline constexpr unsigned char kSpecSerialLifetimesShader[] = {\n'+','.join(map(str,result))+'\n};\n')
    print(json.dumps({'sha256':hashlib.sha256(result).hexdigest(),'guarded_blocks':2,'instructions_skipped_per_false_condition':40,'status':'generated_not_gpu_validated'}))

# WARP fixtures execute the actual 40-instruction blocks; only surrounding hit
# shading is replaced. Alternating samples exercise zero/nonzero radius.
def fixture(data, guarded):
    import struct
    from spec_parallel import dst,imm
    def f(v):return imm(struct.unpack('<I',struct.pack('<f',v))[0])
    def patch(w):
        a=instructions(w);body=[]
        def emit(op,*args,**kw):body.extend(inst(op,*args,**kw))
        emit('and_',dst(28,'w'),src(1,'z'),imm(1))
        emit('ieq',dst(28,'w'),src(28,'w'),imm(1))
        # r22.y is the fallback material roughness; r11.w its complement.
        emit('mov',dst(22,'y'),f(.4));emit('mov',dst(11,'w'),f(.6))
        # MOVC opcode is 55; retain zero-radius division/NaN behavior in native.
        body.extend([0x09000037,*dst(21,'z'),*src(28,'w'),*f(2),*f(0)])
        for r,c,v in [(27,'w',3),(29,'w',.2),(25,'w',4)]:emit('mov',dst(r,c),f(v))
        if guarded:emit('if_',src(28,'w'),flags=0x40000)
        body+=sum(a[568:608],[])
        if guarded:emit('endif')
        body+=a[608]
        emit('mov',dst(18,'w'),src(21,'z'))
        emit('and_',dst(26,'w'),src(1,'x'),imm(1))
        emit('ieq',dst(26,'w'),src(26,'w'),imm(1))
        body.extend([0x09000037,*dst(35,'y'),*src(26,'w'),*f(2),*f(0)])
        for r,c,v in [(24,'w',3),(25,'w',.4),(27,'w',.2),(20,'w',4)]:emit('mov',dst(r,c),f(v))
        if guarded:emit('if_',src(26,'w'),flags=0x40000)
        body+=sum(a[857:897],[])
        if guarded:emit('endif')
        body+=a[897]
        emit('mov',dst(29,'x'),src(18,'w'));emit('mov',dst(29,'y'),src(20,'w'));emit('mov',dst(29,'z'),f(1))
        out=sum(a[:143],[])+body+sum(a[956:],[])
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)
