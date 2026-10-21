"""Generate and numerically validate a compact, tile-clamped PCF shader patch.

Only the native spotlight PCF block is replaced. Compare four integer texel
loads before interpolating visibility; never interpolate depth before testing.
"""
from pathlib import Path
import argparse, hashlib, math, random, struct, sys
from dxbc_checksum import checksum
sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'research/native-integration-audit/tools'))
from decode_dxbc import opnames

def bits(x): return struct.unpack('<I',struct.pack('<f',x))[0]
def reg(n, sw='xyzw', neg=False): return ('reg',n,sw,neg)
def imm(x): return ('imm',float(x))
def dest(n,mask): return (n,mask)
def operand(a):
    if a[0]=='imm': return [0x4001,bits(a[1])]
    _,n,sw,neg=a
    sw=(sw*4)[:4]
    token=0x100006 | sum('xyzw'.index(c) << (4+2*i) for i,c in enumerate(sw))
    return [token|0x80000000,0x41,n] if neg else [token,n]

def program():
    p=[]
    def emit(op,d,*args): p.append((op,d,args))
    # r39.xy/zw hold inclusive tile texel limits; r38.xy base, zw fraction.
    emit('resinfo',dest(38,'xy'),imm(0))
    emit('mul',dest(39,'xy'),reg(35,'xyxx'),reg(38,'xyxx'))
    emit('round_ni',dest(39,'xy'),reg(39,'xyxx'))
    emit('add',dest(39,'zw'),reg(35,'xxxy'),reg(35,'zzzw'))
    emit('mul',dest(39,'zw'),reg(39,'zzzw'),reg(38,'xxxy'))
    emit('round_ni',dest(39,'zw'),reg(39,'zzzw'))
    emit('add',dest(39,'zw'),reg(39,'zzzw'),imm(-1))
    emit('mad',dest(38,'xy'),reg(37,'xzxx'),reg(38,'xyxx'),imm(-.5))
    emit('frc',dest(38,'zw'),reg(38,'xxxy'))
    emit('round_ni',dest(38,'xy'),reg(38,'xyxx'))
    emit('mov',dest(29,'w'),imm(0))
    emit('mov',dest(37,'w'),imm(0))
    for x,y in [(0,0),(1,0),(0,1),(1,1)]:
        emit('add',dest(37,'x'),reg(38,'x'),imm(x))
        emit('add',dest(37,'z'),reg(38,'y'),imm(y))
        emit('max',dest(37,'xz'),reg(37,'xxzx'),reg(39,'xxyx'))
        emit('min',dest(37,'xz'),reg(37,'xxzx'),reg(39,'zzwz'))
        emit('ftoi',dest(37,'xz'),reg(37,'xxzx'))
        emit('ld',dest(30,'w'),reg(37,'xzww'))
        emit('ge',dest(30,'w'),reg(30,'w'),reg(32,'z'))
        emit('and',dest(30,'w'),reg(30,'w'),imm(1))
        if x: emit('mov',dest(37,'x'),reg(38,'z'))
        else: emit('add',dest(37,'x'),imm(1),reg(38,'z',True))
        if y: emit('mov',dest(37,'z'),reg(38,'w'))
        else: emit('add',dest(37,'z'),imm(1),reg(38,'w',True))
        emit('mul',dest(37,'x'),reg(37,'x'),reg(37,'z'))
        emit('mad',dest(29,'w'),reg(30,'w'),reg(37,'x'),reg(29,'w'))
    return p

def encode(p):
    out=[]
    for op,(n,mask),args in p:
        words=[0x100002|sum(16<<'xyzw'.index(c) for c in mask),n]
        for a in args: words+=operand(a)
        # Same SM5.1 resource range identifier and register as native t12.
        if op=='resinfo': words += [0x207e46,12,12]
        if op=='ld': words += [0x207006,12,12] # Depth .x in every destination lane.
        out += [((len(words)+1)<<24)|opnames.index(op),*words]
    return out

def evaluate(p, texture, uv, tile, depth):
    height=len(texture);width=len(texture[0]);r=[[.123]*4 for _ in range(40)]
    r[35]=list(tile);r[37][0],r[37][2]=uv;r[32][2]=depth
    loads=[]
    def read(a):
        if a[0]=='imm':return [a[1]]*4
        sw=(a[2]*4)[:4];return [r[a[1]]['xyzw'.index(c)]*(-1 if a[3] else 1) for c in sw]
    for op,(n,mask),args in p:
        a=[read(v) for v in args]
        if op=='resinfo':v=[width,height,1,0]
        elif op=='ld':
            x,y=int(a[0][0]),int(a[0][1]);loads.append((x,y));v=[texture[y][x]]*4
        else:
            v=[]
            for i in range(4):
                z=[q[i] for q in a]
                if op=='mov':value=z[0]
                elif op=='add':value=z[0]+z[1]
                elif op=='mul':value=z[0]*z[1]
                elif op=='mad':value=z[0]*z[1]+z[2]
                elif op=='round_ni':value=math.floor(z[0])
                elif op=='frc':value=z[0]-math.floor(z[0])
                elif op=='max':value=max(z)
                elif op=='min':value=min(z)
                elif op=='ftoi':value=int(z[0])
                elif op=='ge':value=-1 if z[0]>=z[1] else 0
                elif op=='and':value=1. if z[0]==-1 else 0.
                else:raise AssertionError(op)
                v.append(value)
        for c in mask:r[n]['xyzw'.index(c)]=v['xyzw'.index(c)]
    return r[29][3],loads,r

