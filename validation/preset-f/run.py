from pathlib import Path
import hashlib,json,os,subprocess,sys,tempfile
root=Path(__file__).resolve().parents[2];here=Path(__file__).resolve().parent
sys.path.insert(0,str(root/'validation/native-history'))
from pe_tools import PE
p=PE(sys.argv[1]);assert hashlib.sha256(p.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
assert p.exports['NVSDK_NGX_D3D12_CreateFeature']==0x528d0
assert p.exports['NVSDK_NGX_Parameter_SetUI']==0x51f50
assert p.exports['NVSDK_NGX_Parameter_GetUI']==0x51c70
assert p.data[p.off(0x1d457):p.off(0x1d457)+5]==bytes.fromhex('e974540300')
results={}
with tempfile.TemporaryDirectory() as tmp:
 for name,flags in [('normal',['-O2']),('asan_ubsan',['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie'])]:
  exe=Path(tmp)/name
  subprocess.run(['g++','-std=c++17','-Wall','-Wextra','-Werror',*flags,str(here/'test.cpp'),'-o',str(exe)],check=True)
  results[name]=subprocess.check_output([str(exe)],text=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0:halt_on_error=1','UBSAN_OPTIONS':'halt_on_error=1'}).strip()
paths=['src/rr_preset_f.h','src/rr_native_preset.h','Verify-RRRuntime.ps1','validation/preset-f/test.cpp']
(here/'results.json').write_text(json.dumps({'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','results':results,'native_tail':'1D457 -> 528D0 verified against exact DLL','rr_dll_version':'310.9.1','rr_dll_sha256':'4BC7EA5FCB2F32CF86BC2CB072E8D2860914A0BE511CBDCB2FC206C1D9D83B80','runtime_dll_provenance':'Downloaded official NVIDIA/DLSS v310.9.1 x86_64 rel DLL hash is identical to r19b staged DLL','official_release':'https://github.com/NVIDIA/DLSS/releases/tag/v310.9.1','preset_requested':'Selectable E/F/K/L/M (5/6/11/12/13), all six quality modes, before native creation; F default','windows_gpu_execution':False,'sha256':{x:hashlib.sha256((root/x).read_bytes()).hexdigest() for x in paths}},indent=2)+'\n')
print('PASS selectable RR preset hook tests and native export/tail audit')
