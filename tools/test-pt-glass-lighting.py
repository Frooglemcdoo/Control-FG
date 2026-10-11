"""Check complete captured-instruction coverage and dispatch tiling invariants.
Requires the verified private capture; no game bytecode is committed by this test.
Usage: python tools/test-pt-glass-lighting.py path/to/captured-glass.dxbc
"""
import importlib.util
import pathlib
import re
import subprocess
import sys
import tempfile
root=pathlib.Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('decoder',root/'research/native-integration-audit/tools/decode_dxbc.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
capture=pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as temp:
    out=pathlib.Path(temp)/'glass.hlsl'
    subprocess.run([sys.executable,str(root/'tools/generate-pt-glass-lighting.py'),str(capture),str(out)],check=True,stdout=subprocess.DEVNULL)
    text=out.read_text()
    assert text==(root/'src/shaders/pt_glass_native.generated.hlsl').read_text()
    shader=decoder.decode(capture)
    instructions=[i for i in shader['instructions'] if not i['name'].startswith('dcl_')]
    expected={i['dword'] for i in instructions}
    emitted={int(x) for x in re.findall(r'// native dword (\d+)',text)}
    assert expected==emitted,(expected-emitted,emitted-expected)
    assert len(instructions)==871
    # The second UDIV aliases its numerator with its quotient destination.
    # Require snapshots before either write; otherwise the remainder is wrong.
    assert 'uint4 divNumerator340=r2.xxxx;' in text
    assert 'divNumerator340/max(divDenominator340,1u)' in text
    assert 'divNumerator340%max(divDenominator340,1u)' in text
    wrong=pathlib.Path(temp)/'wrong.dxbc';wrong.write_bytes(capture.read_bytes()+b'\0')
    rejected=subprocess.run([sys.executable,str(root/'tools/generate-pt-glass-lighting.py'),str(wrong),str(out)],capture_output=True)
    assert rejected.returncode!=0
# Native thread-group tiling changes at each 16-group-wide boundary. Prove
# complete, duplicate-free coverage at tiny, partial and common resolutions.
for width,height in [(1,1),(7,15),(8,16),(127,31),(128,32),(129,33),(1920,1080),(2560,1440),(3840,2160)]:
    gx,gy=(width+7)//8,(height+15)//16
    seen=set()
    for group_y in range(gy):
        for group_x in range(gx):
            tile,within=divmod(gx*group_y+group_x,gy*16)
            tile_width=16 if tile<gx//16 else gx%16
            row,column=divmod(within,tile_width)
            linear=row*gx+tile*16+column
            y,x=divmod(linear,gx)
            assert 0<=x<gx and 0<=y<gy
            assert (x,y) not in seen
            seen.add((x,y))
    assert len(seen)==gx*gy
print('PASS: all 871 native instructions emitted; locked capture only; UDIV alias protection; complete native group coverage through 4K.')
