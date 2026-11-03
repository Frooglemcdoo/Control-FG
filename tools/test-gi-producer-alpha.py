"""Audit both existing native libraries; no new capture or GPU run required."""
import argparse,base64,hashlib,pathlib,re,subprocess,tempfile
from gi_long_sequence import HASHES,ROOT
parser=argparse.ArgumentParser();parser.add_argument('--dxc',required=True);args=parser.parse_args()
for variant,sha in enumerate(HASHES):
    data=base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_text())
    assert hashlib.sha256(data).hexdigest()==sha
    with tempfile.TemporaryDirectory() as folder:
        path=pathlib.Path(folder)/'native.dxbc';path.write_bytes(data)
        source=subprocess.check_output([args.dxc,'-dumpbin',str(path)],text=True)
    match=re.search(r'define void @"[^\n]*anyHitAlphaTest[^\n]*\n(.*?)^}',source,re.M|re.S)
    assert match,'Missing native alpha export'
    body=match[1]
    for instruction in (
        'i32 %15, i32 52, i8 15, i32 4)',
        '%17 = extractvalue %dx.types.ResRet.f32 %RawBufferLoad, 3',
        '%70 = mul i32 %15, 35',
        '%tmp46 = fmul fast float %tmp45, 0x3F30010020000000',
        '%tmp49 = fmul fast float %tmp48, 0x3F30010020000000',
        'float %tmp46, float %tmp49, float undef, float undef, i32 undef, i32 undef, i32 undef, float 0.000000e+00)',
        '%76 = extractvalue %dx.types.ResRet.f32 %75, 3',
        '%.i3 = fmul fast float %76, %17',
        '%77 = fcmp fast olt float %.i3, 5.000000e-01',
        'call void @dx.op.ignoreHit(i32 155)',
    ):
        assert body.count(instruction)==1,(variant,instruction)
    # The valid native alpha path preserves its input payload DWORD.
    assert re.findall(r'^  store .*$',body,re.M)==['  store i32 %7, i32* %6, align 4']*2
owned=(ROOT/'src/shaders/gi_producer_alpha.hlsl').read_text()
assert 'register(t5)' in owned and 'register(t0,space1)' in owned and 'register(s5,space1)' in owned
assert 'giAlphaMaterials[material].d[16]' in owned
assert 'NonUniformResourceIndex(material*35u)' in owned
assert 'SampleLevel(giAlphaWrap,uv,0).a' in owned
assert 'if(alpha*multiplier<0.5f)IgnoreHit();' in owned
assert '((uv1*weights.y+uv2*weights.z)+uv0*weights.x)*(1.0f/4095.0f)' in owned
assert 'hit.nativeValue==0xffffffffu' in (ROOT/'src/shaders/gi_producer_geometry.hlsl').read_text()
print('PASS both pinned native alpha policies: packed UV, material stride/index, LOD0, multiplier, threshold and valid-path payload preservation. Owned recompile is not bit-exact GPU parity.')
