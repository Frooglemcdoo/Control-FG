"""Held-out proxy for proposal second moments, not a rendered-variance benchmark.
Native cosine density and independent draws are assumptions. One source frame
cannot provide future visibility, temporal variance, or counterfactual game output.
"""
import json,sys
from pathlib import Path
import numpy as np
from scipy.ndimage import binary_erosion
root=Path(sys.argv[1]);b=next((root/'logs').glob('*.bin'));u=np.memmap(b,'<u4','r',offset=16,shape=(262144,11,64));f=u.view('<f4');xy=u[:,10,:2].astype(int);x,y=xy.T
frame=next((root/'full-screen-rr').glob('*7831'));tag=np.fromfile(frame/'raw-albedo.rgba16f','<f2').reshape(1440,2560,4)[:,:,3];skin=binary_erosion(tag==1,iterations=3)[y,x]
luma=np.array([.2126,.7152,.0722]);origin=f[:,10,48:51].astype(float);norm=f[:,10,52:55].astype(float)
# Split whole receivers, not individual rays. Freeze bounds before evaluating held-out pixels.
train=((x*73856093)^(y*19349663))%2==0
idx=np.flatnonzero(train);r=f[idx,:10];weights=(r[:,:,12:15]@luma)*f[idx,10,15,None]
valid=(u[idx,:10,19]==2)&(weights>0);pos=r[:,:,32:35][valid].astype(float);weight=weights[valid]
# Deterministic world-space cells and local merging; no material-ID or scene-coordinate key.
cellsize=.25;cells=np.floor(pos/cellsize).astype(int);unique,inverse=np.unique(cells,axis=0,return_inverse=True);mass=np.bincount(inverse,weights=weight);bounds=[]
for _ in range(2):
 k=int(np.argmax(mass));center=(unique[k]+.5)*cellsize
 members=np.linalg.norm(pos-center,axis=1)<.4
 if not np.any(members):raise RuntimeError('Empty proposed cell')
 center=np.average(pos[members],weights=weight[members],axis=0)
 distance=np.linalg.norm(pos[members]-center,axis=1);order=np.argsort(distance);cdf=np.cumsum(weight[members][order]);radius=max(.05,float(distance[order[np.searchsorted(cdf,.95*cdf[-1])]]))
 bounds.append({'center':center.tolist(),'radius':radius})
 mass[np.linalg.norm((unique+.5)*cellsize-center,axis=1)<radius+.4]=0
idx=np.flatnonzero(~train&skin);r=f[idx,:10];o=origin[idx,None,:];n=norm[idx,None,:];delta=r[:,:,32:35]-o;direction=delta/np.maximum(np.linalg.norm(delta,axis=-1,keepdims=True),1e-15);base=np.maximum(0,np.sum(direction*n,axis=-1))/np.pi;C=(r[:,:,12:15]@luma)*f[idx,10,15,None]
# For misses, ogiWorld records the encoded ray endpoint. Reject invalid directions.
assert np.all(np.isfinite(direction)) and np.all(base>0)
old=[{'center':[-82.0759007,25.003847,110.55301],'radius':.160592319},{'center':[-82.0279381,25.345521,110.818513],'radius':.136421514}]
def evaluate(bs):
 guide=np.zeros_like(base)
 for b in bs:
  delta=np.array(b['center'])-origin[idx];dist2=np.sum(delta*delta,axis=1);axis=delta/np.maximum(np.sqrt(dist2[:,None]),1e-15);s2=np.minimum(1,b['radius']**2/np.maximum(dist2,1e-15));omc=np.where(dist2<=b['radius']**2,2,np.maximum(1e-6,s2/(1+np.sqrt(1-s2))))
  inside=np.sum(direction*axis[:,None,:],axis=-1)>=1-omc[:,None];guide+=inside/(2*np.pi*omc[:,None]*len(bs))
 q=.9*base+.1*guide;ratio=base/q
 return {'second_moment_proxy_ratio':float(np.sum(C*C*ratio)/np.sum(C*C)),'baseline_energy_inside_cones_fraction':float(C[guide>0].sum()/C.sum()),'ray_directions_inside_cones_fraction':float(np.mean(guide>0)),'max_weight':float(ratio.max()),'bounds':bs}
report={'training_receivers':int(train.sum()),'heldout_skin_receivers':len(idx),'assumptions':['Cosine PDF approximated from recorded receiver normal and endpoint direction','Independent-draw second moment only; ten-ray correlations not modeled','Single-frame spatial holdout is not temporal validation','Fresh tracing and exact producer/evaluator PDF pairing remain mandatory'],'old_fixed_bounds':evaluate(old),'automatic_world_bounds':evaluate(bounds),'automatic_far_bound_only':evaluate([bounds[1]]),'stale_shifted_world_bounds':evaluate([{'center':(np.array(v['center'])+[5,0,0]).tolist(),'radius':v['radius']} for v in bounds]),'stale_proposal_second_moment_upper_bound':1/.9}
print(json.dumps(report,indent=2))
