from pathlib import Path
exec(Path(__file__).with_name('render_buffers.py').read_text().split('for label,roi in rois.items():')[0])
from scipy.ndimage import median_filter,map_coordinates
for label,roi in {'hair':(250,460,560,680),'jaw':(980,680,1020,705)}.items():
 fs=sorted((root/label).glob('rr-input*/')); x,y,X,Y=roi
 for f in fs:
  a={n:crop(read(f,n),roi).astype('float32') for n in ['pre-filter-gi','input-color','lighting-diffuse','lighting-specular','diffuse','normal-roughness','input-depth','raw-albedo']}
  lum=a['pre-filter-gi'][:,:,:3]@np.array([.2126,.7152,.0722]); med=median_filter(lum,size=5); ratio=lum/(med+1e-7);i=np.unravel_index(lum.argmax(),lum.shape)
  print(label,f.name,'GI-max xy',int(x+i[1]),int(y+i[0]),'max_luminance',lum[i],'localmedian',med[i],'ratio',ratio[i],'frac_gt10local',np.mean(ratio>10),'meanGI',np.mean(lum))
  for n in a:print(n,a[n][i].tolist())
  q=a['input-color'][:,:,0];v=q-median_filter(q,size=5);i=np.unravel_index(v.argmax(),v.shape)
  print('input red residual peak xy',int(x+i[1]),int(y+i[0]),'red',q[i],'median',q[i]-v[i])
  for n in a:print(n,a[n][i].tolist())
  norm=np.linalg.norm(a['normal-roughness'][:,:,:3],axis=-1);print('normal_lengths',np.percentile(norm,[0,50,100]).tolist())
