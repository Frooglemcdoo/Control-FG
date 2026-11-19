"""Validate matching full-screen GI/RR frames and stream bounded ZIP parts.
Does not delete/move capture sources or request any gameplay keypress.
"""
import argparse, datetime, hashlib, json, os, pathlib, zipfile


def inspect(root, pid):
    records=[]; issues=[]; files=set()
    for directory in root.glob(f'rr-input-c1-{pid}-*'):
        meta=directory/'metadata.json'
        if not meta.exists():continue
        m=json.loads(meta.read_text(encoding='utf-8-sig'))
        if m.get('scene_capture_build')!='R477':continue
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
        require(d.get('frame')==frame and d.get('gpu_completed') and d.get('width')==m['width'] and d.get('height')==m['height'],'detailed-grid frame/extent mismatch')
        require(db.exists() and db.stat().st_size==d['bytes'] and d['red_offset']+d['columns']*d['rows']*d['red_stride']==d['bytes'],'detailed-grid binary size mismatch')
    for ordinal in (1,2):
        pair=sorted((m for m in records if m.get('capture_ordinal')==ordinal),key=lambda m:m['frame'])
        if len(pair)!=2 or [m.get('pair_index') for m in pair]!=[0,1] or pair[1]['frame']!=pair[0]['frame']+1 or pair[1].get('previous_frame')!=pair[0]['frame']:
            issues.append(f'pair {ordinal}: two consecutive matched frames required')
    if len(records)!=4:issues.append(f'expected 4 frames, found {len(records)}')
    files.update(root.glob(f'probe-*-{pid}.log'))
    return records,issues,sorted(files)


def package(root,files,output,manifest,chunk_bytes=64*1024*1024,part_bytes=256*1024*1024):
    output.mkdir(parents=True,exist_ok=False)
    manifest.update(files=[],parts=[]);archive=None;part_size=0;number=0
    try:
        for path in files:
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
                    entry['chunks'].append({'part':number,'entry':name,'offset':offset,'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()});offset+=len(data)
            if offset!=entry['bytes']:raise ValueError(f'File changed while packaging: {rel}')
            entry['sha256']=digest.hexdigest();manifest['files'].append(entry)
    finally:
        if archive:archive.close()
    for part in manifest['parts']:
        p=output/part['file'];part['bytes']=p.stat().st_size
        with p.open('rb') as f:part['sha256']=hashlib.file_digest(f,'sha256').hexdigest()
    with zipfile.ZipFile(output/'scene-manifest.zip','w',zipfile.ZIP_DEFLATED) as z:z.writestr('scene-manifest.json',json.dumps(manifest,indent=2))
    return output


def main():
    p=argparse.ArgumentParser();p.add_argument('--root',type=pathlib.Path,default=pathlib.Path(os.environ.get('LOCALAPPDATA','.'))/'ControlFGProbe');p.add_argument('--pid',type=int);p.add_argument('--output',type=pathlib.Path);a=p.parse_args()
    if not a.pid:
        candidates=[]
        for meta in a.root.glob('rr-input-c1-*/metadata.json'):
            try:
                m=json.loads(meta.read_text(encoding='utf-8-sig'))
                if m.get('scene_capture_build')=='R477':candidates.append((meta.stat().st_mtime,m['pid']))
            except (ValueError,OSError):pass
        if not candidates:raise SystemExit('No R477 scene frames saved. Wait for Saved, or send compact logs if capture failed.')
        a.pid=max(candidates)[1]
    records,issues,files=inspect(a.root,a.pid)
    if not files:raise SystemExit('No files found for that scene session.')
    output=a.output or pathlib.Path.cwd()/('GI-Scene-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    manifest={'schema':'control-gi-scene-r477','pid':a.pid,'complete':not issues,'issues':issues,'frames':[m['frame'] for m in records],
              'limitations':['physical SRV slice mapping not independently verified','individual light operands and full texture mip contents not exported','instance/primitive IDs not exported','RR output precedes final display processing'],'reassembly':'Concatenate each file chunks in listed order; verify chunk and whole-file SHA256.'}
    print('Packaging full-screen data. This may take several minutes.',flush=True)
    package(a.root,files,output,manifest)
    print(('COMPLETE' if not issues else 'INCOMPLETE DIAGNOSTIC')+': '+str(output))
    for issue in issues:print(issue)
    print('Send scene-manifest.zip and EVERY scene-part ZIP. Original captures remain in place.')
if __name__=='__main__':main()
