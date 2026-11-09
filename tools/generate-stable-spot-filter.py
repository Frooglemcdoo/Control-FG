from pathlib import Path
import struct,hashlib,sys,math
from dxbc_checksum import checksum
import argparse
parser=argparse.ArgumentParser(description="Generate the exact captured spotlight stable-filter patch declarations")
parser.add_argument("native_shader", type=Path)
parser.add_argument("output_directory", type=Path)
args=parser.parse_args()
args.output_directory.mkdir(parents=True, exist_ok=True)
b=args.native_shader.read_bytes();base=11152
assert hashlib.sha256(b).hexdigest()=="bee42ac080862114b7a5926c3053c78162d70de339b20a954b929ca231ccd2ff", "Unsupported native shader"
U=lambda off:struct.unpack_from('<I',b,off)[0]
words=lambda dw,n:list(struct.unpack_from('<'+'I'*n,b,base+dw*4))
f=lambda v:struct.unpack('<I',struct.pack('<f',v))[0]
# Symmetric concentric rings: eight inner and twenty-four outer taps, equal weight.
# Radii sample the equal-area ring centroids; opposite taps cancel first moments.
points=[(math.sqrt(0.125)*math.cos(2*math.pi*i/8),math.sqrt(0.125)*math.sin(2*math.pi*i/8)) for i in range(8)]
points += [(math.sqrt(0.625)*math.cos(2*math.pi*(i+.5)/24),math.sqrt(0.625)*math.sin(2*math.pi*(i+.5)/24)) for i in range(24)]
assert len(set(points))==32 and abs(sum(x for x,y in points))<1e-12
starts=[6141]+[6192+58*i for i in range(14)]+[7004]
add='// R60 fixed atlas-aligned disk: 8 inner and 24 outer samples, equal weights.\n'
names=['SpotStableSoft','SpotStableTight']
for name,scale in zip(names,[1.,.5]):
 edits=[];extra=[]
 # Fixed orthogonal basis: {atlasWidth*radius, 0, 0, atlasHeight*radius}.
 old=words(6127,7);new=old.copy();new[3:7]=[0x100fa6,35,0x100006,34];edits.append((base+6127*4,old,new))
 old=words(6134,7);new=[0x5000036,0x100062,38,0x4001,0,0x100003a,0x100003a];edits.append((base+6134*4,old,new))
 for i,(x,y) in enumerate(points):
  x*=scale;y*=scale
  if i<16:
   dw=starts[i]
   for start,value in [(dw,x),(dw+12,y)]:
    old=words(start,12);new=old.copy()
    # Last native tap writes x/z instead of x/y.
    for j in ([6,8] if i==15 else [6,7]):new[j]=f(value)
    edits.append((base+start*4,old,new))
  else:
   tap=words(6192,58)
   for j,v in [(6,x),(7,x),(18,y),(19,y)]:tap[j]=f(v)
   extra+=tap
 insert=base+7004*4;delta=len(extra)*4
 for off,new in [(base+7062*4+24,f(1/32)),(24,len(b)+delta),(11148,U(11148)+delta),(base+4,U(base+4)+len(extra))]:edits.append((off,[U(off)],[new]))
 for i in range(U(28)):
  off=32+i*4
  if U(off)>=insert:edits.append((off,[U(off)],[U(off)+delta]))
 out=bytearray(b);entries=[]
 for i,(off,old,new) in enumerate(edits):
  key=f'k{name}Edit{i}'
  for suffix,values in [('Expected',old),('Replacement',new)]:add+=f'inline constexpr uint32_t {key}{suffix}[] = {{'+','.join(hex(v)+'u' for v in values)+'};\n'
  entries.append(f'{{{off}, {key}Expected, {key}Replacement, {len(old)}}}')
  struct.pack_into('<'+'I'*len(new),out,off,*new)
 add+=f'inline constexpr uint32_t k{name}Inserted[] = {{\n'
 for i in range(0,len(extra),12):add+='    '+','.join(hex(v)+'u' for v in extra[i:i+12])+',\n'
 add+='};\n'+f'inline constexpr InstructionEdit k{name}Edits[] = {{'+','.join(entries)+'};\n'
 out[insert:insert]=struct.pack('<'+'I'*len(extra),*extra);out[4:20]=checksum(out);(args.output_directory/(name+'.dxbc')).write_bytes(out)
 add+=f'inline constexpr unsigned char k{name}Checksum[] = {{'+','.join(map(str,out[4:20]))+'};\n'
 add+=f'''inline constexpr ShaderPatch k{name} = {{
    "{name}", kDirect.size, kDirect.nativeSha, "{hashlib.sha256(out).hexdigest()}",
    kDirect.instructionOffset, kDirectInstruction, 15, nullptr, 0, k{name}Checksum,
    k{name}Edits, {len(edits)}, {insert}, k{name}Inserted, {len(extra)}
}};
'''

(args.output_directory/"stable-spot-patches.inc").write_text(add)
