"""Instrument the pinned owned evaluator without replacing its lighting arithmetic."""
import argparse, hashlib, pathlib, re
ROOT=pathlib.Path(__file__).resolve().parents[1]
PINS={'gi_evaluator_secondary.generated.hlsl':'20f8d07d5ba8383ef4b211da7af9d6c7e2d776c7dcd3b60f32d8705346557eab',
      'gi_owned_lighting.hlsl':'c208beb15a32855be5506f9d1c9a586b38e5f44d00fe1c8e832c841ce244174f'}
BEGIN='// OGI_TRACE_BEGIN\n'; END='// OGI_TRACE_END\n'
def block(s): return BEGIN+s.rstrip()+'\n'+END
def erase(s): return re.sub(re.escape(BEGIN)+r'.*?'+re.escape(END),'',s,flags=re.S)
def insert(s,anchor,code,before=False):
    if s.count(anchor)!=1: raise ValueError('Ambiguous trace anchor: '+anchor)
    return s.replace(anchor,block(code)+anchor if before else anchor+block(code))
def generate(secondary,entry):
    for name,text in [('gi_evaluator_secondary.generated.hlsl',secondary),('gi_owned_lighting.hlsl',entry)]:
        if hashlib.sha256(text.encode()).hexdigest()!=PINS[name]:raise ValueError('Unreviewed source: '+name)
    s=insert(secondary,'struct GIShadingRegisters {\n','''    float3 ogiVolume, ogiPoint, ogiSpot, ogiWorld, ogiNormal;
    float ogiDistance;
    uint ogiMask;''')
    def at(n,code,before=False):
        nonlocal s
        anchor=next(x+'\n' for x in s.splitlines() if x.endswith('// native dword '+str(n)))
        s=insert(s,anchor,code,before)
    at(995,'''    state.ogiVolume=0; state.ogiPoint=0; state.ogiSpot=0;
    state.ogiWorld=0; state.ogiNormal=0; state.ogiDistance=0; state.ogiMask=4;''',True)
    at(1051,'    state.ogiWorld=asfloat(state.r12.xyz); state.ogiNormal=asfloat(state.r13.xyz);')
    at(2281,'    state.ogiDistance=asfloat(state.r14.w);')
    at(2302,'    state.ogiVolume=asfloat(state.r15.xyz); state.ogiMask=1;')
    at(3807,'    state.ogiPoint=asfloat(state.r14.xyz);')
    at(4982,'    state.ogiSpot=mad(asfloat(state.r14.w),asfloat(state.r20.xyz),state.ogiSpot);')
    at(4999,'''    state.ogiVolume=asfloat(state.r17.xyz)*(asfloat(state.r4.xyz)*asfloat(0x3ea2f983u));
    state.ogiPoint*=asfloat(state.r17.xyz);
    state.ogiSpot*=asfloat(state.r17.xyz);
    state.ogiMask=2;''',True)
    if erase(s)!=secondary:raise ValueError('Instrumentation changed native source')
    # Replace include only; all original statements must survive byte-for-byte.
    e=entry.replace('"gi_evaluator_secondary.generated.hlsl"','"gi_owned_source_secondary.generated.hlsl"')
    e=insert(e,'#include "gi_owned_source_secondary.generated.hlsl"\n',
             '#include "../src/shaders/gi_owned_source_recorder.hlsl"')
    e=insert(e,'        g_rwtHitInfo[id.xy] = 65504.0f;\n','        OGIInvalid(id.xy);')
    e=insert(e,'    float3 radiance = 0;\n','''    uint ogiBase=OGIReserve(id.xy,uint(rays)+1);
    float3 ogiOrigin=asfloat(receiver.r1.xyz), ogiNormal=asfloat(receiver.r7.xyz);
    float3 ogiVolumeSum=0, ogiPointSum=0, ogiSpotSum=0, ogiMissSum=0;''')
    e=insert(e,'        radiance = mad(throughput, asfloat(receiver.r15.xyz), radiance);\n','''        if(ogiBase!=0xffffffffu) {
            ogiVolumeSum=mad(throughput,receiver.ogiVolume,ogiVolumeSum);
            ogiPointSum=mad(throughput,receiver.ogiPoint,ogiPointSum);
            ogiSpotSum=mad(throughput,receiver.ogiSpot,ogiSpotSum);
            if(receiver.ogiMask==1) ogiMissSum=mad(throughput,asfloat(receiver.r15.xyz),ogiMissSum);
            OGIRay(ogiBase+uint(slice)*256,id.xy,uint(slice),uint(rays),receiver,hit,
                   throughput,depth,ogiOrigin,ogiNormal,radiance);
        }''')
    e=insert(e,'    g_rwtHitInfo[id.xy] = asfloat(receiver.r5.w) / float(rays);\n','''    if(ogiBase!=0xffffffffu) OGIFinal(ogiBase+uint(rays)*256,id.xy,uint(rays),
        (scale*radiance)/float(rays),asfloat(receiver.r5.w)/float(rays),throughput,depth,
        (scale*ogiVolumeSum)/float(rays),(scale*ogiPointSum)/float(rays),
        (scale*ogiSpotSum)/float(rays),(scale*ogiMissSum)/float(rays),scale,radiance,ogiOrigin,ogiNormal);''')
    if erase(e).replace('"gi_owned_source_secondary.generated.hlsl"','"gi_evaluator_secondary.generated.hlsl"')!=entry:
        raise ValueError('Instrumentation changed owned entry')
    return s,e
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--out',type=pathlib.Path,default=ROOT/'build');args=p.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    s,e=generate(*[(ROOT/'src/shaders'/name).read_text() for name in PINS])
    (args.out/'gi_owned_source_secondary.generated.hlsl').write_text(s)
    (args.out/'gi_owned_source.generated.hlsl').write_text(e)
    print('Owned source trace: pinned source verified; original arithmetic preserved.')
