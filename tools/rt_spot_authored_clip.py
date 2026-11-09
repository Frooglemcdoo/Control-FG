"""Clip centered spotlight segments against each light's authored near plane.

Uses native cone position, direction and near distance. No mesh IDs or world
axis assumptions. Invalid parameters preserve the original full segment.
"""
import argparse
import base64
import hashlib
import json
import math
import subprocess
from pathlib import Path
from rt_spot_endpoint import BASE_SHA, llvm_float

ENDPOINT_SCALE = 1 - 2**-20
FLOAT_MAX = llvm_float(3.4028234663852886e38)


def clip_distance(origin, endpoint, cone, axis, near):
    """CPU reference for validation; the receiver-side origin never moves."""
    delta = [b-a for a,b in zip(origin, endpoint)]
    length = math.sqrt(sum(x*x for x in delta))
    original = length * ENDPOINT_SCALE
    values = (*origin, *endpoint, *cone, *axis, near)
    if not all(math.isfinite(x) for x in values) or near < 0:
        return original
    norm = math.sqrt(sum(x*x for x in axis))
    if norm <= 0:
        return original
    normal = [x/norm for x in axis]
    start = sum((a-b)*n for a,b,n in zip(origin, cone, normal))-near
    end = start + sum(x*n for x,n in zip(delta, normal))
    if not (start > 0 and end < 0):
        return original
    return min(original, length * (start/(start-end)) * ENDPOINT_SCALE)


def authored_clip(source):
    edits = []
    # Native inverse-view rotation/translation, already live in the ray prefix.
    rows = [('x',176,180,184,188,'%.i0275'),
            ('y',177,181,185,189,'%.i1276'),
            ('z',178,182,186,190,'%.i2277')]
    setup = ['  %ClipAxisLoad = call %dx.types.ResRet.f32 @dx.op.rawBufferLoad.f32(i32 139, %dx.types.Handle %166, i32 %147, i32 32, i8 7, i32 4)']
    for lane in range(3):
        setup.append(f'  %ClipAxisView{lane} = extractvalue %dx.types.ResRet.f32 %ClipAxisLoad, {lane}')
    for axis,a,b,c,t,origin in rows:
        for label,values in [('Axis',[f'%ClipAxisView{i}' for i in range(3)]),('Cone',['%167','%168','%169'])]:
            for i,(value,matrix) in enumerate(zip(values,[a,b,c])):
                setup.append(f'  %Clip{label}{axis}{i} = fmul float {value}, %{matrix}')
            setup += [f'  %Clip{label}{axis}Sum = fadd float %Clip{label}{axis}0, %Clip{label}{axis}1',
                      f'  %Clip{label}{axis}Rotated = fadd float %Clip{label}{axis}Sum, %Clip{label}{axis}2']
        setup += [f'  %ClipCone{axis} = fadd float %ClipCone{axis}Rotated, %{t}',
                  f'  %ClipAxis{axis}Squared = fmul float %ClipAxis{axis}Rotated, %ClipAxis{axis}Rotated']
    setup += ['  %ClipNormXY = fadd float %ClipAxisxSquared, %ClipAxisySquared',
              '  %ClipNormSquared = fadd float %ClipNormXY, %ClipAxiszSquared',
              '  %ClipNormPositive = fcmp ogt float %ClipNormSquared, 0.000000e+00',
              f'  %ClipNormFinite = fcmp olt float %ClipNormSquared, {FLOAT_MAX}',
              '  %ClipNormValid = and i1 %ClipNormPositive, %ClipNormFinite',
              '  %ClipSafeSquared = select i1 %ClipNormValid, float %ClipNormSquared, float 1.000000e+00',
              '  %ClipNorm = call float @dx.op.unary.f32(i32 24, float %ClipSafeSquared)']
    for axis,*_,origin in rows:
        setup += [f'  %ClipNormal{axis} = fdiv float %ClipAxis{axis}Rotated, %ClipNorm',
                  f'  %ClipOrigin{axis} = fsub float {origin}, %ClipCone{axis}',
                  f'  %ClipStart{axis} = fmul float %ClipOrigin{axis}, %ClipNormal{axis}']
    setup += ['  %ClipStartXY = fadd float %ClipStartx, %ClipStarty',
              '  %ClipStartXYZ = fadd float %ClipStartXY, %ClipStartz',
              '  %ClipStart = fsub float %ClipStartXYZ, %170',
              '  %ClipNearPositive = fcmp oge float %170, 0.000000e+00',
              f'  %ClipNearFinite = fcmp olt float %170, {FLOAT_MAX}',
              '  %ClipNearValid = and i1 %ClipNearPositive, %ClipNearFinite',
              '  %ClipParametersValid = and i1 %ClipNearValid, %ClipNormValid',
              '  %ClipStartPositive = fcmp ogt float %ClipStart, 0.000000e+00',
              f'  %ClipStartFinite = fcmp olt float %ClipStart, {FLOAT_MAX}',
              '  %ClipStartValid = and i1 %ClipStartPositive, %ClipStartFinite',
              '  %ClipReceiverValid = and i1 %ClipParametersValid, %ClipStartValid']
    for i in range(4):
        p=f'SpotSegment{i}'
        before=f'  %{p}Max = fmul fast float %{p}Length, 0x3FEFFFFE00000000'
        lines=list(setup) if i==0 else []
        lines.append(f'  %{p}OriginalMax = fmul fast float %{p}Length, 0x3FEFFFFE00000000')
        for axis in 'xyz':lines.append(f'  %{p}Axis{axis} = fmul float %{p}{axis}, %ClipNormal{axis}')
        lines += [f'  %{p}AxisXY = fadd float %{p}Axisx, %{p}Axisy',
                  f'  %{p}AxisDelta = fadd float %{p}AxisXY, %{p}Axisz',
                  f'  %{p}PlaneEnd = fadd float %ClipStart, %{p}AxisDelta',
                  f'  %{p}EndNegative = fcmp olt float %{p}PlaneEnd, 0.000000e+00',
                  f'  %{p}EndFinite = fcmp ogt float %{p}PlaneEnd, {llvm_float(-3.4028234663852886e38)}',
                  f'  %{p}EndValid = and i1 %{p}EndNegative, %{p}EndFinite',
                  f'  %{p}Crossing = and i1 %ClipReceiverValid, %{p}EndValid',
                  f'  %{p}Denominator = fsub float %ClipStart, %{p}PlaneEnd',
                  f'  %{p}DenominatorFinite = fcmp olt float %{p}Denominator, {FLOAT_MAX}',
                  f'  %{p}ClipValid = and i1 %{p}Crossing, %{p}DenominatorFinite',
                  f'  %{p}SafeDenominator = select i1 %{p}ClipValid, float %{p}Denominator, float 1.000000e+00',
                  f'  %{p}Fraction = fdiv float %ClipStart, %{p}SafeDenominator',
                  f'  %{p}Clipped = fmul float %{p}OriginalMax, %{p}Fraction',
                  f'  %{p}Max = select i1 %{p}ClipValid, float %{p}Clipped, float %{p}OriginalMax']
        after='\n'.join(lines)
        if source.count(before)!=1:raise ValueError('Centered segment anchor changed')
        source=source.replace(before,after);edits.append((before,after))
    return source,edits


