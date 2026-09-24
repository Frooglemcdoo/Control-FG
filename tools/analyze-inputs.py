"""Analyze SDR/HDR capture samples. Python + numpy; no game changes."""
import json,sys
from pathlib import Path
import numpy as np

def decode(meta,raw):
    f=meta['format'];bpp={10:8,24:4,28:4,29:4,87:4,91:4,61:1}[f]
    if len(raw)!=589824 or meta['row_pitch']!=1024:raise ValueError('Invalid layout')
    rows=np.frombuffer(raw,dtype=np.uint8).reshape(9,64,1024)[...,:128*bpp].copy()
    if f==10:return rows.view('<f2').reshape(9,64,128,4)[...,:3].astype(np.float32)
    if f==61:return rows.reshape(9,64,128).astype(np.float32)/255
    if f==24:
        v=rows.view('<u4').reshape(9,64,128)
        return np.stack([(v>>shift)&1023 for shift in [0,10,20]],axis=-1).astype(np.float32)/1023
    a=rows.reshape(9,64,128,4)[...,:3].astype(np.float32)/255
    if f in [87,91]:a=a[...,::-1]
    if f in [29,91]:a=np.where(a<=.04045,a/12.92,((a+.055)/1.055)**2.4)
    return a

def mask_expected(color,hudless):
    finite=np.isfinite(color).all(-1)&np.isfinite(hudless).all(-1)
    delta=np.abs(color-hudless).max(-1);t=np.clip((delta-.0025)/(.035-.0025),0,1)
    return np.where(finite,t*t*(3-2*t),0)

def pq_expected(rgb):
    mat=np.array([[.627404,.329282,.0433136],[.069097,.919540,.0113612],[.0163916,.0880132,.895595]])
    lum=np.clip(np.maximum(rgb@mat.T,0)*.008,0,1)**(2610/16384)
    return ((3424/4096+(2413/128)*lum)/(1+(2392/128)*lum))**(2523/32)

def analyze(root):
    groups={}
    for p in Path(root).rglob('b*-f*-*.json'):
        m=json.loads(p.read_text());a=decode(m,p.with_suffix('.raw').read_bytes())
        key=(str(p.parent),m['burst'],m['epoch'],m['frame'],m.get('hdr',1))
        groups.setdefault(key,{})[m['role']]=a
    results=[]
    for k,roles in sorted(groups.items()):
        expected={'producer','hudless','ui_alpha'}|({'bridge','tagged_pq','final_pq'} if k[4] else {'bridge'})
        row=dict(folder=k[0],burst=k[1],epoch=k[2],frame=k[3],hdr=k[4],missing_roles=sorted(expected-set(roles)))
        if {'producer','hudless','ui_alpha'}<=roles.keys():
            pred=mask_expected(roles['producer'],roles['hudless']);actual=roles['ui_alpha'];error=np.abs(pred-actual)
            row.update(mask_mae=float(error.mean()),mask_pixels_over_2_codes=int((error>2/255).sum()),mask_nonzero=int((actual>0).sum()))
        if {'bridge','producer'}<=roles.keys():row['bridge_same_frame_rgb_exact']=bool(np.array_equal(roles['bridge'],roles['producer']))
        for name,source in [('tagged_pq','hudless'),('final_pq','bridge')]:
            if name in roles and source in roles:
                e=np.abs(pq_expected(roles[source])-roles[name]);row[name+'_mae_codes']=float(e.mean()*1023);row[name+'_max_error_codes']=float(e.max()*1023)
        matches={}
        for role,color in roles.items():
            if role!='bridge' and not role.startswith('shadow'):continue
            matches[role]=[]
            for delta in [-1,0,1]:
                other=groups.get((k[0],k[1],k[2],k[3]+delta,k[4]),{})
                if 'producer' in other and np.array_equal(color,other['producer']):
                    matches[role].append(delta)
        row['producer_rgb_match_offsets']=matches
        # Multiple matches in a stationary scene are ambiguous; never choose an age.
        row['bridge_unique_previous_frame_match']=matches.get('bridge')==[-1]
        row['current_frame_shadow_candidates']=[role for role,offsets in matches.items() if role.startswith('shadow') and 0 in offsets]
        results.append(row)
    return {'frames':results,'limits':'Nine tiles only. Captures reflect producer recording points and SDR/HDR presentation work, not Streamline internal consumption. Readbacks can change race timing. Offsets compare RGB samples only, within one burst/epoch/domain; stationary scenes can match several frames. CPU/GPU PQ precision differences are expected; use healthy/broken comparison.'}
if __name__=='__main__':print(json.dumps(analyze(sys.argv[1]),indent=2))
