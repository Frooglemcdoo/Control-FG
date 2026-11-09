"""Blocker distance bands for the exact R97 centered spotlight shader.

Only the visibility output changes. Full-length rays and native hit handling stay intact.
"""
import argparse, base64, hashlib, json, subprocess, tempfile, struct
from pathlib import Path

BASE_SHA='26688736d38b5ea056ae6bb37dc1a738334d94c784003a67957f3677f789a246'

def llvm_float(value):
    """LLVM spells an f32 constant as the hex bits of its exact f64 promotion."""
    rounded=struct.unpack("f",struct.pack("f",value))[0]
    return "0x"+struct.pack(">d",rounded).hex().upper()

def blocker_bands(source):
    before='  %253 = fmul fast float %SpotVisibleSum, 2.500000e-01'
    after='\n'.join([
        '  %DebugRadiusValid = fcmp ord float %174, %174',
        '  %DebugRadius = select i1 %DebugRadiusValid, float %174, float 0.000000e+00',
        '  %DebugPositiveRadius = call float @dx.op.binary.f32(i32 35, float %DebugRadius, float 0.000000e+00)',
        f'  %DebugWideCap = fmul fast float %SpotSegment0Length, {llvm_float(0.1)}',
        '  %DebugWideGuard = call float @dx.op.binary.f32(i32 36, float %DebugPositiveRadius, float %DebugWideCap)',
        f'  %DebugTightRadius = fmul fast float %DebugPositiveRadius, {llvm_float(0.1)}',
        f'  %DebugTightCap = fmul fast float %SpotSegment0Length, {llvm_float(0.01)}',
        '  %DebugTightGuard = call float @dx.op.binary.f32(i32 36, float %DebugTightRadius, float %DebugTightCap)',
        '  %DebugWideEnd = fsub fast float %SpotSegment0Max, %DebugWideGuard',
        '  %DebugTightEnd = fsub fast float %SpotSegment0Max, %DebugTightGuard',
        '  %DebugInsideWide = fcmp fast oge float %251, %DebugWideEnd',
        '  %DebugInsideTight = fcmp fast oge float %251, %DebugTightEnd',
        '  %DebugWideBand = select i1 %DebugInsideWide, float 5.000000e-01, float 0.000000e+00',
        '  %DebugTightBand = select i1 %DebugInsideTight, float 7.500000e-01, float %DebugWideBand',
        '  %253 = select i1 %SpotOccluded0, float %DebugTightBand, float 1.000000e+00'])
    if source.count(before)!=1:raise ValueError('Centered visibility anchor changed')
    return source.replace(before,after),[(before,after)]

def prepare(root):
    folder=root/'assets/rt-spot-blocker'
    manifest=json.loads((folder/'manifest.json').read_text())
    data=base64.b64decode((folder/'rt_spot_blocker.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    out=root/'build';out.mkdir(exist_ok=True)
    (out/'rt_spot_blocker.dxil').write_bytes(data)
    (out/'rt_spot_blocker.h').write_text('#pragma once\nstatic const unsigned char kSpotBlockerShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')
    print('PASS blocker shader hash and embedded header: '+manifest['sha256'])

def generate(root,dxc,assembler,dxv):
    data=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==BASE_SHA
    with tempfile.TemporaryDirectory() as directory:
        directory=Path(directory)
        base=directory/'base.dxil';base.write_bytes(data)
        source=subprocess.check_output([dxc,'-dumpbin',str(base)],text=True)
        patched,edits=blocker_bands(source)
        restored=patched
        for before,after in edits:restored=restored.replace(after,before)
        assert restored==source
        ll=directory/'blocker.ll';ll.write_text(patched)
        unsigned=directory/'unsigned.dxil';signed=directory/'signed.dxil'
        subprocess.run([assembler,str(ll),str(unsigned)],check=True)
        subprocess.run([dxv,str(unsigned),'-o='+str(signed)],check=True)
        result=signed.read_bytes()
    folder=root/'assets/rt-spot-blocker';folder.mkdir(exist_ok=True)
    (folder/'rt_spot_blocker.dxil.b64').write_text(base64.b64encode(result).decode()+'\n')
    (folder/'manifest.json').write_text(json.dumps({'baseline_sha256':BASE_SHA,'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result),'bands':{'clear':1,'inside_r114':0.75,'inside_r113_only':0.5,'earlier_blocker':0},'changed':'Only spotlight visibility output; full-length centered rays and blocker distances unchanged.'},indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate',action='store_true')
    for name in ('dxc','assembler','dxv'):parser.add_argument('--'+name)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    if args.generate:
        if not all((args.dxc,args.assembler,args.dxv)):parser.error('Generation needs DXC, assembler and DXV')
        generate(root,args.dxc,args.assembler,args.dxv)
    prepare(root)

