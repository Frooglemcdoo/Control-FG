"""Translate the hash-locked native glass lighting compute shader to HLSL.
Preserves full native dispatch mapping, miss guard, lighting and output.
Offline integration component; no runtime replacement is enabled.
Usage: python tools/generate-pt-glass-lighting.py captured-glass.dxbc [output]
"""
import sys, pathlib, struct, hashlib, importlib.util, json
root=pathlib.Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('decode',root/'research/native-integration-audit/tools/decode_dxbc.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
p=pathlib.Path(sys.argv[1]);data=p.read_bytes()
assert hashlib.sha256(data).hexdigest()=='b19de5e0b8a216076b6800790b100b91caa83814abdad3ad276736ba72b5e762'
j=mod.decode(p)
for off in struct.unpack_from('<'+'I'*struct.unpack_from('<I',data,28)[0],data,32):
 if data[off:off+4]==b'SHEX':size=struct.unpack_from('<I',data,off+4)[0];w=list(struct.unpack_from('<'+'I'*(size//4),data,off+8))
bindings={b['identifier']:b for b in j['bindings'] if b['kind'] in (2,5)}
def parse(pos):
 t=w[pos];pos+=1;ext=t>>31;modifier=0
 while ext:
  e=w[pos];pos+=1;ext=e>>31
  if e&63==1:modifier=(e>>6)&255
 idx=[]
 for i in range((t>>20)&3):
  k=(t>>(22+3*i))&7
  assert k in (0,2,3)
  val=None
  if k in (0,3):val=str(w[pos])+'u';pos+=1
  if k in (2,3):o,pos=parse(pos);rel='('+src(o)+').x';val=rel if k==2 else '('+val+'+'+rel+')'
  idx.append(val)
 ty=(t>>12)&255;vals=[]
 if ty==4:
  n=1 if t&3==1 else 4;vals=w[pos:pos+n];pos+=n
 return dict(t=t,ty=ty,idx=idx,vals=vals,mod=modifier),pos
def swiz(o):
 t=o['t'];sel=(t>>2)&3
 if t&3!=2:return 'xxxx'
 if sel==0:return ''.join('xyzw'[i] for i in range(4) if t&(16<<i))
 if sel==1:return ''.join('xyzw'[(t>>(4+2*i))&3] for i in range(4))
 return 'xyzw'[(t>>4)&3]*4
def base(o):
 ty=o['ty'];idx=o['idx']
 if ty==13:return 'unused'
 if ty==33:return 'groupId'
 if ty==34:return 'groupThreadId'
 if ty==0:return 'r'+idx[0][:-1]
 if ty==8:return 'ptCB'+idx[0][:-1]+'['+idx[2]+']'
 if ty==4:
  vs=o['vals'];vs=vs*4 if len(vs)==1 else vs
  return 'uint4('+','.join(hex(x)+'u' for x in vs)+')'
 raise ValueError(o)
def src(o):
 b=base(o)
 if o['ty']!=4:b+='.'+swiz(o)
 if o['mod']:
  assert o['mod'] in (1,2,3)
  f='asfloat('+b+')'
  if o['mod'] in (2,3):f='abs('+f+')'
  if o['mod'] in (1,3):f='-'+f
  b='asuint('+f+')'
 return b
def rid(o):return int(o['idx'][0][:-1])
def resource(o):
 b=bindings[rid(o)];name=b['name']
 if b['count']!=1:name+='[NonUniformResourceIndex('+o['idx'][1]+'-'+str(b['slot'])+'u)]'
 return name
samplers={b['identifier']:b['name'] for b in j['bindings'] if b['kind']==3}
out=['// GENERATED from captured native glass lighting SHA256 '+j['sha256'], '// Offline integration component. Runtime activation requires dedicated secondary storage.']
for b in j['bindings']:
 if b['kind']==0:
  cb=next(c for c in j['constant_buffers'] if c['name']==b['name']);out.append('cbuffer '+b['name']+' : register(b'+str(b['slot'])+') { uint4 ptCB'+str(b['slot'])+'['+str((cb['size']+15)//16)+']; };');continue
 if b['kind']==3:out.append('SamplerState '+b['name']+' : register(s'+str(b['slot'])+', space1);');continue
 if b['kind']==5:
  typ='PTNativeRaw'+str(b['slot']);out.append('struct '+typ+' { uint d['+str(b['samples']//4)+']; };');decl='StructuredBuffer<'+typ+'>'
 elif b['kind'] in (2,4):
  decl={4:'Texture2D',5:'Texture2DArray',8:'Texture3D',9:'TextureCube'}[b['dimension']]+'<'+('uint4' if b['return_type']==4 else 'float4')+'>'
  if b['kind']==4:decl='RW'+decl
 else:raise ValueError(b)
 arr='' if b['count']==1 else '['+(str(b['count']) if b['count'] else '')+']'
 out.append(decl+' '+b['name']+arr+' : register('+('u' if b['kind']==4 else 't')+str(b['slot'])+', space'+str(b['space'])+');')
out += ['#ifndef PT_GLASS_SECONDARY_EVALUATION', '#define PT_GLASS_SECONDARY_EVALUATION 0', '#endif',
        '#if PT_GLASS_SECONDARY_EVALUATION', '#include "pt_glass_secondary_layout.hlsli"',
        'StructuredBuffer<PTGlassSecondaryHit> ptGlassSecondaryHits : register(t28, space0);',
        'cbuffer PTGlassDispatch : register(b6) { uint ptGlassEpoch; uint ptGlassMaterialCount; uint2 ptGlassExtent; };',
        '#endif']
out += ['''
uint4 PTBitExtract(uint4 width,uint4 offset,uint4 value) {
    width &= 31u; offset &= 31u;
    return (value>>offset)&((1u<<width)-1u);
}
uint4 PTBitInsert(uint4 width,uint4 offset,uint4 value,uint4 target) {
    width &= 31u; offset &= 31u;
    uint4 mask=((1u<<width)-1u)<<offset;
    return (target&~mask)|((value<<offset)&mask);
}
[numthreads(8,16,1)]
void main(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID) {
''']
out += ['    uint4 r'+str(i)+'=0u;' for i in range(28)]
selected=[i for i in j['instructions'] if not i['name'].startswith('dcl_')]
counts={};depth=0
for ins in selected:
 at=ins['dword'];tok=w[at];op=ins['name'].removesuffix('_sat');sat=ins['name'].endswith('_sat');end=at+((tok>>24)&127);pos=at+1
 while w[pos-1]>>31:pos+=1
 args=[]
 while pos<end:o,pos=parse(pos);args.append(o)
 assert pos==end
 counts[op]=counts.get(op,0)+1
 def emit(s):out.append('    '+s+' // native dword '+str(at))
 if at==435:
  out += ['#if PT_GLASS_SECONDARY_EVALUATION',
          '    if (any(r1.xy >= ptGlassExtent)) return;',
          '    g_rwtRadiance[r1.xy]=0.0f;',
          '    PTGlassSecondaryHit ptHit=ptGlassSecondaryHits[r1.y*ptGlassExtent.x+r1.x];',
          '    if (!PTGlassValidHit(ptHit,ptGlassEpoch,ptGlassMaterialCount)) return;',
          '    r0=uint4(asuint(ptHit.incomingWorld_hitDistance.xyz),ptHit.metadata.x);',
          '    r1.z=asuint(ptHit.incomingWorld_hitDistance.w*ptHit.incomingWorld_hitDistance.w);',
          '    r2=asuint(ptHit.positionView_texcoordY);',
          '    r3.xyz=asuint(asfloat(ptCB0[6]).xyz*ptHit.positionView_texcoordY.x+',
          '        asfloat(ptCB0[7]).xyz*ptHit.positionView_texcoordY.y+',
          '        asfloat(ptCB0[8]).xyz*ptHit.positionView_texcoordY.z+asfloat(ptCB0[9]).xyz);',
          '    r4=asuint(ptHit.normalWorld_texcoordX);',
          '#else']
 if at==680:out.append('#endif')
 if op=='ret':
  emit('return;');continue
 if op=='store_uav_typed':
  assert rid(args[0])==0
  emit('g_rwtRadiance[('+src(args[1])+').xy]=asfloat('+src(args[2])+');');continue
 if op=='udiv':
  numerator='divNumerator'+str(at);denominator='divDenominator'+str(at)
  emit('uint4 '+numerator+'='+src(args[2])+';')
  emit('uint4 '+denominator+'='+src(args[3])+';')
  for dest,operator in zip(args[:2],['/','%']):
   if dest['ty']==13:continue
   mask=swiz(dest)
   emit(base(dest)+'.'+mask+'=(('+denominator+'==0u)?uint4(0xffffffffu,0xffffffffu,0xffffffffu,0xffffffffu):('+numerator+operator+'max('+denominator+',1u))).'+mask+';')
  continue
 if op in ('if','breakc'):
  condition='('+src(args[0])+').x '+('!=' if tok&(1<<18) else '==')+' 0u'
  emit(('if('+condition+') {' if op=='if' else 'if('+condition+') break;'))
  if op=='if':depth+=1
  continue
 if op in ('else','endif','loop','endloop','break'):
  emit({'else':'} else {','endif':'}','loop':'[loop] while(true) {','endloop':'}','break':'break;'}[op]);depth+=1 if op=='loop' else -1 if op in ('endif','endloop') else 0;continue
 dest=args[0];mask=swiz(dest);lhs=base(dest)+'.'+mask
 if op=='ld_structured':
  idx='('+src(args[1])+').x';offset='('+src(args[2])+').x/4u';res=args[3];rs=swiz(res)
  vals=[resource(res)+'['+idx+'].d['+offset+'+'+str('xyzw'.index(rs['xyzw'.index(c)]))+'u]' for c in mask]
  expr=vals[0] if len(vals)==1 else 'uint'+str(len(vals))+'('+','.join(vals)+')'
  emit(lhs+'='+expr+';');continue
 if op in ('ld','sample_l'):
  res=args[2];b=bindings[rid(res)];coord=src(args[1]);coordn=3 if b['dimension'] in (5,8,9) else 2
  if op=='ld':
   coordn=4 if b['dimension'] in (5,8) else 3
   expr=resource(res)+'.Load(asint('+coord+').'+('xyzw'[:coordn])+')'
  else:
   lod='asfloat('+src(args[4])+').x'
   expr=resource(res)+'.SampleLevel('+samplers[rid(args[3])]+',asfloat('+coord+').'+('xyz'[:coordn])+','+lod+')'
  if b['return_type']==5:expr='asuint('+expr+')'
  emit(lhs+'=('+expr+').'+swiz(res)+'.'+mask+';');continue
 if op=='imul':
  assert args[0]['ty']==13;dest=args[1];mask=swiz(dest);lhs=base(dest)+'.'+mask;args=args[1:]
 us=[src(a) for a in args[1:]];fs=['asfloat('+u+')' for u in us];expr=None;floatop=False
 if op in ('add','mul','div','mad','min','max','sqrt','rsq','rcp','round_ni','log','exp','dp2','dp3','dp4'):
  floatop=True
  if op in ('add','mul','div'):expr='('+fs[0]+{'add':'+','mul':'*','div':'/'}[op]+fs[1]+')'
  elif op=='mad':expr='mad('+','.join(fs)+')'
  elif op in ('min','max'):expr=op+'('+','.join(fs)+')'
  elif op.startswith('dp'):n=int(op[-1]);expr='dot('+fs[0]+'.'+('xyzw'[:n])+','+fs[1]+'.'+('xyzw'[:n])+').xxxx'
  elif op=='rcp':expr='(1.0f/'+fs[0]+')'
  else:expr={'rsq':'rsqrt','round_ni':'floor','log':'log2','exp':'exp2'}.get(op,op)+'('+fs[0]+')'
 elif op in ('lt','ge','eq','ne','ilt','ult','uge','ieq','ine'):
  cmp={'lt':'<','ge':'>=','eq':'==','ne':'!=','ilt':'<','ult':'<','uge':'>=','ieq':'==','ine':'!='}[op]
  vs=fs if op in ('lt','ge','eq','ne') else ['asint('+u+')' for u in us] if op=='ilt' else us
  expr='(uint4('+vs[0]+cmp+vs[1]+')*0xffffffffu)'
 elif op in ('and','or','ishl','ushr','iadd','imul'):
  expr='('+us[0]+{'and':'&','or':'|','ishl':'<<','ushr':'>>','iadd':'+','imul':'*'}[op]+( '('+us[1]+'&31u)' if op in ('ishl','ushr') else us[1])+')'
 elif op=='imad':expr='('+us[0]+'*'+us[1]+'+'+us[2]+')'
 elif op=='mov':expr=us[0]
 elif op=='movc':expr='('+us[0]+'!=0u?'+us[1]+':'+us[2]+')'
 elif op=='utof':expr='asuint(float4('+us[0]+'))'
 elif op=='itof':expr='asuint(float4(asint('+us[0]+')))'
 elif op=='ftou':expr='uint4('+fs[0]+')'
 elif op=='ftoi':expr='asuint(int4('+fs[0]+'))'
 elif op=='ubfe':expr='PTBitExtract('+','.join(us)+')'
 elif op=='bfi':expr='PTBitInsert('+','.join(us)+')'
 elif op=='not':expr='(~'+us[0]+')'
 elif op=='countbits':expr='countbits('+us[0]+')'
 else:raise ValueError(ins)
 if sat:
  expr='saturate('+ (expr if floatop else 'asfloat('+expr+')') +')';floatop=True
 if floatop:expr='asuint('+expr+')'
 emit(lhs+'=('+expr+').'+mask+';')
assert depth==0,depth
out+=['}']
path=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else root/'src/shaders/pt_glass_native.generated.hlsl'
path.write_text('\n'.join(out)+'\n')
print('Generated',path,len(selected),'native instructions',counts)
