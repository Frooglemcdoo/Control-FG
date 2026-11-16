"""One-frame learned cone targets from fresh samples; synthetic CPU sequence.
No motion/lighting oracle is used for proposal validity or target positions.
"""
from benchmark import sample,lighting,unit,PI
import numpy as np,json
from scipy.stats import qmc
from pathlib import Path

N=2048;FRAMES=120;luma=np.array([.2126,.7152,.0722]);rng=np.random.default_rng(449)
rv=qmc.Sobol(3,scramble=True,seed=449).random_base2(19)
old_bound=([0,0,3],.3);valid=False;records=[];errors={'native':[],'learned_pilot':[]};bias={'native':[],'learned_pilot':[]}
for frame in range(FRAMES):
 # Move the receiver continuously; move the bright surface and add/remove occlusion.
 origin=np.array([.25*np.sin(frame*.07),.1*np.cos(frame*.09),0])
 angle=.45*np.sin(frame*.06);normal=np.array([np.sin(angle),0,np.cos(angle)])
 light=[.65+.12*np.sin(frame*.09),0,3.]
 if 40<=frame<70:light=[-.65+.12*np.sin(frame*.09),0,3.]
 scene={'light':light}
 if 20<=frame<35 or 80<=frame<95:scene.update(blocker=True,edge=.2+.08*np.sin(frame*.4))
 if 55<=frame<62 or 100<=frame:scene['off']=True
 alpha=.1 if valid else 0
 # Reference uses fresh scene queries; it is never supplied to proposal learning.
 rd,_=sample(origin,normal,old_bound,rv[:,0],rv[:,1],0);reference=lighting(rd,origin,scene,rv[:,2]).mean(0)@luma
 uv=rng.random((N,10,3));outputs={};training=[]
 for mode in ['native','learned_pilot']:
  Cs=[];Ws=[]
  for ray in range(10):
   a=alpha if mode=='learned_pilot' and ray else 0
   direction,w=sample(origin,normal,old_bound,uv[:,ray,0],uv[:,ray,1],a)
   color=lighting(direction,origin,scene,uv[:,ray,2]);Cs.append(color);Ws.append(w)
   if mode=='learned_pilot':
    oc=origin-np.array(light);b=direction@oc;c=oc@oc-.25**2;disc=b*b-c;t=-b-np.sqrt(np.maximum(0,disc));pos=origin+t[:,None]*direction
    # Only actual bright samples train; energy is density corrected. No oracle positions.
    energy=(color@luma)*w;bright=(color@luma)>1
    training.extend(zip(pos[bright],energy[bright]))
  C=np.stack(Cs,1);W=np.stack(Ws,1)
  if mode=='native':result=C.mean(1)
  else:
   coefficient=1-W[:,1:].sum(1)/10;assert coefficient.min()>-1e-12
   result=coefficient[:,None]*C[:,0]+(C[:,1:]*W[:,1:,None]).sum(1)/10
   assert result.min()>-1e-12 and np.all(np.isfinite(result))
  value=result@luma;outputs[mode]=value;errors[mode].append(float(np.mean((value-reference)**2)));bias[mode].extend((value-reference).tolist())
 if scene.get('off'):assert np.max(np.abs(outputs['learned_pilot']-outputs['native']))<1e-12
 # Freeze a new list for the NEXT frame. Empty/insufficient training expires immediately.
 valid=len(training)>=8
 if valid:
  pos,energy=map(np.array,zip(*training));center=np.average(pos,weights=energy,axis=0);dist=np.linalg.norm(pos-center,axis=1);order=np.argsort(dist);cdf=np.cumsum(energy[order]);radius=max(.3,float(dist[order[np.searchsorted(cdf,.99*cdf[-1])]])+.05)
  old_bound=(center.tolist(),radius)
 records.append({'frame':frame,'guide_used':bool(alpha),'next_valid':valid,'training_hits':len(training),'light_off':bool(scene.get('off')),'mse_native':errors['native'][-1],'mse_learned':errors['learned_pilot'][-1]})
 if frame%20==0:print('frame',frame,'training',len(training),'guide',bool(alpha),flush=True)
report={'scope':'Synthetic moving sequence; fresh visibility and prior-frame target learning. No game or GPU claim.','frames':FRAMES,'receivers_per_frame':N,'ray_count':10,'overall_mse_ratio':sum(errors['learned_pilot'])/sum(errors['native']),'mean_error':{k:float(np.mean(v)) for k,v in bias.items()},'frames_detail':records}
for name,ids in [('steady',range(1,20)),('moving_blocker',range(20,35)),('teleport',[40,70]),('light_off',list(range(55,62))+list(range(100,120)))]:
 a=sum(errors['native'][i] for i in ids);b=sum(errors['learned_pilot'][i] for i in ids);report[name]={'mse_native':a,'mse_learned':b,'ratio':b/a if a>1e-15 else None}
Path(__file__).with_name('learning-results.json').write_text(json.dumps(report,indent=2)+'\n');print({k:v for k,v in report.items() if k!='frames_detail'})
