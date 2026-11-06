"""R353: bounded near-field reuse in the exact native view-volume history pass."""
import base64,hashlib
from pathlib import Path
from character_surface import dst,src,imm,op,mov,words
from dxbc_transform import transform_shex
ROOT=Path(__file__).resolve().parents[1]
SHA='566dbe6c0340b6c8b68004cc57093be77fe2d3bace7b32dd16f1605ae9bf5f1d'
def cb(index,lane):return [0x308006|(lane<<4),1,1,index]
def absolute(reg,swizzle=0xe4):
 s=src(reg,swizzle);return [s[0]|0x80000000,0x81,s[1]]
def generate(data):
 if hashlib.sha256(data).hexdigest()!=SHA:raise ValueError('Unknown view-volume history shader')
 def patch(w):
  assert w[48:50]==[0x02000068,5]
  # Preserve unclamped prior coordinates before the native clamp overwrites them.
  xy=op(14,dst(5,3),src(1,0x44),src(1,0xff))
  xy+=op(49,dst(5,8),imm(0),src(1,0xff)) # w > 0, NaN rejects
  z=op(14,dst(5,4),src(1,0),src(3,0))
  gate=mov(dst(10,2),src(1,0)) # native far-slice eligibility mask
  code=mov(dst(7,7),src(1,0x79))+mov(dst(10,1),src(1,0))
  a=list(w[433:446]);a[1:3]=dst(8,7)
  b=list(w[463:476]);b[1:3]=dst(9,15)
  code+=a+b
  # Per-channel relative change over BOTH coefficient textures. Reject changes
  # >=25%; a zero global history weight (camera cut/reset) is never overridden.
  for current,history,mask in ((7,8,7),(2,9,15)):
   neg=src(current);neg=[neg[0]|0x80000000,0x41,neg[1]]
   code+=op(0,dst(6,mask),src(history),neg)
   code+=op(52,dst(11,mask),absolute(current),absolute(history))
   code+=op(52,dst(11,mask),src(11),imm(.001))
   code+=op(14,dst(6,mask),absolute(6),src(11))
   code+=op(52,dst(6,1),src(6,0),src(6,0x55))
   code+=op(52,dst(6,1),src(6,0),src(6,0xaa))
   if mask==15:code+=op(52,dst(6,1),src(6,0),src(6,0xff))
   if current==7:code+=mov(dst(10,4),src(6,0))
   else:code+=op(52,dst(10,4),src(10,0xaa),src(6,0))
  code+=op(50|0x2000,dst(10,4),src(10,0xaa),imm(-4),imm(1))
  code+=op(51,dst(10,8),cb(5,0),imm(.75))
  code+=op(52,dst(10,8),src(10,0xff),imm(0))
  code+=op(56,dst(10,8),src(10,0xff),src(10,0xaa))
  # Unclamped XYZ must lie in the old frustum. Comparisons reject NaN/Inf.
  code+=op(29,dst(6,7),src(5),imm(0))+op(29,dst(11,7),imm(1),src(5))
  code+=op(1,dst(6,7),src(6),src(11))
  code+=op(1,dst(6,1),src(6,0),src(6,0x55))
  code+=op(1,dst(6,1),src(6,0),src(6,0xaa))
  code+=op(1,dst(6,1),src(6,0),src(5,0xff))
  code+=op(1,dst(10,8),src(10,0xff),src(6,0))
  # Far slices retain the exact native weight. Only previously unfiltered near
  # slices receive the bounded adaptive weight.
  code+=op(55,dst(10,1),src(10,0x55),src(10,0),src(10,0xff))
  for current,history,out,mask in ((7,8,3,7),(2,9,1,15)):
   neg=src(current);neg=[neg[0]|0x80000000,0x41,neg[1]]
   code+=op(0,dst(6,mask),src(history),neg)
   code+=op(50,dst(out,mask),src(10,0),src(6),src(current))
   # A rejected/nonexistent history cannot poison the current sample via 0*NaN.
   code+=op(55,dst(out,mask),src(10,0),src(out),src(current))
  w[433:493]=code
  w[398:398]=gate
  w[382:382]=z
  w[326:326]=xy
  w[49]=12
 return transform_shex(data,patch)
def write_header():
 data=base64.b64decode((ROOT/'research/volume-filter-r344/native-filter.dxbc.b64').read_text())
 result=generate(data);out=ROOT/'build';out.mkdir(exist_ok=True)
 (out/'volume-temporal-native.dxbc').write_bytes(data);(out/'volume-temporal.dxbc').write_bytes(result)
 lines=['#pragma once']
 for name,b in [('kTemporalNative',data),('kTemporalNear',result)]:
  lines+=['inline constexpr unsigned char '+name+'[] = {']
  lines += [','.join(map(str,b[i:i+24]))+',' for i in range(0,len(b),24)]
  lines+=['};']
 (out/'volume_temporal.generated.h').write_text('\n'.join(lines)+'\n')
 return data,result
if __name__=='__main__':write_header()
