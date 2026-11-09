"""Follow actual DXIL SSA dependencies for reflection-position stores.
Requires the untouched 32-container corpus and Microsoft's dxc executable.
Checks both writers against an independent matrix reference; never executes DXIL.
"""
import pathlib,sys,re,subprocess,hashlib,json,random,math
corpus=pathlib.Path(sys.argv[1]);dxc=pathlib.Path(sys.argv[2]);out=pathlib.Path(sys.argv[3]);out.mkdir(parents=True,exist_ok=True)
var=r'%[-\w.]+'
functions=['reflectionClosestHit','reflectionMiss','reflectionRayGeneration']
def function(s,name):
 m=re.search(r'^define [^\n]*\?'+name+r'@@[^\n]*\n(.*?)^}',s,re.M|re.S)
 if not m:raise ValueError('function missing '+name)
 return m[1]
def store(f,resource):
 load=re.search(r'('+var+r') = load [^\n]*@"[^\n]*\?'+resource+r'@@',f)
 if not load:raise ValueError('resource load missing '+resource)
 handles=re.findall(r'('+var+r') = call %dx.types.Handle @[^\n]*\(i32 160, [^\n]* '+re.escape(load[1])+r'\)',f)
 assert len(handles)==1,(resource,handles)
 stores=[l.strip() for l in f.splitlines() if 'call void @dx.op.textureStore' in l and '%dx.types.Handle '+handles[0]+',' in l]
 assert len(stores)==1,(resource,stores)
 return stores[0]
def writer(f,kind,rng):
 line=store(f,'g_rwtPosition_TexcoordY')
 values=re.findall(r'float ('+var+r'|[-+0-9.e]+|0x[0-9A-F]+)',line)
 assert len(values)==4
 definitions=dict(re.findall(r'^  ('+var+r') = (.*)$',f,re.M))
 # Position and material writes use the same payload-derived array layer.
 material=store(f,'g_rwtMaterialId')
 layer=re.search(r'%dx.types.Handle '+var+r', i32 '+var+r', i32 '+var+r', i32 ('+var+r')',line)[1]
 assert ', i32 '+layer+',' in material
 loadptr=re.fullmatch(r'load i32, i32\* ('+var+r'), align 4,.*',definitions[layer])[1]
 assert 'getelementptr inbounds %struct.HitData, %struct.HitData* %payload, i32 0, i32 0' in definitions[loadptr]
 if kind=='reflectionMiss':assert 'i32 65535, i32 65535, i32 65535, i32 65535' in material
 worst=0
 for sample in range(64):
  origin=[rng.uniform(-1e3,1e3) for _ in range(3)];direction=[rng.uniform(-1,1) for _ in range(3)];t=rng.uniform(0,1e3)
  # Cbuffer registers hold columns. Translation occupies register 5.
  columns=[[rng.uniform(-2,2) for _ in range(3)] for _ in range(4)]
  cache={}
  def val(token):
   token=token.strip()
   if not token.startswith('%'):return float(token)
   if token in cache:return cache[token]
   s=definitions[token]
   m=re.match(r'f(mul|add|sub) fast float ([^,]+), ([^ ;]+)',s)
   if m:
    a,b=val(m[2]),val(m[3]);v=a*b if m[1]=='mul' else a+b if m[1]=='add' else a-b
   elif 'dx.op.worldRayOrigin.f32' in s:v=origin[int(re.search(r'i8 (\d)',s)[1])]
   elif 'dx.op.worldRayDirection.f32' in s:v=direction[int(re.search(r'i8 (\d)',s)[1])]
   elif 'dx.op.rayTCurrent.f32' in s:v=t
   elif 'dx.op.cbufferLoadLegacy.f32' in s:
    assert '%sys_constants,' in s
    reg=int(re.search(r', i32 (\d+)\)',s)[1]);assert 2<=reg<=5;v=columns[reg-2]
   elif s.startswith('extractvalue %dx.types.CBufRet.f32'):
    m=re.search(r'('+var+r'), (\d)',s);v=val(m[1])[int(m[2])]
   elif 'dx.op.tertiary.f32(i32 46,' in s:
    vlist=re.findall(r'float ('+var+r'|[-+0-9.e]+)',s);assert len(vlist)==3;v=val(vlist[0])*val(vlist[1])+val(vlist[2])
   else:raise ValueError('unsupported position dependency '+s)
   cache[token]=v;return v
  actual=[val(v) for v in values[:3]]
  world=[origin[i]+t*direction[i] for i in range(3)]
  expected=[sum(columns[j][i]*world[j] for j in range(3))+columns[3][i] for i in range(3)]
  error=max(abs(a-b)/(1+abs(b)) for a,b in zip(actual,expected));assert error<1e-10;worst=max(worst,error)
 return {'kind':kind,'position_store':line,'material_store':material,'matrix_trials':64,'worst_error':worst}
