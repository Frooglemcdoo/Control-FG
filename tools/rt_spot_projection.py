"""Optional projection-origin rays; retain centered offsets and authored clipping."""
import argparse,base64,hashlib,json,subprocess
from pathlib import Path
from rt_spot_authored_clip import authored_clip
from rt_spot_endpoint import BASE_SHA

def projection_origin(source):
    edits=[]
    # Decode the per-light cone apex with the native inverse-view transform.
    lines=[]
    for axis,a,b,c,t in [('x',176,180,184,188),('y',177,181,185,189),('z',178,182,186,190)]:
        lines += [f'  %Projection{axis}0 = fmul float %167, %{a}',
                  f'  %Projection{axis}1 = fmul float %168, %{b}',
                  f'  %Projection{axis}2 = fmul float %169, %{c}',
                  f'  %Projection{axis}01 = fadd float %Projection{axis}0, %Projection{axis}1',
                  f'  %Projection{axis}012 = fadd float %Projection{axis}01, %Projection{axis}2',
                  f'  %Projection{axis} = fadd float %Projection{axis}012, %{t}']
    anchor='  %SpotSegment0xCenter = fsub fast float %192, %.i0275'
    if source.count(anchor)!=1:raise ValueError('Unknown centered-ray prefix')
    setup='\n'.join(lines)+'\n'
    source=source.replace(anchor,setup+anchor);edits.append((anchor,setup+anchor))
    for ray in range(4):
        for axis,center,origin in [('x',192,'%.i0275'),('y',194,'%.i1276'),('z',196,'%.i2277')]:
            before=f'  %SpotSegment{ray}{axis}Center = fsub fast float %{center}, {origin}'
            after=f'  %SpotSegment{ray}{axis}Center = fsub fast float %Projection{axis}, {origin}'
            if source.count(before)!=1:raise ValueError('Centered-ray anchor changed')
            source=source.replace(before,after);edits.append((before,after))
    return source,edits

def prepare(root):
    folder=root/'assets/rt-spot-projection';m=json.loads((folder/'manifest.json').read_text())
    data=base64.b64decode((folder/'rt_spot_projection.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==m['sha256'] and len(data)==m['bytes']
    out=root/'build';out.mkdir(exist_ok=True)
    (out/'rt_spot_projection.dxil').write_bytes(data)
    (out/'rt_spot_projection.h').write_text('#pragma once\nstatic const unsigned char kSpotProjectionShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--source');p.add_argument('--assembler');p.add_argument('--dxv');a=p.parse_args()
    root=Path(__file__).resolve().parents[1]
    if a.source:
        baseline=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
        assert hashlib.sha256(baseline).hexdigest()==BASE_SHA
        source=Path(a.source).read_text();patched,edits=projection_origin(source)
        # Reverse in opposite order, including the inserted transform prefix.
        restored=patched
        for before,after in reversed(edits):restored=restored.replace(after,before)
        assert restored==source
        patched,_=authored_clip(patched)
        build=root/'build';build.mkdir(exist_ok=True);ll=build/'projection.ll';ll.write_text(patched)
        unsigned=build/'projection-unsigned.dxil';signed=build/'projection-signed.dxil'
        subprocess.run([a.assembler,str(ll),str(unsigned)],check=True)
        subprocess.run([a.dxv,str(unsigned),'-o='+str(signed)],check=True)
        data=signed.read_bytes();folder=root/'assets/rt-spot-projection';folder.mkdir(exist_ok=True)
        (folder/'rt_spot_projection.dxil.b64').write_text(base64.b64encode(data).decode()+'\n')
        (folder/'manifest.json').write_text(json.dumps(dict(baseline_sha256=BASE_SHA,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),policy='cone_projection_origin_with_authored_clip',extra_rays=0),indent=2)+'\n')
    prepare(root)
