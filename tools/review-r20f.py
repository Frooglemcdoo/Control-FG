"""Reproduce the recovery-relevant evidence from the supplied r20f log archive."""
from pathlib import Path
import hashlib,json,re,sys,zipfile
archive=Path(sys.argv[1]);root=Path(__file__).resolve().parents[1]
def number(line,key):
 return int(re.search(r'\b'+key+r'=(\d+)',line)[1])
with archive.open('rb') as f:archive_hash=hashlib.file_digest(f,'sha256').hexdigest()
with zipfile.ZipFile(archive) as z:
 names={n.replace('\\','/'):n for n in z.namelist()}
 build=json.loads(z.read(names['package/build__build-validation.json']).decode('utf-8-sig'))
 assert build['Version']=='1.0.0-RR-Native-G12-r20f'
 logs={n:z.read(v) for n,v in names.items() if n.startswith('probe/') and n.endswith('.log')}
 main=max(logs,key=lambda n:logs[n].count(b'RR_NATIVE_EVALUATED'))
 lines=logs[main].decode('utf-8-sig').splitlines()
 def events(token):return [line for line in lines if token in line]
 evaluated=events(' RR_NATIVE_EVALUATED ')
 assert evaluated and all('success=1' in l and 'result=0x00000001' in l for l in evaluated)
 noisy=events(' RR_NOISY_REFLECTION ')
 assert all('copied=1 failed=0' in l for l in noisy)
 frames=[number(l,'frame') for l in noisy]
 assert frames==list(range(2244,6532))
 stops=events(' RR_FRAME_STOP ');assert not stops
 modes=events(' RR_FRAME_MODE ')
 first_stopped=next(l for l in modes if 'stopped=1' in l)
 history=events(' RR_NATIVE_HISTORY_RESET ');assert history and 'success=1' in history[0]
 counters=events(' COUNTERS ');assert all('aa_failures=0' in l for l in counters)
 rr_calls=events(' RR_G12_EVALUATION_ENTRY branch=1 ')
 resume=next(l for l in lines[lines.index(history[0])+1:] if ' CAMERA_SNAPSHOT ' in l)
 report={
  'status':'RR_EXECUTION_CONFIRMED_RECOVERY_FAILED','archive':archive.name,'archive_sha256':archive_hash,
  'main_log':main,'log_sha256':{n:hashlib.sha256(b).hexdigest() for n,b in logs.items()},
  'version':build['Version'],'preset_requested':build['RRNativePreset'],'runtime':build['RRRuntimeVersion'],
  'rr_evaluation_success_samples':len(evaluated),'rr_evaluation_counter_lower_bound':max(number(l,'call') for l in rr_calls),
  'first_evaluation':evaluated[0],'last_evaluation_sample':evaluated[-1],
  'noisy_copies':len(noisy),'copy_frames':[frames[0],frames[-1]],
  'explicit_stop_events':len(stops),'first_stopped_mode_sample':first_stopped,
  'native_history_reset':history[0],'resumed_native_aa_frame':number(resume,'engine_frame'),
  'last_counters':counters[-1],
  'diagnosis':'Rendering stopped after frame 6531 while Present and early mode calls continued; native history reset and AA resumed at 6893. Source treats a selected Ready frame with no lighting as terminal on the next Begin.',
  'limits':['Exact first stopped frame and previous coordinator stage were not logged; Ready abandonment is inferred from source plus the absence of lighting/AA/errors and stable extent/frame progression.','The log does not identify which screenshot/capture API initiated the pause.','Successful native evaluations do not establish final RR image quality.']
 }
(root/'validation/r20f-runtime.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
