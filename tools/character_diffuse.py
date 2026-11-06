"""R360: native-map-aware, material/normal/radiance guarded diffuse reconstruction.
No new bindings, passes, histories or changes to material/rig/specular shading.
MSAA paths deliberately retain the original sample: coverage-index remapping is
not validated for neighboring pixels. Normal/material guards are NOT depth or
object identity tests; same-material overlapping surfaces remain a limitation.
"""
import struct
from character_surface import words,dst,src,imm,vec,reg,transform_shex
from decode_dxbc import opnames

def ins(name,*args,flags=0):
 body=sum((list(a) for a in args),[]);return [((len(body)+1)<<24)|opnames.index(name)|flags]+body

def ui(*v):return [0x4001,v[0]&0xffffffff] if len(v)==1 else [0x4002]+[x&0xffffffff for x in v]
def neg(r,sw=0xe4):return [0x80100006|(sw<<4),0x41,r]
def absolute(r,sw=0xe4):return [0x80100006|(sw<<4),0x81,r]
def resource(slot):return [0x207e46,10 if slot==11 else slot,slot]
def cb(slot,element,sw):return [0x308006|(sw<<4),slot,slot,element]

def normal(n,a,b):
 # Same stereographic packed normal decoder as ControlPrimaryGuideDecode.hlsli.
 code=ins('mad',dst(a,7),src(n,0xf4),imm(255),imm(.5))
 code+=ins('ftou',dst(a,7),src(a))
 code+=ins('and',dst(n,8),src(a,0xaa),ui(1)) # separate native mask bit
 code+=ins('ushr',dst(b,3),src(a,0xaa),ui(1,4,0,0))
 code+=ins('and',dst(b,3),src(b),ui(7,15,0,0))
 code+=ins('ishl',dst(a,3),src(a),ui(3,4,0,0))
 code+=ins('or',dst(a,3),src(a),src(b))
 code+=ins('utof',dst(a,3),src(a))
 code+=ins('mad',dst(b,3),src(a),vec(2/2047,2/4095,0,0),imm(-1))
 code+=ins('mul',dst(b,3),src(b),imm(1.3))
 code+=ins('dp2',dst(a,8),src(b),src(b))
 code+=ins('mul',dst(n,3),src(b),imm(2))
 code+=ins('add',dst(n,4),src(a,0xff),imm(-1))
 code+=ins('add',dst(a,8),src(a,0xff),imm(1))
 code+=ins('div',dst(n,7),src(n),src(a,0xff))
 return code

