"""Extract the exact R97 center-ray inputs into a private 48-byte record.

The native ray construction instructions are retained verbatim. Only pixel
selection, the terminal store and the no-spot exit change. No rays are traced.
"""
import argparse
import base64
import hashlib
import json
import re
import subprocess
from pathlib import Path
from rt_spot_blocker import BASE_SHA

EXPORT = r'\01?spotRayGeneration@@YAXXZ'
NEW_EXPORT = 'captureNativeInputs'

def transform(source):
    start = source.index('define void @"' + EXPORT + '"()')
    body_start = source.index('\n', start) + 1
    trace = source.index('  call void @dx.op.traceRay.', body_start)
    function_end = source.index('\n}', trace) + 2
    prefix = source[body_start:trace]
    if 'float %SpotSegment0Max' not in source[trace:source.index('\n',trace)]:
        raise ValueError('Not the centered native ray')
    if prefix.count('label %.loopexit32') != 1:
        raise ValueError('Native spotlight selection changed')
    # Name numeric SSA values/labels so added control flow cannot renumber them.
    prefix = re.sub(r'; <label>:(\d+)[^\n]*',r'native\1:',prefix)
    prefix = re.sub(r'%(\d+)\b',r'%native\1',prefix)
    prefix = prefix.replace('label %.loopexit32','label %captureEnd')
    for name, axis in [('DispatchRaysIndex',0),('DispatchRaysIndex164',1)]:
        pattern = rf'  %{name} = call i32 @dx.op.dispatchRaysIndex.i32\(i32 145, i8 {axis}\)[^\n]*\n'
        prefix, count = re.subn(pattern,'',prefix)
        if count != 1: raise ValueError('Native pixel source changed')
    entry = r'''  %captureOutput = load %struct.RWByteAddressBuffer, %struct.RWByteAddressBuffer* @captureOutput, align 4
  %captureHandle = call %dx.types.Handle @dx.op.createHandleForLib.struct.RWByteAddressBuffer(i32 160, %struct.RWByteAddressBuffer %captureOutput)
  %captureLayout = load %CaptureInputLayout, %CaptureInputLayout* @CaptureInputLayout, align 4
  %captureLayoutHandle = call %dx.types.Handle @dx.op.createHandleForLib.CaptureInputLayout(i32 160, %CaptureInputLayout %captureLayout)
  %captureControls = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %captureLayoutHandle, i32 0)
  %captureWidth = extractvalue %dx.types.CBufRet.i32 %captureControls, 0
  %captureHeight = extractvalue %dx.types.CBufRet.i32 %captureControls, 1
  %captureTag = extractvalue %dx.types.CBufRet.i32 %captureControls, 2
  %captureGrid = extractvalue %dx.types.CBufRet.i32 %captureControls, 3
  %captureX = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 0)
  %captureY = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 1)
  %captureZ = call i32 @dx.op.dispatchRaysIndex.i32(i32 145, i8 2)
  %captureXOk = icmp ult i32 %captureX, 64
  %captureYOk = icmp ult i32 %captureY, 64
  %captureZOk = icmp eq i32 %captureZ, 0
  %captureXYOk = and i1 %captureXOk, %captureYOk
  %captureIndexOk = and i1 %captureXYOk, %captureZOk
  br i1 %captureIndexOk, label %captureClear, label %captureEnd
captureClear:
  %captureRow = mul i32 %captureY, 64
  %captureIndex = add i32 %captureRow, %captureX
  %captureByte = mul i32 %captureIndex, 48
  %captureByte16 = add i32 %captureByte, 16
  %captureByte32 = add i32 %captureByte, 32
'''
    def store(offset, values):
        return f'  call void @dx.op.rawBufferStore.i32(i32 140, %dx.types.Handle %captureHandle, i32 %{offset}, i32 undef, '+', '.join('i32 '+v for v in values)+', i8 15, i32 4)\n'
    for offset in ('captureByte','captureByte16','captureByte32'):
        entry += store(offset,['0']*4)
    entry += '''  %captureWidthOk = icmp ult i32 %captureWidth, 16385
  %captureHeightOk = icmp ult i32 %captureHeight, 16385
  %captureWidthNonzero = icmp ne i32 %captureWidth, 0
  %captureHeightNonzero = icmp ne i32 %captureHeight, 0
  %captureTagOk = icmp ne i32 %captureTag, 0
  %captureGridOk = icmp eq i32 %captureGrid, 64
  %captureExtentA = and i1 %captureWidthOk, %captureHeightOk
  %captureExtentB = and i1 %captureWidthNonzero, %captureHeightNonzero
  %captureExtentOk = and i1 %captureExtentA, %captureExtentB
  %captureLayoutOk = and i1 %captureTagOk, %captureGridOk
  %captureValid = and i1 %captureExtentOk, %captureLayoutOk
  br i1 %captureValid, label %nativeBody, label %captureEnd
nativeBody:
  %captureDoubleX = mul i32 %captureX, 2
  %captureDoubleY = mul i32 %captureY, 2
  %captureCenterX = add i32 %captureDoubleX, 1
  %captureCenterY = add i32 %captureDoubleY, 1
  %captureScaledX = mul i32 %captureCenterX, %captureWidth
  %captureScaledY = mul i32 %captureCenterY, %captureHeight
  %DispatchRaysIndex = udiv i32 %captureScaledX, 128
  %DispatchRaysIndex164 = udiv i32 %captureScaledY, 128
'''
    values = ['%.i0275','%.i1276','%.i2277','%SpotSegment0Max',
              '%SpotSegment0xDirection','%SpotSegment0yDirection','%SpotSegment0zDirection']
    writes = ''
    for index, value in enumerate(values):
        writes += f'  %captureBits{index} = bitcast float {value} to i32\n'
    writes += store('captureByte',[f'%captureBits{i}' for i in range(4)])
    writes += store('captureByte16',[f'%captureBits{i}' for i in range(4,7)]+['%native147'])
    writes += store('captureByte32',['%DispatchRaysIndex','%DispatchRaysIndex164','%captureTag','0'])
    writes += '  br label %captureEnd\ncaptureEnd:\n  ret void\n}'
    patched = source[:start] + 'define void @"'+EXPORT+'"() #0 {\n'+entry+prefix+writes+source[function_end:]
    # New resources live only in the capture export. The native outputs are never written.
    types = '%struct.RWByteAddressBuffer = type { i32 }\n%CaptureInputLayout = type { i32, i32, i32, i32 }\n@captureOutput = external constant %struct.RWByteAddressBuffer, align 4\n@CaptureInputLayout = external constant %CaptureInputLayout\n'
    patched = patched[:start]+types+patched[start:]
    declarations = '''declare %dx.types.Handle @dx.op.createHandleForLib.struct.RWByteAddressBuffer(i32, %struct.RWByteAddressBuffer) #2
declare %dx.types.Handle @dx.op.createHandleForLib.CaptureInputLayout(i32, %CaptureInputLayout) #2
declare void @dx.op.rawBufferStore.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32, i32, i8, i32) #5
'''
    patched = patched.replace('attributes #0 =',declarations+'\nattributes #0 =',1)
    patched = patched.replace('!22 = !{!23, !24}','!22 = !{!23, !24, !437}')
    patched = patched.replace('!25 = !{!26, !27}','!25 = !{!26, !27, !438}')
    patched += '''\n!437 = !{i32 2, %struct.RWByteAddressBuffer* @captureOutput, !"captureOutput", i32 0, i32 2, i32 1, i32 11, i1 false, i1 false, i1 false, null}
!438 = !{i32 2, %CaptureInputLayout* @CaptureInputLayout, !"CaptureInputLayout", i32 0, i32 4, i32 1, i32 16, null}
'''
    for line in patched.splitlines():
        if line.startswith('declare '):
            symbol=re.search(r'(@"[^"\n]+"|@[A-Za-z0-9_.]+)\(',line).group(1)
            if patched.count(symbol+'(')==1: patched=patched.replace(line+'\n','')
    return patched.replace(EXPORT,NEW_EXPORT)

