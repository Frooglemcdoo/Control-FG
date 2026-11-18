"""Create two launch-configured packages from the same verified R465 binary."""
import argparse, copy, hashlib, pathlib, zipfile
p=argparse.ArgumentParser();p.add_argument('base');p.add_argument('output');a=p.parse_args()
out=pathlib.Path(a.output);out.mkdir(parents=True,exist_ok=True)
with zipfile.ZipFile(a.base) as source:
    assert source.testzip() is None
    dll=source.read('dxgi.dll');assert b'GI_AUTO_R465' in dll
    for rays in (10,4):
        name=f'ControlTOH-GI-Auto-R465-{rays}Rays.zip';path=out/name
        launcher=f'Start-GI-Auto-R465-{rays}Rays.cmd'
        with zipfile.ZipFile(path,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as target:
            for entry in source.infolist():
                if 'Start-GI-Auto-R465-' in entry.filename and not entry.filename.endswith(launcher):continue
                data=source.read(entry.filename)
                if entry.filename.endswith(launcher):assert f'CONTROLTOH_GI_AUTO_RAYS={rays}'.encode() in data
                if entry.filename.replace('\\','/').endswith('ControlTOH/README.txt'):
                    data=(f'THIS PACKAGE: {rays} GI RAYS\r\nRun ControlTOH\\Diagnostics\\{launcher}\r\n\r\n').encode()+data
                target.writestr(copy.copy(entry),data)
        with zipfile.ZipFile(path) as check:
            assert check.testzip() is None and check.read('dxgi.dll')==dll
            launchers=[n for n in check.namelist() if 'Start-GI-Auto-R465-' in n]
            assert len(launchers)==1 and launchers[0].endswith(launcher)
        print(name,path.stat().st_size,hashlib.sha256(path.read_bytes()).hexdigest())
