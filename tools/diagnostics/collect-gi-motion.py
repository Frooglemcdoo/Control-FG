"""Validate a complete motion session, reconstruct GI, and build a full-screen review.
Only reads saved game data; never injects keys or alters rendering/settings.
"""
import argparse,base64,hashlib,platform,tempfile,datetime,importlib.util,json,os,pathlib,struct,subprocess,sys,traceback,zlib,zipfile
ACTIVE_PID=None
SPEC=importlib.util.spec_from_file_location('scene',pathlib.Path(__file__).with_name('collect-gi-scene.py'))
scene=importlib.util.module_from_spec(SPEC);SPEC.loader.exec_module(scene)

def png_from_ppm(path):
    with path.open('rb') as f:
        if f.readline()!=b'P6\n':raise ValueError('bad preview header')
        w,h=map(int,f.readline().split())
        if f.readline()!=b'255\n':raise ValueError('bad preview precision')
        compressed=zlib.compressobj(1);parts=[]
        for _ in range(h):
            row=f.read(w*3)
            if len(row)!=w*3:raise ValueError('truncated preview')
            parts.append(compressed.compress(b'\0'+row))
        parts.append(compressed.flush())
    def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
    return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>2I5B',w,h,8,2,0,0,0))+chunk(b'IDAT',b''.join(parts))+chunk(b'IEND',b'')

def review(root,pid,records,folder,verifier):
    reports=[];previews=[]
    for m in records:
        frame=m['frame'];r=root/f'rr-input-c1-{pid}-{frame}';d=root/f'gi-scene-detail-{pid}-{frame}'
        h=json.loads((r/'gi-hit-slices.json').read_text());raw=h['raw_gi']
        print(f'Validating every ray and reconstructed GI: frame {frame}',flush=True)
        cmd=[str(verifier),str(d/'motion-rays.bin'),str(r/h['file']),str(m['width']),str(m['height']),str(h['rays_per_batch']),str(frame),str(raw['offset']),str(raw['row_pitch'])]
        result=subprocess.run(cmd,capture_output=True,text=True)
        if not result.stdout.strip():raise ValueError(f'Frame {frame}: verifier failed: {result.stderr}')
        v=json.loads(result.stdout);reports.append(v)
        (folder/'verification.json').write_text(json.dumps(reports,indent=2))
        if result.returncode:raise ValueError(f'Frame {frame}: data provenance/reconstruction mismatch; see verification.json. Keep capture; do not rerun.')
        rr=next(a for a in m['resources'] if a['file'].startswith('rr-output.'))
        normal=folder/f'{frame}-normal.ppm';bright=folder/f'{frame}-bright.ppm'
        print(f'Creating full-resolution preview: frame {frame}',flush=True)
        subprocess.run([str(verifier),'--preview',str(r/rr['file']),str(rr['dxgi_format']),str(rr['width']),str(rr['height']),str(normal),str(bright)],check=True)
        item={'frame':frame,'width':rr['width'],'height':rr['height'],'tick':m.get('capture_tick_ms'), 'keys':m.get('movement_key_bits'), 'index':m['motion_index']}
        for key,path in [('normal',normal),('bright',bright)]:
            item[key]='data:image/png;base64,'+base64.b64encode(png_from_ppm(path)).decode('ascii');path.unlink()
        previews.append(item)
    duration=records[-1]['capture_tick_ms']-records[0]['capture_tick_ms']
    summary={'schema':'gi-motion-review-r484','pid':pid,'data_complete':True,'visible_speckle_confirmed':False,'movement_to_rest_verified':False,'frame_count':len(records),'duration_ms':duration,'stop_cue_index':4,'frames':[{'frame':m['frame'],'tick':m['capture_tick_ms'],'movement_key_bits':m.get('movement_key_bits')} for m in records],'ray_verification':reports,'limitations':['RR internal history unavailable','capture changes frame timing','lighting terms aggregate lights; individual shadow-map taps not recorded','preview tone mapping is not an exact HDR/SDR screenshot','file completeness does not establish that a visible speckle or movement-to-rest transition was captured']}
    (folder/'summary.json').write_text(json.dumps(summary,indent=2))
    html='''<!doctype html><meta charset="utf-8"><title>R484 motion capture review</title>
<style>body{background:#171a20;color:#eee;font:14px system-ui;margin:16px}button,select,input{margin:8px}#view{overflow:auto;max-height:82vh}img{image-rendering:auto;cursor:crosshair}#status{padding:8px;background:#282c35}</style>
<p>Full-screen consecutive RR output. Preview uses linear tone mapping, not the game's SDR/HDR display. Brightened exposes dark detail and can exaggerate normal highlights. A complete file set does not prove a speckle was captured.</p>
<button id="prev">Previous</button><input id="frame" type="range" min="0" max="15" value="0"><button id="next">Next</button><button id="play">Play / pause</button>
<select id="gain"><option value="normal">Standard</option><option value="bright">Brightened 8×</option></select><select id="zoom"><option value="1">100%</option><option value="0.5">50%</option><option value="2">200%</option></select>
<div id="status"></div><div id="view"><img id="image"></div><script>
const frames=PAYLOAD;let playing=false,timer;const slider=document.getElementById('frame'),img=document.getElementById('image'),status=document.getElementById('status'),gain=document.getElementById('gain'),zoom=document.getElementById('zoom');
function draw(){const f=frames[+slider.value];img.src=f[gain.value];img.style.width=(f.width*+zoom.value)+'px';status.textContent='R484 frame '+f.frame+' | '+(+slider.value+1)+' / '+frames.length+' | '+(f.tick-frames[0].tick)+' ms | movement-key bits '+f.keys+' | '+(f.index>=4?'STOP cue issued':'Move phase');}
slider.oninput=gain.onchange=zoom.onchange=draw;document.getElementById('prev').onclick=()=>{slider.value=Math.max(0,+slider.value-1);draw()};document.getElementById('next').onclick=()=>{slider.value=Math.min(frames.length-1,+slider.value+1);draw()};document.getElementById('play').onclick=()=>{playing=!playing;clearInterval(timer);if(playing)timer=setInterval(()=>{slider.value=(+slider.value+1)%frames.length;draw()},250)};
img.onclick=e=>{const f=frames[+slider.value],r=img.getBoundingClientRect(),x=Math.floor((e.clientX-r.left)*f.width/r.width),y=Math.floor((e.clientY-r.top)*f.height/r.height);status.textContent='R484 frame '+f.frame+' — output pixel ('+x+', '+y+')';};draw();</script>'''.replace('PAYLOAD',json.dumps(previews,separators=(',',':')))
    (folder/'Review.html').write_text(html,encoding='utf-8')
    return summary

