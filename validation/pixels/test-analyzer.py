from pathlib import Path
import numpy as np,json,importlib.util
r=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('analyzer',r/'tools/analyze-pixels.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
import tempfile
root=Path(tempfile.mkdtemp(prefix='controlfg-pixel-test-'))
for burst,shift in [(1,0),(2,1),(3,0)]:
 for ordinal,frame in enumerate(range(100,104),1):
  for role in ['producer','bridge']:
   value=(frame-(shift if role=='bridge' else 0)) if burst!=3 else 1
   data=np.full((9,64,128,4),value,dtype='<f2');data[...,3]=1
   stem=root/f'b{burst}-f{frame}-{role}'
   stem.with_suffix('.raw').write_bytes(data.tobytes())
   stem.with_suffix('.json').write_text(json.dumps(dict(burst=burst,ordinal=ordinal,frame=frame,epoch=burst,role=role,width=3840,height=2160,format=10,tiles=9,tile_width=128,tile_height=64,row_pitch=1024)))
a=m.analyze(root)['bursts'];a.sort(key=lambda x:x['burst'])
assert all(x['exact_matching_offsets']==[0] for x in a[0]['results'])
assert a[1]['results'][0]['exact_matching_offsets']==[]
assert all(x['exact_matching_offsets']==[1] for x in a[1]['results'][1:])
assert all(x['ambiguous_exact_match'] for x in a[2]['results'])
for w,h in [(128,64),(1920,1080),(3840,2160),(7680,4320),(16384,16384)]:
 for t in range(9):
  x=(w-128)*(t%3)//2;y=(h-64)*(t//3)//2
  assert 0<=x<=w-128 and 0<=y<=h-64
  assert (t*64*1024)%512==0 and (128*8)%256==0
print('PASS analyzer aligned/one-frame-old/static ambiguity; tile bounds/alignment')
