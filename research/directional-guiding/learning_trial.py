"""Discrete toy learning trial, NOT captured directional radiance or game replay.
A coherent tile shares an integrand deliberately; real geometry need not do so.
Compare proposal learning at 2 or 256 training rays per update. No image filtering.
"""
import json
import numpy as np
rng=np.random.default_rng(424)
C=64

def moments(signal,p):
 value=signal/(C*p)
 return np.sum(p*value),np.sum(p*value*value)-np.mean(signal)**2

def run(training_samples,period):
 repetitions=512;frames=256;learned=np.ones((repetitions,C));var=[];base=[];meanerr=[]
 for frame in range(frames):
  hot=7 if period==0 else (7+(frame//period)*29)%C
  signal=np.full(C,.05);signal[hot]=100
  p=.5/C+.5*learned/learned.sum(axis=1,keepdims=True)
  mean,v=moments(signal,p[0]);meanerr.append(abs(mean-signal.mean()))
  if frame>=32:
   var.append(np.mean(np.sum(signal[None,:]**2/(C*C*p),axis=1)-signal.mean()**2)/2)
   base.append(np.var(signal)/2)
  # Training samples use the frozen proposal. Only next update may use them.
  u=rng.random((repetitions,training_samples));cdf=np.cumsum(p,axis=1);cdf[:,-1]=1
  cell=np.sum(u[:,:,None]>=cdf[:,None,:],axis=-1)
  rows=np.repeat(np.arange(repetitions),training_samples);cols=cell.ravel()
  estimate=np.zeros_like(learned)
  np.add.at(estimate,(rows,cols),signal[cols]/p[rows,cols]/training_samples)
  learned=.8*learned+.2*estimate
 return {'training_rays_per_update':training_samples,'hotspot_moves_every_frames':period,'guided_two_ray_variance':float(np.mean(var)),'uniform_independent_two_ray_variance':float(np.mean(base)),'ratio':float(np.mean(var)/np.mean(base)),'max_analytic_mean_error':max(meanerr)}
results=[run(n,period) for n in (2,256) for period in (0,1,8,32)]
print(json.dumps({'scope':'synthetic coherent-tile integrand, no actual light directions, PDF mismatch or GPU cost measured','trials':results},indent=2))
