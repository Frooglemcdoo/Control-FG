"""DXBC container checksum; validated against the native captured containers."""
import math,struct
from pathlib import Path
K=[int(abs(math.sin(i+1))*2**32)&0xffffffff for i in range(64)]
S=[7,12,17,22]*4+[5,9,14,20]*4+[4,11,16,23]*4+[6,10,15,21]*4

def checksum(data):
 data=data[20:]; bits=len(data)*8; n=len(data)//64*64; tail=data[n:]
 if len(tail)>=56: padded=data[:n]+tail+b'\x80'+bytes(63-len(tail))+struct.pack('<I',bits)+bytes(56)+struct.pack('<I',(bits>>2)|1)
 else: padded=data[:n]+struct.pack('<I',bits)+tail+b'\x80'+bytes(55-len(tail))+struct.pack('<I',(bits>>2)|1)
 state=[0x67452301,0xefcdab89,0x98badcfe,0x10325476]
 for off in range(0,len(padded),64):
  w=struct.unpack_from('<16I',padded,off);a,b,c,d=state
  for i in range(64):
   if i<16:f=(b&c)|(~b&d);g=i
   elif i<32:f=(d&b)|(~d&c);g=(5*i+1)%16
   elif i<48:f=b^c^d;g=(3*i+5)%16
   else:f=c^(b|~d);g=7*i%16
   x=(a+f+K[i]+w[g])&0xffffffff;x=((x<<S[i])|(x>>(32-S[i])))&0xffffffff
   a,d,c,b=d,c,b,(b+x)&0xffffffff
  state=[(u+v)&0xffffffff for u,v in zip(state,[a,b,c,d])]
 return struct.pack('<4I',*state)
