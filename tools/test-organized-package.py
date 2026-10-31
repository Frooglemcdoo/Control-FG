"""Validate the shipped paths and execute reversible upgrade cleanup on Windows."""
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
package = Path(sys.argv[1]).resolve()
assert {p.name for p in package.iterdir() if p.is_file()} == {'dxgi.dll', 'dstorage.dll', 'dstoragecore.dll'}
shader_names = set()
for header in ('src/rt_spot_runtime.h', 'src/pt_glass_runtime.h'):
    shader_names.update(re.findall(r'PTReflectionLoadSidecar\(L"([^"]+)"', (root/header).read_text()))
assert len(shader_names) == 17, shader_names
for name in shader_names:
    path = package/'ControlTOH/Shaders'/name
    assert path.is_file() and path.stat().st_size > 32, path
for relative in ('Tools/ControlFG-Restart.exe', 'Tools/Collect-ControlFG-Compact-Logs.ps1',
                 'Tools/Collect-Texture-Trace.ps1', 'Diagnostics/Capture-RR-Suite.ps1',
                 'Diagnostics/Capture-RR-Boundary.ps1', 'Diagnostics/Package-Lighting-Performance.ps1',
                 'Diagnostics/DirectStorage/Validate-DirectStorage.cmd'):
    assert (package/'ControlTOH'/relative).is_file(), relative
print('PASS package: 17 runtime shaders, helper and diagnostic paths; only three root DLLs')
if os.name != 'nt':
    raise SystemExit('Windows is required for migration integration test')
with tempfile.TemporaryDirectory(prefix='ControlTOH install with spaces ') as folder:
    game = Path(folder)/'Game with spaces'
    shutil.copytree(package, game)
    protected = {'Control_DX12.exe': b'game', 'other-mod.dll': b'foreign',
                 'README.txt': b'foreign readme', 'custom.dxil': b'foreign shader'}
    for name, data in protected.items(): (game/name).write_bytes(data)
    for name in ('dxgi.dll', 'dstorage.dll', 'dstoragecore.dll'):
        protected[name] = (game/name).read_bytes()
    old = {'rt_spot_full.dxil': b'old shader', 'ControlFG-Restart.exe': b'old helper',
           'Collect-Texture-Trace.ps1': b'old collector',
           'Start-R256-Storage-Diagnostics.cmd': b'old launcher',
           'DirectStorage/dstorage.dll': b'old validator dependency'}
    for name, data in old.items():
        path = game/name; path.parent.mkdir(exist_ok=True); path.write_bytes(data)
    script = game/'ControlTOH/Tools/Organize-Existing-Install.ps1'
    # No GameDirectory override: exercise installed relative path resolution.
    for attempt in range(2):
        subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', str(script)], check=True)
        for name, data in protected.items(): assert (game/name).read_bytes() == data, name
        backups = list((game/'ControlTOH/LegacyBackup').iterdir())
        assert len(backups) == 1
        for name, data in old.items():
            assert not (game/name).exists(), name
            assert (backups[0]/name).read_bytes() == data, name
        assert (game/'ControlTOH/Shaders/rt_spot_full.dxil').read_bytes() == (package/'ControlTOH/Shaders/rt_spot_full.dxil').read_bytes()
print('PASS migration: spaces, default game path, backup integrity, repeat execution, game and foreign files preserved')
