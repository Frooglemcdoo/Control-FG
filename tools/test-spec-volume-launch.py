"""Execute the launcher's actual selection block under Windows CMD."""
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
source=(root/'Start-Spec-Volume-Test.cmd').read_text()
start=source.index('rem Branch on CHOICE immediately:')
block=source[start:source.index('echo Starting Control',start)]
assert source[source.index('choice /C 12'):start].count('\n')==1
if os.name!='nt':
    raise SystemExit('Windows CMD is required; no emulated success')
with tempfile.TemporaryDirectory() as folder:
    for choice, expected in [(1,'on'),(2,'off'),(3,None),(0,None),(255,None)]:
        path=Path(folder)/'test.cmd'
        # A stale inherited selection must also be replaced or cleared.
        script='@echo off\nsetlocal\nset "CONTROLFG_SPEC_VOLUME_ROUGHNESS=stale"\n'
        script+=f'cmd /c exit {choice}\n'+block
        script+='echo RESULT=[%CONTROLFG_SPEC_VOLUME_ROUGHNESS%]\nexit /b 0\n'
        path.write_text(script)
        result=subprocess.run(['cmd','/d','/c',str(path)],text=True,capture_output=True)
        if expected is None:
            assert result.returncode!=0 and 'RESULT=' not in result.stdout, result
        else:
            assert result.returncode==0 and f'RESULT=[{expected}]' in result.stdout,result
        print(f'PASS actual CMD choice={choice} expected={expected!r}')
    # Reproduce the old ordering to establish whether SET loses CHOICE's code
    # on this Windows runtime, instead of merely assuming it from the upload.
    path.write_text('@echo off\nsetlocal\nset "CONTROLFG_SPEC_VOLUME_ROUGHNESS=stale"\ncmd /c exit 2\nset "CONTROLFG_SPEC_VOLUME_ROUGHNESS="\necho OLD_ERRORLEVEL=%errorlevel%\n')
    result=subprocess.run(['cmd','/d','/c',str(path)],text=True,capture_output=True,check=True)
    print(result.stdout.strip())
