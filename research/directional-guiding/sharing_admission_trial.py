"""Adversarial admission tests, not captured lighting or a game benchmark.
Geometry/material are deliberately identical while illumination varies within
a group. Report minority receivers separately; a group mean can hide regressions.
"""
import argparse
import json
import numpy as np


def run(case, repetitions=8192, seed=427):
    n,c=128,64
    signal=np.full((n,c),.05)
    labels=np.zeros(n,dtype=int)
    if case=='coherent': signal[:,7]=100
    elif case=='balanced':
        labels[64:]=1;signal[:64,7]=100;signal[64:,36]=100
    elif case=='minority_1':
        labels[-1]=1;signal[:-1,7]=100;signal[-1,36]=100
    elif case=='minority_8':
        labels[-8:]=1;signal[:-8,7]=100;signal[-8:,36]=100
    elif case=='mixed_16':
        labels=np.arange(n)%16;signal[np.arange(n),(7+labels*29)%64]=100
    elif case=='constant': pass
    elif case=='flat_minority':
        labels[-8:]=1;signal[:-8,7]=100
    else: raise ValueError(case)
    rng=np.random.default_rng(seed)
    means=signal.mean(1);base=signal.var(1)
    modes=('ungated','group_evidence','receiver_evidence')
    variance={m:np.zeros(n) for m in modes};admitted={m:np.zeros(n) for m in modes}
    mean_error=0
    for start in range(0,repetitions,256):
        b=min(256,repetitions-start);rows=np.repeat(np.arange(b),n)
        pilot=rng.integers(c,size=(b,n))
        value=np.take_along_axis(signal[None,:,:],pilot[:,:,None],2)[:,:,0]
        energy=np.zeros((b,c));votes=np.zeros((b,c))
        np.add.at(energy,(rows,pilot.ravel()),value.ravel()*c/n)
        # At least three strong observations in the dominant cell, accounting
        # for >=50% of observed energy. This changes sampling only, never RGB.
        strong=value>=.25*value.max(1,keepdims=True)
        np.add.at(votes,(rows,pilot.ravel()),strong.ravel())
        peak=energy.argmax(1);ix=np.arange(b)
        group=(energy[ix,peak]>=.5*energy.sum(1))&(votes[ix,peak]>=3)
        guide=energy/energy.sum(1,keepdims=True)
        for mode in modes:
            accept=np.ones((b,n),dtype=bool)
            if mode!='ungated': accept &= group[:,None]
            if mode=='receiver_evidence': accept &= strong & (pilot==peak[:,None])
            fraction=accept[:,:,None]*.125
            q=(1-fraction)/c+fraction*guide[:,None,:]
            conditional_mean=np.sum(q*(signal[None,:,:]/(c*q)),2)
            mean_error=max(mean_error,float(np.max(abs(conditional_mean-means))))
            second=np.sum(signal[None,:,:]**2/(c*c*q),2)-means**2
            variance[mode]+=second.sum(0)
            admitted[mode]+=accept.sum(0)
    assert mean_error<1e-10
    result={}
    for mode in modes:
        pair=(base+variance[mode]/repetitions)/4
        cohorts=[]
        for label in np.unique(labels):
            mask=labels==label;native=float(base[mask].mean()/2);new=float(pair[mask].mean())
            cohorts.append(dict(label=int(label),receivers=int(mask.sum()),native_pair_variance=native,
                                candidate_pair_variance=new,variance_ratio=new/native if native>1e-20 else None,
                                admission_fraction=float(admitted[mode][mask].mean()/repetitions)))
        result[mode]={'group_mean_pair_variance':float(pair.mean()),'cohorts':cohorts}
    return dict(case=case,conditional_mean_error=mean_error,modes=result)


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',required=True);a=p.parse_args()
    cases=[run(c) for c in ('coherent','balanced','minority_1','minority_8','mixed_16','constant','flat_minority')]
    from pathlib import Path
    Path(a.output).write_text(json.dumps({'scope':__doc__.strip(),'seed':427,'repetitions':8192,
        'native_comparator':'two independent uniform discrete samples, not Control stratification',
        'guide_fraction':.125,'cases':cases},indent=2)+'\n')
