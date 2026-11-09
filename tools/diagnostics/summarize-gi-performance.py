"""Bounded, single-ZIP performance report. No ETL or resource captures."""
import argparse, collections, csv, io, json, math, re, statistics, zipfile
from pathlib import Path

LIMIT = 4 * 1024 * 1024
KEEP = re.compile(r'GI_(?:FRAME_TIMES|OWNED_CPU|BUTTON_CAPTURE|OWNED_ADMI|OWNED_STATUS|OWNED_SCENE|OWNED_SKIP|OWNED_SCRATCH|OWNED_RECORDED)|RR_PERF_(?:FRAME|GPU)|PROBE v|CAPABILITIES|MONITORING|GI_.*(?:FAIL|REJECT)|DEVICE_REMOVED')
FIELDS = re.compile(r'(\w+)=([^\s]+)')

def summarize(folder, logs, pid, send):
    marks = [json.loads(x) for x in (folder/'phases.jsonl').read_text(encoding='utf-8-sig').splitlines()] if (folder/'phases.jsonl').exists() else []
    phases = {}
    for name in ('STILL', 'PAN'):
        a = next((x for x in marks if x['phase']==name+'_BEGIN'), None)
        b = next((x for x in marks if x['phase']==name+'_END'), None)
        if a and b: phases[name]=(int(a['qpc']), int(b['qpc']), int(a['frequency']))
    lo=min((x[0] for x in phases.values()),default=0)
    hi=max((x[1] for x in phases.values()),default=0)
    focused=collections.deque(); size=0; discarded=0; frames=[]; cpu=collections.defaultdict(lambda:[0,0.,0.]); mode=[]; settings=[]
    for path in sorted(logs.glob('*')):
        if not path.is_file() or path.suffix!='.log' or not re.search(r'-'+str(pid)+r'(?:-|\.)',path.name): continue
        with path.open(encoding='utf-8-sig', errors='replace') as f:
            for line in f:
                if not KEEP.search(line): continue
                fields=dict(FIELDS.findall(line)); qpc=int(fields.get('qpc',0))
                if 'GI_BUTTON_CAPTURE ' in line and qpc<=hi: mode=[line.strip()]
                if 'RR_PERF_FRAME ' in line and qpc<=hi: settings=[line.strip()]
                in_window=lo<=qpc<=hi+20_000_000 if hi else False
                if 'GI_FRAME_TIMES ' in line:
                    try:
                        start=int(fields['start_qpc']); freq=int(fields['frequency']); ticks=[int(x) for x in fields['ticks'].split(',')]
                        if len(ticks)!=int(fields['count']) or start+sum(ticks)!=int(fields['end_qpc']): continue
                        for delta in ticks:
                            end=start+delta
                            for phase,(a,b,_) in phases.items():
                                if a<=start and end<=b: frames.append((phase,start,end,delta*1000/freq))
                            start=end
                    except (KeyError,ValueError,ZeroDivisionError): pass
                if in_window:
                    encoded=line.encode('utf-8');focused.append(encoded);size+=len(encoded)
                    while size>LIMIT and focused: size-=len(focused.popleft());discarded+=1
                if 'GI_OWNED_CPU ' in line:
                    for phase,(a,b,_) in phases.items():
                        if a<=qpc<=b:
                            key=(phase,fields['stage']);v=cpu[key]
                            v[0]+=int(fields['calls']);v[1]+=float(fields['total_ms']);v[2]=max(v[2],float(fields['max_ms']))
    report={'measurement':'native game Present-entry intervals; not generated/displayed FPS or GPU pass time', 'phases':{},'mode_record':mode,'last_settings_record':settings,'focused_lines_discarded':discarded,'cpu_stage_note':'Report-time phase assignment; boundary intervals may overlap. Stages are inclusive/nested and must not be added.'}
    for phase,(a,b,freq) in phases.items():
        vals=[r[3] for r in frames if r[0]==phase];vals.sort();duration=(b-a)/freq
        report['phases'][phase]={'window_seconds':duration,'frame_intervals':len(vals),'covered_seconds':sum(vals)/1000,'coverage_fraction':sum(vals)/1000/duration if duration>0 else 0}
        if vals:
            report['phases'][phase].update(mean_ms=statistics.mean(vals),fps=1000/statistics.mean(vals),p95_ms=vals[math.ceil(.95*len(vals))-1],p99_ms=vals[math.ceil(.99*len(vals))-1],max_ms=max(vals),over_33ms=sum(x>33.333 for x in vals),over_50ms=sum(x>50 for x in vals))
    report['cpu_stages']=[dict(phase=p,stage=s,calls=v[0],total_ms=v[1],mean_ms=v[1]/v[0] if v[0] else 0,max_ms=v[2]) for (p,s),v in sorted(cpu.items())]
    foreground=[]
    sample_path=folder/'process-samples.csv'
    if sample_path.exists():
        with sample_path.open(encoding='utf-8-sig',newline='') as f:
            foreground=[r for r in csv.DictReader(f) if r.get('game_pid')!=r.get('foreground_pid')]
    report['nonforeground_process_samples']=len(foreground)
    rows=['Control GI compact performance capture','',report['measurement'],'FG should be OFF. Compare the same scene/settings and same phase.','CPU stages are nested; do not add them. No GPU attribution without a separate focused trace.','']
    for phase,v in report['phases'].items():
        if 'fps' in v: rows.append(f"{phase}: {v['fps']:.2f} FPS | mean {v['mean_ms']:.3f} ms | p95 {v['p95_ms']:.3f} ms | p99 {v['p99_ms']:.3f} ms | coverage {v['coverage_fraction']:.1%}")
        else: rows.append(f'{phase}: NO FRAME SAMPLES. Requires the compact-recorder DLL; not a valid FPS result.')
    if foreground: rows+=['WARNING: game was not foreground during some process samples; inspect process-samples.csv.']
    rows+=['']+mode+settings
    if any(v['coverage_fraction']<.9 for v in report['phases'].values()) or len(report['phases'])!=2: rows+=['WARNING: incomplete phase/frame coverage; inspect capture-status.json.']
    samples=io.StringIO();writer=csv.writer(samples);writer.writerow(['phase','start_qpc','end_qpc','frame_ms']);writer.writerows(frames)
    send.mkdir(parents=True,exist_ok=True);target=send/(folder.name+'-compact.zip')
    # Explicit allowlist: never recursively include traces, buffers, or full logs.
    with zipfile.ZipFile(target,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
        z.writestr('SUMMARY.txt','\n'.join(rows)+'\n');z.writestr('summary.json',json.dumps(report,indent=2));z.writestr('frame-times.csv',samples.getvalue());z.writestr('focused.log',b''.join(focused))
        for name in ('phases.jsonl','process-samples.csv','session.json','installed-dll.json','gpu.json','capture-status.json','error.txt'):
            path=folder/name
            if path.is_file() and path.stat().st_size<1024*1024:z.write(path,name)
    (send/'SUMMARY.txt').write_text('\n'.join(rows)+'\n',encoding='utf-8')
    print(f'Send one ZIP: {target} ({target.stat().st_size:,} bytes)')
    return report

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('folder',type=Path);p.add_argument('--logs',required=True,type=Path);p.add_argument('--pid',required=True,type=int);p.add_argument('--send',required=True,type=Path);a=p.parse_args();summarize(a.folder,a.logs,a.pid,a.send)
