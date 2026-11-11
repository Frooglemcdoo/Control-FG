from pathlib import Path
exec(Path(__file__).with_name('render_buffers.py').read_text().split('for label,roi in rois.items():')[0])
from scipy.ndimage import median_filter,maximum_filter
regions={'hair':(250,460,560,680),'jaw':(980,680,1032,711)}
for label,roi in regions.items():
 for f in sorted((root/label).glob('rr-input*/')):
  arrays={n:read(f,n) for n in ['pre-filter-gi','lighting-diffuse','lighting-specular','input-color','diffuse','rr-output','input-motion','input-depth']}
  print(label,f.name)
  for n,a in arrays.items():
   c=crop(a,roi); print(n,'finite',float(np.isfinite(c).mean()),'p50/95/99/max',np.percentile(c[:,:,:min(3,c.shape[2])],[50,95,99,100]).tolist())
  # Luminance residual against 5x5 median, evaluated in the source grid.
  vals={}
  for n in ['pre-filter-gi','lighting-diffuse','lighting-specular','input-color']:
   a=crop(arrays[n],roi)[:,:,:3].astype('float32'); y=a@np.array([.2126,.7152,.0722]); v=y-median_filter(y,size=5); vals[n]=v.ravel()
  for n in list(vals)[:-1]: print('residual correlation with input',n,float(np.corrcoef(vals[n],vals['input-color'])[0,1]))
  x,y,X,Y=roi;a=arrays['rr-output'][:,:,:3]; c=a[round(y*1.5):round(Y*1.5),round(x*1.5):round(X*1.5),0].astype('float32');c=c-median_filter(c,size=7)
  ij=np.unravel_index(c.argmax(),c.shape); oy=round(y*1.5)+ij[0];ox=round(x*1.5)+ij[1]; sy=round((oy+.5)/1.5-.5);sx=round((ox+.5)/1.5-.5)
  print('strongest output red local peak','output xy',ox,oy,'input xy',sx,sy,'residual',c[ij],'RGB',a[oy,ox].tolist())
  for n in ['pre-filter-gi','lighting-diffuse','lighting-specular','input-color','diffuse','input-motion','input-depth']:
   print('peak',n,arrays[n][sy,sx].tolist())
