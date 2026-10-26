"""Hash-locked SPEC16 scheduling experiment; preserves native hit shading tokens."""
import argparse, base64, hashlib, json, pathlib, struct
from dxbc_transform import transform_shex
SOURCE_SHA='4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9'
OP=dict(add=0,and_=1,breakc=3,endif=21,endloop=22,iadd=30,if_=31,ieq=32,ilt=34,imad=35,loop=48,mov=54,ld=45,ld_structured=167,store_structured=168,sync=190)
def reg(n, components='xyzw', dest=False, ty=0, indices=None):
    indices=([n] if ty==0 or ty==31 else []) if indices is None else indices
    token=2|(ty<<12)|(len(indices)<<20)
    if dest: token|=sum(1<<('xyzw'.index(c)+4) for c in components)
    elif len(components)==1: token|=8|('xyzw'.index(components)<<4)
    else:
        assert len(components)==4
        token|=4|sum('xyzw'.index(c)<<(4+2*i) for i,c in enumerate(components))
    return [token,*indices]
def dst(n,c='xyzw'):return reg(n,c,True)
def src(n,c='xyzw'):return reg(n,c)
def inp(ty,c):return reg(0,c,ty=ty)
def imm(n):return [0x4001,n]
def inst(name,*args,flags=0):
    words=[x for a in args for x in a]
    return [OP[name]|flags|((len(words)+1)<<24),*words]
def instructions(w):
    result=[];p=2
    while p<len(w):
        length=(w[p]>>24)&127
        if not length: raise ValueError('Unexpected custom/zero length instruction')
        result.append(w[p:p+length]);p+=length
    assert p==len(w)
    return result

def parallelize(data):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unsupported native shader')
    def patch(w):
        a=instructions(w);assert len(a)==984
        assert a[138][0]&2047==48 and a[964][0]&2047==22
        assert OP['ilt']==(a[140][0]&2047) # retain native signed sample-limit comparison
        out=[]
        def emit(name,*args,flags=0):out.extend(inst(name,*args,flags=flags))
        def copy(lo,hi):
            for ins in a[lo:hi]:out.extend(ins)
        copy(0,39)
        out.extend([0x0200005f,0x21072]) # group ID xyz (z selects original pixel row)
        copy(40,41)
        out.extend([0x02000068,46])
        copy(42,43)
        out.extend([0x050000a0,*reg(0,'',True,31),16,128]) # g0, stride16, count128
        emit('imad',dst(1,'x'),inp(33,'x'),imm(8),inp(34,'x'))
        emit('imad',dst(1,'y'),inp(33,'y'),imm(16),inp(33,'z'))
        emit('imad',dst(43,'x'),inp(34,'y'),imm(8),inp(34,'x'))
        emit('mov',dst(44),imm(0))
        emit('store_structured',reg(0,'xyzw',True,31),src(43,'x'),imm(0),src(44))
        copy(60,63)
        emit('mov',dst(43,'y'),src(0,'z')) # save bounds across native shading
        copy(63,137) # includes native bounds IF and invariants
        emit('mov',dst(1,'z'),inp(34,'y'))
        # Native signed sample limit, before accessing a layer.
        emit('ilt',dst(43,'z'),src(1,'z'),reg(0,'z',ty=8,indices=[6,6,0]))
        emit('if_',src(43,'z'),flags=0x40000)
        emit('mov',dst(1,'z'),imm(0))
        emit('loop')
        out.extend(a[963]) # material ID of current layer
        emit('ieq',dst(43,'w'),src(5,'w'),imm(0xfffe))
        emit('if_',src(43,'w'),flags=0x40000)
        emit('mov',dst(43,'z'),imm(0))
        out.extend([0x01000002]) # unconditional break
        emit('endif')
        emit('ieq',dst(43,'w'),src(1,'z'),inp(34,'y'))
        emit('breakc',src(43,'w'),flags=0x40000)
        out.extend(a[962])
        emit('endloop')
        emit('if_',src(43,'z'),flags=0x40000)
        copy(143,961) # untouched per-hit lighting and clamp, no sample accumulation
        emit('mov',dst(44,'xyz'),src(14,'xywx'))
        emit('mov',dst(44,'w'),imm(1))
        emit('store_structured',reg(0,'xyzw',True,31),src(43,'x'),imm(0),src(44))
        emit('endif')
        emit('endif')
        emit('endif') # bounds; all threads reach barrier
        emit('sync',flags=0x1800)
        emit('ieq',dst(43,'z'),inp(34,'y'),imm(0))
        emit('and_',dst(43,'z'),src(43,'z'),src(43,'y'))
        emit('if_',src(43,'z'),flags=0x40000)
        emit('mov',dst(18,'xyz'),imm(0))
        emit('mov',dst(1,'z'),imm(0))
        emit('loop')
        emit('ieq',dst(43,'z'),src(1,'z'),imm(16))
        emit('breakc',src(43,'z'),flags=0x40000)
        emit('imad',dst(43,'x'),src(1,'z'),imm(8),inp(34,'x'))
        emit('ld_structured',dst(44),src(43,'x'),imm(0),reg(0,ty=31))
        emit('breakc',src(44,'w')) # native sentinel/sample-limit termination
        emit('mov',dst(14,'xyw'),src(44,'xyxz'))
        out.extend(a[961]) # exact native addition order
        out.extend(a[962])
        emit('endloop')
        copy(965,984) # original normalization, final multiplier, store, endif, ret
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,required=True);p.add_argument('--header',type=pathlib.Path);args=p.parse_args()
    data=base64.b64decode(args.source.read_text());result=parallelize(data)
    if args.header:
        args.header.parent.mkdir(parents=True,exist_ok=True)
        args.header.write_text('inline constexpr unsigned char kSpecParallelShader[] = {\n'+','.join(str(b) for b in result)+'\n};\n')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_bytes(result)
    print(json.dumps(dict(source_sha256=SOURCE_SHA,parallel_sha256=hashlib.sha256(result).hexdigest(),bytes=len(result),status='generated_not_gpu_validated')))

# GPU differential fixtures retain the production scheduling, clamp and resolve.
# Only hit shading is replaced by a positive, pixel/sample-dependent color.
def fixture(data, native):
    holder=[]
    transform_shex(native,lambda w:holder.extend(instructions(w)))
    body=sum(holder[143:956],[])
    test=[]
    for component in 'xyz':
        index={'x':'z','y':'x','z':'y'}[component]
        test+=inst('iadd',dst(29,component),src(1,index),imm(1))
    test+=[(5<<24)|86,*dst(29,'xyz'),*src(29,'xyzx')] # utof
    def patch(w):
        positions=[i for i in range(len(w)) if w[i:i+len(body)]==body]
        assert len(positions)==1
        p=positions[0];w[p:p+len(body)]=test
    return transform_shex(data,patch)
