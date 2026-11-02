"""Validate mixed same-dispatch spotlight/ray records; never join separate runs."""
import math,struct,argparse,json
from pathlib import Path
from collections import defaultdict
from analyze_gi_ray_trace import analyze,CAPACITY,STRIDE

def analyze_joint(data):
 if len(data)<16:raise ValueError('missing header')
 count,magic,ox,oy=struct.unpack_from('<4I',data)
 dense=magic in (0x281,0x282)
 if (not dense and any((magic,ox,oy))) or count>CAPACITY or len(data)!=16+count*STRIDE:raise ValueError('invalid or overflowed stream')
 rays={};summaries={};spots=[];native=[];candidates=[]
 for offset in range(16,len(data),STRIDE):
  b=data[offset:offset+STRIDE];x,y,ray,kind=struct.unpack_from('<4I',b)
  if dense and not (ox<=x<ox+32 and oy<=y<oy+32):raise ValueError('record outside ROI')
  if kind in (1,2):
   native.append(b)
   if kind==1:rays[x,y,ray]=b
   else:summaries[x,y]=b
  elif kind==3:
   if struct.unpack_from('<I',b,172)[0]!=0x281:raise ValueError('unknown spotlight marker')
   spots.append((x,y,ray,b))
  elif kind==4 and magic==0x282:candidates.append((x,y,ray,b))
  else:raise ValueError('unknown mixed record kind')
 verified=analyze(struct.pack('<4I',len(native),0,0,0)+b''.join(native),sampling_step=1 if dense else 32)
 totals=defaultdict(lambda:[0.,0.,0.]);seen=set();records=[];spot_by_key={}
 for x,y,ray,b in spots:
  light,n,secondary,primary=struct.unpack_from('<4I',b,144);key=(x,y,ray,light)
  if key in seen:raise ValueError('duplicate pixel/ray/light; ambiguous dispatch or loop')
  seen.add(key);spot_by_key[key]=b
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
 candidate_records=[];candidate_keys=set();executed_keys=set()
 for x,y,ray,b in candidates:
  u=lambda o:struct.unpack_from('<I',b,o)[0]
  fs=lambda o,n=3:struct.unpack_from('<'+'f'*n,b,o)
  flags,light,secondary,primary=struct.unpack_from('<4I',b,96);bits=u(76);key=(x,y,ray,light)
  if key in candidate_keys:raise ValueError('duplicate candidate')
  candidate_keys.add(key)
  if u(176)!=0x282 or bits&~255 or not bits&1:raise ValueError('invalid candidate marker or validity')
  r=rays.get((x,y,ray))
  if r is None or (secondary,u(180))!=struct.unpack_from('<2I',r,144) or primary!=struct.unpack_from('<I',r,108)[0] or secondary>=65534:raise ValueError('candidate context mismatch')
  if (bits&4 and not bits&2) or (bits&8 and not bits&4) or (bits&16 and not bits&8) or (bits&224 and not bits&4):raise ValueError('candidate stage ordering')
  # Projection may be nonfinite for a rejected light (e.g. clip W=0).
  # Keep this observable rather than silently turning it into valid shading.
  projection=fs(16,4);projection_finite=all(math.isfinite(v) for v in projection)
  record={'pixel':[x,y],'ray':ray,'light':light,'validity':bits,'native_flags':flags,
          'projection_finite':projection_finite,'projection':list(projection) if projection_finite else None,
          'secondary_material':secondary,'primary_material':primary}
  groups=[]
  if bits&4:groups+=[(32,4)]
  if bits&32:groups+=[(80,4)]
  if bits&8:groups+=[(48,4),(64,3),(112,3),(128,3),(192,3),(208,3),(224,3)]
  if bits&16:groups+=[(144,2)]
  if not all(math.isfinite(v) for o,n in groups for v in fs(o,n)):raise ValueError('nonfinite valid stage')
  if bits&4:record['visibility']=list(fs(32,4))
  if bits&32:record['depth_lookup']=list(fs(80,4))
  if bits&8:
   executed_keys.add(key)
   if key not in seen:raise ValueError('executed candidate missing contribution')
   spot=spot_by_key[key]
   def close(a,b):return all(abs(x-y)<=1e-6+1e-5*abs(y) for x,y in zip(a,b))
   if not close(fs(224),struct.unpack_from('<3f',spot,32)) or not close(fs(56,1),struct.unpack_from('<f',spot,140)):raise ValueError('candidate contribution mismatch')
   if not close(fs(208),[x*y for x,y in zip(fs(192),fs(64))]):raise ValueError('cookie product mismatch')
   if not close(fs(224),[v*fs(44,1)[0]*fs(60,1)[0] for v in fs(208)]):raise ValueError('visibility product mismatch')
   record.update(response=list(fs(48,4)),cookie=list(fs(64)),shading_normal=list(fs(112)),light_direction=list(fs(128)),pre_cookie=list(fs(192)),post_cookie=list(fs(208)),gated_rgb=list(fs(224)))
   if bits&16:record['cookie_uv']=list(fs(144,2))
  candidate_records.append(record)
 if magic==0x282 and executed_keys!=seen:raise ValueError('contribution missing candidate')
 return {'schema':'gi-joint-light-analysis-v2','source_marker':hex(magic),
         'candidate_count':len(candidates),'candidate_stages':candidate_records,
         'candidate_counts':{'outside_projection':sum(not r['validity']&2 for r in candidate_records),
                             'zero_visibility':sum(bool(r['validity']&4) and not r['validity']&8 for r in candidate_records),
                             'executed':len(executed_keys),'cookie_sampled':sum(bool(r['validity']&16) for r in candidate_records)},'native_accounting':verified,'executed_spot_records':len(spots),
         'pixels_with_executed_spots':len(totals),'brightest_spot_terms':sorted(records,key=lambda r:max(r['normalized_rgb']),reverse=True)[:20],
         'limits':['Absence of a light record is not a measured zero.','Session-local light ID does not identify the desk lamp.','Spatial measurements do not establish temporal variance or invalid shading.']}

if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('trace',type=Path);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
 a.output.write_text(json.dumps(analyze_joint(a.trace.read_bytes()),indent=2,allow_nan=False)+'\n')
