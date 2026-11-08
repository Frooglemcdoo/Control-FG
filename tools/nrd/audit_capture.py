"""Audit existing GI path exports for NRD replay without guessing missing guides."""
import argparse,json,pathlib
import numpy as np

def audit(folder):
    folder=pathlib.Path(folder);m=json.loads((folder/'metadata.json').read_text(encoding='utf-8-sig'))
    resources={r['file']:r for r in m['resources']}
    result={'frame':m['frame'],'pre_gi_frame':m.get('pre_gi_frame'),
      'export_stride':m.get('export_stride',1),'files':{},'missing_contracts':[]}
    for name,channels,dtype in [('pre-filter-gi.rgba16f',4,'<f2'),('normal-roughness.rgba16f',4,'<f2'),('input-depth.r32f',1,'<f4'),('input-motion.rg16f',2,'<f2'),('diffuse.rgba16f',4,'<f2')]:
        r=resources.get(name)
        if not r or not (folder/name).is_file():
            result['missing_contracts'].append(name);continue
        a=np.fromfile(folder/name,dtype=dtype)
        expected=r['width']*r['height']*channels
        if a.size!=expected:raise ValueError(f'{name}: {a.size} values != {expected}')
        a=a.astype(np.float64).reshape(-1,channels)
        finite=np.isfinite(a);valid=a[finite]
        v={'shape':[r['height'],r['width'],channels], 'nonfinite':int((~finite).sum()),
           'minimum':float(valid.min()) if valid.size else None,'maximum':float(valid.max()) if valid.size else None}
        if name.startswith('normal'):
            v['normal_length_error_p99']=float(np.quantile(np.abs(np.linalg.norm(a[:,:3],axis=1)-1),.99))
        if name.startswith('pre-filter'):
            luma=a[:,:3]@np.array([.2126,.7152,.0722]);v['luminance_quantiles']=np.quantile(luma,[.5,.9,.99,.999,1]).tolist()
        result['files'][name]=v
    if m.get('export_stride',1)!=1:
        result['missing_contracts'].append('full_resolution_neighborhoods: point-sampled export cannot reproduce spatial filtering')
    if not m.get('hit_distance_bound'):
        result['missing_contracts'].append('diffuse_first_bounce_hit_distance')
    if 'world_to_view' not in m or 'view_to_world' not in m:
        result['missing_contracts'].append('current_and_previous_unjittered_world_camera_transforms')
    if not m.get('motion_jitter_convention_verified'):
        result['missing_contracts'].append('motion_jitter_convention')
    if not m.get('gi_material_modulation_verified'):
        result['missing_contracts'].append('gi_radiance_material_modulation')
    result['ready_for_end_to_end_nrd_replay']=not result['missing_contracts']
    return result
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('directories',nargs='+');args=p.parse_args()
    print(json.dumps([audit(x) for x in args.directories],indent=2))
