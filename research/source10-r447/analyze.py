"""Validate complete Source10 blocks and attribute interior-skin source spikes."""
import sys,json,struct
from pathlib import Path
import numpy as np
from scipy.ndimage import median_filter,binary_erosion
root=Path(sys.argv[1]);meta=json.loads(next((root/'logs').glob('GI-Owned*.json')).read_text());b=next((root/'logs').glob('GI-Owned*.bin'))
count,magic,stride,frame=struct.unpack('<4I',b.open('rb').read(16));assert magic==0x4f474931 and stride==256 and frame==meta['engine_frame'];assert b.stat().st_size==16+count*256
u=np.memmap(b,dtype='<u4',mode='r',offset=16,shape=(count//11,11,64));f=u.view('<f4');assert count==512*512*11 and not meta['overflow']
errors={k:0. for k in ['weighted','running','final','split']}
for start in range(0,len(u),4096):
 U=u[start:start+4096];F=f[start:start+4096];R=F[:,:10];S=F[:,10]
 assert np.all(U[:,:10,3]==1) and np.all(U[:,10,3]==2) and np.all(U[:,:10,2]==np.arange(10))
 assert np.all(U[:,:10,39]==10) and np.all(U[:,10,2]==10) and np.all(U[:,:,51]==frame)
 assert np.all(U[:,:,:2]==U[:,:1,:2]) and np.all(U[:,:,60:]==0)
 assert np.all(np.isin(U[:,:10,19],[1,2,4]))
 for k,actual,expected in [('weighted',R[:,:,12:15],R[:,:,4:7]*R[:,:,8:11]),('running',R[:,:,56:59],np.cumsum(R[:,:,12:15],axis=1)),('final',S[:,4:7],S[:,15,None]*R[:,-1,56:59]/10),('split',S[:,4:7],S[:,16:19]+S[:,20:23]+S[:,24:27])]:
  assert np.all(np.isfinite(actual)) and np.all(np.isfinite(expected));err=np.abs(actual-expected);errors[k]=max(errors[k],float(err.max()));assert np.all(err<=2e-5*np.maximum(1,np.maximum(abs(actual),abs(expected)))),k
p=root/'full-screen-rr'/f'rr-input-c1-{meta["pid"]}-{frame}'
def load(name):return np.fromfile(p/(name+'.rgba16f'),'<f2').reshape(1440,2560,4).astype(np.float32)
g=load('pre-filter-gi');at=load('gi-at-rr-input');tag=load('raw-albedo')[:,:,3];luma=np.array([.2126,.7152,.0722]);L=g[:,:,:3]@luma;median=median_filter(L,size=5);ratio=L/np.maximum(median,1e-10)
xy=u[:,10,:2].astype(int);x,y=xy.T;assert len(np.unique(y*2560+x))==512*512
# Positive surface tag, eroded away from hair/skin boundary, inside the captured head region.
mask=binary_erosion(tag==1,iterations=3)[y,x];skin=np.flatnonzero(mask)
summary=f[:,10];source_error=np.max(abs(summary[:,4:7]-g[y,x,:3]));lum=summary[:,4:7]@luma;spot=summary[:,24:27]@luma
spikes=skin[(ratio[y[skin],x[skin]]>10)&(lum[skin]>.001)]
selected=sorted(spikes,key=lambda i:lum[i],reverse=True)[:15]
report={'metadata':meta,'validated_pixels':len(xy),'validation_max_errors':errors,'source_to_half_gi_max_error':float(source_error),'raw_equals_rr_input':bool(np.array_equal(g[:,:,:3],at[:,:,:3])),'interior_skin_pixels':len(skin),'skin_spikes_ratio_above10_luminance_above001':len(spikes),'examples':[]}
for i in selected:
 R=f[i,:10];weighted=R[:,12:15]@luma;dominant=int(np.argmax(weighted));r=R[dominant];U=u[i,dominant]
 report['examples'].append({'pixel':[int(x[i]),int(y[i])],'luminance':float(lum[i]),'local_ratio':float(ratio[y[i],x[i]]),'spot_fraction':float(spot[i]/lum[i]),'point_fraction':float((summary[i,20:23]@luma)/lum[i]),'dominant_ray_fraction':float(weighted[dominant]/weighted.sum()),'dominant_ray':dominant,'secondary_material':int(U[35]),'secondary_world':r[32:35].tolist(),'secondary_normal':r[36:39].tolist(),'receiver_world':r[48:51].tolist(),'throughput':r[8:11].tolist(),'secondary_rgb':r[4:7].tolist(),'spot_rgb':r[24:27].tolist(),'ray_luminances':weighted.tolist()})
if len(spikes):
 R=f[spikes,:10];w=R[:,:,12:15]@luma
 report['spike_statistics']={'spot_fraction_p50_p90':np.percentile(spot[spikes]/lum[spikes],[50,90]).tolist(),'dominant_ray_fraction_p50_p90':np.percentile(w.max(1)/w.sum(1),[50,90]).tolist(),'spot_over_half_count':int(np.sum(spot[spikes]/lum[spikes]>.5))}
print(json.dumps(report,indent=2))
