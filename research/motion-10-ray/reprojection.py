"""Evaluate captured motion conventions on interior skin, excluding hair edges.
This is two-frame image agreement, not a proof of visibility or bone history.
"""
import json, sys
from pathlib import Path
import numpy as np
from scipy.ndimage import map_coordinates, binary_erosion

frames=sorted(Path(sys.argv[1]).glob('rr-input*'))
meta=[json.loads((p/'metadata.json').read_text()) for p in frames]
def load(i,name):
    r=next(r for r in meta[i]['resources'] if r['file'].split('.')[0]==name)
    return np.fromfile(frames[i]/r['file'],'<f2').reshape(r['height'],r['width'],-1).astype(float)
albedo=[load(i,'diffuse') for i in range(2)]
normal=[load(i,'normal-roughness') for i in range(2)]
tag=[load(i,'raw-albedo')[...,3] for i in range(2)]
mv=load(1,'input-motion')*meta[1]['motion_scale']
jdelta=np.array(meta[0]['jitter'])-meta[1]['jitter']
report={'frames':[m['frame'] for m in meta],'previous_minus_current_jitter':jdelta.tolist(),'regions':{}}
for name,(x,y,X,Y) in {'neck':(440,710,560,800),'hand':(945,890,1010,970)}.items():
    yy,xx=np.mgrid[y:Y,x:X].astype(float)
    v=mv[y:Y,x:X];a=albedo[1][y:Y,x:X,:3];n=normal[1][y:Y,x:X,:3]
    mask=binary_erosion(tag[1][y:Y,x:X]==1,iterations=3)
    rows=[]
    for label,sign,j in [('none',0,np.zeros(2)),('motion',1,np.zeros(2)),
        ('motion_plus_jitter_delta',1,jdelta),('motion_minus_jitter_delta',1,-jdelta),
        ('negative_motion_plus_jitter_delta',-1,jdelta)]:
        coords=[yy+sign*v[...,1]+j[1],xx+sign*v[...,0]+j[0]]
        def warp(source):
            return np.stack([map_coordinates(source[...,i],coords,order=1) for i in range(3)],-1)
        wa=warp(albedo[0]);wn=warp(normal[0])
        valid=mask&(map_coordinates(tag[0],coords,order=0)==1)
        diff=np.mean(abs(wa-a),axis=-1)
        dot=np.sum(wn*n,axis=-1)/np.maximum(np.linalg.norm(wn,axis=-1)*np.linalg.norm(n,axis=-1),1e-9)
        rows.append({'convention':label,'pixels':int(valid.sum()),'albedo_mae':float(np.mean(diff[valid])),
                     'albedo_error_p50_p95':np.percentile(diff[valid],[50,95]).tolist(),
                     'normal_dot_p05_p50':np.percentile(dot[valid],[5,50]).tolist()})
    report['regions'][name]=rows
print(json.dumps(report,indent=2))
