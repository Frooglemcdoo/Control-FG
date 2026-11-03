"""Opt-in live GI producer: jittered equal-area strata, native traversal/shading ABI.

This is the first rendering stage, not spatial/temporal ReSTIR reuse. No resource
or SBT changes. Both audited native libraries are transformed at three random
coordinate definitions only; native cosine mapping and range law remain intact.
"""
import argparse, base64, hashlib, json, re, subprocess
from pathlib import Path
from gi_long_sequence import ROOT, HASHES
from gi_producer_support import audit_source


def coordinate_ir(variant):
    load,count,batch,y=(228,226,234,'DispatchRaysIndex90') if variant==0 else (288,286,294,'DispatchRaysIndex198')
    # Names must match the pinned source. Inputs are full frame, pixel, batch,
    # and ray index; no float-converted growing sequence or 256-frame wrap.
    ray_name='rayIndex.03' if variant==0 else 'rayIndex2.038'
    lines=[]
    def op(name,text):
        lines.append(f'  %GI307{name} = {text}')
        return '%GI307'+name
    frame=op('Frame',f'extractvalue %dx.types.CBufRet.i32 %{load}, 2')
    px=op('PixelX','mul i32 %DispatchRaysIndex, 73856093')
    py=op('PixelY',f'mul i32 %{y}, 19349663')
    pf=op('FrameSeed',f'mul i32 {frame}, 83492791')
    pb=op('BatchSeed',f'mul i32 %{batch}, 2654435761')
    a=op('SeedA',f'xor i32 {px}, {py}')
    b=op('SeedB',f'xor i32 {pf}, {pb}')
    base=op('Seed',f'xor i32 {a}, {b}')
    ray=op('RaySeed',f'mul i32 %{ray_name}, 1597334677')
    seed=op('SampleSeed',f'xor i32 {base}, {ray}')
    random=[]
    for dimension,salt in enumerate((0x68bc21eb,0x02e5be93,0x967a889b)):
        prefix=f'D{dimension}'
        v=op(prefix+'Seed',f'xor i32 {seed}, {salt}')
        for index,(shift,mul) in enumerate(((16,0x7feb352d),(15,0x846ca68b),(16,None))):
            s=op(prefix+f'S{index}',f'lshr i32 {v}, {shift}')
            v=op(prefix+f'X{index}',f'xor i32 {v}, {s}')
            if mul is not None:v=op(prefix+f'M{index}',f'mul i32 {v}, {mul}')
        bits=op(prefix+'Bits',f'lshr i32 {v}, 8')
        val=op(prefix+'Float',f'uitofp i32 {bits} to float')
        random.append(op(prefix+'Unit',f'fmul float {val}, 0x3E70000000000000')) # 2^-24
    even=op('EvenBit',f'and i32 %{count}, 1')
    even=op('Even',f'icmp eq i32 {even}, 0')
    cols=op('Columns',f'select i1 {even}, i32 2, i32 1')
    rows=op('Rows',f'udiv i32 %{count}, {cols}')
    x=op('CellX',f'urem i32 %{ray_name}, {cols}')
    y=op('CellY',f'udiv i32 %{ray_name}, {cols}')
    xf=op('CellXF',f'uitofp i32 {x} to float')
    yf=op('CellYF',f'uitofp i32 {y} to float')
    cf=op('ColumnsF',f'uitofp i32 {cols} to float')
    rf=op('RowsF',f'uitofp i32 {rows} to float')
    u=op('U',f'fadd float {xf}, {random[0]}')
    v=op('V',f'fadd float {yf}, {random[1]}')
    names=('Frc','Frc26','Frc27') if variant==0 else ('Frc','Frc85','Frc86')
    u=op('UnitU',f'fdiv float {u}, {cf}')
    v=op('UnitV',f'fdiv float {v}, {rf}')
    lines.extend((f'  %{names[0]} = call float @dx.op.binary.f32(i32 36, float {u}, float 0x3FEFFFFFE0000000)',f'  %{names[1]} = call float @dx.op.binary.f32(i32 36, float {v}, float 0x3FEFFFFFE0000000)',f'  %{names[2]} = fadd float {random[2]}, 0.000000e+00'))
    return '\n'.join(lines)+'\n'


def transform(source,variant):
    audit_source(source,variant)
    # Strictly replace the three audited coordinate definitions, nothing else.
    names=('Frc','Frc26','Frc27') if variant==0 else ('Frc','Frc85','Frc86')
    matches=[re.findall(r'^  %'+name+r' = call float @dx.op.unary.f32\(i32 22, float [^\n]+\n',source,re.M) for name in names]
    if any(len(m)!=1 for m in matches):raise ValueError('Coordinate definition mismatch')
    old=''.join(m[0] for m in matches)
    if old not in source:raise ValueError('Coordinates not contiguous')
    ir=coordinate_ir(variant)
    for symbol in re.findall(r'%(DispatchRaysIndex\w*|rayIndex(?:2)?\.0(?:38|3))',ir):
        if not re.search(r'%'+re.escape(symbol)+r' = ',source):raise ValueError('Missing producer input '+symbol)
    result=source.replace(old,ir)
    assert result.replace(ir,old)==source
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True);a=p.parse_args()
    build=ROOT/'build';build.mkdir(exist_ok=True);header=['#pragma once'];reports=[]
    for v in (0,1):
        data=base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{v}.dxbc.b64').read_bytes());assert hashlib.sha256(data).hexdigest()==HASHES[v]
        native=build/f'gi-live-native-{v}.dxbc';native.write_bytes(data)
        source=subprocess.check_output([a.dxc,'-dumpbin',str(native)],text=True)
        ir=build/f'gi-live-{v}.ll';ir.write_text(transform(source,v))
        output=build/f'gi-live-{v}.dxil'
        subprocess.run([a.assembler,str(ir),str(output),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
        blob=output.read_bytes();assert blob[:4]==b'DXBC' and any(blob[4:20])
        header.append(f'static const unsigned char kGILiveShader{v}[]={{\n'+','.join(f'0x{x:02x}' for x in blob)+'\n};')
        reports.append(dict(variant=v,native_sha256=HASHES[v],sha256=hashlib.sha256(blob).hexdigest(),bytes=len(blob),stage='stratified_initial_rays',extra_dispatches=0,ray_count_changed=False,restir_reuse=False,quality_verified=False))
    (build/'gi_live_sampling.generated.h').write_text('\n'.join(header)+'\n')
    (build/'gi-live-sampling-validation.json').write_text(json.dumps(reports,indent=2)+'\n')
    print(json.dumps(reports))
if __name__=='__main__':main()
