"""Validate matching full-screen GI/RR frames and stream bounded ZIP parts.
Does not delete/move capture sources or request any gameplay keypress.
"""
import argparse, datetime, hashlib, json, os, pathlib, zipfile, shutil, sys, traceback


def inspect(root, pid, motion=False):
    records=[]; issues=[]; files=set()
    for directory in root.glob(f'rr-input-c1-{pid}-*'):
        meta=directory/'metadata.json'
        if not meta.exists():continue
        m=json.loads(meta.read_text(encoding='utf-8-sig'))
        if m.get('scene_capture_build')!='R484' or bool(m.get('motion_sequence'))!=motion:continue
        frame=m['frame'];records.append(m);files.update(p for p in directory.iterdir() if p.is_file())
        def require(ok,reason):
            if not ok:issues.append(f'frame {frame}: {reason}')
        require(m.get('rr_active') and m.get('rr_result')==1 and m.get('output_captured'), 'RR output not verified')
        require(m.get('export_stride')==1 and m.get('scene_full_resolution'), 'not full resolution')
        require(m.get('pre_gi_captured') and m.get('pre_gi_frame')==frame, 'temporal-source frame missing/mismatched')
        for resource in m.get('resources',[]):
            p=directory/resource['file']
            require(resource.get('copied') and p.is_file() and p.stat().st_size==resource['width']*resource['height']*resource['pixel_bytes'],f"invalid image {resource['file']}")
        hit=directory/'gi-hit-slices.json'
        if not hit.exists():require(False,'hit manifest missing');continue
        h=json.loads(hit.read_text());binary=directory/h['file']
        require(h.get('full_screen') and h.get('raw_gi_captured') and h.get('detailed_frame_matched')==1 and h.get('frame')==frame,'GI stage/frame not verified')
        require(h.get('gpu_fence_completed') and h.get('all_batches_recorded') and h.get('recorded_batches')==1,'GI batch incomplete')
        require(binary.exists() and binary.stat().st_size==h['bytes'],'hit binary size mismatch')
        raw=h['raw_gi'];require(raw['width']==m['width'] and raw['height']==m['height'] and raw['offset']+raw['bytes']<=h['bytes'],'raw GI extent/range mismatch')
        for b in h['batches']:
            require(b['region_width']==m['width'] and b['region_height']==m['height'] and b['region_count']==1 and b['captured_slices']==h['rays_per_batch'],'cropped or missing hit slices')
            for r in b['resources']:require(r['offset']+r['slice_region_bytes']*b['captured_slices']<=h['bytes'],'hit slice range overflow')
        detail=root/f'gi-scene-detail-{pid}-{frame}';dm=detail/'metadata.json'
        if detail.exists():files.update(p for p in detail.iterdir() if p.is_file())
        if not dm.exists():require(False,'detailed-grid export not complete');continue
        d=json.loads(dm.read_text());db=detail/d['file']
        require(d.get('schema')=='gi-scene-details-r484','wrong detailed capture build')
        require(d.get('frame')==frame and d.get('gpu_completed') and d.get('width')==m['width'] and d.get('height')==m['height'],'detailed-grid frame/extent mismatch')
        require(db.exists() and db.stat().st_size==d['bytes'] and d['red_offset']+d['columns']*d['rows']*d['red_stride']==d['bytes'],'detailed-grid binary size mismatch')
        guide=d.get('guides',{});gp=detail/guide.get('file','missing');selection=guide.get('selection',{})
        require(guide.get('bytes')==688128 and guide.get('table_bytes')==344064 and guide.get('stride')==84 and guide.get('slots')==4096 and guide.get('output_offset')==0 and guide.get('input_offset')==344064 and guide.get('immutable_input_used_by_both_stages') is True, 'frozen guide layout/provenance missing')
        require(gp.is_file() and gp.stat().st_size==688128,'frozen guide binary missing/size mismatch')
        require(selection.get('sampleFrame')==frame and selection.get('rayCount')==h['rays_per_batch'] and selection.get('version',0)>0 and 0<selection.get('sourceFrame',0)<=frame and selection.get('fraction')==(.25 if h['rays_per_batch']==4 else .2),'frozen guide selection mismatch')
        if motion:
            trace=d.get('motion',{});tp=detail/trace.get('file','missing')
            require(trace.get('full_screen') is True and trace.get('all_rays') is True and trace.get('float_precision')==32 and trace.get('rays')==h['rays_per_batch'], 'full-precision all-pixel/ray trace missing')
            require(trace.get('header_bytes')==2048 and trace.get('ray_stride')==224 and trace.get('bytes')==2048+m['width']*m['height']*h['rays_per_batch']*224, 'motion trace layout mismatch')
            require(tp.is_file() and tp.stat().st_size==trace.get('bytes'), 'motion trace binary missing/truncated')
            require(m.get('projection_valid') and m.get('motion_scale_valid') and m.get('reset')==0, 'RR transform/motion scale unknown or history reset')
            names={r['file'] for r in m.get('resources',[])}
            required={'normal-roughness.rgba16f','specular.rgba16f','diffuse.rgba16f','raw-albedo.rgba16f','input-depth.r32f','native-gbuffer1.rgba8','native-gbuffer2.rgba8','gi-at-rr-input.rgba16f','pre-filter-gi.rgba16f','native-reconstruction-map.r32u'}
            require(required<=names, 'required RR/GI guides missing: '+str(sorted(required-names)))
            require(any(n.startswith('input-motion.') for n in names) and any(n.startswith('input-color.') for n in names) and any(n.startswith('rr-output.') for n in names), 'motion/color/output lane missing')
            continue
        pair=d.get('paired',{});pb=detail/pair.get('file','missing')
        require(pair.get('baseline_sampling')=='native_samplelevel0' and pair.get('comparison_sampling')=='sampler_independent_bilinear_mip1_clamped_to_view','R484 sampling comparison missing')
        require(pair.get('same_hits_weights_and_scene') and pair.get('writes_game_outputs') is False and pair.get('before_rr'), 'paired comparison provenance missing')
        require(pair.get('texture_probe_stride')==192 and pair.get('texture_probe_points')==min(m['width'],512)*min(m['height'],512) and pair.get('texture_probe_offset')==m['width']*m['height']*32 and pair.get('total_file_bytes')==m['width']*m['height']*32+min(m['width'],512)*min(m['height'],512)*192, 'texture probe layout missing/mismatched')
        require(pair.get('stride')==32 and pair.get('bytes')==m['width']*m['height']*32 and pb.is_file() and pb.stat().st_size==pair.get('total_file_bytes'), 'paired comparison binary missing/size mismatch')
    if motion:
        records.sort(key=lambda m:m['frame'])
        if len(records)!=16:issues.append(f'expected 16 consecutive motion frames, found {len(records)}')
        for i,m in enumerate(records):
            if m.get('motion_index')!=i or m.get('motion_frames')!=16 or m.get('capture_ordinal')!=1 or (i and (m['frame']!=records[i-1]['frame']+1 or m.get('previous_frame')!=records[i-1]['frame'])):
                issues.append(f"frame {m['frame']}: sequence index/continuity mismatch")
    else:
        for ordinal in (1,2):
            pair=sorted((m for m in records if m.get('capture_ordinal')==ordinal),key=lambda m:m['frame'])
            if len(pair)!=2 or [m.get('pair_index') for m in pair]!=[0,1] or pair[1]['frame']!=pair[0]['frame']+1 or pair[1].get('previous_frame')!=pair[0]['frame']:
                issues.append(f'pair {ordinal}: two consecutive matched frames required')
        if len(records)!=4:issues.append(f'expected 4 frames, found {len(records)}')
    files.update(root.glob(f'probe-*-{pid}.log'))
    return records,issues,sorted(files)