rng=random.Random(624892);report=[]
for index in range(32):
 p=corpus/f'rt_reflection-{index:03}.dxbc';raw=p.read_bytes()
 s=subprocess.check_output([str(dxc),'-dumpbin',str(p)],text=True)
 (out/(p.stem+'.ll')).write_text(s)
 expected=json.loads((pathlib.Path(__file__).parent/'native-shader-hashes.json').read_text())
 assert hashlib.sha256(raw).hexdigest()==expected[p.name], 'native shader SHA mismatch'
 writer_present='g_rwtPosition_TexcoordY' in s
 assert writer_present==(index%2==0)
 if writer_present:assert re.search(r'g_mWorldToView;\s*; Offset:\s*32',s), 'position matrix metadata mismatch'
 entry={'file':p.name,'sha256':hashlib.sha256(raw).hexdigest(),'position_writer':writer_present,'writers':[]}
 for name in functions[:2]:
  f=function(s,name)
  if writer_present:entry['writers'].append(writer(f,name,rng))
  else:assert not any(l.strip() and not l.strip().startswith(';') and l.strip()!='ret void' for l in f.splitlines()),name
 raygen=function(s,functions[2]);entry['adaptive_end_marker']=index>=16
 definitions=dict(re.findall(r'^  ('+var+r') = (.*)$',raygen,re.M))
 traces=[l for l in raygen.splitlines() if 'call void @dx.op.traceRay.struct.HitData' in l and ', i32 0, i32 1, i32 0, i32 2, i32 0,' in l]
 assert len(traces)==1
 payload=re.search(r'%struct.HitData\* nonnull ('+var+r')\)',traces[0])[1]
 ptr=[v for v,d in definitions.items() if d=='getelementptr inbounds %struct.HitData, %struct.HitData* '+payload+', i32 0, i32 0']
 assert len(ptr)==1
 layer=re.search(r'store i32 ('+var+r'), i32\* '+re.escape(ptr[0])+r', align 8',raygen)[1]
 phi=definitions[layer];assert phi.startswith('phi i32 ') and '[ 0,' in phi
 increment=re.search(r'phi i32 \[ ('+var+r'),',phi)[1]
 assert definitions[increment]=='add nuw nsw i32 '+layer+', 1'
 entry['layer_rule']='payload receives zero-based reflection-ray loop index, incremented by one'
 if index==0:
  original=function(s,'reflectionClosestHit')
  miss=function(s,'reflectionMiss')
  for mutated,name in [(original.replace('fmul fast float %RayTCurrent, %WorldRayDirection','fadd fast float %RayTCurrent, %WorldRayDirection'), 'reflectionClosestHit'),(miss.replace('i32 65535','i32 65534'),'reflectionMiss')]:
   try:writer(mutated,name,rng)
   except (AssertionError,ValueError):pass
   else:raise AssertionError('bad writer mutation accepted')
  entry['negative_checks']=['wrong hit-position math rejected','wrong miss sentinel rejected']
 if index>=16:
  marker=store(raygen,'g_rwtMaterialId');assert 'i32 65534, i32 65534, i32 65534, i32 65534' in marker
  entry['end_marker_store']=marker
 else:assert 'i32 65534' not in raygen
 report.append(entry)
(out/'writer-audit.json').write_text(json.dumps({'containers':report,'scope':'static SSA dataflow; not runtime identity/lifetime proof','checks':'32 variants; 16 position writers; 2048 writer matrix trials'},indent=2)+'\n')
print('PASS 32 variants; 16 position writers; 2048 independent writer matrix trials; miss and end-marker stores')
