"""Build paired automatic GI shaders, retaining the accepted R442 combined-load optimization."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--output',default='build');p.add_argument('--assembler');p.add_argument('--regional',action='store_true');p.add_argument('--r495',action='store_true');p.add_argument('--ray-clamp',type=float);p.add_argument('--r495-stages',action='store_true');a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
secondary=(ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text()
if a.regional:
 from gi_secondary_filter import patch
 secondary=patch(secondary)
(out/'gi_auto_secondary.generated.hlsl').write_text(secondary)
lighting=out/'gi-auto-lighting.hlsl';lightingText=((ROOT/('src/shaders/gi_regional_lighting.hlsl' if a.regional else 'src/shaders/gi_auto_lighting.hlsl')).read_text().replace('"gi_evaluator_secondary.generated.hlsl"','"gi_auto_secondary.generated.hlsl"'))
if a.ray_clamp is not None:
 if not (1.0<=a.ray_clamp<=64.0):raise ValueError('ray clamp out of range')
 lighting.write_text('#define GI_RAY_CLAMP %.4ff\n'%a.ray_clamp+lightingText)
 # Unmodified reference, used only to report whether this tree reproduces R495.
 (out/'gi-auto-lighting-unclamped.hlsl').write_text(lightingText)
else:
 lighting.write_text(lightingText)
stages=[('Raygen',ROOT/('src/shaders/gi_regional_raygen.hlsl' if a.regional else 'src/shaders/gi_auto_raygen.hlsl'),'lib_6_3',None),('Lighting',lighting,'cs_6_0','main')]+[(n,ROOT/('src/shaders/gi_regional_target_build.hlsl' if a.regional else 'src/shaders/gi_auto_target_build.hlsl'),'cs_6_0',n) for n in (['Clear','Claim','First','Second','Freeze'] if a.regional else ['Clear','Claim','Accumulate','Freeze'])]
if a.regional:
 (out/'gi_compare_secondary.generated.hlsl').write_text(patch((ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text(),paired=True))
 compare=out/'gi-paired-compare.hlsl';compare.write_text((ROOT/'src/shaders/gi_paired_compare.hlsl').read_text().replace('"gi_evaluator_secondary.generated.hlsl"','"gi_compare_secondary.generated.hlsl"'))
 stages.append(('Compare',compare,'cs_6_0','main'))
arrays=[];manifest={}
for name,source,target,entry in stages:
 binary=out/('gi-auto-'+name.lower()+'.dxil');command=[a.dxc,'-T',target,'-HV','2018','-O3','-WX','-I',str(ROOT/'src/shaders'),str(source),'-Fo',str(binary)]
 if entry:command+=['-E',entry]
 subprocess.run(command,check=True)
 if name=='Lighting' and a.r495:
  binary.write_bytes((ROOT/'research/gi-r495/lighting.dxil').read_bytes())
 elif name=='Lighting' and a.assembler:
  reference=binary.with_name('gi-auto-lighting-reference.dxil');reference.write_bytes(binary.read_bytes())
  subprocess.run([sys.executable,str(ROOT/'tools/gi_evaluator_loads.py'),'--dxc',a.dxc,'--assembler',a.assembler,'--input',str(reference),'--output',str(binary)],check=True)
 if name=='Compare':
  dump=subprocess.check_output([a.dxc,'-dumpbin',str(binary)],text=True)
  for forbidden in ('dx.op.textureStore','dx.op.traceRay','dx.op.atomicBinOp','dx.op.atomicCompareExchange'):
   if forbidden in dump:raise RuntimeError('Comparison shader contains forbidden side effect: '+forbidden)
 data=binary.read_bytes();assert data[:4]==b'DXBC'
 arrays.append('inline constexpr unsigned char k'+name+'[]={'+','.join(map(str,data))+'};')
 manifest[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
if a.r495_stages:
 pinned=json.loads((ROOT/'research/gi-r495/manifest.json').read_text())
 for name in manifest:
  if name!='Lighting' and manifest[name]!=pinned[name]:raise ValueError('R495 GI stage differs: '+name)
 if a.ray_clamp is not None:
  ref=out/'gi-auto-lighting-unclamped.dxil'
  subprocess.run([a.dxc,'-T','cs_6_0','-HV','2018','-O3','-WX','-I',str(ROOT/'src/shaders'),str(out/'gi-auto-lighting-unclamped.hlsl'),'-Fo',str(ref),'-E','main'],check=True)
  if a.assembler:
   fast=out/'gi-auto-lighting-unclamped-fast.dxil'
   subprocess.run([sys.executable,str(ROOT/'tools/gi_evaluator_loads.py'),'--dxc',a.dxc,'--assembler',a.assembler,'--input',str(ref),'--output',str(fast)],check=True);ref=fast
  digest=hashlib.sha256(ref.read_bytes()).hexdigest()
  print('R495_LIGHTING_IDENTITY unclamped_source_matches_pinned=%d sha256=%s pinned=%s'%(digest==pinned['Lighting']['sha256'],digest,pinned['Lighting']['sha256']))
  print('R495_RAY_CLAMP k=%.4f lighting_sha256=%s'%(a.ray_clamp,manifest['Lighting']['sha256']))
if a.r495 and manifest!=json.loads((ROOT/'research/gi-r495/manifest.json').read_text()):
 raise ValueError('The R495 baseline GI shader set differs')
(out/'gi_auto.generated.h').write_text('#pragma once\nnamespace gi_auto_shader {\n'+'\n'.join(arrays)+'\n}\n')
(out/'gi-auto-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
