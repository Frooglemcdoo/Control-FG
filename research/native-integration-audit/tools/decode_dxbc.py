import pathlib,struct,re,json,hashlib
ROOT=pathlib.Path(__file__).resolve().parent
h=(ROOT/'d3d12TokenizedProgramFormat.hpp').read_text()
enum=re.search(r'typedef enum D3D10_SB_OPCODE_TYPE\s*\{(.*?)\}',h,re.S).group(1)
enum=re.sub(r'//[^\n]*','',enum); opnames=[x.strip().split('_OPCODE_')[-1].lower() for x in enum.split(',') if x.strip()]
regtypes={0:'r',1:'v',2:'o',4:'imm',5:'imm64',6:'s',7:'t',8:'cb',9:'icb',13:'null'}
def operand(w,pos):
 start=pos;t=w[pos];pos+=1;nc=t&3;sel=(t>>2)&3;ty=(t>>12)&255;dim=(t>>20)&3;ext=t>>31;mods=[]
 while ext:
  e=w[pos];pos+=1;ext=e>>31;mods.append(e)
 indices=[]
 for i in range(dim):
  k=(t>>(22+3*i))&7
  if k in (0,1,3,4):
   im=w[pos];pos+=1
   if k in (1,4):im+=w[pos]<<32;pos+=1
  if k in (2,3,4):rel,pos=operand(w,pos)
  indices.append(str(im) if k in (0,1) else rel if k==2 else str(im)+'+'+rel)
 if ty==4:
  count=1 if nc==1 else 4
  val=w[pos:pos+count];pos+=count
  s='l('+','.join((str(v) if v<1000 else f'{struct.unpack("<f",struct.pack("<I",v))[0]:.9g}') for v in val)+')'
 else:
  s=regtypes.get(ty,f'type{ty}')+('['+']['.join(indices)+']' if indices else '')
  if nc==2:
   if sel==0:s+='.'+''.join('xyzw'[i] for i in range(4) if t&(16<<i))
   elif sel==1:s+='.'+''.join('xyzw'[(t>>(4+2*i))&3] for i in range(4))
   elif sel==2:s+='.'+'xyzw'[(t>>4)&3]
 for e in mods:
  if e&63==1:
   m=(e>>6)&255
   if m==1:s='-'+s
   elif m==2:s='abs('+s+')'
   elif m==3:s='-abs('+s+')'
 return s,pos

def decode(p):
 d=p.read_bytes();n=struct.unpack_from('<I',d,28)[0];chunks={}
 for off in struct.unpack_from('<'+'I'*n,d,32):
  tag=d[off:off+4].decode();count=struct.unpack_from('<I',d,off+4)[0];chunks[tag]=d[off+8:off+8+count]
 r=chunks['RDEF'];U=lambda off:struct.unpack_from('<I',r,off)[0];S=lambda off:r[off:r.index(b'\0',off)].decode()
 bindings=[]
 for i in range(U(8)):
  off=U(12)+40*i;x=struct.unpack_from('<10I',r,off)
  bindings.append(dict(name=S(x[0]),kind=x[1],return_type=x[2],dimension=x[3],samples=x[4],slot=x[5],count=x[6],flags=x[7],space=x[8],identifier=x[9]))
 def typ(off):
  cls,base,rows,cols,arr,members=struct.unpack_from('<6H',r,off);moff=U(off+12);ret=dict(type_offset=off,cls=cls,base=base,rows=rows,cols=cols,array=arr,members=[])
  for i in range(members):
   no,to,offset=struct.unpack_from('<3I',r,moff+12*i);ret['members'].append(dict(name=S(no),byte_offset=offset,type=typ(to)))
  return ret
 cbs=[]
 for i in range(U(0)):
  off=U(4)+24*i;x=struct.unpack_from('<6I',r,off);cb=dict(name=S(x[0]),size=x[3],flags=x[4],kind=x[5],variables=[])
  for j in range(x[1]):
   y=struct.unpack_from('<10I',r,x[2]+40*j);cb['variables'].append(dict(name=S(y[0]),offset=y[1],size=y[2],flags=y[3],type=typ(y[4])))
  cbs.append(cb)
 w=list(struct.unpack('<'+'I'*(len(chunks['SHEX'])//4),chunks['SHEX']));ins=[];pos=2
 while pos<len(w):
  start=pos;t=w[pos];op=t&2047;ln=(t>>24)&127;name=opnames[op];end=pos+ln;pos+=1
  ex=t>>31
  while ex:ex=w[pos]>>31;pos+=1
  vals=[]
  if name.startswith('dcl_'):vals=['raw:'+','.join(hex(x) for x in w[pos:end])];pos=end
  else:
   while pos<end:
    val,pos=operand(w,pos);vals.append(val)
  assert pos==end,(name,pos,end)
  ins.append(dict(dword=start,name=name+('_sat' if (t&8192 and not name.startswith('dcl_')) else ''),operands=vals))
 lines=[f'{x["dword"]:04d}: {x["name"]} '+', '.join(x['operands']) for x in ins]
 return dict(file=p.name,sha256=hashlib.sha256(d).hexdigest(),bytes=len(d),bindings=bindings,constant_buffers=cbs,instructions=ins)
if __name__ == '__main__':
 import argparse
 ap=argparse.ArgumentParser(description='Focused G5 DXBC decoder; not a full shader validator')
 ap.add_argument('shader',type=pathlib.Path)
 args=ap.parse_args()
 print(json.dumps(decode(args.shader),indent=2))
