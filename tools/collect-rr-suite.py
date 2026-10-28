"""Validate and package one F9 location, preserving incomplete evidence."""
import argparse,importlib.util,json,os,shutil,time
from pathlib import Path

def validate(frames):
    if len(frames)!=16:raise ValueError(f'Expected 16 frames; found {len(frames)}')
    for step in range(1,9):
        pair=sorted((d for _,d in frames if d.get('suite_step')==step),key=lambda d:d['pair_index'])
        if len(pair)!=2 or [d['pair_index'] for d in pair]!=[0,1]:raise ValueError(f'Step {step}: missing pair')
        a,b=pair
        if b['frame']!=a['frame']+1 or b['previous_frame']!=a['frame'] or not b['consecutive']:raise ValueError(f'Step {step}: nonconsecutive frames')
        for d in pair:
            if not d.get('rr_active') or d.get('rr_result')!=1 or d.get('reset')!=0 or not d.get('output_captured'):raise ValueError(f'Step {step}: invalid RR output')
            if d.get('suite_samples')!=(16 if step==8 else 4) or d.get('suite_raw_rr_clamp')!=(step==2):raise ValueError(f'Step {step}: wrong settings')
            if any(not r.get('copied') for r in d['resources']):raise ValueError(f'Step {step}: missing resource')

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--pid',required=True,type=int);p.add_argument('--session',required=True,type=int);p.add_argument('--output',required=True,type=Path);a=p.parse_args()
    root=Path(os.environ['LOCALAPPDATA'])/'ControlFGProbe'
    dest=a.output/f'RR-F9-R191-{a.pid}-{a.session}-{time.time_ns()}';dest.mkdir(parents=True)
    frames=[];errors=[]
    for folder in root.glob(f'rr-input-c1-{a.pid}-*'):
        meta=folder/'metadata.json'
        if not meta.is_file():continue
        d=json.loads(meta.read_text())
        if d.get('suite_session')!=a.session:continue
        frames.append((folder,d))
        target=dest/f"{d['suite_step']:02d}"/folder.name;target.mkdir(parents=True)
        shutil.copy2(meta,target/meta.name)
        for resource in d['resources']:
            name=resource['file']
            if Path(name).name!=name:raise ValueError('Unsafe capture path')
            source=folder/name
            expected=resource['width']*resource['height']*resource['pixel_bytes']
            if not source.is_file() or source.stat().st_size!=expected:errors.append(f'{folder.name}/{name}: missing or wrong size');continue
            shutil.copy2(source,target/name)
    try:validate(frames)
    except ValueError as e:errors.append(str(e))
    local=root.parent
    for directory,pattern in [(root,f'probe-*-{a.pid}.log'),(local/'Control-FG'/'Diagnostics',f'Lighting-*-{a.pid}-*.log'),(Path(os.environ.get('TEMP',str(local)))/'Control-FG'/'Diagnostics',f'Lighting-*-{a.pid}-*.log')]:
        if directory.exists():
            for source in directory.rglob(pattern):
                (dest/'logs').mkdir(exist_ok=True);shutil.copy2(source,dest/'logs'/source.name)
    (dest/'suite-report.json').write_text(json.dumps({'build':'R191','pid':a.pid,'session':a.session,'complete':not errors,'errors':errors,'frames':[d for _,d in frames],'notes':['All isolation is gain 1.','Clamp comparison is separate settled stages, not simultaneous samples.','No RR-boundary clamp does not remove the per-hit resolve cap.','Captures are intrusive and not FPS benchmarks.']},indent=2))
    spec=importlib.util.spec_from_file_location('package',Path(__file__).with_name('package-lighting-capture.py'));module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    out=module.package(dest);print(f'Send all ZIPs from {out}',flush=True)
    if errors:print('\n'.join(errors),flush=True)
    if os.name=='nt':os.startfile(out)
    return int(bool(errors))
if __name__=='__main__':raise SystemExit(main())
