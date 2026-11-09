"""Reproduce static r18 follow-up findings. Requires exact uploaded renderer DLL."""
import hashlib,json,pathlib,sys,re
from pe_tools import PE
from decode_dxbc import decode
root=pathlib.Path(__file__).resolve().parents[1];out=root/'evidence';out.mkdir(exist_ok=True)
p=PE(sys.argv[1]);sha=hashlib.sha256(p.data).hexdigest()
if sha!='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433':raise ValueError('renderer SHA mismatch')
checks=[]
def check(name,ok):
 if not ok:raise ValueError(name)
 checks.append(name)
def code(name,rva,hexbytes):
 b=bytes.fromhex(hexbytes);check(name,p.data[p.off(rva):p.off(rva)+len(b)]==b)
def ins(d,offset,name,operands):
 check(f'{d["file"]}:{offset} {name}',any(i==dict(dword=offset,name=name,operands=operands) for i in d['instructions']))
for r,s in [(0x62f0b8,'dlss:Enable DLSS RR (requires SR)'),(0x62ee50,'rt:Reflection Filter Pass Count'),(0x62ee20,'rt:DGI Filter Pass Count'),(0x61f920,'temporal_feedback'),(0x61f9c0,'evaluate_reflected_color')]:check(s,p.string(r)==s)
code('native RR option read',0x12b8a6,'48 8d 0d c3 8c 7e 00 e8 6e 84 ee ff 84 c0 74 04 33 c0 eb 0c')
code('specular filter receives selected count',0x12b8c6,'45 0f b6 cd 44 8b c0 48 8d 15 44 72 7e 00 49 8b cc e8 f4 a2 ee ff')
code('temporal dispatch precedes zero-pass check',0x16786,'ff 15 34 95 5c 00')
check('dispatch import identity','?dispatch@DeviceUtil@' in p.imports[0x5dfcc0])
code('zero count skips spatial loop',0x167f5,'44 8b ad e0 01 00 00 45 85 ed 0f 84 51 03 00 00')
code('reflection color evaluation depends on fourth argument',0x16bb7,'40 38 bd e8 01 00 00 0f 84 27 03 00 00')
code('position UAV texture field',0xa356b,'48 8b 4f 10')
code('position SRV texture field',0xa3c3b,'48 8b 4f 10')
shaderhash={}
for f in sorted((root/'shaders').glob('*.dxbc')):
 d=decode(f);shaderhash[f.name]=d['sha256'];(out/(f.stem+'.json')).write_text(json.dumps(d,indent=2)+'\n')
 if f.stem.endswith('-000'):
  cb=next(b for b in d['constant_buffers'] if b['name']=='deferredlight_filtering')
  check('temporal shader pass count unused',next(v for v in cb['variables'] if v['name']=='g_DLF_iFilterPassCount')['flags']&2==0)
  check('temporal shader reads history',any(b['name']=='g_DLF_tInputHistory' for b in d['bindings']))
  check('temporal executable does not read pass-count component',all('cb[1][1][1]' not in str(i['operands']) for i in d['instructions']))
 if f.stem.endswith('-001'):
  check('position source is Texture2DArray t8',any(b['name']=='g_tPosition_TexcoordY' and b['slot']==8 and b['dimension']==5 for b in d['bindings']))
  ins(d,1656,'ld',['r[9].xyz','r[0].xyzw','t[8][8].xyzw'])
  ins(d,1664,'mad',['r[2].xyz','r[2].xyzx','r[2].wwww','-r[9].xyzx'])
  ins(d,1674,'dp3',['r[0].z','r[2].xyzx','r[2].xyzx'])
  ins(d,1681,'sqrt',['r[0].z','r[0].z'])
 if f.stem.endswith('-003'):
  ins(d,559,'sample_l',['r[0].zw','r[3].xyxx','t[4][4].zwxy','s[0][6]','l(0)'])
  ins(d,572,'mad',['r[2].xyzw','r[2].xyzw','r[0].zzzz','r[0].wwww'])
  ins(d,581,'mul',['r[1].xyzw','r[1].xyzx','r[2].xyzw'])
  ins(d,588,'store_uav_typed',['type30[0][0].xyzw','r[0].xyyy','r[1].xyzw'])
for a,name in [(0x14c90,'diffuse-filter'),(0x15bd0,'specular-filter'),(0x128d20,'raytracing-passes'),(0xa3270,'position-uav-bind'),(0xa3940,'position-srv-bind')]:
 s=p.disasm(a)
 def annotate(m):
  r=int(m[1],16)-p.base
  if r in p.imports:return m[0]+' ; '+p.imports[r]
  try:t=p.string(r)
  except Exception:return m[0]
  return m[0]+' ; '+repr(t) if len(t)>3 and t.isprintable() else m[0]
 s=re.sub(r'# 0x([0-9a-f]+)',annotate,s);(out/(name+'.asm')).write_text(s)
report={'renderer_sha256':sha,'status':'static audit PASS; runtime behavior not changed','checks':checks,'shaders':shaderhash,'rr_enabled':False,'native_bypass_implemented':False,'hit_distance_proven':False}
(out/'audit.json').write_text(json.dumps(report,indent=2)+'\n');print(f'PASS {len(checks)} static checks; no runtime claims')