def environment(game):
    files=[]
    for name in ('dxgi.dll','nvngx_dlss.dll','ControlFGStreamline/nvngx_dlssd.dll','ControlFGStreamline/sl.dlss_d.dll','ControlFGStreamline/sl.interposer.dll'):
        path=game/name
        if path.is_file():
            digest=hashlib.sha256()
            with path.open('rb') as f:
                for block in iter(lambda:f.read(1024*1024),b''):digest.update(block)
            files.append({'file':name,'bytes':path.stat().st_size,'sha256':digest.hexdigest()})
    driver={'available':False}
    try:
        run=subprocess.run(['nvidia-smi','--query-gpu=name,driver_version','--format=csv,noheader'],capture_output=True,text=True,timeout=15)
        driver={'available':run.returncode==0,'gpu_driver':run.stdout.strip(),'error':run.stderr.strip()}
    except (OSError,subprocess.TimeoutExpired) as exc:driver['error']=str(exc)
    return {'os':platform.platform(),'driver':driver,'runtime_files_on_disk_at_packaging':files,'loaded_runtime_identity':'see capture-session game/Streamline logs','driver_AF_override':'not queried; application sampler descriptor in game log is not proof of driver override state','cache_state':'not altered','rendering_settings':'not altered by packager'}

def main():
    global ACTIVE_PID
    p=argparse.ArgumentParser();p.add_argument('--root',type=pathlib.Path,default=pathlib.Path(os.environ.get('LOCALAPPDATA','.'))/'ControlFGProbe');p.add_argument('--pid',type=int);p.add_argument('--output',type=pathlib.Path);p.add_argument('--review-only',action='store_true');a=p.parse_args()
    if not a.pid:
        candidates=[]
        for f in a.root.glob('rr-input-c1-*/metadata.json'):
            try:
                m=json.loads(f.read_text(encoding='utf-8-sig'))
                if m.get('scene_capture_build')=='R484' and m.get('motion_sequence'):candidates.append((f.stat().st_mtime,m['pid']))
            except (ValueError,OSError):pass
        if not candidates:raise ValueError('No completed R484 motion frame metadata. Send the packager log and compact game logs; do not repeat the run.')
        a.pid=max(candidates)[1]
    ACTIVE_PID=a.pid
    records,issues,files=scene.inspect(a.root,a.pid,motion=True)
    if issues:raise ValueError('Incomplete capture, no success package created:\n'+'\n'.join(issues)+'\nKeep all saved files; send this log before rerunning.')
    folder=a.root/f'gi-motion-review-{a.pid}';folder.mkdir(exist_ok=True)
    verifier=pathlib.Path(__file__).with_name('validate-gi-motion.exe')
    (folder/'environment.json').write_text(json.dumps(environment(pathlib.Path.cwd()),indent=2))
    summary=review(a.root,a.pid,records,folder,verifier)
    review_zip=folder/'Motion-Review.zip'
    with zipfile.ZipFile(review_zip,'w',zipfile.ZIP_DEFLATED,compresslevel=1) as z:
        for name in ('Review.html','summary.json','verification.json','environment.json'):z.write(folder/name,name)
    if hasattr(os,'startfile'):
        try:os.startfile(str(folder))
        except OSError:pass
    print(f'REVIEW READY: {review_zip}\nOpen Review.html to inspect the saved sequence. No new game run required.',flush=True)
    if a.review_only:return
    files+= [folder/name for name in ('summary.json','verification.json','environment.json')]
    output=a.output or pathlib.Path.cwd()/('GI-Motion-'+datetime.datetime.now().strftime('%Y%m%d-%H%M%S'))
    manifest={'schema':'control-gi-motion-r484','pid':a.pid,'complete':True,'frames':[m['frame'] for m in records],'review':summary,'reassembly':'Concatenate chunks in listed order; verify all SHA256 values.'}
    scene.package(a.root,files,output,manifest,part_bytes=400*1024*1024,compresslevel=1,compressed_limit=True)
    print(f'COMPLETE: {output}\nSend Motion-Review.zip first. Full data stays available in the scene-manifest.zip and scene-part ZIPs. Do not delete the originals.',flush=True)
