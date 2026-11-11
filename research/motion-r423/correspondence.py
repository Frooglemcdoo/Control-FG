"""Motion-following guide feasibility from captured guides; not surface identity proof.
Normal comparisons remain in captured view space: camera rotation is NOT supplied.
"""
import json,sys
from pathlib import Path
import numpy as np
from scipy.ndimage import map_coordinates
root=Path(sys.argv[1]);frames=sorted(p for p in root.glob('rr-input*') if p.is_dir());meta=[json.loads((f/'metadata.json').read_text()) for f in frames]
def load(i,n):
 r=next(r for r in meta[i]['resources'] if r['file'].split('.')[0]==n)
 d='u1' if r['file'].endswith('rgba8') else '<f4' if r['file'].endswith('r32f') else '<f2'
 return np.fromfile(frames[i]/r['file'],d).reshape(r['height'],r['width'],-1).astype('float32')
a=[{n:load(i,n) for n in ['diffuse','raw-albedo','normal-roughness','input-depth','input-motion','native-gbuffer1','native-gbuffer2']} for i in range(2)]
def decode(g):
 b=g.astype('uint32');q=(np.stack((b[...,0]*8+((b[...,3]>>1)&7),b[...,1]*16+(b[...,3]>>4)),axis=-1)/np.array([2047,4095])*2-1)*1.3
 d=np.sum(q*q,axis=-1);return np.concatenate((2*q,(d-1)[...,None]),axis=-1)/(d+1)[...,None]
for d in a:d['native-normal']=decode(d['native-gbuffer1'])
regions={'hair':(210,470,600,720),'neck':(300,815,470,900),'arm':(790,960,870,1010)}
results={}
for label,(x,y,X,Y) in regions.items():
 yy,xx=np.mgrid[y:Y,x:X].astype(float);cur={n:v[y:Y,x:X] for n,v in a[1].items()}
 motion=cur['input-motion']*np.array(meta[1]['motion_scale']);jd=np.array(meta[1]['jitter'])-np.array(meta[0]['jitter'])
 candidates=[]
 for sign in [0,1,-1]:
  for j in [np.zeros(2),jd,-jd,jd*[1,-1],jd*[-1,1]]:
   coords=[yy+sign*motion[...,1]+j[1],xx+sign*motion[...,0]+j[0]]
   warped=np.stack([map_coordinates(a[0]['diffuse'][...,i],coords,order=1,mode='constant') for i in range(3)],axis=-1)
   candidates.append((float(np.mean(abs(warped-cur['diffuse'][...,:3]))),sign,j.tolist()))
 best=min(candidates);coords=[yy+best[1]*motion[...,1]+best[2][1],xx+best[1]*motion[...,0]+best[2][0]]
 ix=np.rint(coords[1]).astype(int);iy=np.rint(coords[0]).astype(int)
 inbounds=(ix>=0)&(iy>=0)&(ix<2560)&(iy<1440);ix=np.clip(ix,0,2559);iy=np.clip(iy,0,1439)
 prev={n:v[iy,ix] for n,v in a[0].items()}
 same_material=np.all(prev['native-gbuffer2'][...,2:]==cur['native-gbuffer2'][...,2:],axis=-1)
 same_tag=np.sign(prev['raw-albedo'][...,3])==np.sign(cur['raw-albedo'][...,3])
 prevn=prev['native-normal'];curn=cur['native-normal'];dot=np.sum(prevn*curn,axis=-1)
 rrdot=np.sum(prev['normal-roughness'][...,:3]*cur['normal-roughness'][...,:3],axis=-1)
 match=inbounds&same_material&same_tag
 pairs=np.stack(((prev['native-gbuffer2'][...,2].astype('uint32')<<8)|prev['native-gbuffer2'][...,3].astype('uint32'),(cur['native-gbuffer2'][...,2].astype('uint32')<<8)|cur['native-gbuffer2'][...,3].astype('uint32')),axis=-1)
 up,ct=np.unique(pairs.reshape(-1,2),axis=0,return_counts=True);top=np.argsort(ct)[-5:][::-1]
 transitions=[{'previous_id':int(up[i,0]),'current_id':int(up[i,1]),'pixels':int(ct[i])} for i in top]
 results[label]={'dominant_reprojected_id_pairs':transitions,'best_albedo_fit':{'mae':best[0],'mv_sign':best[1],'jitter_adjustment':best[2]},'unwarped_albedo_mae':float(np.mean(abs(a[0]['diffuse'][y:Y,x:X,:3]-cur['diffuse'][...,:3]))),'same_material_fraction':float(np.mean(same_material)),'same_tag_fraction':float(np.mean(same_tag)),'same_material_and_tag_fraction':float(np.mean(match)),'native_normal_dot_percentiles_matched':(np.percentile(dot[match],[1,10,50,90,99]).tolist() if np.any(match) else None),'rr_normal_dot_percentiles_matched':(np.percentile(rrdot[match],[1,10,50,90,99]).tolist() if np.any(match) else None),'matched_native_dot_gt_095_fraction':float(np.mean(match&(dot>.95))),'native_vs_rr_normal_dot_percentiles':np.percentile(np.sum(curn*cur['normal-roughness'][...,:3],axis=-1),[1,10,50,90,99]).tolist()}
print(json.dumps({'scope':'best fit over sign/jitter candidates; same material/tag is not object identity; view-normal dot is not world-normal comparison; depth is clip depth without captured previous view transform; no claim of proven visibility or guidance validity','regions':results},indent=2))
