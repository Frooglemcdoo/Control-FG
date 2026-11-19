"""Build paired automatic GI shaders, retaining the accepted R442 combined-load optimization."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--output',default='build');p.add_argument('--assembler');p.add_argument('--regional',action='store_true');a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
secondary=(ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text()
if a.regional:
 from gi_secondary_filter import patch
 secondary=patch(secondary)
(out/'gi_auto_secondary.generated.hlsl').write_text(secondary)
lighting=out/'gi-auto-lighting.hlsl';lighting.write_text((ROOT/('src/shaders/gi_regional_lighting.hlsl' if a.regional else 'src/shaders/gi_auto_lighting.hlsl')).read_text().replace('"gi_evaluator_secondary.generated.hlsl"','"gi_auto_secondary.generated.hlsl"'))
stages=[('Raygen',ROOT/('src/shaders/gi_regional_raygen.hlsl' if a.regional else 'src/shaders/gi_auto_raygen.hlsl'),'lib_6_3',None),('Lighting',lighting,'cs_6_0','main')]+[(n,ROOT/('src/shaders/gi_regional_target_build.hlsl' if a.regional else 'src/shaders/gi_auto_target_build.hlsl'),'cs_6_0',n) for n in (['Clear','Claim','First','Second','Freeze'] if a.regional else ['Clear','Claim','Accumulate','Freeze'])]
arrays=[];manifest={}
for name,source,target,entry in stages:
 binary=out/('gi-auto-'+name.lower()+'.dxil');command=[a.dxc,'-T',target,'-HV','2018','-O3','-WX','-I',str(ROOT/'src/shaders'),str(source),'-Fo',str(binary)]
 if entry:command+=['-E',entry]
 subprocess.run(command,check=True)
 if name=='Lighting' and a.assembler:
  reference=binary.with_name('gi-auto-lighting-reference.dxil');reference.write_bytes(binary.read_bytes())
  subprocess.run([sys.executable,str(ROOT/'tools/gi_evaluator_loads.py'),'--dxc',a.dxc,'--assembler',a.assembler,'--input',str(reference),'--output',str(binary)],check=True)
 data=binary.read_bytes();assert data[:4]==b'DXBC'
 arrays.append('inline constexpr unsigned char k'+name+'[]={'+','.join(map(str,data))+'};')
 manifest[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
(out/'gi_auto.generated.h').write_text('#pragma once\nnamespace gi_auto_shader {\n'+'\n'.join(arrays)+'\n}\n')
(out/'gi-auto-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
