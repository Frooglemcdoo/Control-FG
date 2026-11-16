"""Synthetic fresh-visibility stress test. Not Control output or GPU timings.
Compare ten independent cosine draws, defensive cone mixture, and one independent
cosine pilot + nine cone-mixture draws with a nonnegative control-variate resolve.
"""
import json
from pathlib import Path
import numpy as np
from scipy.stats import qmc
PI=np.pi

def unit(v):return v/np.linalg.norm(v,axis=-1,keepdims=True)
def local(axis,z,angle):
 axis=np.asarray(axis);sign=np.where(axis[...,2]>=0,1.,-1.);a=-1/(sign+axis[...,2]);b=axis[...,0]*axis[...,1]*a
 t=np.stack([1+sign*axis[...,0]**2*a,sign*b,-sign*axis[...,0]],-1)
 bt=np.stack([b,sign+axis[...,1]**2*a,-axis[...,1]],-1)
 radius=np.sqrt(np.maximum(0,1-z*z));return z[...,None]*axis+radius[...,None]*(np.cos(angle)[...,None]*t+np.sin(angle)[...,None]*bt)
def cone(origin,center,radius):
 delta=np.array(center)-np.array(origin);d2=delta@delta;axis=unit(delta) if d2>1e-20 else np.array([0.,0.,1.]);s2=min(1,radius*radius/max(d2,1e-20));omc=2 if d2<=radius*radius else max(1e-6,s2/(1+np.sqrt(1-s2)));return axis,omc
def sample(origin,normal,bound,u,v,alpha):
 axis,omc=cone(origin,*bound);g=u<alpha
 if alpha:
  direction=np.where(g[...,None],local(axis,1-v*omc,2*PI*u/alpha),local(normal,np.sqrt(v),2*PI*(u-alpha)/(1-alpha)))
 else:direction=local(normal,np.sqrt(v),2*PI*u)
 base=np.maximum(0,direction@normal)/PI;guide=((direction@axis)>=1-omc)/(2*PI*omc);pdf=(1-alpha)*base+alpha*guide
 w=np.divide(base,pdf,out=np.zeros_like(base),where=pdf>0);return direction,w

def lighting(direction,origin,scene,range_u):
 # Fresh analytic sphere intersection, with an independent random ray range.
 center=np.asarray(scene['light']);radius=.25;oc=np.asarray(origin)-center;b=direction@oc;c=oc@oc-radius*radius;disc=b*b-c
 t=-b-np.sqrt(np.maximum(0,disc));hit=(disc>=0)&(t>0)&(t<=2.5+1.5*range_u)
 if scene.get('blocker'):
  # Moving opaque plane in front of the secondary emitter, half-plane edge.
  denom=direction[...,2];tb=np.divide(1-origin[2],denom,out=np.full_like(denom,np.inf),where=denom>0)
  bx=origin[0]+tb*direction[...,0];hit &= ~((tb>0)&(tb<t)&(bx>scene['edge']))
 if scene.get('off'):hit[:]=False
 base=np.array(scene.get('base',[.05,.06,.08]));bright=np.array([100.,40.,12.])
 return base+hit[...,None]*bright

base_bound=([.65,0,3],.30)
scenes=[
 {'name':'stationary_bright_patch','light':[.65,0,3]},
 {'name':'light_moves_guide_stale','light':[-.65,0,3]},
 {'name':'light_switches_off','light':[.65,0,3],'off':True},
 {'name':'blocker_enters','light':[.65,0,3],'blocker':True,'edge':.19},
 {'name':'blocker_moves_across','light':[.65,0,3],'blocker':True,'edge':.23},
 {'name':'blocker_leaves','light':[.65,0,3]},
 {'name':'receiver_turns','light':[.65,0,3],'normal':[.85,0,.5267826876]},
 {'name':'receiver_turns_away','light':[.65,0,3],'normal':[0,0,-1]},
 {'name':'receiver_translates','light':[.65,0,3],'origin':[.7,.2,0]},
 {'name':'receiver_inside_old_bound','light':[.65,0,4],'origin':[.65,0,3]},
 {'name':'black_after_light_off','light':[.65,0,3],'off':True,'base':[0,0,0]},
 {'name':'camera_cut_invalid_guide','light':[.65,0,3],'alpha':0},
]
def main():
 N=180000;luma=np.array([.2126,.7152,.0722]);report=[]
 for case,scene in enumerate(scenes):
  origin=np.array(scene.get('origin',[0,0,0]));normal=unit(np.array(scene.get('normal',[0,0,1.])));alpha=scene.get('alpha',.1)
  # Independent scrambled Sobol integrations establish a numerical reference.
  refs=[]
  for seed in [91,927]:
   rv=qmc.Sobol(3,scramble=True,seed=seed).random_base2(20);rd,_=sample(origin,normal,base_bound,rv[:,0],rv[:,1],0);refs.append(lighting(rd,origin,scene,rv[:,2]).mean(0))
  reference=np.mean(refs,axis=0);reference_uncertainty=np.max(np.abs(np.array(refs)-reference),axis=0)
  rng=np.random.default_rng(448+case);rows={};shared_uv=rng.random((N,10,3));native_result=None
  for mode in ['native','plain_guided','pilot_control_variate']:
   uv=shared_uv;fractions=np.full(10,alpha if mode!='native' else 0.)
   if mode=='pilot_control_variate':fractions[0]=0
   Cs=[];Ws=[]
   for ray in range(10):
    d,w=sample(origin,normal,base_bound,uv[:,ray,0],uv[:,ray,1],fractions[ray]);Cs.append(lighting(d,origin,scene,uv[:,ray,2]));Ws.append(w)
   C=np.stack(Cs,axis=1);W=np.stack(Ws,axis=1)
   if mode=='pilot_control_variate':
    pilot_coefficient=1-W[:,1:].sum(1)/10
    assert pilot_coefficient.min()>-1e-12
    result=pilot_coefficient[:,None]*C[:,0]+(C[:,1:]*W[:,1:,None]).sum(1)/10
   else:result=(C*W[:,:,None]).mean(1)
   if mode=='native':native_result=result.copy()
   if alpha==0:assert np.allclose(result,native_result,rtol=0,atol=1e-12)
   mean=result.mean(0);se=result.std(axis=0)/np.sqrt(N);error=abs(mean-reference)
   assert np.all(np.isfinite(result)) and result.min()>=-1e-10
   assert np.all(error<=7*se+4*reference_uncertainty+1e-8),(scene['name'],mode,mean,reference,se)
   var=float(np.var(result@luma));rows[mode]={'rgb_mean':mean.tolist(),'luminance_variance':var,'rgb_standard_error':se.tolist(),'minimum_rgb':float(result.min())}
   if scene.get('off') and mode=='pilot_control_variate':assert np.max(np.abs(result-np.array(scene.get('base',[.05,.06,.08]))))<1e-12
  for mode in rows:
   rows[mode]['variance_ratio_vs_native']=rows[mode]['luminance_variance']/rows['native']['luminance_variance'] if rows['native']['luminance_variance']>1e-20 else None
  report.append({'case':scene['name'],'reference_rgb':reference.tolist(),'reference_half_spread':reference_uncertainty.tolist(),'methods':rows})
  print(scene['name'],{k:round(v['luminance_variance'],7) for k,v in rows.items()},flush=True)
 out={'scope':'Synthetic CPU math; fresh analytic visibility, independent draws, 10 rays. No game, GPU, or FPS claim.','groups_per_case':N,'cases':report}
 Path(__file__).with_name('results.json').write_text(json.dumps(out,indent=2)+'\n')

if __name__=="__main__":main()
