"""Coalesce adjacent scalar reads of pinned GI structured-buffer records.
Only share reads in one basic block. Preserve byte addresses and all arithmetic.
"""
import re,json,argparse,subprocess
from pathlib import Path
STRIDES={5:336,7:160,11:320,13:80,15:16}
LOAD=re.compile(r'^(  (%\w+) = call %dx.types.ResRet.i32 @dx.op.bufferLoad.i32\(i32 68, %dx.types.Handle (%\w+), i32 ([^,]+), i32 )(\d+)(\).*)$')

def transform(source):
    # Name numeric SSA values/blocks so deleting a load does not leave LLVM slot holes.
    named=re.sub(r'%(\d+)\b',r'%GI442v\1',source)
    named=re.sub(r'^; <label>:(\d+)\s*;.*$',r'GI442v\1:',named,flags=re.M)
    named=named.replace('define void @main() {','define void @main() {\nGI442v0:')
    allowed={}
    for slot,stride in STRIDES.items():
        m=re.search(r'!\d+ = !\{i32 (\d+), %"class.StructuredBuffer<GINativeRaw'+str(slot)+r'>"\* undef, !"[^"]*", i32 0, i32 '+str(slot)+r', i32 1, i32 12, i32 0, !(\d+)\}',named)
        if m:
            if not re.search(r'^!'+m[2]+r' = !\{i32 1, i32 '+str(stride)+r'\}$',named,re.M):raise ValueError('Unexpected GI buffer stride')
            allowed[int(m[1])]=stride
    handles={}
    for h,rid in re.findall(r'(%\w+) = call %dx.types.Handle @dx.op.createHandle\(i32 57, i8 0, i32 (\d+), i32 \d+, i1 false\)',named):
        if int(rid) in allowed:handles[h]=allowed[int(rid)]
    if not handles:raise ValueError('No audited GI structured buffers')
    lines=named.splitlines();groups={};block=0;total=0
    for i,line in enumerate(lines):
        if re.match(r'^[\w.]+:',line) or re.search(r'@dx.op.(?:bufferStore|textureStore|atomic|barrier)',line):block+=1
        m=LOAD.match(line)
        if not m:continue
        total+=1;handle=m[3];offset=int(m[5]);base=offset&~15
        if handle not in handles:continue
        if offset%4 or base+16>handles[handle]:raise ValueError('GI structured read outside audited record')
        # No residency/status use or whole-vector user may be silently rewritten.
        uses=[x for x in lines if re.search(re.escape(m[2])+r'(?![\w.])',x) and x!=line]
        if not uses or any(not re.match(r'^  %\w+ = extractvalue %dx.types.ResRet.i32 '+re.escape(m[2])+r', 0(?:\s*;.*)?$',x) for x in uses):continue
        groups.setdefault((block,handle,m[4],base),[]).append((i,m))
    replacements={};removed=set();mapping=[]
    for (_,handle,index,base),group in groups.items():
        if len(group)<2:continue
        first=group[0][1][2]
        i,m=group[0];lines[i]=m[1]+str(base)+m[6]
        for n,(i,m) in enumerate(group):
            lane=(int(m[5])-base)//4
            replacements[m[2]]=(first,lane)
            mapping.append(dict(value=m[2],handle=handle,index=index,old_offset=int(m[5]),new_offset=base,lane=lane))
            if n:removed.add(i)
    output=[]
    for i,line in enumerate(lines):
        if i in removed:continue
        m=re.match(r'^(  %\w+ = extractvalue %dx.types.ResRet.i32 )(%\w+), 0(.*)$',line)
        if m and m[2] in replacements:
            value,lane=replacements[m[2]];line=m[1]+value+', '+str(lane)+m[3]
        output.append(line)
    if not removed:raise ValueError('No adjacent GI loads found')
    return '\n'.join(output)+'\n',dict(before=total,after=total-len(removed),removed=len(removed),reads=mapping)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True);p.add_argument('--input',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    source=subprocess.check_output([a.dxc,'-dumpbin',a.input],text=True)
    output,report=transform(source);target=Path(a.output);ir=target.with_suffix('.ll');ir.write_text(output)
    subprocess.run([a.assembler,str(ir),str(target),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
    target.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({k:v for k,v in report.items() if k!='reads'}))
