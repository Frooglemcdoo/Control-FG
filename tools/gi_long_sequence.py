"""R264 diagnostic: change only the two exact native GI ray-library phase sources."""
import argparse,base64,hashlib,json,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
HASHES=('6297319e1fec315c06ee75993e797b51def658e7d41f1588d7df3feb41a1ea2f','1ff8a93460d94f58fc0e70afff994a334f1c45be292e3e858a63f51070e5734a')
def edits(variant):
    load,frame,dst,passes=(228,229,232,231) if variant==0 else (288,289,292,291)
    old=f'  %{dst} = mul i32 %{passes}, %{frame}\n'
    new=(f'  %GICurrentFrame = extractvalue %dx.types.CBufRet.i32 %{load}, 2\n'
         '  %GISequenceFrame = and i32 %GICurrentFrame, 255\n'
         f'  %{dst} = mul i32 %{passes}, %GISequenceFrame\n')
    return old,new,load,frame

def transform(source,variant):
    assert variant in (0,1)
    old,new,load,frame=edits(variant)
    assert ';       uint g_uCurrentFrame;' in source
    assert '!"g_uCurrentFrame", i32 3, i32 1112, i32 7, i32 5' in source
    assert f'%{load} = call %dx.types.CBufRet.i32 @dx.op.cbufferLoadLegacy.i32(i32 59, %dx.types.Handle %sys_constants, i32 69)' in source
    assert f'%{frame} = extractvalue %dx.types.CBufRet.i32 %{load}, 1' in source
    assert source.count(old)==1
    result=source.replace(old,new)
    assert result.replace(new,old)==source
    return result

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True);a=p.parse_args()
    build=ROOT/'build';build.mkdir(exist_ok=True);header=['#pragma once'];reports=[]
    for i in range(2):
        data=base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{i}.dxbc.b64').read_text());assert hashlib.sha256(data).hexdigest()==HASHES[i]
        native=build/f'gi-sequence-native-{i}.dxbc';native.write_bytes(data)
        source=subprocess.check_output([a.dxc,'-dumpbin',str(native)],text=True)
        result=transform(source,i);ir=build/f'gi-sequence-{i}.ll';ir.write_text(result)
        output=build/f'gi-sequence-{i}.dxil'
        subprocess.run([a.assembler,str(ir),str(output),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
        blob=output.read_bytes();assert blob[:4]==b'DXBC' and any(blob[4:20])
        header.append(f'static const unsigned char kGISequenceShader{i}[]={{\n'+','.join(f'0x{x:02x}' for x in blob)+'\n};')
        reports.append(dict(variant=i,source_sha256=HASHES[i],sha256=hashlib.sha256(blob).hexdigest(),bytes=len(blob),frame_mask=255,ray_count_changed=False,dxil_validated=True,game_quality_verified=False))
    (build/'gi_sequence.generated.h').write_text('\n'.join(header)+'\n')
    (build/'gi-sequence-validation.json').write_text(json.dumps(reports,indent=2)+'\n');print(json.dumps(reports))
if __name__=='__main__':main()
