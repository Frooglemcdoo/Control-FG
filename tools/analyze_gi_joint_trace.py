"""Validate mixed same-dispatch spotlight/ray records; never join separate runs."""
import math,struct
from collections import defaultdict
from analyze_gi_ray_trace import analyze,CAPACITY,STRIDE

def analyze_joint(data):
 if len(data)<16:raise ValueError('missing header')
 count,*reserved=struct.unpack_from('<4I',data)
 if any(reserved) or count>CAPACITY or len(data)!=16+count*STRIDE:raise ValueError('invalid or overflowed stream')
 rays={};summaries={};spots=[];native=[]
 for offset in range(16,len(data),STRIDE):
  b=data[offset:offset+STRIDE];x,y,ray,kind=struct.unpack_from('<4I',b)
  if kind in (1,2):
   native.append(b)
   if kind==1:rays[x,y,ray]=b
   else:summaries[x,y]=b
  elif kind==3:
   if struct.unpack_from('<I',b,172)[0]!=0x281:raise ValueError('unknown spotlight marker')
   spots.append((x,y,ray,b))
  else:raise ValueError('unknown mixed record kind')
 verified=analyze(struct.pack('<4I',len(native),0,0,0)+b''.join(native))
 totals=defaultdict(lambda:[0.,0.,0.]);seen=set();records=[]
 for x,y,ray,b in spots:
  light,n,secondary,primary=struct.unpack_from('<4I',b,144);key=(x,y,ray,light)
  if key in seen:raise ValueError('duplicate pixel/ray/light; ambiguous dispatch or loop')
  seen.add(key)
  r=rays.get((x,y,ray));summary=summaries.get((x,y))
  if r is None or summary is None:raise ValueError('unmatched spotlight record')
  if (secondary,n)!=struct.unpack_from('<2I',r,144) or primary!=struct.unpack_from('<I',r,108)[0] or secondary>=65534:raise ValueError('mismatched spotlight context')
  fs=lambda offset,n=3:struct.unpack_from('<'+'f'*n,b,offset)
  # Coefficient W is not part of the audited three-coefficient contract.
  values=[v for offset,nc in [(16,3),(32,3),(48,3),(64,3),(80,3),(96,3),(112,4),(128,4),(160,3)] for v in fs(offset,nc)]
  if not all(math.isfinite(v) for v in values):raise ValueError('nonfinite spotlight data')
  factor=fs(140,1)[0];term=fs(16)
  expected=[g*t*a*factor for g,t,a in zip(fs(32),fs(48),fs(64))]
  if any(abs(a-b)>1e-6+1e-5*abs(b) for a,b in zip(term,expected)):raise ValueError('spotlight product mismatch')
  if any(abs(a-b)>1e-6+1e-5*abs(b) for a,b in zip(fs(48),struct.unpack_from('<3f',r,48))):raise ValueError('primary throughput mismatch')
  scale=struct.unpack_from('<f',summary,208)[0];normalized=[v*scale/n for v in term]
  totals[x,y]=[a+b for a,b in zip(totals[x,y],normalized)]
  records.append({'pixel':[x,y],'ray':ray,'light':light,'normalized_rgb':normalized})
 return {'schema':'gi-joint-light-analysis-v1','native_accounting':verified,'executed_spot_records':len(spots),
         'pixels_with_executed_spots':len(totals),'brightest_spot_terms':sorted(records,key=lambda r:max(r['normalized_rgb']),reverse=True)[:20],
         'limits':['Absence of a light record is not a measured zero.','Session-local light ID does not identify the desk lamp.','Spatial measurements do not establish temporal variance or invalid shading.']}
