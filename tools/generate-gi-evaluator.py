"""Translate the hash-pinned complete Control GI6 evaluator to editable HLSL.
No arbitrary shader accepted: locked SHA, raw immediate bits, masked writes,
zero/nonzero control flow, resource swizzles and native byte offsets preserved.
Usage: python tools/generate-gi-evaluator.py captured-native.dxbc [output.hlsl]
"""
import sys, pathlib, struct, hashlib, importlib.util, json
root=pathlib.Path(__file__).resolve().parent.parent
spec=importlib.util.spec_from_file_location('decode',root/'research/native-integration-audit/tools/decode_dxbc.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
p=pathlib.Path(sys.argv[1]);data=p.read_bytes()
assert hashlib.sha256(data).hexdigest()=='1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de'
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
 if ty==32:return 'uint4(dispatchId,0u)'
 if ty==0:return 'r'+idx[0][:-1]
 if ty==8:return 'giCB'+idx[0][:-1]+'['+idx[2]+']'
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
uavs={b['identifier']:b for b in j['bindings'] if b['kind']==4}
samplers={b['identifier']:b['name'] for b in j['bindings'] if b['kind']==3}
out=['// GENERATED native GI6 reference: '+j['sha256'], '// Complete evaluator port. Not a new estimator or sample reuse.']
for b in j['bindings']:
 if b['kind']==0:
  cb=next(c for c in j['constant_buffers'] if c['name']==b['name']);out.append('cbuffer '+b['name']+' : register(b'+str(b['slot'])+') { uint4 giCB'+str(b['slot'])+'['+str((cb['size']+15)//16)+']; };');continue
 if b['kind']==3:out.append('SamplerState '+b['name']+' : register(s'+str(b['slot'])+', space1);');continue
 if b['kind']==5:
  typ='GINativeRaw'+str(b['slot']);out.append('struct '+typ+' { uint d['+str(b['samples']//4)+']; };');decl='StructuredBuffer<'+typ+'>'
 elif b['kind'] in (2,4):
  decl={4:'Texture2D',5:'Texture2DArray',8:'Texture3D',9:'TextureCube'}[b['dimension']]+'<'+('uint4' if b['return_type']==4 else 'float4')+'>'
  if b['kind']==4:decl='RW'+decl
 else:raise ValueError(b)
 arr='' if b['count']==1 else '['+(str(b['count']) if b['count'] else '')+']'
 out.append(decl+' '+b['name']+arr+' : register('+('u' if b['kind']==4 else 't')+str(b['slot'])+', space'+str(b['space'])+');')
out += ['''
uint4 GIBitExtract(uint4 width,uint4 offset,uint4 value) {
    width &= 31u; offset &= 31u;
    return (value>>offset)&((1u<<width)-1u);
}
uint4 GIBitInsert(uint4 width,uint4 offset,uint4 value,uint4 target) {
    width &= 31u; offset &= 31u;
    uint4 mask=((1u<<width)-1u)<<offset;
    return (target&~mask)|((value<<offset)&mask);
}
''' ]
out += ['[numthreads(8,16,1)]','void main(uint3 dispatchId : SV_DispatchThreadID) {']
out += ['    uint4 r'+str(i)+'=0u;' for i in range(30)]
selected=[i for i in j['instructions'] if i['dword']>=245]
counts={};depth=0
for ins in selected:
 at=ins['dword'];tok=w[at];op=ins['name'].removesuffix('_sat');sat=ins['name'].endswith('_sat');end=at+((tok>>24)&127);pos=at+1
 while w[pos-1]>>31:
  extension=w[pos];pos+=1
  if extension&63==1 and extension&0x1ffe00:raise ValueError('nonzero sample offset needs explicit translation')
 args=[]
 while pos<end:o,pos=parse(pos);args.append(o)
 assert pos==end
 counts[op]=counts.get(op,0)+1
 def emit(s):out.append('    '+s+' // native dword '+str(at))
 if op=='ret':emit('return;');continue
 if op=='store_uav_typed':
  res,coord,value=args;b=uavs[rid(res)]
  emit(b['name']+'[('+src(coord)+').xy]=asfloat('+src(value)+');');continue
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
   expr=resource(res)+'.Load(asint('+coord+').'+('xyzw' if coordn==4 else 'xyw')+')'
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
 elif op in ('lt','ge','eq','ne','ilt','ige','ult','uge','ieq','ine'):
  cmp={'lt':'<','ge':'>=','eq':'==','ne':'!=','ilt':'<','ige':'>=','ult':'<','uge':'>=','ieq':'==','ine':'!='}[op]
  vs=fs if op in ('lt','ge','eq','ne') else ['asint('+u+')' for u in us] if op in ('ilt','ige') else us
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
 elif op=='ubfe':expr='GIBitExtract('+','.join(us)+')'
 elif op=='bfi':expr='GIBitInsert('+','.join(us)+')'
 elif op=='not':expr='(~'+us[0]+')'
 elif op=='countbits':expr='countbits('+us[0]+')'
 else:raise ValueError(ins)
 if sat:
  expr='saturate('+ (expr if floatop else 'asfloat('+expr+')') +')';floatop=True
 if floatop:expr='asuint('+expr+')'
 emit(lhs+'=('+expr+').'+mask+';')
assert depth==0,depth
out+=['}']
path=pathlib.Path(sys.argv[2]) if len(sys.argv)>2 else root/'src/shaders/gi_evaluator_reference.generated.hlsl'
path.parent.mkdir(parents=True,exist_ok=True);path.write_text('\n'.join(out)+'\n');print('Generated',path,len(selected),'native instructions',counts)
