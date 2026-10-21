"""Execute the emitted history-clamp instructions against synthetic images."""
import base64, importlib.util, random, struct
from pathlib import Path
from rt_spot_history import history_instructions, OP
from dxbc_checksum import checksum
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('decoder',root/'research/native-integration-audit/tools/decode_dxbc.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)

def execute(image, x, y, history, point):
    regs=[[0.0]*4 for _ in range(6)]
    regs[0][:2]=[x,y];regs[1][2]=image[y][x];regs[2][:2]=[history,point]
    words=history_instructions();pos=0
    def read(i):
        t=words[i];i+=1;kind=(t>>12)&255;dim=(t>>20)&3
        indices=words[i:i+dim];i+=dim
        if kind==4:
            value=[v if v<0x80000000 else v-0x100000000 for v in words[i:i+4]]
            return value,i+4
        if kind==0: value=regs[indices[0]][:]
        elif kind==8:value=[len(image[0]),len(image),0,0]
        elif kind==7:return indices,i
        else:raise AssertionError(kind)
        return [value[(t>>(4+2*c))&3] for c in range(4)],i
    while pos<len(words):
        token=words[pos];end=pos+(token>>24);name=decoder.opnames[token&2047]
        mask=(words[pos+1]>>4)&15;reg=words[pos+2];p=pos+3
        a,p=read(p)
        if name not in ('mov','ftoi'): b,p=read(p)
        if name=='mov':v=a
        elif name=='ftoi':v=list(map(int,a))
        elif name=='iadd':v=[a[i]+b[i] for i in range(4)]
        elif name in ('min','imin'):v=[min(a[i],b[i]) for i in range(4)]
        elif name in ('max','imax'):v=[max(a[i],b[i]) for i in range(4)]
        elif name=='ld':
            assert b==[3,3] and a[3]==0
            v=[image[int(a[1])][int(a[0])]]*4
        else:raise AssertionError(name)
        assert p==end
        for c in range(4):
            if mask&(1<<c):regs[reg][c]=v[c]
        pos=end
    return regs[2][:2]

random.seed(68)
for height,width in [(1,1),(2,2),(3,5),(9,11)]:
    for _ in range(30):
        image=[[random.choice([0.0,0.25,0.75,1.0]) for _ in range(width)] for _ in range(height)]
        for y in range(height):
            for x in range(width):
                history=random.random();point=random.random()
                neighbors=[image[max(0,min(height-1,y+dy))][max(0,min(width-1,x+dx))] for dy in (-1,0,1) for dx in (-1,0,1)]
                actual=execute(image,x,y,history,point)
                assert actual==[max(min(neighbors),min(max(neighbors),history)),point]
for current in [0.0,1.0]:
    assert execute([[current]*3 for _ in range(3)],1,1,1-current,0.37)==[current,0.37]
# Validate the checked-in container, including bounds and instruction parsing.
data=base64.b64decode((root/'assets/rt-spot/rt_spot_history.dxbc.b64').read_text())
assert data[4:20]==checksum(data)
import tempfile
with tempfile.TemporaryDirectory() as folder:
    path=Path(folder)/'history.dxbc';path.write_bytes(data)
    decoder.decode(path)
print('PASS emitted spotlight history clamp: borders, uniform/disoccluded regions, mixed visibility, unchanged point channel')
