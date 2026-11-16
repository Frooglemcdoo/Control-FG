"""Build paired automatic GI shaders, retaining the accepted point-shadow gate."""
import argparse,hashlib,json,subprocess
from pathlib import Path
from gi_point_shadow_gate import transform
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);p.add_argument('--output',default='build');a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
(out/'gi_auto_secondary.generated.hlsl').write_text(transform((ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text()))
lighting=out/'gi-auto-lighting.hlsl';lighting.write_text((ROOT/'src/shaders/gi_auto_lighting.hlsl').read_text().replace('"gi_evaluator_secondary.generated.hlsl"','"gi_auto_secondary.generated.hlsl"'))
stages=[('Raygen',ROOT/'src/shaders/gi_auto_raygen.hlsl','lib_6_3',None),('Lighting',lighting,'cs_6_0','main')]+[(n,ROOT/'src/shaders/gi_auto_target_build.hlsl','cs_6_0',n) for n in ['Clear','Claim','Accumulate','Freeze']]
arrays=[];manifest={}
for name,source,target,entry in stages:
 binary=out/('gi-auto-'+name.lower()+'.dxil');command=[a.dxc,'-T',target,'-HV','2018','-O3','-WX','-I',str(ROOT/'src/shaders'),str(source),'-Fo',str(binary)]
 if entry:command+=['-E',entry]
 subprocess.run(command,check=True);data=binary.read_bytes();assert data[:4]==b'DXBC'
 arrays.append('inline constexpr unsigned char k'+name+'[]={'+','.join(map(str,data))+'};')
 manifest[name]={'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()}
(out/'gi_auto.generated.h').write_text('#pragma once\nnamespace gi_auto_shader {\n'+'\n'.join(arrays)+'\n}\n')
(out/'gi-auto-manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
