"""Independently verify every used buffer word retains its original byte address."""
import re,sys
from pathlib import Path
from gi_evaluator_loads import transform,STRIDES

def addresses(source):
 loads={m[1]:(m[2],m[3],int(m[4])) for m in re.finditer(r'(%\w+) = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32\(i32 68, %dx.types.Handle (%\w+), i32 ([^,]+), i32 (\d+)\)',source)}
 return {m[1]:(loads[m[2]][0],loads[m[2]][1],loads[m[2]][2]+4*int(m[3])) for m in re.finditer(r'(%\w+) = extractvalue %dx.types.ResRet.i32 (%\w+), ([0-3])',source) if m[2] in loads}

def verify(source):
 result,report=transform(source)
 original=re.sub(r'%(\d+)\b',r'%GI442v\1',source)
 assert addresses(original)==addresses(result), 'Used field address changed'
 def arithmetic(s):
  return [x for x in s.splitlines() if x.startswith('  ') and ' = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32(' not in x and ' = extractvalue %dx.types.ResRet.i32 ' not in x]
 assert arithmetic(original)==arithmetic(result), 'Non-load instruction changed'
 for x in report['reads']:assert x['old_offset']==x['new_offset']+x['lane']*4
 assert report['after']<report['before']
 print('PASS byte-address equality, all non-load instructions unchanged:',report['before'],'->',report['after'])

if len(sys.argv)>1 and sys.argv[1]=='--fixture':
 lines=[]
 for slot,stride in STRIDES.items():
  lines += [f'struct GINativeRaw{slot} {{ uint d[{stride//4}]; }};',f'StructuredBuffer<GINativeRaw{slot}> b{slot}:register(t{slot});']
 lines += ['RWStructuredBuffer<uint4> result:register(u0);','[numthreads(8,1,1)] void main(uint3 id:SV_DispatchThreadID){','uint row=id.x;if(row>=17)return;']
 group=0
 for slot,stride in STRIDES.items():
  for first in range(0,stride//4,4):
   order=[(i+group)%4 for i in range(4)]
   lines.append(f'result[row*57+{group}]=uint4('+','.join(f'b{slot}[row].d[{first+i}]' for i in order)+');')
   group+=1
 assert group==57
 lines+=['}'];Path(sys.argv[2]).write_text('\n'.join(lines)+'\n')
else:
 for p in sys.argv[1:]:verify(Path(p).read_text())
