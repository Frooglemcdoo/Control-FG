"""Four-ray-only progressive scrambled Sobol coordinates; no radiance reuse.
Independent nested binary prefix scrambling, inspired by the Sobol/Owen method
explained at https://pbr-book.org/4ed/Sampling_and_Reconstruction/Sobol_Samplers.
Only two angular coordinates change. Other counts and range sampling stay native.
"""
import re
from gi_single_producer import edit_export

def arithmetic(frame='%GI307Frame',pixel='%GI307SeedA',batch='%GI307BatchSeed',ray='%rayIndex.03'):
 lines=[];serial=0
 def op(code):
  nonlocal serial
  serial+=1;name='%G445S'+str(serial);lines.append('  '+name+' = '+code);return name
 def binary(code,a,b):return op(f'{code} i32 {a}, {b}')
 def hashed(x):
  for shift,mul in [(16,0x7feb352d),(15,0x846ca68b),(16,None)]:
   x=binary('xor',x,binary('lshr',x,shift))
   if mul is not None:x=binary('mul',x,mul)
  return x
 index=binary('add',binary('mul',frame,4),ray)
 gray=binary('xor',index,binary('lshr',index,1))
 a='0';b='0';v=1<<31
 for bit in range(32):
  on=binary('and',binary('lshr',gray,bit),1)
  a=binary('xor',a,binary('mul',on,1<<(31-bit)))
  b=binary('xor',b,binary('mul',on,v));v^=v>>1
 out=[]
 for value,salt in [(a,0x68bc21eb),(b,0x02e5be93)]:
  seed=hashed(binary('xor',binary('xor',pixel,batch),salt));result='0'
  for bit in range(31,7,-1):
   mask=((0xffffffff<<(bit+1))&0xffffffff) if bit<31 else 0
   prefix=binary('and',value,mask)
   flip=binary('and',hashed(binary('xor',binary('xor',seed,prefix),(bit*0x9e3779b9)&0xffffffff)),1)
   digit=binary('xor',binary('and',binary('lshr',value,bit),1),flip)
   result=binary('or',result,binary('shl',digit,bit))
  bits=binary('lshr',result,8);f=op(f'uitofp i32 {bits} to float')
  out.append(op(f'fmul float {f}, 0x3E70000000000000'))
 return lines,out

def transform(source,variant):
 def patch(body):
  names=('Frc','Frc26') if variant==0 else ('Frc','Frc85');count=226 if variant==0 else 286
  originals=[re.search(r'^  %'+name+r' = .*$',body,re.M)[0] for name in names]
  old='\n'.join(originals)
  if old not in body:raise ValueError('Angular coordinate definitions moved')
  before=body[:body.index(old)]
  labels=re.findall(r'^([^ ;\n][^\n]*?):(?:\s*;.*)?$',before,re.M)
  if not labels:raise ValueError('No predecessor')
  pred='%'+labels[-1]
  sequence,out=arithmetic(ray='%rayIndex.03' if variant==0 else '%rayIndex2.038')
  entry=[originals[i].replace('%'+names[i]+' =','%G445Old'+str(i)+' =') for i in range(2)]
  entry += [f'  %G445Four = icmp eq i32 %{count}, 4','  br i1 %G445Four, label %G445Sequence, label %G445Merge','G445Sequence:']
  merge=['  br label %G445Merge','G445Merge:']
  merge += [f'  %{names[i]} = phi float [ {out[i]}, %G445Sequence ], [ %G445Old{i}, {pred} ]' for i in range(2)]
  # Only outgoing phi edges of the split block move. Preserve preheader edges.
  body=re.sub(r'( = phi [^\n]*)',lambda m:m[0].replace(', '+pred+' ]',', %G445Merge ]'),body)
  return body.replace(old,'\n'.join(entry+sequence+merge))
 return edit_export(source,'rayGenMain',patch)

def hlsl():
 lines,outputs=arithmetic(frame='%frame',pixel='%pixel',batch='%batch',ray='%ray')
 out=[]
 for line in lines:
  name,expr=line.strip().split(' = ');name=name[1:]
  expr=expr.replace('%','')
  if expr.startswith('uitofp'):text='float('+expr.split()[2]+')';typ='float'
  elif expr.startswith('fmul'):text=expr.split()[2].rstrip(',')+' * (1.0f/16777216.0f)';typ='float'
  else:
   op,_,args=expr.split(' ',2);a,b=args.split(', ');a=a+'u' if a.isdigit() else a;b=b+'u' if b.isdigit() else b
   text=f'({a} '+{'add':'+','mul':'*','xor':'^','and':'&','or':'|','lshr':'>>','shl':'<<'}[op]+f' {b})';typ='uint'
  out.append(f' {typ} {name}={text};')
 return 'float2 Progressive(uint frame,uint pixel,uint batch,uint ray){\n'+'\n'.join(out)+'\n return float2('+','.join(x[1:] for x in outputs)+');\n}\n'
