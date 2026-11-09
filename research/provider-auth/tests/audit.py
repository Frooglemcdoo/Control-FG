from pathlib import Path
import sys,hashlib,json
from pe_tools import PE
root=Path(__file__).resolve().parents[1]
d=PE(sys.argv[1]);r=PE(sys.argv[2])
assert hashlib.sha256(d.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
assert hashlib.sha256(r.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
checks=[]
def check(name,p,a,b,need):
 text=p.disasm(a,b)
 for token in need: assert token in text,(name,token)
 (root/'evidence'/f'{name}.asm').write_text(text)
 checks.append(name)
check('compare_tls',d,0x16a40,0x16a9a,['shl    rax,0x4','0x180112840','[rax+rcx*1+0x8]','gs:0x58','0x1801115fc','0x4250','0x180058616','mov    DWORD PTR [rbx],eax'])
check('set_tls',d,0x16980,0x16a04,['0x180112840','gs:0x58','0x1801115fc','0x4250','0x1800585fe'])
check('clip_matrix_upload',r,0x17f15a,0x17f170,['[rdi+0x5b0]','[rdi+0x5b8]','0x1805dff90'])
check('clip_depth_upload_and_mirror',r,0x12500b,0x125039,['0x181296350','0x1805dff90','0x1805dfc90'])
check('inv_output_upload_and_mirror',r,0x20edb5,0x20edd5,['0x18129e5e8','0x1805dff90','0x18129e5f0'])
check('whole_provider_set',d,0x16a10,0x16a3c,['0x180112848','0x180016980'])
check('provider_arena_allocation',d,0x1450,0x14c0,['0x44250'])
check('reflection_thread_view',r,0x128dff,0x128e2d,['0x18080283c','gs:0x58','edx,0x8','[rdx+rax*1]'])
check('thread_view_upload',r,0x127cef,0x127d27,['0x18080283c','esi,0x8','rcx,rsi','0x18017eb60'])
check('clip_provider_name',r,0x17e2cf,0x17e2fc,['[rsi+0x5b0]','0x180635a60','0x180210270'])
assert r.string(0x635a60)=='g_mClipToView'
check('world_provider_name',r,0x17e20f,0x17e23c,['[rsi+0x510]','0x180635a20','0x180210270'])
assert r.string(0x635a20)=='g_mViewToWorld'
assert r.imports[0x5dff90]=='?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z'
assert d.exports['?cmpProviderData@ConstantValueProviderRegistry@d3d@@SA_NHPEBXPEAH@Z']==0x16a40
(root/'evidence'/'audit.json').write_text(json.dumps({'checks':checks,'status':'PASS','scope':'Static DLL evidence only; no runtime authentication or GPU validation'},indent=2)+'\n')
print('PASS',len(checks),'native provider audit groups')
