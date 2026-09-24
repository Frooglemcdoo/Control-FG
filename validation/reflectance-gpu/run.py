from pathlib import Path
import ctypes as ct
import hashlib,json,os,subprocess,sys,tempfile,zipfile
import numpy as np
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
results={};env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}
with tempfile.TemporaryDirectory() as temp:
    temp=Path(temp)
    for mode,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
        for name in ('projection-test','compare-test','export-test'):
            binary=temp/(name+'-'+mode);out=temp/(name+'-output-'+mode);out.mkdir()
            subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,'-I',str(root/'validation/part1/shim'),str(here/(name+'.cpp')),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True,capture_output=True,text=True,env={**env,'CONTROL_EXPORT_TEST_ROOT':str(out)})
            if name=='export-test':
                dirs=list((out/'ControlFGProbe').iterdir());assert len(dirs)==2
                done=[d for d in dirs if (d/'metadata.json').exists()];assert len(done)==1
                d=done[0];meta=json.loads((d/'metadata.json').read_text());assert meta['gpu_reflectance_candidates'] and not meta['reflectance_rr_validated']
                for kind,suffix,start,end in [('diffuse','rgba16f',0,16),('specular','rgba32f',16,48)]:
                    data=(d/f'{kind}-reflectance-candidate.{suffix}').read_bytes();assert data==bytes(range(start,end))
                    h=14695981039346656037
                    for b in data:h=((h^b)*1099511628211)&((1<<64)-1)
                    assert meta[f'reflectance_{kind}_fnv1a64']==f'{h:016x}'
                    assert meta[f'reflectance_{kind}_bytes']==len(data)
            results[name+'-'+mode]='PASS'
    archive=Path(sys.argv[1])
    with zipfile.ZipFile(archive) as z:
        prefix=next(n[:-13] for n in z.namelist() if n.endswith('metadata.json'))
        meta=json.loads(z.read(prefix+'metadata.json'));w,h=meta['width'],meta['height'];n=w*h
        raw={name:z.read(prefix+name) for name in ('gbuffer1.rgba8','gbuffer2.rgba8','native-albedo-target0.rgba16f','material-part1.bin')}
    g1=np.frombuffer(raw['gbuffer1.rgba8'],dtype=np.uint8).reshape(n,4)
    g2=np.frombuffer(raw['gbuffer2.rgba8'],dtype=np.uint8).reshape(n,4)
    albedo=np.frombuffer(raw['native-albedo-target0.rgba16f'],dtype=np.uint8)
    table=np.frombuffer(raw['material-part1.bin'],dtype=np.uint8)
    lib=temp/'reference.so';subprocess.run(['g++','-std=c++17','-O2','-shared','-fPIC',str(here/'capture-reference.cpp'),'-o',str(lib)],check=True)
    api=ct.CDLL(str(lib));api.reference.argtypes=[ct.c_void_p]*4+[ct.c_size_t,ct.c_uint,ct.c_uint,ct.c_float,ct.c_float,ct.c_void_p,ct.c_void_p]
    out=np.empty(n*24,dtype=np.uint8);cosines=np.empty(n,dtype=np.float32)
    sx,sy=(np.float32(meta['camera']['view_to_clip'][k]) for k in (0,5))
    assert api.reference(g1.ctypes.data,g2.ctypes.data,albedo.ctypes.data,table.ctypes.data,table.nbytes,w,h,sx,sy,out.ctypes.data,cosines.ctypes.data)==0
    nx=(g1[:,0].astype(np.uint32)<<3)|((g1[:,3]>>1)&7);ny=(g1[:,1].astype(np.uint32)<<4)|(g1[:,3]>>4)
    qx=1.3*(2*nx/2047.0-1);qy=1.3*(2*ny/4095.0-1);q2=qx*qx+qy*qy
    normal=np.stack([2*qx,2*qy,q2-1],axis=1)/(q2+1)[:,None];normal/=np.linalg.norm(normal,axis=1)[:,None]
    ix=np.arange(n)%w;iy=np.arange(n)//w
    ray=np.stack([(2*(ix+.5)/w-1)/sx,(1-2*(iy+.5)/h)/sy,np.ones(n)],axis=1);ray/=np.linalg.norm(ray,axis=1)[:,None]
    independent=-np.sum(normal*ray,axis=1);error=float(np.max(np.abs(independent-cosines)))
    assert error<1e-6
    actual=out[:n*8].reshape(n,8);original=albedo.reshape(n,8)
    assert np.array_equal(actual[:,:6],original[:,:6]) and np.all(actual[:,6]==0) and np.all(actual[:,7]==0x3c)
    results['r5_full_capture_cpu_reference_and_comparator']='PASS'
    results['r5_projection_double_reference']='PASS'
sourcePaths=[p for p in (root/'src').glob('rr_reflectance*') if p.is_file()]+list(here.glob('*.cpp'))+[Path(__file__).resolve(),root/'src/shaders/rr_reflectance_capture.hlsl',root/'src/shaders/rr_envbrdf_generated.hlsli',root/'src/shaders/ControlPrimaryGuideDecode.hlsli',root/'tools/compile-reflectance.cpp',root/'Collect-ControlFG-Logs.ps1',root/'src/rr_guide_export.h',root/'src/rr_guide_render.h']
report={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'r5_frame':meta['engine_frame'],'r5_pixels':n,'max_double_projection_cosine_error':error,
    'view_convention':'unjittered symmetric perspective pixel centers; native jitter/coverage not validated',
    'shader_compile':'NOT_RUN_LOCAL; mandatory Build.cmd compilation/reflection gate','gpu_execution':'NOT_RUN','rr_evaluation':False,
    'input_sha256':{name:hashlib.sha256(b).hexdigest() for name,b in raw.items()},
    'tested_sha256':{p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sourcePaths}}
(here/'validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