def prepare(root):
    folder=root/'assets/rt-spot-capture-inputs'
    manifest=json.loads((folder/'manifest.json').read_text())
    data=base64.b64decode((folder/'rt_spot_capture_inputs.dxil.b64').read_text())
    if hashlib.sha256(data).hexdigest()!=manifest['sha256']: raise ValueError('Capture input shader hash mismatch')
    build=root/'build';build.mkdir(exist_ok=True)
    (build/'rt_spot_capture_inputs.dxil').write_bytes(data)
    (build/'rt_spot_capture_inputs.h').write_text('#pragma once\nstatic const unsigned char kSpotCaptureInputsShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')

def generate(root,dxc,assembler,dxv):
    data=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
    if hashlib.sha256(data).hexdigest()!=BASE_SHA: raise ValueError('Unknown native ray baseline')
    build=root/'build';build.mkdir(exist_ok=True)
    original=build/'capture-input-baseline.dxil';original.write_bytes(data)
    source=subprocess.check_output([dxc,'-dumpbin',str(original)],text=True)
    patched=transform(source)
    ir=build/'capture-input.ll';ir.write_text(patched)
    unsigned=build/'capture-input-unsigned.dxil';signed=build/'rt_spot_capture_inputs.dxil'
    subprocess.run([assembler,str(ir),str(unsigned)],check=True)
    subprocess.run([dxv,str(unsigned),'-o='+str(signed)],check=True)
    output=signed.read_bytes();folder=root/'assets/rt-spot-capture-inputs';folder.mkdir(exist_ok=True)
    (folder/'rt_spot_capture_inputs.dxil.b64').write_text(base64.b64encode(output).decode()+'\n')
    (folder/'manifest.json').write_text(json.dumps({'baseline_sha256':BASE_SHA,'sha256':hashlib.sha256(output).hexdigest(),'bytes':len(output),'grid':64,'record_bytes':48,'export':NEW_EXPORT,'traces_rays':False},indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--generate',action='store_true')
    for key in ('dxc','assembler','dxv'):parser.add_argument('--'+key)
    args=parser.parse_args();root=Path(__file__).resolve().parents[1]
    if args.generate:
        if not all((args.dxc,args.assembler,args.dxv)):parser.error('Generation requires DXC, assembler and DXV')
        generate(root,args.dxc,args.assembler,args.dxv)
    prepare(root)
