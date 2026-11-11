"""Upper bound on same-frame grouping from R423; NOT lighting coherence proof.
Counts equal material IDs and replay-tag signs inside native 8x16 tiles.
IDs are never compared between frames. No world normal or visibility claim.
"""
import json
import sys
from pathlib import Path
import numpy as np

regions={'hair':(210,470,600,720),'neck':(300,815,470,900),'arm':(790,960,870,1010)}
frames=[]
for root in sorted(Path(sys.argv[1]).glob('rr-input*')):
    if not root.is_dir(): continue
    meta=json.loads((root/'metadata.json').read_text())
    def load(name,dtype):
        r=next(r for r in meta['resources'] if r['file'].split('.')[0]==name)
        return np.memmap(root/r['file'],dtype=dtype,mode='r').reshape(r['height'],r['width'],-1)
    g=load('native-gbuffer2','u1');tag=load('raw-albedo','<f2')[...,3]
    h,w=g.shape[:2];y,x=np.indices((h,w),dtype=np.uint64)
    material=(g[...,2].astype(np.uint64)<<8)|g[...,3].astype(np.uint64)
    key=(((y//16)*((w+7)//8)+x//8)*65536+material)*3+(np.sign(tag).astype(np.int64)+1).astype(np.uint64)
    _,inverse,counts=np.unique(key,return_inverse=True,return_counts=True)
    size=counts[inverse].reshape(h,w)
    report={}
    for name,(a,b,A,B) in regions.items():
        s=size[b:B,a:A]
        report[name]={'group_size_percentiles_10_50_90':np.percentile(s,[10,50,90]).tolist(),
                      'fraction_with_at_least_16_members':float(np.mean(s>=16)),
                      'fraction_with_at_least_64_members':float(np.mean(s>=64))}
    frames.append({'capture':root.name,'regions':report})
print(json.dumps({'scope':__doc__.strip(),'tile_extent':[8,16],
                  'warning':'ROI rectangles are not segmentation. Equal current IDs/tags do not establish equal lighting, geometric connectivity or visibility. Counts are an upper bound before normal/depth rejection.',
                  'frames':frames},indent=2))
