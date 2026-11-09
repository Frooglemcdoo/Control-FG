"""Extract the allocation blocker from the supplied r20g log archive."""
from pathlib import Path
import hashlib,json,re,sys,zipfile
archive=Path(sys.argv[1]);root=Path(__file__).resolve().parents[1]
def number(line,key):
 return int(re.search(r'\b'+key+r'=(\d+)',line)[1])
with archive.open('rb') as f:archive_hash=hashlib.file_digest(f,'sha256').hexdigest()
with zipfile.ZipFile(archive) as z:
 names={n.replace('\\','/'):n for n in z.namelist()}
 build=json.loads(z.read(names['package/build__build-validation.json']).decode('utf-8-sig'))
 assert build['Version']=='1.0.0-RR-Native-G12-r20g'
 logs={n:z.read(v) for n,v in names.items() if n.startswith('probe/') and n.endswith('.log')}
 main=max(logs,key=lambda n:logs[n].count(b'RR_REFLECTION_PREPARE_REQUEST'))
 lines=logs[main].decode('utf-8-sig').splitlines()
 def events(token):return [line for line in lines if ' '+token+' ' in line]
 requests=events('RR_REFLECTION_PREPARE_REQUEST');assert len(requests)==1
 request=requests[0];width=number(request,'width');height=number(request,'height');layers=number(request,'layers')
 assert (width,height,layers)==(3840,2160,4)
 stops=events('RR_REFLECTION_STOP');assert len(stops)==1
 assert 'reason=allocation_or_512MiB_budget_rejected' in stops[0]
 evaluated=events('RR_NATIVE_EVALUATED');assert not evaluated
 modes=events('RR_FRAME_MODE');assert all('selected=0' in l for l in modes)
 status=events('RR_REFLECTION_STATUS')[-1]
 assert all(number(status,k)==0 for k in ('recorded','submitted','retired'))
 raw_bytes=width*height*layers*(2+8)*3
 assert raw_bytes>number(request,'budget_bytes')
 report={
  'status':'REFLECTION_BUDGET_BLOCKED_BEFORE_RR',
  'archive':archive.name,'archive_sha256':archive_hash,'main_log':main,
  'log_sha256':{n:hashlib.sha256(b).hexdigest() for n,b in logs.items()},
  'version':build['Version'],'build_budget_bytes':build['RRReflectionCaptureBudgetBytes'],
  'request':request,'stop':stops[0],'last_reflection_status':status,
  'last_counters':events('COUNTERS')[-1],
  'rr_evaluation_events':len(evaluated),'rr_selected_mode_samples':sum('selected=1' in l for l in modes),
  'pause_events':len(events('RR_FRAME_PAUSED')),'recovery_sr_events':len(events('RR_FRAME_RECOVERY_SR')),
  'capture_shape':[width,height,layers],'slots':3,
  'uncompressed_capture_payload_bytes':raw_bytes,'uncompressed_capture_payload_mib':raw_bytes/1048576,
  'diagnosis':'The source preflights three full R16_UINT/RGBA16F array pairs against 512 MiB. The requested payload alone is 949.21875 MiB, so the budget rejects this capture before creation; no RR frames run.',
  'limits':['The combined old stop message does not report driver allocation sizes or HRESULTs; no VRAM exhaustion is established.','Capture payload excludes device alignment and all other game/guide allocations.','Screenshot recovery was not exercised because RR never activated.']
 }
(root/'validation/r20g-runtime.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
