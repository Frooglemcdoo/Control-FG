"""Candidate: skip point-light shadow lookup only when its existing cosine is zero.
Native light lists, range attenuation, BRDF, accumulation and rays stay intact.
"""
import argparse,re,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OLD='    if((state.r24.yyyy).x != 0u) { // native dword 3080'
NEW='    [branch] if((state.r24.yyyy).x != 0u && saturate(dot(asfloat(state.r13.xyz),asfloat(state.r23.xyz))) != 0.0f) { // native dword 3080'
def transform(source):
    if source.count(OLD)!=1:raise ValueError('Point shadow branch changed')
    # Same operands as the final native cosine. Neither is written in shadow block.
    a=source.index(OLD);b=source.index('// native dword 3669',a)
    for line in source[a:b].splitlines():
        m=re.match(r'\s*state\.r(13|23)\.([xyzw]+)=',line)
        if m and set(m[2])&set('xyz'):raise ValueError('Cosine operands modified')
    assert 'saturate(dot(asfloat(state.r13.xyzx).xyz,asfloat(state.r23.xyzx).xyz)' in source
    return source.replace(OLD,NEW)
def fixture(source):
    lines=source.splitlines();a=next(i for i,x in enumerate(lines) if x.endswith('// native dword 2826'));b=next(i for i,x in enumerate(lines) if x.endswith('// native dword 3743'))
    body='\n'.join(lines[a:b+1]);body=re.sub(r'g_sbDeferredLightPointData\[\(state.r14.wwww\).x\]','record',body)
    body=body.replace('g_tPointLightShadowAtlas.SampleLevel(g_sLinearClamp,asfloat(state.r21.xyxx).xy,asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x)','Shadow(asfloat(state.r21.xy),mode)')
    body=body.replace('    state.r21.xy=(asuint(Shadow','    ++calls;\n    state.r21.xy=(asuint(Shadow')
    gated=transform(body+'\n')
    return '''struct GIShadingRegisters {'''+''.join('uint4 r%d;'%i for i in range(30))+'''};
struct GINativeRaw7 {uint d[40];};
float4 Shadow(float2 uv,uint mode){float m=frac(abs(uv.x*0.73f+uv.y*0.37f));return float4(m,m*m+(mode%5)*0.001f,0,0);}
void Reference(inout GIShadingRegisters state,GINativeRaw7 record,uint mode,out uint calls){calls=0;
'''+body+'''\n}
void Candidate(inout GIShadingRegisters state,GINativeRaw7 record,uint mode,out uint calls){calls=0;
'''+gated+'''\n}
RWStructuredBuffer<uint4> output:register(u0);
uint Hash(uint x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
float F(uint n){return (Hash(n)&65535)/65535.0f;}
[numthreads(64,1,1)]void main(uint3 id:SV_DispatchThreadID){
 uint n=id.x;if(n>=65536)return;
 GIShadingRegisters a=(GIShadingRegisters)0;
 GINativeRaw7 record=(GINativeRaw7)0;
 float3 pos=float3(F(n*11+1),F(n*11+2),F(n*11+3))*20-10;
 float3 delta=float3(F(n*13+4),F(n*13+5),F(n*13+6))*8-4;
 if(dot(delta,delta)<0.001f)delta=float3(1,2,3);
 float3 light=pos+delta;float3 normal=normalize(delta)*(n&1?1:-1);
 if((n&7)==0)normal=normalize(float3(F(n+33)-0.5f,F(n+39)-0.5f,F(n+71)-0.5f));
 if((n&63)==0){delta=float3(2,0,0);light=pos+delta;normal=float3(0,1,0);}
 a.r12.xyz=asuint(pos);a.r13.xyz=asuint(normal);a.r9=asuint(float4(pos+normal*0.07f,1));
 a.r11.xyz=asuint(normalize(float3(0.3f,0.4f,1)));a.r7.w=asuint(F(n+7));a.r11.w=asuint(1-asfloat(a.r7.w));
 a.r14.xyz=asuint(float3(F(n+9),F(n+19),F(n+29)));a.r14.w=0;
 for(uint i=0;i<40;++i)record.d[i]=asuint(F(n*41+i));
 record.d[0]=asuint(light.x);record.d[1]=asuint(light.y);record.d[2]=asuint(light.z);record.d[3]=asuint(0.95f);
 record.d[4]=asuint(F(n+3)*20);record.d[5]=asuint(F(n+4)*20);record.d[6]=asuint(F(n+5)*20);record.d[7]=asuint(0.01f+F(n+6)*20);
 record.d[9]=asuint(F(n+10)*0.1f);record.d[10]=asuint(1.2f);record.d[11]=asuint(0.7f);
 record.d[12]=asuint(0.0f);record.d[13]=asuint(0.0f);record.d[14]=asuint(1.0f);record.d[15]=asuint(1.0f);
 // Identity world-to-light transform, with nonzero vectors across all cube faces.
 record.d[16]=asuint(1.0f);record.d[17]=0;record.d[18]=0;
 record.d[20]=0;record.d[21]=asuint(1.0f);record.d[22]=0;
 record.d[24]=0;record.d[25]=0;record.d[26]=asuint(1.0f);
 record.d[28]=asuint(0.3f);record.d[29]=asuint(0.4f);record.d[30]=asuint(0.5f);
 record.d[32]=asuint(0.1f);record.d[33]=asuint(0.2f);record.d[34]=asuint(0.5f);record.d[35]=asuint(0.6f);
 record.d[36]=asuint(0.01f+F(n+31));record.d[39]=(n%3==0?0:2)|(n%5==0?4:0);
 GIShadingRegisters b=a;uint ca,cb;Reference(a,record,n,ca);Candidate(b,record,n,cb);
 output[n*3]=uint4(a.r14.xyz,ca);output[n*3+1]=uint4(b.r14.xyz,cb);
 output[n*3+2]=uint4(asuint(saturate(dot(normal,normalize(delta)))),record.d[39],0,0);
}
'''
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--fixture',type=Path);p.add_argument('--output',type=Path);p.add_argument('--kernels',type=Path);a=p.parse_args()
 source=(ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl').read_text()
 if a.fixture:a.fixture.write_text(fixture(source))
 if a.output:a.output.write_text(transform(source))

 if a.kernels:
  a.kernels.mkdir(parents=True,exist_ok=True)
  (a.kernels/'gi_point_secondary.generated.hlsl').write_text(transform(source))
  for name in ['gi_owned_lighting.hlsl','gi_guided_lighting.hlsl']:
   kernel=(ROOT/'src/shaders'/name).read_text().replace('"gi_evaluator_secondary.generated.hlsl"','"gi_point_secondary.generated.hlsl"')
   (a.kernels/name).write_text(kernel)