def failure_bundle(log):
    root=pathlib.Path(os.environ.get('LOCALAPPDATA','.'))/'ControlFGProbe'
    output=pathlib.Path(tempfile.gettempdir())/'ControlTOH-GI-Motion-Failure.zip'
    with zipfile.ZipFile(output,'w',zipfile.ZIP_DEFLATED,compresslevel=1) as z:
        if log.exists():z.write(log,log.name)
        logs=sorted(root.glob(f'probe-*-{ACTIVE_PID}.log' if ACTIVE_PID else 'probe-*.log'),key=lambda p:p.stat().st_mtime,reverse=True)[:3]
        for path in logs:
            with path.open('rb') as f:
                f.seek(max(0,path.stat().st_size-2*1024*1024));z.writestr(path.name+'.tail.txt',f.read())
        if ACTIVE_PID:
            for pattern in (f'rr-input-c1-{ACTIVE_PID}-*/metadata.json',f'rr-input-c1-{ACTIVE_PID}-*/gi-hit-slices.json',f'gi-scene-detail-{ACTIVE_PID}-*/metadata.json',f'gi-motion-review-{ACTIVE_PID}/verification.json',f'gi-motion-review-{ACTIVE_PID}/environment.json'):
                for path in root.glob(pattern):z.write(path,path.relative_to(root).as_posix())
        z.writestr('status.json',json.dumps({'complete':False,'pid':ACTIVE_PID,'original_capture_files_preserved':True,'next_step':'Send this failure ZIP; do not repeat gameplay yet.'}))
    print(f'DIAGNOSTICS READY: {output} — send this ZIP; no new game run requested.',flush=True)
    if hasattr(os,'startfile'):
        try:os.startfile(str(output.parent))
        except OSError:pass

class Tee:
    def __init__(self,console,log):self.console,self.log=console,log
    def write(self,text):self.console.write(text);self.log.write(text);self.flush();return len(text)
    def flush(self):self.console.flush();self.log.flush()
if __name__=='__main__':
    log=pathlib.Path(tempfile.gettempdir())/'ControlTOH-GI-Motion-Packager.log'
    with log.open('w',encoding='utf-8') as handle:
        sys.stdout=Tee(sys.stdout,handle);sys.stderr=Tee(sys.stderr,handle)
        try:main()
        except Exception:
            traceback.print_exc()
            try:failure_bundle(log)
            except Exception:traceback.print_exc()
            sys.exit(1)
        finally:
            sys.stdout=sys.stdout.console;sys.stderr=sys.stderr.console