def prepare(root):
    folder=root/'assets/rt-spot-authored-clip';m=json.loads((folder/'manifest.json').read_text())
    data=base64.b64decode((folder/'rt_spot_authored_clip.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==m['sha256']
    build=root/'build';build.mkdir(exist_ok=True)
    (build/'rt_spot_authored_clip.dxil').write_bytes(data)
    (build/'rt_spot_authored_clip.h').write_text('#pragma once\nstatic const unsigned char kSpotAuthoredClipShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--generate',action='store_true')
    for name in ('dxc','assembler','dxv'):parser.add_argument('--'+name)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    if args.generate:
        data=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text());assert hashlib.sha256(data).hexdigest()==BASE_SHA
        build=root/'build';base=build/'clip-baseline.dxil';base.write_bytes(data)
        source=subprocess.check_output([args.dxc,'-dumpbin',str(base)],text=True)
        patched,edits=authored_clip(source);restored=patched
        for before,after in edits:restored=restored.replace(after,before)
        assert restored==source
        ir=build/'authored-clip.ll';ir.write_text(patched)
        unsigned=build/'authored-clip-unsigned.dxil';signed=build/'authored-clip-signed.dxil'
        subprocess.run([args.assembler,str(ir),str(unsigned)],check=True)
        subprocess.run([args.dxv,str(unsigned),'-o='+str(signed)],check=True)
        result=signed.read_bytes();folder=root/'assets/rt-spot-authored-clip';folder.mkdir(exist_ok=True)
        (folder/'rt_spot_authored_clip.dxil.b64').write_text(base64.b64encode(result).decode()+'\n')
        (folder/'manifest.json').write_text(json.dumps(dict(baseline_sha256=BASE_SHA,sha256=hashlib.sha256(result).hexdigest(),bytes=len(result),policy='authored_cone_near_plane',receiver_origin_unchanged=True,extra_rays=0),indent=2)+'\n')
    prepare(root)

if __name__=='__main__':main()
