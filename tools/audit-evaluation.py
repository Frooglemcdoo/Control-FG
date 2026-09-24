"""Build-locked static proof of both parameter-setup -> NGX evaluation tails."""
from pathlib import Path
import hashlib,json,re,struct,sys
from pe_tools import PE
root=Path(__file__).resolve().parents[1];p=PE(sys.argv[1])
sha=hashlib.sha256(p.data).hexdigest()
assert sha=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
target=p.exports['NVSDK_NGX_D3D12_EvaluateFeature_C'];assert target==0x52a20
proof=[]
for helper,tail in [(0x1cdf0,0x1d30c),(0x1d460,0x1e156)]:
    raw=p.data[p.off(tail):p.off(tail)+5];assert raw[0]==0xe9
    assert tail+5+struct.unpack_from('<i',raw,1)[0]==target
    s=p.disasm(helper)
    (root/'validation'/f'helper-{helper:x}.asm').write_text(s)
    keys=[]
    for line in s.splitlines():
        if 'lea    rdx,[rip+' not in line:continue
        m=re.search(r'# 0x([0-9a-f]+)',line)
        if m:
            try:key=p.string(int(m[1],16)-p.base)
            except (ValueError,UnicodeError):continue
            keys.append(key)
    for needed in ('Color','Output','Depth','MotionVectors','Jitter.Offset.X','Jitter.Offset.Y','Reset'):assert needed in keys,(helper,needed)
    assert 'xor    r9d,r9d' in s
    assert 'mov    rcx,rbp' in s and 'mov    rdx,rsi' in s
    proof.append({'helper_rva':hex(helper),'tail_rva':hex(tail),'tail_bytes':raw.hex(),
                  'target_rva':hex(target),'parameter_names':keys})
aa=p.disasm(0x1f990);(root/'validation/doAntiAliasing.asm').write_text(aa)
for site,targetHelper in [(0x1fdbb,0x1cdf0),(0x1fd9f,0x1d460)]:
    raw=p.data[p.off(site):p.off(site)+5];assert raw[0]==0xe8
    assert site+5+struct.unpack_from('<i',raw,1)[0]==targetHelper
keys={}
for offset,rva in [(0,0x1ce1d),(8,0x1ce30),(0x18,0x1ce43),(0x20,0x1ce56),(0x28,0x1ce6a),(0x2c,0x1ce7e),(0x38,0x1cea5)]:
    stringRva=rva+7+struct.unpack_from('<i',p.data,p.off(rva)+3)[0];keys[hex(offset)]=p.string(stringRva)
report={'status':'STATIC_EXACT_BINARY_PASS','d3d_sha256':sha,'evaluation_export':'NVSDK_NGX_D3D12_EvaluateFeature_C',
        'branches':proof,'standard_input_struct_offsets':keys,
        'correction':'The old post-setup helper-return boundary is post-evaluation: both helpers tail-jump to EvaluateFeature_C.',
        'tail_abi':{'rcx':'original graphics command list','rdx':'original NGX feature handle','r8':'populated NGX parameters','r9':'null callback'},
        'live_hook_installed':False,'rr_evaluation_enabled':False}
(root/'validation/evaluation-boundary.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: both exact-binary setup/evaluation tails and current-frame parameter names')
