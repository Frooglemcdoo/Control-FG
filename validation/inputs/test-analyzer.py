from pathlib import Path
import importlib.util,tempfile,json
import numpy as np
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('inputs',root/'tools/analyze-inputs.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
folder=Path(tempfile.mkdtemp(prefix='controlfg-inputs-test-'))
def pack(f,a):
 rows=np.zeros((9,64,1024),dtype=np.uint8)
 if f==10:
  pixels=np.concatenate([a,np.ones((*a.shape[:-1],1))],axis=-1).astype('<f2').view('u1').reshape(9,64,1024)
 elif f==24:
  c=np.rint(np.clip(a,0,1)*1023).astype('uint32');v=c[...,0]|(c[...,1]<<10)|(c[...,2]<<20)|(3<<30);pixels=v.astype('<u4').view('u1').reshape(9,64,512)
 else:pixels=np.rint(np.clip(a,0,1)*255).astype('u1').reshape(9,64,128)
 rows[...,:pixels.shape[-1]]=pixels
 return rows.tobytes()
def save(frame,role,f,a,hdr):
 meta=dict(burst=hdr+1,ordinal=1,frame=frame,epoch=hdr,hdr=hdr,role=role,width=3840,height=2160,format=f,tiles=9,tile_width=128,tile_height=64,row_pitch=1024)
 p=folder/f'b{hdr+1}-f{frame}-{role}.json';raw=pack(f,a);p.write_text(json.dumps(meta));p.with_suffix('.raw').write_bytes(raw)
 return m.decode(meta,raw)
for hdr,f in [(0,24),(1,10)]:
 frame=10+hdr
 h=save(frame,'hudless',f,np.full((9,64,128,3),.1),hdr)
 c=save(frame,'producer',f,np.full((9,64,128,3),.12),hdr)
 save(frame,'ui_alpha',61,m.mask_expected(c,h),hdr)
 save(frame,'bridge',f,c,hdr)
 if hdr:
  save(frame,'tagged_pq',24,m.pq_expected(h),hdr);save(frame,'final_pq',24,m.pq_expected(c),hdr)
a=m.analyze(folder)['frames'];assert len(a)==2
for row in a:assert not row['missing_roles'] and row['mask_pixels_over_2_codes']==0
hdr=next(row for row in a if row['hdr']);assert hdr['bridge_same_frame_rgb_exact'];assert hdr['tagged_pq_max_error_codes']<=.51;assert hdr['final_pq_max_error_codes']<=.51
save(10,'ui_alpha',61,np.zeros((9,64,128)),0)
a=m.analyze(folder)['frames'];assert next(row for row in a if not row['hdr'])['mask_pixels_over_2_codes']==9*64*128
# Eight-bit formats, sRGB linearization and BGRA ordering.
for f in [28,29,87,91]:
 raw=np.zeros((9,64,1024),dtype=np.uint8);pixel=np.tile(np.array([32,64,128,255],dtype='u1'),128);raw[...,:512]=pixel
 x=m.decode({'format':f,'row_pitch':1024},raw.tobytes());assert x.shape==(9,64,128,3)
 assert (x[0,0,0,0]>x[0,0,0,2]) == (f in [87,91])
print('PASS SDR/HDR layouts; correct/corrupt UI masks; PQ reference; BGRA/sRGB decoding')

# Distinguish selected stale content from a current alternate buffer in both domains.
for domain,f in [(0,24),(1,10)]:
 for frame in [20,21,22]:
  save(frame,'producer',f,np.full((9,64,128,3),frame*.02),domain)
  save(frame,'bridge',f,np.full((9,64,128,3),(frame-1)*.02),domain)
  save(frame,'shadow0',f,np.full((9,64,128,3),frame*.02),domain)
rows=m.analyze(folder)['frames']
for row in rows:
 if row['frame'] in [21,22]:
  assert row['bridge_unique_previous_frame_match']
  assert row['producer_rgb_match_offsets']['bridge']==[-1]
  assert row['current_frame_shadow_candidates']==['shadow0']
# Identical neighboring frames must remain ambiguous, not be labelled stale.
for frame in [30,31]:
 for role in ['producer','bridge']:save(frame,role,24,np.full((9,64,128,3),.8),0)
row=next(x for x in m.analyze(folder)['frames'] if x['frame']==31)
assert row['producer_rgb_match_offsets']['bridge']==[-1,0]
assert not row['bridge_unique_previous_frame_match']
print('PASS SDR/HDR previous-frame matching, alternate-buffer selection, stationary ambiguity')
