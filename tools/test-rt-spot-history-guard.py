"""Execute the emitted guard words; independently check the packaged shader."""
import base64, math, random, struct, sys
from pathlib import Path
from rt_spot_history_guard import generate, guard_instructions, guarded_history, MAX_HISTORY_DIFFERENCE, BLEND_OFFSET, ROOT
sys.path.insert(0,str(ROOT/'research/native-integration-audit/tools'))
from decode_dxbc import decode
from dxbc_checksum import checksum

def run(current,history,point):
    regs=[[0.0]*4 for _ in range(6)]
    regs[1][2]=current;regs[2]=[history,point,history,history]
    words=guard_instructions();offset=0
    def read(i):
        token=words[i];i+=1;modifier=0
        if token>>31:
            modifier=(words[i]>>6)&255;i+=1
        kind=(token>>12)&255
        if kind==4:
            value=struct.unpack('<f',struct.pack('<I',words[i]))[0]
            return [value]*4,i+1
        assert kind==0
        value=regs[words[i]];i+=1
        value=[value[(token>>(4+2*c))&3] for c in range(4)]
        if modifier==1:value=[-x for x in value]
        elif modifier==2:value=[abs(x) for x in value]
        else:assert modifier==0
        return value,i
    while offset<len(words):
        op=words[offset]&2047;end=offset+(words[offset]>>24)
        mask=(words[offset+1]>>4)&15;reg=words[offset+2]
        a,i=read(offset+3);b,i=read(i)
        if op==0:result=[x+y for x,y in zip(a,b)]
        elif op==29:result=[0xffffffff if x>=y else 0 for x,y in zip(a,b)]
        elif op==55:
            c,i=read(i);result=[b[j] if a[j] else c[j] for j in range(4)]
        else:raise AssertionError(op)
        assert i==end
        for j in range(4):
            if mask&(1<<j):regs[reg][j]=result[j]
        offset=end
    return regs[2]

random.seed(141)
for current,history in [(0,1),(1,0),(.5,.625),(.5,.625001),(.5,.374999),(.5,.5),(0,float('nan')),(1,float('inf'))]+[(random.random(),random.random()) for _ in range(10000)]:
    point=.372
    expected=history if abs(history-current)<=MAX_HISTORY_DIFFERENCE else current
    actual=run(current,history,point)
    assert actual==[expected,point,expected,expected]
    for weight in [0,.35,.7]:
        output=current+weight*(actual[0]-current)
        assert abs(output-current)<=weight*MAX_HISTORY_DIFFERENCE+1e-12
# Stale history inside a mixed [0,1] neighborhood used to pass the min/max clamp.
assert run(0,1,.372)==[0,.372,0,0]
assert run(.5,.55,.372)==[.55,.372,.55,.55]
generate()
original=base64.b64decode((ROOT/'assets/rt-spot/rt_spot_history.dxbc.b64').read_text())
result=(ROOT/'build/rt_spot_guarded_history.dxbc').read_bytes()
assert result[4:20]==checksum(result)
# Independent decode proves exact destination lanes, modifiers and blend order.
new=decode(ROOT/'build/rt_spot_guarded_history.dxbc')['instructions']
idx=next(i for i,x in enumerate(new) if x['dword']==BLEND_OFFSET)
assert [(x['name'],x['operands']) for x in new[idx:idx+3]]==[
 ('add',['r[3].x','r[2].xxxx','-r[1].zzzz']),
 ('ge',['r[3].x','l(0.125)','abs(r[3].xxxx)']),
 ('movc',['r[2].xzw','r[3].xxxx','r[2].xxxx','r[1].zzzz'])]
import tempfile
with tempfile.TemporaryDirectory() as d:
    p=Path(d)/'original.dxbc';p.write_bytes(original);old=decode(p)['instructions']
    restored=new[:idx]+new[idx+3:]
    assert [(x['name'],x['operands']) for x in restored]==[(x['name'],x['operands']) for x in old]
try:guarded_history(original[:-1]+bytes([original[-1]^1]))
except ValueError:pass
else:raise AssertionError('Unsupported input accepted')
print('PASS: emitted guard, threshold edges, nonfinite history, 10000 signals, point lane and native instructions preserved')
