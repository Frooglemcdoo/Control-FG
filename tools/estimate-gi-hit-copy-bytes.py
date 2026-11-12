#!/usr/bin/env python3
"""Measure the union of hit-source views in EXISTING native binding captures.
This is a lower bound: allocation-wide packing gaps/alignment and committed
storage are not recoverable from these rows. It is not GPU time or copy count.
"""
import argparse,hashlib,json,struct
from pathlib import Path

def measure(path):
    raw=path.read_bytes()
    if not raw or len(raw)%64:raise ValueError('Invalid native 64-byte binding rows')
    ranges=[];referenced=0
    for offset in range(0,len(raw),64):
        row=struct.unpack_from('<IIQQ10I',raw,offset)
        if row[1]>1 or row[5] not in (2,4):raise ValueError('Unsupported binding type/index stride')
        for address,length in ((row[2],row[12]),(row[3],row[13])):
            if not address or not length or address+length>2**64-1:raise ValueError('Invalid source view')
            ranges.append((address,address+length));referenced+=length
    merged=[]
    for start,end in sorted(ranges):
        if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
        else:merged.append([start,end])
    return dict(file=path.name,sha256=hashlib.sha256(raw).hexdigest(),bindings=len(raw)//64,
                referenced_sum_bytes=referenced,source_union_lower_bound_bytes=sum(b-a for a,b in merged),
                union_ranges=len(merged))

def main():
    p=argparse.ArgumentParser();p.add_argument('directory');p.add_argument('--output',required=True);a=p.parse_args()
    files=sorted(Path(a.directory).glob('*-bindings.bin'))
    if not files:raise ValueError('No existing binding captures')
    result={'schema':'gi-hit-copy-lower-bound-r428','samples':[measure(path) for path in files],
            'actual_copy_count_known':False,'actual_packed_bytes_known':False,'gpu_time_measured':False,
            'limits':'Original allocation identities are absent. Packing may include gaps and alignment. Union range count is not resource/copy command count. Old captures do not establish current scene cost.'}
    Path(a.output).write_text(json.dumps(result,indent=2)+'\n')
    print('Measured',len(files),'existing binding captures; lower bound only, no GPU/FPS claim.')
if __name__=='__main__':main()
