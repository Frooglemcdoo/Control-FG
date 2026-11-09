import hashlib,json,struct,sys
from pathlib import Path
from pe_tools import PE
renderer,d3d,out=map(Path,sys.argv[1:])
out.mkdir(parents=True,exist_ok=True)
r=PE(renderer);d=PE(d3d);checks=[]
def check(name,value):
 if not value:raise AssertionError(name)
 checks.append(name)
def raw(pe,rva,n):return pe.data[pe.off(rva):pe.off(rva)+n]
def exact(name,pe,rva,h):check(name,raw(pe,rva,len(bytes.fromhex(h)))==bytes.fromhex(h))
def target(pe,rva,prefix):
 b=raw(pe,rva,len(prefix)+4);assert b[:len(prefix)]==prefix
 return rva+len(b)+struct.unpack('<i',b[len(prefix):])[0]
check('exact renderer SHA256',hashlib.sha256(r.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433')
check('exact d3d SHA256',hashlib.sha256(d.data).hexdigest()=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5')
check('reflection call target',target(r,0x129662,b'\xe8')==0xa3940)
check('GI shares call target',target(r,0x12c61c,b'\xe8')==0xa3940)
exact('reflection owner member',r,0x129658,'49 8b 0f 48 8b 89 38 02 00 00')
exact('GI owner member',r,0x12c60a,'49 8b 0f 48 81 c1 40 02 00 00')
exact('GI owner getter',r,0x133a0,'48 8b 01 c3')
check('material ShaderTexture object',target(r,0xa3a1b,b'\x48\x8d\x2d')==0x1291888)
check('position ShaderTexture object',target(r,0xa3bc6,b'\x48\x8d\x2d')==0x12918d8)
exact('material owner field',r,0xa3a90,'48 8b 0f')
exact('position owner field',r,0xa3c3b,'48 8b 4f 10')
check('container getter distinct ABI',r.imports[0x5dfd30]=='?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ')
exact('material pixel format argument',r,0xa2d7c,'c6 44 24 20 07')
exact('position pixel format argument',r,0xa2e9a,'c6 44 24 20 17')
exact('material allocation destination',r,0xa2db1,'48 89 03')
exact('position allocation destination',r,0xa2ed9,'48 89 43 10')
check('nativeFormat export',d.exports['?nativeFormat@d3d@@YA?AW4DXGI_FORMAT@@W4PixelFormat@1@@Z']==0x23990)
check('material DXGI R16_UINT',d.u32(0x62420+20*7)==57)
check('position DXGI RGBA16_FLOAT',d.u32(0x62420+20*23)==10)
check('material SRV name',r.string(0x627840)=='g_tMaterialId')
check('position SRV name',r.string(0x627868)=='g_tPosition_TexcoordY')
check('native thread enum import',d.imports[0x5d5d8]=='?getCurrentThread@r@@YA?AW4ThreadID@1@XZ')
check('native Present export',d.exports['?present@DeviceUtil@d3d@@SAXXZ']==0x31b50)
exact('submit compares Remedy thread enum',d,0x3287a,'3b 05 b4 43 0c 00')
exact('Present invokes renderer submit',d,0x31c52,'e8 f9 0b 00 00')
exact('Present flushes Direct queue',d,0x31c57,'48 8b 0d da ff 0d 00 e8 3d 18 01 00')
exact('submit tests global command count',d,0x328d2,'48 8b 05 3f f3 0d 00 48 83 78 08 00 75 4c')
exact('queue retrieves native D3D12 interface',d,0x434f9,'4d 8b 65 10')
exact('queue ExecuteCommandLists call',d,0x4354e,'49 8b 04 24 4d 8b c5 8b d7 49 8b cc ff 50 50')
exact('native barrier command bookkeeping',d,0x3a732,'48 ff 41 08 48 8b 49 18')
for getter in ('?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ','?getNativeTexture@ShaderTexture@d3d@@QEAAPEAVNativeTexture@2@XZ'):
 check('getter export '+getter,getter in d.exports)
(out/'submission-native-disassembly.txt').write_text(d.disasm(0x31c3b,0x31c63)+d.disasm(0x32850,0x32931)+d.disasm(0x434a0,0x4355d))
(out/'binding-native-disassembly.txt').write_text('\n'.join(r.disasm(a,b) for a,b in [(0x1294b1,0x129667),(0x12c60a,0x12c621),(0xa3940,0xa3c90),(0xa2d74,0xa2db9),(0xa2e92,0xa2edd)])+d.disasm(0x23990,0x239a7))
result={'status':'PASS','checks':checks,'runtime_hook_installed':False,'active_DXR_writer_verified':False}
(out/'binding-native-audit.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(checks),'native binding assertions')
