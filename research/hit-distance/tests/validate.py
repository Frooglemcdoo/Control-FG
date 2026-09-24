"""Host validation: g++, ASan/UBSan, DXC compilation, native-writer SSA audit."""
import pathlib,subprocess,json,os,sys,hashlib
root=pathlib.Path(__file__).resolve().parents[1];out=root/'evidence';out.mkdir(exist_ok=True)
dxc=pathlib.Path(sys.argv[1]).resolve()
commands=[]
def run(args,name,env=None):
 cp=subprocess.run([str(a) for a in args],capture_output=True,text=True,env=env)
 (out/(name+'.txt')).write_text(cp.stdout+cp.stderr)
 if cp.returncode:raise RuntimeError(name+' failed: '+cp.stdout+cp.stderr)
 commands.append({'name':name,'returncode':cp.returncode});print(name,'PASS')
source=root/'tests/test_distance.cpp'
run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror',source,'-o',out/'distance-host'],'host-compile')
run([out/'distance-host'],'host-results')
run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer',source,'-o',out/'distance-sanitized'],'sanitizer-compile')
env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0')
run([out/'distance-sanitized'],'sanitizer-results',env)
run([dxc,'-T','cs_6_0','-E','main','-Ges','-WX','-Fo',out/'hit-distance.dxil','-Fc',out/'hit-distance.ll',root/'src/rr_hit_distance.hlsl'],'hlsl-compile')
run([sys.executable,root/'tests/audit_writer.py',root/'evidence/original-shaders',dxc,out/'native-writer'],'native-writer-results')
(out/'validation.json').write_text(json.dumps({'commands':commands,'source_sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted((root/'src').glob('*'))},'runtime_wired':False,'gpu_executed':False,'rr_enabled':False,'scope':'host numerical/classification tests, HLSL compilation, static native DXIL SSA interpretation'},indent=2)+'\n')
