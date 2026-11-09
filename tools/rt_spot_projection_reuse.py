"""R204: retain the accepted one-trace reuse in the opt-in projection comparison."""
import argparse,base64,hashlib,json,subprocess
from pathlib import Path
from rt_spot_centered_reuse import reuse_centered_rays,TRACE


def main():
    p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--assembler',required=True);a=p.parse_args()
    root=Path(__file__).resolve().parents[1];build=root/'build'
    manifest=json.loads((root/'assets/rt-spot-projection/manifest.json').read_text())
    data=base64.b64decode((root/'assets/rt-spot-projection/rt_spot_projection.dxil.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==manifest['sha256']
    source_path=build/'projection-reuse-original.dxil';source_path.write_bytes(data)
    source=subprocess.check_output([a.dxc,'-dumpbin',str(source_path)],text=True)
    patched,edits=reuse_centered_rays(source)
    assert len(edits)==3
    # Exact original-minus-three-duplicate-traces proof; all point and clipping
    # instructions and all handlers remain byte-for-byte unchanged in the IR.
    for edit in edits:assert edit not in patched
    expected=source
    for edit in edits:expected=expected.replace(edit,'')
    assert expected==patched
    ir=build/'projection-reuse.ll';ir.write_text(patched)
    output=build/'rt_spot_projection_reuse.dxil'
    subprocess.run([a.assembler,str(ir),str(output),str(Path(a.dxc).parent/'dxcompiler.dll')],check=True)
    result=output.read_bytes();assert result[:4]==b'DXBC' and any(result[4:20])
    check=subprocess.check_output([a.dxc,'-dumpbin',str(output)],text=True)
    assert sum(line.startswith(TRACE) for line in check.splitlines())==2
    (build/'rt_spot_projection.h').write_text('#pragma once\nstatic const unsigned char kSpotProjectionShader[]={\n'+','.join(f'0x{x:02x}' for x in result)+'\n};\n')
    report=dict(source_sha256=manifest['sha256'],sha256=hashlib.sha256(result).hexdigest(),spot_traces=1,point_traces=1,removed_instructions=6,shader_validated=True,target='cone_apex',scope='opt_in_native_blend_comparison')
    (build/'projection-reuse-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
if __name__=='__main__':main()
