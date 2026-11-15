"""Move invariant CB reads out of natural-loop headers; reuse dominating CB reads.
Never move dynamic-index reads, arithmetic, SRV/UAV reads, or conditional-body reads.
"""
import re, json, argparse, subprocess
from pathlib import Path
LOAD = re.compile(r'^  (%\w+) = call %dx.types.CBufRet\.(i32|f32) @dx.op.cbufferLoadLegacy\.\2\(i32 59, %dx.types.Handle (%\w+), i32 (\d+)\)(?:\s*;.*)?$')

def named(source):
    source = re.sub(r'%(\d+)\b', r'%GI443v\1', source)
    source = re.sub(r'^; <label>:(\d+)\s*;.*$', r'GI443v\1:', source, flags=re.M)
    # R442 inputs already have a named entry; raw compiler dumps do not.
    return re.sub(r'(define void @main\(\) \{\n)(?!\w+:)', r'\1GI443entry:\n', source)

def cfg(source):
    prefix, body = source.split('define void @main() {\n', 1)
    body, suffix = body.split('\n}', 1)
    blocks = {}; current = None
    for line in body.splitlines():
        m = re.match(r'^(\w+):(?:\s*;.*)?$', line)
        if m: current = m[1]; blocks[current] = []
        elif line.strip():
            if current is None: raise ValueError('Unnamed entry')
            blocks[current].append(line)
    succ = {}; pred = {b:set() for b in blocks}
    for b, lines in blocks.items():
        term = lines[-1]
        if not re.match(r'  (br |ret void)', term): raise ValueError('Unsupported terminator')
        succ[b] = set(re.findall(r'label %(\w+)', term))
        for s in succ[b]: pred[s].add(b)
    entry = next(iter(blocks)); dom = {b:({b} if b==entry else set(blocks)) for b in blocks}
    changed = True
    while changed:
        changed = False
        for b in blocks:
            if b==entry: continue
            if not pred[b]: raise ValueError('Unreachable block')
            new = {b} | set.intersection(*(dom[p] for p in pred[b]))
            if new != dom[b]: dom[b] = new; changed = True
    return prefix, suffix, blocks, succ, pred, dom

def transform(source):
    source = named(source)
    prefix, suffix, blocks, succ, pred, dom = cfg(source)
    handles = {}
    for b, lines in blocks.items():
        for line in lines:
            m = re.match(r'  (%\w+) = call %dx.types.Handle @dx.op.createHandle\(i32 57, i8 2, i32 \d+, i32 \d+, i1 false\)', line)
            if m: handles[m[1]] = b
    # Build natural loops from dominance-qualified back edges.
    loops = {}
    for tail in blocks:
        for head in succ[tail]:
            if head not in dom[tail]: continue
            members = {head}; pending = [tail]
            while pending:
                b = pending.pop()
                if b in members: continue
                members.add(b); pending.extend(pred[b])
            loops.setdefault(head,set()).update(members)
    moved = []
    for head, members in loops.items():
        outside = pred[head] - members
        if len(outside)!=1: continue
        pre = next(iter(outside))
        if succ[pre] != {head}: continue # No speculative work on skipped loops.
        for line in list(blocks[head]):
            m = LOAD.match(line)
            if not m or m[3] not in handles or handles[m[3]] not in dom[pre]: continue
            blocks[head].remove(line); blocks[pre].insert(-1,line)
            moved.append(dict(value=m[1],source=head,destination=pre,handle=m[3],index=int(m[4])))
    # Immutable CBV contents may be shared only if the original read dominates.
    loads = []
    for b, lines in blocks.items():
        for i,line in enumerate(lines):
            m = LOAD.match(line)
            if m and m[3] in handles: loads.append((b,i,m))
    removed = {}; records=[]
    for b,i,m in loads:
        for other,j,n in loads:
            if n[1] in removed or n[1]==m[1] or (m[2],m[3],m[4])!=(n[2],n[3],n[4]): continue
            if (other==b and j<i) or (other!=b and other in dom[b]):
                removed[m[1]]=n[1]; records.append(dict(value=m[1],replacement=n[1],handle=m[3],index=int(m[4]))); break
    out=[]
    for b, lines in blocks.items():
        out.append(b+':')
        for line in lines:
            m=LOAD.match(line)
            if m and m[1] in removed: continue
            line=re.sub(r'%\w+',lambda m:removed.get(m[0],m[0]),line)
            out.append(line)
    if not moved and not removed: raise ValueError('No invariant CB work found')
    report=dict(hoisted=len(moved),eliminated=len(removed),moves=moved,reuses=records)
    return prefix+'define void @main() {\n'+'\n'.join(out)+'\n}'+suffix,report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True);p.add_argument('--input',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    source=subprocess.check_output([a.dxc,'-dumpbin',a.input],text=True)
    output,report=transform(source);target=Path(a.output);ir=target.with_suffix('.ll');ir.write_text(output)
    subprocess.run([a.assembler,str(ir),str(target),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
    target.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
    print('GI constant reads:',report['hoisted'],'hoisted;',report['eliminated'],'eliminated')
