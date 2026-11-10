"""Audit saved PERF COMPARE archives; no game changes or new measurements.
Usage: python analyze_existing.py ARCHIVE_DIRECTORY OUTPUT_JSON
"""
import csv, hashlib, io, json, re, statistics, sys, zipfile
from collections import defaultdict
from pathlib import Path
results=[]
for path in sorted(Path(sys.argv[1]).rglob('Performance-R*.zip')):
    with zipfile.ZipFile(path) as z:
        assert z.testzip() is None
        files={n.removeprefix('./'):z.read(n).decode('utf-8-sig') for n in z.namelist() if not n.endswith('/')}
    rows=list(csv.DictReader(io.StringIO(files['comparison.csv'])))
    frame=defaultdict(list)
    for r in csv.DictReader(io.StringIO(files['frame-times.csv'])):
        frame[r['phase']].append(float(r['interval_ms']))
    stages=list(csv.DictReader(io.StringIO(files['gpu-stage-summary.csv'])))
    gpu=defaultdict(lambda:defaultdict(float))
    for r in csv.DictReader(io.StringIO(files['gpu-timings.csv'])):
        gpu[r['phase'],r['stage']][r['present']]+=float(r['gpu_ms'])
    for r in rows:
        v=frame[r['phase']]
        assert len(v)==int(r['intervals'])
        assert abs(statistics.mean(v)-float(r['mean_frame_ms']))<0.0001
    for r in stages:
        v=list(gpu[r['phase'],r['stage']].values())
        assert len(v)==int(r['sampled_presents'])
        assert abs(statistics.mean(v)-float(r['mean_gpu_ms_per_sampled_present']))<0.0001
    summary=files['SUMMARY.txt']; indexed={r['phase']:r for r in rows}
    mean=lambda p:float(indexed[p]['mean_frame_ms'])
    drift=100*(mean('native_rr_end')/mean('native_rr_begin')-1)
    full_drift=100*(mean('full_rr_repeat')/mean('full_rr')-1)
    missing=[r['phase'] for r in rows if r['coverage_ok']!='1']
    skips=sum(int(r['skipped_gpu_samples']) for r in rows)
    verdict=not missing and abs(drift)<=5 and abs(full_drift)<=5 and skips==0 and 'Profiler fault=0 dropped_frame_rows=0' in summary
    blocks=re.split(r'\n\n([a-z0-9_]+) evidence:\n',summary)
    evidence={}
    for i in range(1,len(blocks)-1,2):
        body=blocks[i+1];evidence[blocks[i]]={}
        for key in ('effective_GI_sample_mask','relax_recorded','relax_rejections','relax_gpu_completed','relax_work_bundles_created','relax_work_bundles_reused','native_specular_filter_calls_with_rr','native_broad_diffuse_filter_calls_with_rr','rr_evaluations_failed'):
            m=re.search(r'^'+key+r'=(\d+)',body,re.M)
            if m:evidence[blocks[i]][key]=int(m[1])
        evidence[blocks[i]]['cpu_spans']={name:{'mean_ms':float(ms),'calls':int(calls)} for name,ms,calls in re.findall(r'(\w+) mean_cpu_ms=([\d.eE+-]+) calls=(\d+)',body)}
    results.append({'file':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'members_read':len(files),'raw_means_and_counts_verified':True,'usable_complete_historical_comparison':verdict,'missing_coverage':missing,'skipped_gpu_samples':skips,'baseline_drift_pct':drift,'full_drift_pct':full_drift,'full_minus_initial_native_ms':mean('full_rr')-mean('native_rr_begin'),'incremental_deltas':[{'from':a['phase'],'to':b['phase'],'frame_ms':float(b['mean_frame_ms'])-float(a['mean_frame_ms'])} for a,b in zip(rows[:6],rows[1:7])],'comparison':rows,'gpu_stages':stages,'evidence':evidence,'renderer':json.loads(files['renderer.ini']),'settings_original':files['settings-original.txt'],'settings_restored':files['settings-restored.txt'],'mod_settings':files['mod-settings.ini']})
Path(sys.argv[2]).write_text(json.dumps(results,indent=2)+'\n')
for r in results:print(r['file'], 'usable=',r['usable_complete_historical_comparison'],'drift=',round(r['baseline_drift_pct'],3),'missing=',r['missing_coverage'])
