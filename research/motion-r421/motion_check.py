from pathlib import Path
exec(Path(__file__).with_name('render_buffers.py').read_text().split('for label,roi in rois.items():')[0])
from scipy.ndimage import map_coordinates
for label,roi in {'hair':(250,460,560,680),'jaw':(980,680,1020,705)}.items():
 fs=sorted((root/label).glob('rr-input*/'));m=[json.loads((f/'metadata.json').read_text()) for f in fs];x,y,X,Y=roi
 mv=crop(read(fs[1],'input-motion'),roi).astype('float32')*np.array([2560,1440]);yy,xx=np.mgrid[y:Y,x:X].astype('float32');jd=np.array(m[1]['jitter'])-np.array(m[0]['jitter']);
 print(label,'mv pixels percentiles',np.percentile(mv.reshape(-1,2),[0,50,100],axis=0).tolist(),'jitterdelta',jd.tolist())
 prev=read(fs[0],'diffuse').astype('float32');cur=crop(read(fs[1],'diffuse'),roi).astype('float32')
 prevn=read(fs[0],'normal-roughness').astype('float32');curn=crop(read(fs[1],'normal-roughness'),roi).astype('float32')
 results=[]
 for sign in [0,1,-1]:
  for j in [np.array([0,0]),jd,-jd,jd*np.array([1,-1]),jd*np.array([-1,1])]:
   coords=[yy+sign*mv[:,:,1]+j[1],xx+sign*mv[:,:,0]+j[0]]
   warped=np.stack([map_coordinates(prev[:,:,i],coords,order=1) for i in range(3)],axis=-1)
   wn=np.stack([map_coordinates(prevn[:,:,i],coords,order=1) for i in range(3)],axis=-1)
   results.append((float(np.mean(np.abs(warped-cur[:,:,:3]))),sign,j.tolist(),float(np.mean(np.linalg.norm(wn-curn[:,:,:3],axis=-1)))))
 print('albedo_mae sign jitter normal_error',sorted(results)[:8])
 for n in ['normal-roughness','specular','diffuse','raw-albedo','hair-responsivity']:
  a=crop(read(fs[1],n),roi).astype('float32');print(n,'finite',bool(np.isfinite(a).all()),'minmax',float(a.min()),float(a.max()))
