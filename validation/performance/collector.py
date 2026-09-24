from pathlib import Path
import json,os,subprocess,tempfile,zipfile
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='rr performance collection ') as directory:
 base=Path(directory);logs=base/'logs';logs.mkdir();out=base/'out'
 lines=['qpc=1 tid=7 PROBE v1.0.0-RR-Native-G12-r20m frequency=1000000']
 for frame,mode,eligible,segment in [(8,'SR',0,1),(16,'SR',1,1),(24,'SR',1,1),(32,'RR',0,2),(40,'RR',1,2),(48,'RR',1,2),(56,'SR',1,3)]:
  lines.append(f'qpc={frame*20000} tid=7 RR_PERF_FRAME frame={frame} segment={segment} mode={mode} requested={int(mode=="RR")} ready=1 eligible={eligible} width=3840 height=2160 output_width=3840 output_height=2160 fg=0 hdr=0 rt_effects=107 reset=0 cadence_ms=20 guides_cpu_ms=1 eval_cpu_ms=0.2 replay_prepare_cpu_ms=0.5 replay_workers_cpu_ms=0.3 filter_cpu_ms=0.4')
  for stage in ['reflection_copy','live_guides','hit_distance','native_evaluation']:
   cost=4 if mode=='RR' else 2
   lines.append(f'qpc=123 tid=7 RR_PERF_GPU frame={frame} stage={stage} planned={mode} width=3840 height=2160 ms={cost} completed=1')
 # Must never count orphan, mismatched mode, extent or unfinished GPU samples.
 for frame,mode,width,completed in [(999,'RR',3840,1),(40,'SR',3840,1),(40,'RR',2560,1),(40,'RR',3840,0)]:
  lines.append(f'qpc=123 tid=7 RR_PERF_GPU frame={frame} stage=native_evaluation planned={mode} width={width} height=2160 ms=999 completed={completed}')
 name='probe-20260915-180000-001-123.log';(logs/name).write_text('\n'.join(lines)+'\n')
 subprocess.run(['/tmp/control-powershell/pwsh','-NoProfile','-File',str(root/'Collect-ControlFG-Compact-Logs.ps1'),'-LogDirectory',str(logs),'-ProjectDirectory',str(base),'-OutputDirectory',str(out)],check=True)
 archive,=out.glob('*.zip')
 with zipfile.ZipFile(archive) as z:
  report=json.loads(z.read('performance/'+Path(name).stem+'.json').decode('utf-8-sig'))
  groups={s['Segment']:s for s in report['Segments']}
  assert groups[2]['RTEffects']=='107'
  assert len(groups)==3 and groups[1]['StableFrameSamples']==2 and groups[2]['StableFrameSamples']==2
  assert groups[2]['GPU']['native_evaluation']['Samples']==2 and groups[2]['GPU']['native_evaluation']['MeanMs']==4
  assert groups[1]['GPU']['native_evaluation']['MeanMs']==2
  assert groups[1]['CPU']['cadence_ms']['MeanMs']==20 and groups[3]['StableFrameSamples']==1
  manifest=json.loads(z.read('collection-report.json').decode('utf-8-sig'))
  assert not any('unavailable' in n for n in manifest['Notes'])
  assert any(f['File'].startswith('performance/') for f in manifest['Files'])
 print('PASS real PowerShell collector and performance summary: OFF/ON/OFF segments, warmup exclusion, delayed matching GPU samples, invalid pair rejection, summary inventory and bounded ZIP')
