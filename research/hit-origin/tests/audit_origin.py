"""Interpret native DXIL SSA for primary view position; no GPU execution claimed."""
from pathlib import Path
import subprocess,re,json,hashlib,random,sys
corpus,dxc,hashes,out=map(Path,sys.argv[1:]);out.mkdir(exist_ok=True,parents=True)
expected=json.loads(hashes.read_text());rng=random.Random(417190)
var=r'%[-\w.]+';reports=[];fixtures=[]
def analyze(text,samples,emit):
 f=re.search(r'^define [^\n]*\?reflectionRayGeneration@@[^\n]*\n(.*?)^}',text,re.M|re.S)[1]
 definitions=dict(re.findall(r'^  ('+var+r') = (.*)$',f,re.M))
 reciprocal=re.search(r'^  ('+var+r') = fdiv fast float 1\.000000e\+00, ('+var+r')',f,re.M)
 assert reciprocal
 roots=re.findall(r'^  ('+var+r') = fmul fast float '+re.escape(reciprocal[1])+', '+var,f,re.M)[:3]
 assert len(roots)==3
 assert re.search(r'g_mClipToView;\s*; Offset:\s*224',text)
 assert re.search(r'g_vInvOutputRes;\s*; Offset:\s*24',text)
 for sample in samples:
  px,py,ix,iy,depth,columns=sample;cache={};seen=set()
  def val(token):
   if not token.startswith('%'):return float(token)
   if token in cache:return cache[token]
   s=definitions[token]
   m=re.match(r'f(mul|add|sub|div) fast float ([^,]+), ([^ ;]+)',s)
   if m:
    a,b=val(m[2]),val(m[3]);v={'mul':lambda:a*b,'add':lambda:a+b,'sub':lambda:a-b,'div':lambda:a/b}[m[1]]()
   elif s.startswith('uitofp i32 '):v=float(val(re.search(r'uitofp i32 ('+var+')',s)[1]))
   elif 'dx.op.dispatchRaysIndex.i32' in s:v=(px,py)[int(re.search(r'i8 (\d)',s)[1])]
   elif 'dx.op.cbufferLoadLegacy.f32' in s:
    assert '%sys_constants,' in s
    reg=int(re.search(r', i32 (\d+)\)',s)[1]);seen.add(reg)
    assert reg in (1,14,15,16,17),reg
    v=[0,0,ix,iy] if reg==1 else columns[reg-14]
   elif s.startswith('extractvalue '):
    m=re.search(r'('+var+r'), (\d)',s);v=val(m[1])[int(m[2])]
   elif 'dx.op.tertiary.f32(i32 46,' in s:
    args=re.findall(r'float ('+var+r'|[-+0-9.e]+)',s);assert len(args)==3;v=val(args[0])*val(args[1])+val(args[2])
   elif 'dx.op.textureLoad.f32' in s:
    handle=re.search(r'%dx.types.Handle ('+var+')',s)[1]
    load=re.search(r' ('+var+r')\)\s*;',definitions[handle])[1]
    assert '?g_tClipDepth@@' in definitions[load]
    v=[depth,0,0,0]
   else:raise AssertionError('unknown primary dependency '+s)
   cache[token]=v;return v
  actual=[val(root) for root in roots];assert seen=={1,14,15,16,17}
  clip=[(px+.5)*ix*2-1,1-(py+.5)*iy*2,depth,1]
  homogeneous=[sum(clip[j]*columns[j][i] for j in range(4)) for i in range(4)]
  reference=[v/homogeneous[3] for v in homogeneous[:3]]
  assert max(abs(a-b) for a,b in zip(actual,reference))<1e-10
  if emit:fixtures.append([px,py,ix,iy,depth,*[v for col in columns for v in col],*actual])
 return roots
for index in range(32):
 p=corpus/f'rt_reflection-{index:03}.dxbc';assert hashlib.sha256(p.read_bytes()).hexdigest()==expected[p.name]
 text=subprocess.check_output([str(dxc.resolve()),'-dumpbin',str(p.resolve())],text=True)
 samples=[]
 for case in range(64):
  width,height=rng.choice([(2560,1440),(1920,1080),(1280,720)])
  columns=[[rng.uniform(-2,2) for _ in range(4)] for _ in range(4)]
  for col in columns[:3]:col[3]=rng.uniform(-.1,.1)
  columns[3][3]=1.2
  samples.append((rng.randrange(width),rng.randrange(height),1/width,1/height,rng.uniform(.001,.999),columns))
 roots=analyze(text,samples,True)
 if index==0:
  bad=text.replace('%sys_constants, i32 14)','%sys_constants, i32 13)')
  try:analyze(bad,samples[:1],False)
  except AssertionError:pass
  else:raise AssertionError('wrong constant register was accepted')
 reports.append({'shader':p.name,'sha256':expected[p.name],'primary_roots':roots,'cases':len(samples)})
(out/'native-origin-fixtures.txt').write_text('\n'.join(' '.join(format(v,'.17g') for v in row) for row in fixtures)+'\n')
(out/'native-origin-audit.json').write_text(json.dumps({'status':'PASS','variants':reports,'cases':len(fixtures),'source':'native DXIL SSA interpreter compared to independent homogeneous matrix formula','inv_resolution':'sys_constants register 1 components 2/3: g_vInvOutputRes','matrix':'sys_constants registers 14..17: g_mClipToView','negative_check':'wrong matrix register rejected','limits':'Does not prove which constants, resource or shader variant are bound in a running frame.'},indent=2)+'\n')
print('PASS native primary-position SSA:',len(reports),'variants,',len(fixtures),'cases')