def block(temp,output,center_source,position,depth_cb):
 orig,center,dim,cn,cg,q,sn,sg,a,b,total,weight,tap=range(temp,temp+13)
 code=ins('uge',dst(a,1),ui(1),cb(depth_cb,2,0x55))
 code+=ins('if',src(a,0),flags=1<<18)
 code+=ins('lt',dst(a,7),absolute(output),imm(1e20))
 code+=ins('and',dst(a,1),src(a,0),src(a,0x55))+ins('and',dst(a,1),src(a,0),src(a,0xaa))
 code+=ins('if',src(a,0),flags=1<<18)
 code+=ins('mov',dst(center,15),center_source)
 code+=ins('ftoi',dst(orig,3),position)
 code+=ins('mov',dst(orig,12),ui(0))
 code+=ins('resinfo',dst(dim,3),ui(0),resource(3),flags=2<<11)
 code+=ins('ld',dst(cn,15),src(center),resource(0))+normal(cn,a,b)
 code+=ins('ld',dst(cg,15),src(center),resource(1))
 code+=ins('mul',dst(cg,12),src(cg),imm(255))+ins('ftou',dst(cg,12),src(cg))
 code+=ins('mul',dst(total,7),src(output),imm(4))+ins('mov',dst(total,8),imm(4))
 for yy in (-1,0,1):
  for xx in (-1,0,1):
   if not (xx or yy):continue
   code+=ins('iadd',dst(q,15),src(orig),ui(xx*2,yy*2,0,0))
   code+=ins('ult',dst(a,3),src(q),src(dim))+ins('and',dst(a,1),src(a,0),src(a,0x55))
   code+=ins('if',src(a,0),flags=1<<18)
   code+=ins('ushr',dst(a,3),src(q),ui(1))+ins('mov',dst(a,12),ui(0))
   code+=ins('ld',dst(b,1),src(a),resource(11))
   # Native reconstruction map: two bits per MSAA sample, four pixels/tile.
   # This path admits sample count <=1, so the coverage sample index is zero.
   code+=ins('and',dst(a,3),src(q),ui(1))
   code+=ins('ishl',dst(a,3),src(a),ui(3,4,0,0))+ins('iadd',dst(a,1),src(a,0),src(a,0x55))
   code+=ins('ushr',dst(b,1),src(b,0),src(a,0))
   code+=ins('and',dst(a,1),src(b,0),ui(1))+ins('ushr',dst(a,2),src(b,0),ui(1))
   code+=ins('and',dst(a,2),src(a,0x55),ui(1))
   code+=ins('and',dst(q,3),src(q),ui(0xfffffffe))+ins('or',dst(q,3),src(q),src(a))
   code+=ins('ult',dst(a,3),src(q),src(dim))+ins('and',dst(a,1),src(a,0),src(a,0x55))
   code+=ins('if',src(a,0),flags=1<<18)
   code+=ins('ld',dst(sn,15),src(q),resource(0))+normal(sn,a,b)
   code+=ins('ld',dst(sg,15),src(q),resource(1))
   code+=ins('mul',dst(sg,12),src(sg),imm(255))+ins('ftou',dst(sg,12),src(sg))
   code+=ins('ieq',dst(a,12),src(sg),src(cg))
   code+=ins('ieq',dst(a,1),src(sn,0xff),src(cn,0xff))
   code+=ins('and',dst(a,1),src(a,0),src(a,0xaa))+ins('and',dst(a,1),src(a,0),src(a,0xff))
   code+=ins('if',src(a,0),flags=1<<18)
   code+=ins('dp3',dst(weight,1),src(sn),src(cn))
   code+=ins('mad',dst(weight,1),src(weight,0),imm(20),imm(-19),flags=1<<13) # saturate
   code+=ins('mul',dst(weight,1),src(weight,0),src(weight,0))
   code+=ins('mul',dst(weight,1),src(weight,0),imm(2 if xx==0 or yy==0 else 1))
   code+=ins('ld',dst(tap,7),src(q),resource(3))
   code+=ins('lt',dst(a,7),absolute(tap),imm(1e20))
   code+=ins('and',dst(a,1),src(a,0),src(a,0x55))+ins('and',dst(a,1),src(a,0),src(a,0xaa))
   code+=ins('if',src(a,0),flags=1<<18)
   # Reject strong radiance discontinuities; never filter material/albedo RGB.
   code+=ins('add',dst(a,7),src(tap),neg(output))
   code+=ins('max',dst(b,7),absolute(tap),absolute(output))
   code+=ins('mad',dst(b,7),src(b),imm(.5),imm(.0001))
   code+=ins('div',dst(a,7),absolute(a),src(b))
   code+=ins('max',dst(a,1),src(a,0),src(a,0x55))+ins('max',dst(a,1),src(a,0),src(a,0xaa))
   code+=ins('add',dst(a,1),imm(1),neg(a,0),flags=1<<13)
   code+=ins('mul',dst(weight,1),src(weight,0),src(a,0))
   code+=ins('mad',dst(total,7),src(tap),src(weight,0),src(total))
   code+=ins('add',dst(total,8),src(total,0xff),src(weight,0))
   code+=ins('endif')*4
 code+=ins('div',dst(output,7),src(total),src(total,0xff))
 code+=ins('endif')*2
 return code

def filtered_diffuse(data,info):
 w=words(data);x=info['instructions'];loads=[i for i in x if i['name']=='ld' and i['operands'][-1]=='t[3][3].xyzw'];assert len(loads)==1
 load=loads[0];at=load['dword']+(w[load['dword']]>>24);output=reg(load['operands'][0]);assert load['operands'][0].endswith('.xyz')
 native=w[load['dword']:at];assert len(native)==8
 # Audit the native remap's parity/coverage layout before sharing it with neighbors.
 prefix=x[:x.index(load)];ishr=next(i for i in reversed(prefix) if i['name']=='ishr')
 pos=prefix[prefix.index(ishr)-1];assert pos['name']=='ftoi' and pos['operands'][1].startswith('v[')
 block_original=prefix[prefix.index(ishr):]
 assert [i['name'] for i in block_original]==['ishr','mov','ld','bfi','bfi','ishl','iadd','ushr','and','ubfe','bfi','iadd','itof','add','ftoi','mov']
 assert block_original[3]['operands'][1:3]==['l(1)','l(4)'] and block_original[4]['operands'][1:3]==['l(1)','l(3)']
 map_dest,_,map_src=block_original[2]['operands'];assert map_src.startswith('t[10][11].')
 assert map_src.split('.')[-1]['xyzw'.index(map_dest.split('.')[-1])]=='x'
 depth=next(b['slot'] for b in info['bindings'] if b['name']=='common_depth_general')
 position_reg=int(pos['operands'][1].split('[')[1].split(']')[0]);position=[0x101e46,position_reg]
 decl=next(i['dword'] for i in x if i['name']=='dcl_temps');count=w[decl+1]
 code=block(count,output,native[3:5],position,depth)
 def apply(v):v[at:at]=code;v[decl+1]=count+13
 result=transform_shex(data,apply);restored=words(result);assert restored[at:at+len(code)]==code
 del restored[at:at+len(code)];restored[decl+1]=count;restored[1]=len(restored);assert restored==w
 return result