def validate(p):
    rng=random.Random(61);width,height=16,12;tile=(.25,.25,.5,.5)
    texture=[[rng.random() for _ in range(width)] for _ in range(height)]
    for _ in range(1000):
        uv=(rng.uniform(.24,.76),rng.uniform(.24,.76));depth=rng.random()
        value,loads,r=evaluate(p,texture,uv,tile,depth)
        px,py=uv[0]*width-.5,uv[1]*height-.5;bx,by=math.floor(px),math.floor(py);fx,fy=px-bx,py-by
        expected=0
        for dx,dy in [(0,0),(1,0),(0,1),(1,1)]:
            x=min(11,max(4,bx+dx));y=min(8,max(3,by+dy))
            expected+=(texture[y][x]>=depth)*(fx if dx else 1-fx)*(fy if dy else 1-fy)
        assert abs(value-expected)<1e-12 and 0<=value<=1
        assert len(loads)==4 and all(4<=x<=11 and 3<=y<=8 for x,y in loads)
        assert r[37][1]==.123 and r[28]==[.123]*4 and r[32][2]==depth
    # A sharp depth boundary must produce a fractional visibility, not binary steps.
    texture=[[0. if x<8 else 1. for x in range(width)] for _ in range(height)]
    value,_,_=evaluate(p,texture,(.5,.5),tile,.5);assert value==.5
    print('PASS: 1000 visibility/atlas-boundary cases; half-visible edge; live register preservation')

def main():
    ap=argparse.ArgumentParser();ap.add_argument('native_shader',type=Path);ap.add_argument('output_directory',type=Path);args=ap.parse_args()
    source=args.native_shader.read_bytes();assert hashlib.sha256(source).hexdigest()=='bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff'
    p=program();validate(p);code=encode(p)
    # Replace full PCF sampling/normalization block; branches before/after stay intact.
    offset=11152+6141*4;count=7069-6141
    assert len(code)<=count
    code += [0x100003a]*(count-len(code))
    old=struct.unpack_from('<'+'I'*count,source,offset);output=bytearray(source)
    struct.pack_into('<'+'I'*count,output,offset,*code);output[4:20]=checksum(output)
    args.output_directory.mkdir(parents=True,exist_ok=True)
    (args.output_directory/'SpotBilinear.dxbc').write_bytes(output)
    text='// Generated by tools/generate-spot-bilinear-filter.py; four depth tests, then bilinear visibility.\n'
    for suffix,values in [('Expected',old),('Replacement',code)]:
        text+=f'inline constexpr uint32_t kSpotBilinear{suffix}[] = {{\n'
        for i in range(0,len(values),12):text+='    '+','.join(hex(v)+'u' for v in values[i:i+12])+',\n'
        text+='};\n'
    text+=f'inline constexpr InstructionEdit kSpotBilinearEdits[] = {{{{{offset}, kSpotBilinearExpected, kSpotBilinearReplacement, {count}}}}};\n'
    text+='inline constexpr unsigned char kSpotBilinearChecksum[] = {'+','.join(map(str,output[4:20]))+'};\n'
    text+=f'''inline constexpr ShaderPatch kSpotBilinear = {{
    "SpotBilinear", kDirect.size, kDirect.nativeSha, "{hashlib.sha256(output).hexdigest()}",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, kSpotBilinearChecksum,
    kSpotBilinearEdits, 1
}};
'''
    (args.output_directory/'spot-bilinear.inc').write_text(text)
    # Standalone validation container: identical block, known initialized inputs,
    # same texture range identifier, and an output UAV so the work remains live.
    get=lambda dw,n:list(struct.unpack_from('<'+'I'*n,source,11152+dw*4))
    declarations=get(139,7)+get(202,7)+[0x2000068,40,0x400009b,1,1,1]
    declarations[2]=0 # Standalone shader has one SRV range; keep binding register t12.
    initial=[]
    for n,mask,value in [(35,'x',.25),(35,'y',.25),(35,'z',.5),(35,'w',.5),(37,'x',.5),(37,'z',.5),(32,'z',.5)]:
        initial+=encode([('mov',dest(n,mask),(imm(value),))])
    store=[0xb0000a4,0x21e0f2,0,0,0x4002,0,0,0,0,0x100ff6,29]
    block=encode(p)
    position=0
    while position<len(block):
        length=block[position]>>24&127
        if block[position]&2047 in (opnames.index('resinfo'),opnames.index('ld')):
            block[position+length-2]=0 # Remap range ID only; same register and operations.
        position+=length
    shex=[0x50051,0]+declarations+initial+block+store+[0x100003e]
    shex[1]=len(shex);payload=struct.pack('<'+'I'*len(shex),*shex)
    chunks=[(b'ISGN',struct.pack('<2I',0,8)),(b'OSGN',struct.pack('<2I',0,8)),(b'SHEX',payload)]
    offset=32+4*len(chunks);offsets=[];body=bytearray()
    for tag,data in chunks:
        offsets.append(offset);body+=tag+struct.pack('<I',len(data))+data;offset+=8+len(data)
    container=bytearray(b'DXBC'+bytes(16)+struct.pack('<3I',1,offset,len(chunks))+struct.pack('<3I',*offsets)+body)
    container[4:20]=checksum(container)
    header='// Generated standalone shader containing the exact R61 PCF block.\ninline constexpr unsigned char kSpotBilinearValidationShader[] = {\n'
    for i in range(0,len(container),24):header+='    '+','.join(str(v) for v in container[i:i+24])+',\n'
    header+='};\n'
    (args.output_directory/'spot-bilinear-validation.h').write_text(header)

if __name__=='__main__':main()
