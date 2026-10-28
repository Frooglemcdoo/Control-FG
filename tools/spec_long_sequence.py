"""R193: extend only primary reflection sampling to a bounded 256-frame cycle."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from spec_retry_reuse import ROOT, SOURCE_SHA, transform as reuse

OLD = '  %271 = mul i32 %270, %139\n'
NEW = ('  %ReflectionCurrentFrame = extractvalue %dx.types.CBufRet.i32 %269, 2\n'
       '  %ReflectionSequenceFrame = and i32 %ReflectionCurrentFrame, 255\n'
       '  %271 = mul i32 %ReflectionSequenceFrame, %139\n')

def transform(source):
    assert ';       uint g_uCurrentFrame;' in source
    assert '!"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5' in source
    baseline = reuse(source)
    assert baseline.count(OLD) == 1
    assert '%269 = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)' in baseline
    result = baseline.replace(OLD, NEW)
    assert result.replace(NEW, OLD) == baseline
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True)
    a=p.parse_args();build=ROOT/'build';build.mkdir(exist_ok=True)
    native=ROOT/'research/hit-distance/evidence/original-shaders/rt_reflection-000.dxbc'
    assert hashlib.sha256(native.read_bytes()).hexdigest()==SOURCE_SHA
    source=subprocess.check_output([a.dxc,'-dumpbin',str(native)],text=True)
    ir=build/'spec-long-sequence.ll';ir.write_text(transform(source))
    output=build/'spec-long-sequence.dxil'
    subprocess.run([a.assembler,str(ir),str(output),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
    data=output.read_bytes();assert data[:4]==b'DXBC' and any(data[4:20])
    (build/'spec_long_sequence.generated.h').write_text('#pragma once\nstatic const unsigned char kSpecLongSequenceShader[]={\n'+','.join(f'0x{x:02x}' for x in data)+'\n};\n')
    report=dict(source_sha256=SOURCE_SHA,sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),
                counter='g_uCurrentFrame',byte_offset=1112,frame_mask=255,
                primary_only=True,retry_sequence_changed=False,ray_count_changed=False,
                native_mapping_preserved=True,dxil_validated=True,game_quality_verified=False)
    (build/'spec-long-sequence-validation.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report))
if __name__=='__main__': main()
