"""Validate R393 native instance records; report exact slot churn, not FPS."""
import hashlib,json,math,struct,sys
from pathlib import Path
HEADER=struct.Struct('<6Q');RECORD=struct.Struct('<6Q');INSTANCE=struct.Struct('<12fIIQ')
def analyze(path):
 data=Path(path).read_bytes()
 if len(data)<HEADER.size:raise ValueError('Truncated header')
 magic,version,count,uploads,rejected,busy=HEADER.unpack_from(data)
 if magic!=0x3130333933545243 or version!=1 or not 1<=count<=48:raise ValueError('Unsupported capture header')
 offset=HEADER.size;previous=[];records=[];last=0
 for index in range(count):
  if offset+RECORD.size>len(data):raise ValueError('Truncated record')
  tick,address,resource,wrapper,thread,size=RECORD.unpack_from(data,offset);offset+=RECORD.size
  if not all((address,resource,wrapper,thread)) or size==0 or size>2*1024*1024 or size%64:raise ValueError('Invalid native upload record')
  if index and tick<last:raise ValueError('Out-of-order capture')
  if offset+size>len(data):raise ValueError('Truncated instance bytes')
  block=data[offset:offset+size];offset+=size
  current=[block[x:x+64] for x in range(0,size,64)]
  active=0;blas=set();duplicates=0;ids=set();unsupported=0
  for raw in current:
   values=INSTANCE.unpack(raw);idmask,contribution,acceleration=values[12:]
   if not all(math.isfinite(x) for x in values[:12]):raise ValueError('Nonfinite instance transform')
   # Report flags unsupported by the current backend; do not discard data.
   if acceleration%256 or contribution>>24>15:unsupported+=1
   if idmask>>24:
    active+=1;blas.add(acceleration)
   user_id=idmask&0xffffff;duplicates+=user_id in ids;ids.add(user_id)
  changed=sum(a!=b for a,b in zip(previous,current))+max(0,len(current)-len(previous))
  removed=max(0,len(previous)-len(current))
  records.append(dict(index=index,elapsed_ms=tick-last if index else 0,instances=len(current),active=active,
                      changed_slots=changed,removed_tail_slots=removed,distinct_active_blas=len(blas),
                      duplicate_user_ids=duplicates,unsupported_current_backend=unsupported,address=address,
                      source_sha256=hashlib.sha256(block).hexdigest()))
  previous=current;last=tick
 if offset!=len(data):raise ValueError('Unexpected trailing bytes')
 return dict(status='PASS_SOURCE_DATA',samples=count,uploads=uploads,rejected=rejected,busy=busy,
             capture_sha256=hashlib.sha256(data).hexdigest(),records=records,
             synchronized_cpu_upload_source=True,gpu_completion_proven=False,native_retirement=False,performance_claim=False)
if __name__=='__main__':
 try:print(json.dumps(analyze(sys.argv[1]),indent=2))
 except Exception as exc:print(json.dumps(dict(status='FAIL',error=str(exc))));sys.exit(1)