def package(root,files,output,manifest,chunk_bytes=64*1024*1024,part_bytes=256*1024*1024):
    total=sum(path.stat().st_size for path in files)
    output.mkdir(parents=True,exist_ok=False)
    print(f"Output: {output.resolve()}",flush=True)
    print(f"Source data: {total/2**30:.2f} GiB; destination free: {shutil.disk_usage(output).free/2**30:.2f} GiB",flush=True)
    print("Original captures will be kept. Compression size is not known in advance.",flush=True)
    manifest.update(files=[],parts=[]);archive=None;part_size=0;number=0
    try:
        for path in files:
            print(f'File {len(manifest["files"])+1}/{len(files)}: {path.name}',flush=True)
            rel=path.relative_to(root).as_posix();entry={'path':rel,'bytes':path.stat().st_size,'chunks':[]};digest=hashlib.sha256()
            with path.open('rb') as source:
                offset=0
                while True:
                    data=source.read(chunk_bytes)
                    if not data:break
                    if archive is None or part_size+len(data)>part_bytes:
                        if archive:archive.close()
                        number+=1;name=f'scene-part-{number:03d}.zip';manifest['parts'].append({'file':name})
                        archive=zipfile.ZipFile(output/name,'w',zipfile.ZIP_DEFLATED,compresslevel=6,allowZip64=True);part_size=0
                    name=f'payload/{len(manifest["files"]):05d}-{len(entry["chunks"]):05d}.bin'
                    archive.writestr(name,data);part_size+=len(data);digest.update(data)
                    print(f'  Part {number:03d}: {offset+len(data)}/{entry["bytes"]} source bytes',flush=True)
                    entry['chunks'].append({'part':number,'entry':name,'offset':offset,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()});offset+=len(data)
            if offset!=entry['bytes']:raise ValueError(f'File changed while packaging: {rel}')
            entry['sha256']=digest.hexdigest();manifest['files'].append(entry)
    finally:
        if archive:archive.close()
    for part in manifest['parts']:
        p=output/part['file'];part['bytes']=p.stat().st_size
        print(f'Verifying part hash: {p.name}',flush=True)
        digest=hashlib.sha256()
        with p.open('rb') as f:
            for data in iter(lambda:f.read(1024*1024),b''):digest.update(data)
        part['sha256']=digest.hexdigest()
    with zipfile.ZipFile(output/'scene-manifest.zip','w',zipfile.ZIP_DEFLATED) as z:z.writestr('scene-manifest.json',json.dumps(manifest,indent=2))
    return output


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=pathlib.Path,default=pathlib.Path(os.environ.get('LOCALAPPDATA','.'))/'ControlFGProbe');p.add_argument('--pid',type=int);p.add_argument('--output',type=pathlib.Path);a=p.parse_args()
    if not a.pid:
        candidates=[]
        for meta in a.root.glob('rr-input-c1-*/metadata.json'):
            try:
                m=json.loads(meta.read_text(encoding='utf-8-sig'))
                if m.get('scene_capture_build')=='R484':candidates.append((meta.stat().st_mtime,m['pid']))
            except (ValueError,OSError):pass
        if not candidates:raise SystemExit('No R484 scene frames saved. Wait for Saved, or send compact logs if capture failed.')
        a.pid=max(candidates)[1]
    records,issues,files=inspect(a.root,a.pid)
    if issues:
        raise SystemExit('R484 capture is incomplete; no upload ZIPs were created.\n'+'\n'.join(issues)+'\nKeep the saved captures. Send this message before rerunning the game.')
    if not files:raise SystemExit('No files found for that scene session.')
    output=a.output or pathlib.Path.cwd()/('GI-Scene-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    manifest={'schema':'control-gi-scene-r484','pid':a.pid,'complete':not issues,'issues':issues,'frames':[m['frame'] for m in records],
              'limitations':['physical SRV slice mapping not independently verified','individual light operands and full texture mip contents not exported','instance/primitive IDs not exported','RR output precedes final display processing'],'reassembly':'Concatenate each file chunks in listed order; verify chunk and whole-file SHA256.'}
    print('Packaging full-screen data. This may take several minutes.',flush=True)
    package(a.root,files,output,manifest)
    print(('COMPLETE' if not issues else 'INCOMPLETE DIAGNOSTIC')+': '+str(output))
    for issue in issues:print(issue)
    print('Send scene-manifest.zip and EVERY scene-part ZIP. Original captures remain in place.')
if __name__=='__main__':
    print(f'GI scene packager R484 fix 2; Python {sys.version}',flush=True)
    try:main()
    except Exception:
        traceback.print_exc()
        print('FAILED: Keep the original captures. Do not upload partial ZIPs without scene-manifest.zip. Send the packager log; no new game capture is needed.',file=sys.stderr,flush=True)
        sys.exit(1)
