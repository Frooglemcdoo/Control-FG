"""Skip the R434 one-ray guide arithmetic when its result cannot be selected.
All existing arithmetic and TraceRay operands are preserved on their used paths.
"""
import re
from gi_single_producer import edit_export, replace_once
from gi_guided_live import transform as reference


def transform(source):
    guided = reference(source)
    def patch(body):
        lines = body.splitlines()
        starts = [i for i, line in enumerate(lines) if re.match(r'  %G434_1 = ', line)]
        if len(starts) != 1: raise ValueError('Guide entry changed')
        start = starts[0]
        end = start
        while end < len(lines) and re.match(r'  %G434_\d+ = ', lines[end]): end += 1
        trace = lines[end]
        if not trace.startswith('  call void @dx.op.traceRay.struct.HitData('): raise ValueError('Guide exit changed')
        labels = [line.split(':', 1)[0] for line in lines[:start] if re.match(r'^"[^"\n]+":', line)]
        if not labels: raise ValueError('Missing guide predecessor')
        predecessor = '%'+labels[-1]
        selected = re.findall(r'float (%G434_\d+)', trace)
        mask = re.search(r'%247, i32 0, i32 (%G434_\d+),', trace)
        if len(selected) != 3 or not mask: raise ValueError('Trace operands changed')
        # Splitting the loop latch requires updating its original outgoing phi edges.
        changes = 0
        for i,line in enumerate(lines):
            if ' = phi ' in line and predecessor in line:
                changes += line.count(predecessor)
                lines[i] = line.replace(predecessor, '%G441Trace')
        if changes != 5: raise ValueError('Unexpected loop/exit phi edges')
        entry = ['  %G441OneRay = icmp eq i32 %226, 1',
                 '  br i1 %G441OneRay, label %G441Guide, label %G441Trace',
                 'G441Guide:']
        merge = ['  br label %G441Trace', 'G441Trace:']
        for i,value in enumerate(selected):
            merge.append(f'  %G441Dir{i} = phi float [ {value}, %G441Guide ], [ %GISingleDir{i}, {predecessor} ]')
            trace = trace.replace('float '+value, 'float %G441Dir'+str(i))
        merge.append(f'  %G441Mask = phi i32 [ {mask[1]}, %G441Guide ], [ 1, {predecessor} ]')
        trace = trace.replace('i32 '+mask[1]+',', 'i32 %G441Mask,')
        return '\n'.join(lines[:start]+entry+lines[start:end]+merge+[trace]+lines[end+1:])
    return edit_export(guided, 'rayGenMain', patch)
