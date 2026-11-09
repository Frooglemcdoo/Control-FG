"""Verify embedded evidence against uploaded native bytecode and its albedo dataflow."""
import base64,hashlib,json,re,sys,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
hashes={'Vertex':'a954d6e56122c5c0008775c37371d1bf240c6349c89c3397c455df655cdc1ea1','OriginalPixelR266':'3755ae5f574c639ee79de158a309414dec754dcc4964ccfa477322f7ef1f658a','OriginalPixel':'68b095104f50891b21ebd4ff28c3ec038f1d6062c84f4f4568929a5037bc16a7','ReplayPixel':'cd3f9865a91912865a87dbae62a8e41f8a8a7e9281e6f123e90106e6aae73b0b'}
header=(root/'src/rr_foliage_evidence.h').read_text();decoded={}
with tempfile.TemporaryDirectory() as tmp:
 for name,sha in hashes.items():
  source=root/'research/foliage-guide-r266/OriginalPixel.dxbc.b64' if name=='OriginalPixelR266' else root/f'research/foliage-guide-r265/{name}.dxbc.b64'
  blob=base64.b64decode(source.read_text())
  assert hashlib.sha256(blob).hexdigest()==sha
  array=re.search(r'kRRFoliage'+name+r'\[\]=\{([^}]*)\}',header)[1]
  assert bytes(int(x,16) for x in array.split(','))==blob
  path=Path(tmp)/(name+'.dxbc');path.write_bytes(blob);decoded[name]=decode(path)
def instruction(name,offset,op,args):
 assert next(x for x in decoded[name]['instructions'] if x['dword']==offset)==dict(dword=offset,name=op,operands=args)
instruction('OriginalPixel',174,'imul',['null','r[0].x','cb[4][4][0].x','l(35)'])
instruction('OriginalPixel',1312,'sample',['r[2].xyz','v[1].xyxx','t[9][r[0].x].xyzw','s[2][19]'])
instruction('OriginalPixel',1325,'mad',['r[0].xyz','r[0].yzwy','r[2].xyzx','r[1].xyzx'])
instruction('ReplayPixel',31,'imul',['null','r[0].x','cb[0][0][0].x','l(35)'])
instruction('ReplayPixel',41,'sample',['r[0].xyz','v[1].xyxx','t[0][r[0].x].xyzw','s[0][19]'])
instruction('ReplayPixel',54,'mov',['o[0].xyz','r[0].xyzx'])
instruction('ReplayPixel',59,'mov',['o[0].w','l(1)'])
instruction('OriginalPixelR266',417,'imul',['null','r[0].w','cb[3][3][0].x','l(35)'])
instruction('OriginalPixelR266',427,'sample',['r[2].xyz','v[1].xyxx','t[6][r[0].w].xyzw','s[1][19]'])
instruction('OriginalPixelR266',440,'mad',['r[0].xyz','r[1].xyzx','r[2].xyzx','r[0].xyzx'])
for name in ('OriginalPixel','OriginalPixelR266','ReplayPixel'):
 assert not any(x['name']=='discard' for x in decoded[name]['instructions'])
 for binding,slot,space in [('g_sMaterialArrayGlobal',19,1),('g_sMaterialTextureArray',0,1)]:
  b=next(x for x in decoded[name]['bindings'] if x['name']==binding);assert b['slot']==slot and b['space']==space
print('PASS pinned uploaded bytes, embedded evidence identity, matching native albedo texture/UV/sampler and unlit RGB output')
