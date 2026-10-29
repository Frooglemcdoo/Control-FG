"""R212: bound reprojected GI history by current filtered neighborhood color."""
import base64,hashlib,struct
from pathlib import Path
from dxbc_transform import transform_shex
SOURCE_SHA='d54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15'
def generate(data,history=True,outliers=False,view=0,smooth=False):
 if smooth and (history or outliers or view):raise ValueError("Smoothing requires a normal zero-history path")
 if view not in (0,1,2) or (view and (history or not outliers)):raise ValueError("Diagnostic requires zero-history outlier path")
 if outliers and history:raise ValueError("Outlier variant must have zero history")
 if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unknown native GI shader')
 def patch(w):
  # SM5.1 operands. Reuse the audited native sample/coordinate operands;
  # only scratch registers r5-r8 are introduced. Native r0-r4 stay intact.
  def dst(r,mask=7):return [0x100002|(mask<<4),r]
  def src(r,swizzle=0xe4):return [0x100006|(swizzle<<4),r]
  def imm(v):return [0x4001,struct.unpack('<I',struct.pack('<f',v))[0]]
  def vec(v):return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in v]
  def ins(op,*args):
   body=sum(args,[]);return [((len(body)+1)<<24)|op]+body
  assert w[76]==5
  w[76]=9
  # r5/r6 hold RGB bounds initialized from the native mip-filtered current sample.
  bounds=ins(54,dst(5),src(3))+ins(54,dst(6),src(3))
  # Four one-texel offsets around that sample, at the same native LOD (1.2).
  # Native UV is r1.zw; cb0[0].zw is the current texture inverse size.
  inv=list(w[111:115]);inv[0]=(inv[0]&~0xff0)|(0xae<<4)
  assert len(inv)==4
  for x,y in ((-1,0),(1,0),(0,-1),(0,1)):
   bounds+=ins(50,dst(8,3),vec((x,y,0,0)),inv,src(1,0xae))
   sample=list(w[131:144]);sample[1:3]=dst(7);sample[3:5]=src(8)
   bounds+=sample
   bounds+=ins(51,dst(5),src(5),src(7))+ins(52,dst(6),src(6),src(7))
  # Include the native energy-clamped center; retain a 25% + 0.001
  # radiance margin so ordinary noise can still be temporally averaged.
  pad=ins(51,dst(5),src(5),src(3))+ins(52,dst(6),src(6),src(3))
  pad+=ins(50,dst(7),src(6),imm(.25),imm(.001))
  pad+=ins(0,dst(6),src(6),src(7))
  neg7=[0x80100e46,0x41,7]
  pad+=ins(0,dst(5),src(5),neg7)+ins(52,dst(5),src(5),imm(0))
  clip=ins(52,dst(1),src(1),src(5))+ins(51,dst(1),src(1),src(6))
  for start,end,old,new in [(311,318,.03,.01),(356,366,40.,400.)]+([] if history else [(219,226,.7,0.)]):
   bits=struct.unpack('<I',struct.pack('<f',old))[0]
   matches=[i for i in range(start+1,end) if w[i]==bits]
   if len(matches)!=1:raise ValueError('Unexpected GI immediate layout')
   w[matches[0]]=struct.unpack('<I',struct.pack('<f',new))[0]
  if smooth:
   # R217: 3x3 binomial current-frame average, independently energy-clamp
   # every neighbor, reject relative depth jumps. No extra resources/history.
   w[76]=11
   def sample(r,mask,uv,texture,lod):
    code=list(w[131:144]);code[1:3]=dst(r,mask);code[3:5]=uv
    code[6]=code[7]=texture;code[-1]=imm(lod)[1]
    if texture==0:code[5]=0x207006;code[9:11]=[0,6] # Native current-depth sampler.
    return code
   smooth_code=ins(56,dst(5),src(3),imm(4))+ins(54,dst(5,8),imm(4))
   smooth_code+=sample(6,1,src(1,0xae),0,0)
   smooth_code+=ins(52,dst(6,2),src(6,0),imm(.000001))
   smooth_code+=ins(56,dst(6,2),src(6,0x55),imm(.01))
   for x,y,weight in ((-1,0,2),(1,0,2),(0,-1,2),(0,1,2),(-1,-1,1),(1,-1,1),(-1,1,1),(1,1,1)):
    smooth_code+=ins(50,dst(8,3),vec((x,y,0,0)),inv,src(1,0xae))
    smooth_code+=sample(7,7,src(8),3,0)+sample(9,7,src(8),3,1.2)
    for reg,mask,inputreg in ((10,1,7),(10,2,9)):
     luma=list(w[144:154]);luma[1:3]=dst(reg,mask);luma[3:5]=src(inputreg);smooth_code+=luma
    smooth_code+=ins(51,dst(10,2),src(10,0),src(10,0x55))
    smooth_code+=ins(52,dst(10,1),src(10,0),imm(.00000001))
    smooth_code+=ins(14,dst(10,2),src(10,0x55),src(10,0))
    smooth_code+=ins(56,dst(7),src(7),src(10,0x55))
    smooth_code+=sample(9,8,src(8),0,0)
    # abs(neighborDepth-centerDepth) <= max(centerDepth,1e-6)*.01
    smooth_code+=ins(0,dst(9,8),src(9,0xff),[0x80100006,0x41,6])
    smooth_code+=ins(29,dst(10,4),src(6,0x55),[0x80100ff6,0x81,9])
    smooth_code+=ins(1,dst(10,4),src(10,0xaa),imm(weight))
    smooth_code+=ins(50,dst(5),src(7),src(10,0xaa),src(5))
    smooth_code+=ins(0,dst(5,8),src(5,0xff),src(10,0xaa))
   smooth_code+=ins(14,dst(3),src(5),src(5,0xff))
   # Native zero-history output stores red again in alpha. Preserve that ABI.
   smooth_code+=ins(54,dst(3,8),src(3,0))
   w[204:204]=smooth_code
   return
  if outliers:
   # Current-frame-only radiance limit. Use the second-largest of four
   # adjacent LOD0 luminances; one bright neighbor cannot lift the limit.
   # Two supporting neighbors preserve lines/edges and broad bright regions.
   guard=ins(54,dst(5,3),imm(0))
   for x,y in ((-1,0),(1,0),(0,-1),(0,1)):
    guard+=ins(50,dst(8,3),vec((x,y,0,0)),inv,src(1,0xae))
    sample=list(w[131:144]);sample[1:3]=dst(7);sample[3:5]=src(8);sample[-1]=0
    guard+=sample
    luma=list(w[144:154]);luma[1:3]=dst(7,1);luma[3:5]=src(7)
    guard+=luma
    guard+=ins(51,dst(7,2),src(5,0),src(7,0))
    guard+=ins(52,dst(5,2),src(5,0x55),src(7,0x55))
    guard+=ins(52,dst(5,1),src(5,0),src(7,0))
   guard+=ins(50,dst(5,1),src(5,0x55),imm(1.5),imm(.002))
   guard+=ins(51,dst(0,4),src(0,0xaa),src(5,0))
   if view==1:
    # Preserve the pre-cleanup native clamp luminance. Native code uses r0-r4;
    # r6 remains untouched until the final diagnostic output override.
    guard=ins(54,dst(6,1),src(0,0xaa))+guard
    negZ=[0x80100aa6,0x41,0]
    guard+=ins(0,dst(6,1),src(6,0),negZ)
    guard+=ins(49,dst(6,1),imm(.000001),src(6,0))
    guard+=ins(1,dst(6,1),src(6,0),imm(1))
    w[412:412]=ins(56,dst(1,15),src(6,0),vec((8,0,8,8)))
   elif view==2:
    w[412:412]=ins(54,dst(1,15),imm(0))
   w[197:197]=guard
   return
  # Insert backwards to preserve original audited dword positions.
  w[393:393]=clip
  w[204:204]=pad
  w[144:144]=bounds
 return transform_shex(data,patch)
