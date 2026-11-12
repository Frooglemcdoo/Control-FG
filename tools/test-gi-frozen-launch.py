"""Execute the shipped CMD with a fake game; verify child env and parent isolation."""
import os
from pathlib import Path
import subprocess
import shutil
import tempfile

root = Path(__file__).resolve().parents[1]
if os.name != 'nt':
    raise SystemExit('Windows CMD is required')
source = r'''
#include <cstdlib>
#include <cstring>
#include <fstream>
int main() {
 const auto equal=[](const char* key,const char* expected){
  const auto value=std::getenv(key);return value&&std::strcmp(value,expected)==0;
 };
 const bool ok=equal("CONTROLTOH_GI_SINGLE","1")&&equal("CONTROLTOH_GI_COMPACT","0")&&
  equal("CONTROLTOH_GI_FROZEN_PREPARE","1")&&equal("CONTROLTOH_GI_FROZEN_RENDER","1")&&
  equal("CONTROLFG_NATIVE_CPU_TIMING","0")&&!std::getenv("CONTROLTOH_GI_METADATA")&&
  !std::getenv("CONTROLTOH_GI_SCENE_CHECK")&&!std::getenv("CONTROLTOH_GI_EVALUATOR")&&
  equal("CONTROLTOH_OTHER_OPTION","preserved");
 std::ofstream("launch-result.txt")<<(ok?"PASS":"FAIL");return ok?0:1;
}
'''
with tempfile.TemporaryDirectory(prefix='R430 launch with spaces ') as temporary:
    game = Path(temporary)
    diagnostics = game/'ControlTOH/Diagnostics'
    diagnostics.mkdir(parents=True)
    launcher = diagnostics/'Start-GI-Frozen-R430.cmd'
    launcher.write_bytes((root/'tools/diagnostics'/launcher.name).read_bytes())
    (game/'fake.cpp').write_text(source)
    compile_args=['cl','/nologo','/std:c++17','/EHsc','/W4','/WX','/D_CRT_SECURE_NO_WARNINGS',
                  'fake.cpp','/Fe:Control_DX12.exe']
    if shutil.which('cl'):
        subprocess.run(compile_args,cwd=game,check=True,timeout=60)
    else:
        # The package PowerShell step inherits VSROOT, not vcvars' PATH/LIB.
        vcvars=Path(os.environ['VSROOT'])/'VC/Auxiliary/Build/vcvars64.bat'
        assert vcvars.is_file(),vcvars
        compile_script=game/'compile.cmd'
        compile_script.write_text('@echo off\ncall "'+str(vcvars)+'"\n'
                                  'if errorlevel 1 exit /b 1\n'+
                                  subprocess.list2cmdline(compile_args)+'\nexit /b %errorlevel%\n')
        subprocess.run(['cmd.exe','/d','/c',str(compile_script)],cwd=game,check=True,timeout=60)
    inherited = os.environ.copy()
    inherited.update(CONTROLTOH_GI_METADATA='1',CONTROLTOH_GI_SCENE_CHECK='1',
                     CONTROLTOH_GI_EVALUATOR='1',CONTROLTOH_GI_FROZEN_RENDER='0',
                     CONTROLTOH_OTHER_OPTION='preserved')
    wrapper=game/'test.cmd'
    wrapper.write_text('@echo off\ncall "'+str(launcher)+'"\n'
                       'if not "%CONTROLTOH_GI_METADATA%"=="1" exit /b 2\n'
                       'if not "%CONTROLTOH_GI_FROZEN_RENDER%"=="0" exit /b 3\n'
                       'exit /b 0\n')
    subprocess.run(['cmd.exe','/d','/c',str(wrapper)],cwd=game,env=inherited,check=True,timeout=45)
    assert (game/'launch-result.txt').read_text()=='PASS'
print('PASS R430 actual CMD: spaces, guided flags, stale diagnostics cleared, unrelated and parent env preserved')
