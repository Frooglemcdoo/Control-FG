"""Signed hit-position bands relative to the selected light center, in native world axes."""
import argparse
import base64
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path
from rt_spot_blocker import BASE_SHA, llvm_float

AXES = 'xyz'

def hit_position(source, axis):
    if axis not in AXES:
        raise ValueError('Invalid axis')
    before = '  %253 = fmul fast float %SpotVisibleSum, 2.500000e-01'
    # Centered ray endpoint is the light center. This avoids subtracting two large world coordinates.
    after = '\n'.join([
        '  %HitDeltaT = fsub fast float %251, %SpotSegment0Length',
        f'  %HitAxisDelta = fmul fast float %HitDeltaT, %SpotSegment0{axis}Direction',
        '  %HitRadiusValid = fcmp ord float %174, %174',
        '  %HitRadius = select i1 %HitRadiusValid, float %174, float 0.000000e+00',
        '  %HitPositiveRadius = call float @dx.op.binary.f32(i32 35, float %HitRadius, float 0.000000e+00)',
        f'  %HitTolerance = fmul fast float %HitPositiveRadius, {llvm_float(.1)}',
        '  %HitNegativeTolerance = fsub fast float 0.000000e+00, %HitTolerance',
        '  %HitBelow = fcmp fast olt float %HitAxisDelta, %HitNegativeTolerance',
        '  %HitAbove = fcmp fast ogt float %HitAxisDelta, %HitTolerance',
        '  %HitLowerBand = select i1 %HitBelow, float 0.000000e+00, float 5.000000e-01',
        '  %HitAxisBand = select i1 %HitAbove, float 7.500000e-01, float %HitLowerBand',
        '  %253 = select i1 %SpotOccluded0, float %HitAxisBand, float 1.000000e+00'])
    if source.count(before) != 1:
        raise ValueError('Centered visibility anchor changed')
    return source.replace(before, after), (before, after)

def prepare(root):
    folder = root/'assets/rt-spot-hit-position'
    manifest = json.loads((folder/'manifest.json').read_text())
    out = root/'build';out.mkdir(exist_ok=True)
    header = '#pragma once\n'
    for axis in AXES:
        name = f'rt_spot_hit_{axis}.dxil'
        data = base64.b64decode((folder/(name+'.b64')).read_text())
        assert hashlib.sha256(data).hexdigest() == manifest[axis]['sha256']
        (out/name).write_bytes(data)
        header += f'static const unsigned char kSpotHit{axis.upper()}Shader[]={{\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n'
    (out/'rt_spot_hit_position.h').write_text(header)
    print('PASS hit-position shader hashes and embedded arrays')

def generate(root,dxc,assembler,dxv):
    data = base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest() == BASE_SHA
    folder = root/'assets/rt-spot-hit-position';folder.mkdir(exist_ok=True)
    manifest = {'baseline_sha256':BASE_SHA,'tolerance_radius_fraction':.1}
    with tempfile.TemporaryDirectory() as tmp:
        tmp=Path(tmp);(tmp/'base.dxil').write_bytes(data)
        source=subprocess.check_output([dxc,'-dumpbin',str(tmp/'base.dxil')],text=True)
        for axis in AXES:
            patched,(before,after)=hit_position(source,axis)
            assert patched.replace(after,before)==source
            ll=tmp/'hit.ll';ll.write_text(patched)
            subprocess.run([assembler,str(ll),str(tmp/'unsigned.dxil')],check=True)
            subprocess.run([dxv,str(tmp/'unsigned.dxil'),'-o='+str(tmp/'signed.dxil')],check=True)
            result=(tmp/'signed.dxil').read_bytes()
            (folder/f'rt_spot_hit_{axis}.dxil.b64').write_text(base64.b64encode(result).decode()+'\n')
            manifest[axis]={'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result)}
    (folder/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate',action='store_true')
    for key in ('dxc','assembler','dxv'):parser.add_argument('--'+key)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    if args.generate:
        if not all((args.dxc,args.assembler,args.dxv)):parser.error('DXC, assembler and DXV required')
        generate(root,args.dxc,args.assembler,args.dxv)
    prepare(root)
