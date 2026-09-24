"""Verify release ZIP identities and pinned payload against the actual build."""
from pathlib import Path
import hashlib
import json
import zipfile

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'target-manifest.json').read_text())
version = manifest['ProbeVersion']
validation = json.loads((root / 'build/build-validation.json').read_text(encoding='utf-8-sig'))
sdk = json.loads((root / 'third_party/streamline/sdk-info.json').read_text(encoding='utf-8-sig'))
assert validation['Version'] == version == '2.1.1'
assert validation['AbiCheck'] == validation['ExportCheck'] == 'Passed'
assert validation['Architecture'] == 'x64'
runtime_names = ('sl.interposer.dll', 'sl.common.dll', 'sl.pcl.dll', 'sl.reflex.dll',
                 'sl.dlss_g.dll', 'nvngx_dlssg.dll', 'sl.dlss_d.dll', 'nvngx_dlssd.dll')
expected_dlls = {'dxgi.dll', 'ControlFGStreamline/ControlFG.RTX40MFG.dll'}
expected_dlls.update('ControlFGStreamline/' + name for name in runtime_names)
checks = []

with zipfile.ZipFile(root / f'release/Control-FG-v{version}.zip') as package:
    names = [n.replace('\\', '/') for n in package.namelist() if not n.endswith('/')]
    assert len(names) == len(set(names)), 'Duplicate ZIP entries'
    assert all(not n.startswith('/') and '..' not in n.split('/') for n in names)
    assert {n for n in names if n.lower().endswith('.dll')} == expected_dlls
    assert not any(n.lower().endswith(('.exe', '.pdb', '.obj')) for n in names)
    contents = {n.replace('\\', '/'): package.read(n) for n in package.namelist() if not n.endswith('/')}
    support_collectors = {'Collect-ControlFG-Compact-Logs.cmd', 'Collect-ControlFG-Compact-Logs.ps1'}
    assert {n for n in names if Path(n).name.startswith('Collect-')} == support_collectors, 'Only the compact support collector belongs in the deployment package'
    for name in support_collectors:
        assert contents[name] == (root / name).read_bytes(), name
    checks.append('Only the compact support collector ships; both files match source')
    for path, expected in [('dxgi.dll', validation['SHA256']),
                           ('ControlFGStreamline/ControlFG.RTX40MFG.dll', validation['MFGSidecarSHA256'])]:
        assert hashlib.sha256(contents[path]).hexdigest() == expected.lower(), path
    for name in runtime_names:
        record, = [r for r in sdk['Files'] if r['Path'] == 'bin/' + name]
        assert hashlib.sha256(contents['ControlFGStreamline/' + name]).hexdigest() == record['SHA256'].lower(), name
    checks.append('Proxy, sidecar and all eight pinned runtime DLL hashes match')
    dll = contents['dxgi.dll']
    assert b'PROBE v2.1.1 internal_build=2.1.1' in dll
    assert 'v2.1.1'.encode('utf-16le') in dll
    for target in manifest['SupportedTargets']:
        for file in target['RequiredFiles']:
            assert file['SHA256'].encode('ascii') in dll
    assert {t['Storefront'] for t in manifest['SupportedTargets']} == {'Steam', 'Epic', 'GOG'}
    checks.append('Version labels and all three exact storefront identities are present')
    for name in ('README.md', 'INSTALL.md', 'KNOWN_ISSUES.md', 'TROUBLESHOOTING.md', 'RELEASE_NOTES.md', 'target-manifest.json'):
        assert contents[name] == (root / name).read_bytes(), name
    for name in ('STREAMLINE-LICENSE.txt', 'RENODX-LICENSE.txt', 'RESHade-LICENSE.txt',
                 'THIRD-PARTY-NOTICES.md', 'RTX40MFG-MINIMAL-LICENSE.txt', 'RTX40MFG-UPSTREAM-LICENSE.txt'):
        assert contents['legal/' + name] == (root / 'legal' / name).read_bytes(), name
    checks.append('Deployment documentation, target manifest and legal notices match source')

report = {'status': 'PASS', 'version': version, 'checks': checks,
          'files': len(names), 'gpu_runtime': 'Not run; inherited R3 user validation'}
(root / 'build/deployment-validation.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
