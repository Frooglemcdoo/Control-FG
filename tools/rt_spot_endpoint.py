"""Bounded emitter-end exclusion for the exact R97 centered spotlight shader.

Only four segment endpoints change. The receiver origin, direction, hit handlers,
closest-hit search, point branch and filter inputs remain unchanged.
"""
import argparse, base64, hashlib, json, subprocess, tempfile
from pathlib import Path

BASE_SHA='26688736d38b5ea056ae6bb37dc1a738334d94c784003a67957f3677f789a246'
CAP_FRACTION=0.1

def endpoint_guard(source):
    replacements=[]
    for i in range(4):
        p=f'SpotSegment{i}'
        before=f'  %{p}Max = fmul fast float %{p}Length, 0x3FEFFFFE00000000'
        after='\n'.join([
            f'  %{p}OriginalMax = fmul fast float %{p}Length, 0x3FEFFFFE00000000',
            f'  %{p}RadiusValid = fcmp ord float %174, %174',
            f'  %{p}Radius = select i1 %{p}RadiusValid, float %174, float 0.000000e+00',
            f'  %{p}PositiveRadius = call float @dx.op.binary.f32(i32 35, float %{p}Radius, float 0.000000e+00)',
            f'  %{p}Cap = fmul fast float %{p}Length, 0x3FB99999A0000000',
            f'  %{p}Guard = call float @dx.op.binary.f32(i32 36, float %{p}PositiveRadius, float %{p}Cap)',
            f'  %{p}Max = fsub fast float %{p}OriginalMax, %{p}Guard'])
        if source.count(before)!=1:raise ValueError('Centered endpoint anchor changed')
        source=source.replace(before,after);replacements.append((before,after))
    return source,replacements

def prepare(root):
    folder=root/'assets/rt-spot-endpoint'
    manifest=json.loads((folder/'manifest.json').read_text())
    data=base64.b64decode((folder/'rt_spot_endpoint.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    out=root/'build';out.mkdir(exist_ok=True)
    (out/'rt_spot_endpoint.dxil').write_bytes(data)
    (out/'rt_spot_endpoint.h').write_text('#pragma once\nstatic const unsigned char kSpotEndpointShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')
    print('PASS endpoint shader hash and embedded header: '+manifest['sha256'])

def generate(root,dxc,assembler,dxv):
    data=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==BASE_SHA
    with tempfile.TemporaryDirectory() as directory:
        directory=Path(directory)
        base=directory/'base.dxil';base.write_bytes(data)
        source=subprocess.check_output([dxc,'-dumpbin',str(base)],text=True)
        patched,edits=endpoint_guard(source)
        restored=patched
        for before,after in edits:restored=restored.replace(after,before)
        assert restored==source
        ll=directory/'endpoint.ll';ll.write_text(patched)
        unsigned=directory/'unsigned.dxil';signed=directory/'signed.dxil'
        subprocess.run([assembler,str(ll),str(unsigned)],check=True)
        subprocess.run([dxv,str(unsigned),'-o='+str(signed)],check=True)
        result=signed.read_bytes()
    folder=root/'assets/rt-spot-endpoint';folder.mkdir(exist_ok=True)
    (folder/'rt_spot_endpoint.dxil.b64').write_text(base64.b64encode(result).decode()+'\n')
    (folder/'manifest.json').write_text(json.dumps({'baseline_sha256':BASE_SHA,'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result),'radius_scale':1,'maximum_segment_fraction':CAP_FRACTION,'changed':'Four spotlight TMax equations only; existing payload initialization and hit comparison share the new endpoint.'},indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate',action='store_true')
    for name in ('dxc','assembler','dxv'):parser.add_argument('--'+name)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    if args.generate:
        if not all((args.dxc,args.assembler,args.dxv)):parser.error('Generation needs DXC, assembler and DXV')
        generate(root,args.dxc,args.assembler,args.dxv)
    prepare(root)