if __name__=='__main__':
 root=Path(__file__).resolve().parents[1]
 source=base64.b64decode((root/'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text())
 result=generate(source);out=root/'build';out.mkdir(exist_ok=True)
 (out/'gi-motion-history.dxbc').write_bytes(result)
 (out/'gi_motion_history.generated.h').write_text('#pragma once\ninline constexpr unsigned char kGIMotionHistoryShader[]={\n'+','.join(map(str,result))+'\n};\ninline constexpr unsigned char kGINativeHistoryShader[]={\n'+','.join(map(str,source))+'\n};\n')
 with (out/'gi_motion_history.generated.h').open('a') as f:
  f.write('inline constexpr unsigned char kGINoHistoryShader[]={\n'+','.join(map(str,generate(source,False)))+'\n};\n')
 with (out/'gi_motion_history.generated.h').open('a') as f:
  f.write('inline constexpr unsigned char kGIOutlierShader[]={\n'+','.join(map(str,generate(source,False,True)))+'\n};\n')
 with (out/'gi_motion_history.generated.h').open('a') as f:
  for name,view in [('kGICleanupMaskShader',1),('kGIZeroShader',2)]:
   f.write('inline constexpr unsigned char '+name+'[]={\n'+','.join(map(str,generate(source,False,True,view)))+'\n};\n')
 with (out/'gi_motion_history.generated.h').open('a') as f:
  f.write('inline constexpr unsigned char kGISmoothShader[]={\n'+','.join(map(str,generate(source,False,smooth=True)))+'\n};\n')
 print('GI motion shader SHA256',hashlib.sha256(result).hexdigest())
