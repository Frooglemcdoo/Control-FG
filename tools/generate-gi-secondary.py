"""Separate secondary shading from GI6 without changing its native reconstruction.
Uses the complete hash-pinned reference translation, not a shortened lighting model.
The sample's material/UV/normal/position are explicit; receiver registers originate
from the target pixel. No source RGB is reinterpreted as reusable radiance.
"""
import pathlib,re,sys,hashlib
ROOT=pathlib.Path(__file__).resolve().parent.parent
NATIVE='1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de'

def generate(source):
    if hashlib.sha256(source.encode()).hexdigest()!='a25457fbf63a3fee91066f1bd67980c47d2e87c70117fc4cba7d645404390a91':
        raise ValueError('Reference translation changed; review before extraction')
    if not source.startswith('// GENERATED native GI6 reference: '+NATIVE+'\n'):
        raise ValueError('Unrecognized evaluator source')
    lines=source.splitlines()
    def index(at):
        found=[n for n,line in enumerate(lines) if line.endswith('// native dword '+str(at))]
        if len(found)!=1: raise ValueError('Missing or repeated native instruction '+str(at))
        return found[0]
    first,last=index(995),index(5028)
    body=lines[first:last+1]
    if body[0].strip()!='r7.w=(g_tMaterialId.Load(asint(r0.xyzw).xyzw)).yzwx.w; // native dword 995':
        raise ValueError('Material load changed')
    replacements={995:'r7.w=sample.material;',1003:'r11=sample.positionUV;',1051:'r13=sample.normalUV;'}
    expected={1003:'r11.xyzw=(asuint(g_tPosition_TexcoordY.Load(asint(r0.xyzw).xyzw))).xyzw.xyzw;',
              1051:'r13.xyzw=(asuint(g_tNormal_TexcoordX.Load(asint(r0.xyzw).xyzw))).xyzw.xyzw;'}
    for at,text in expected.items():
        if lines[index(at)].strip()!=text+' // native dword '+str(at):raise ValueError('Sample load changed')
    def registers(line):return re.sub(r'\br(\d+)\b',r'state.r\1',line)
    for n,line in enumerate(body):
        at=int(re.search(r'native dword (\d+)$',line)[1])
        if at in replacements:body[n]='    '+replacements[at]+' // native dword '+str(at)
    # Each original branch and nested light/volume loop remains in order.
    if sum(x.count('{')-x.count('}') for x in body)!=0:raise ValueError('Unbalanced secondary block')
    resources='\n'.join(lines[:lines.index('[numthreads(8,16,1)]')])
    declarations=['#ifdef GI_SECONDARY_TERMS', '#include "gi_secondary_terms.hlsl"', '#endif', 'struct GIShadingRegisters {', '#ifdef GI_SECONDARY_TERMS', '    bool traceEnabled; GISecondaryTerms terms;', '#endif']+['    uint4 r'+str(i)+';' for i in range(30)]+['};']
    declarations+=['struct GISecondaryInput { uint material; uint4 positionUV; uint4 normalUV; };',
        '// Receiver registers must be initialized by the target primary shading.',
        '// The input normal is a shading normal, never a geometric PDF normal.',
        'void GIEvaluateSecondaryCore(inout GIShadingRegisters state,GISecondaryInput sample,',
        '                             bool useWorldPosition,float3 worldPosition) {']
    hooks={
        1086:'terms.texture.w=state.r10.w!=0u?1:3;',
        2193:'if(state.r23.x==0xffffffffu){terms.tint.w+=1;terms.fallback.xyz+=asfloat(state.r17.xyz)*asfloat(state.r17.w);terms.fallback.w+=asfloat(state.r17.w);}',
        2250:'terms.volume=float4(asfloat(state.r4.xyz),asfloat(state.r12.w));',
        2302:'terms.brdf.w=asfloat(state.r3.w);',
        2310:'terms.tint.xyz=asfloat(state.r17.xyz);',
        2338:'terms.texture.xyz=asfloat(state.r20.xyz);',
        2377:'terms.texture.w=2;terms.surface.w=asfloat(state.r10.w);',
        2582:'terms.brdf.xyz=asfloat(state.r14.xyz);',
        2589:'terms.surface.xyz=asfloat(state.r4.xyz)*asfloat(0x3ea2f983u);',
        3774:'terms.point=float4(asfloat(state.r14.xyz),float(state.r13.w));',
        4998:'terms.spot=float4(asfloat(state.r9.xyz)-terms.point.xyz,float(state.r6.x));',
    }
    declarations += ['#ifdef GI_SECONDARY_TERMS', '    state.terms=(GISecondaryTerms)0;', '#endif']
    for line in body:
        declarations.append(registers(line))
        at=int(re.search(r'native dword (\d+)$',line)[1])
        if at in hooks:
            declarations += ['#ifdef GI_SECONDARY_TERMS', '    if(state.traceEnabled){'+hooks[at].replace('terms.', 'state.terms.')+'}', '#endif']
        if line.endswith('// native dword 1042'):
            declarations.append('    if(useWorldPosition) state.r12.xyz=asuint(worldPosition);')
    declarations += ['}',
        'void GIEvaluateSecondary(inout GIShadingRegisters state,GISecondaryInput sample) {',
        '    GIEvaluateSecondaryCore(state,sample,false,float3(0,0,0));', '}',
        '// Owned producer supplies an unbiased WORLD point and world shading normal.',
        '// UV stays in native order: position.w=V, normal.w=U.',
        'void GIEvaluateSecondaryWorld(inout GIShadingRegisters state,uint material,',
        '                              float3 position,float3 shadingNormal,float2 uv) {',
        '    GISecondaryInput sample;',
        '    // Native light-cell indexing and hit distance still consume view position.',
        '    float3 viewPosition=position.x*asfloat(giCB0[2].xyz)+position.y*asfloat(giCB0[3].xyz)',
        '                       +position.z*asfloat(giCB0[4].xyz)+asfloat(giCB0[5].xyz);',
        '    sample.material=material;sample.positionUV=asuint(float4(viewPosition,uv.y));',
        '    sample.normalUV=asuint(float4(shadingNormal,uv.x));',
        '    GIEvaluateSecondaryCore(state,sample,true,position);', '}']
    primary=lines[index(245):index(977)+1]
    primary=[line.replace('if((r1.xxxx).x != 0u) {','if((r1.xxxx).x == 0u) return false;') for line in primary]
    declarations += ['bool GIInitializeReceiver(uint2 targetPixel,out GIShadingRegisters state) {',
        '    state=(GIShadingRegisters)0;', '    uint3 dispatchId=uint3(targetPixel,0u);']
    declarations += [registers(x) for x in primary]+['    return true;','}']
    kernel=['[numthreads(8,16,1)]','void main(uint3 dispatchId : SV_DispatchThreadID) {',
        '    GIShadingRegisters state;',
        '    if(GIInitializeReceiver(dispatchId.xy,state)) {']
    kernel += [registers(x) for x in lines[index(982):first]]
    kernel += ['    GISecondaryInput sample;',
        '    sample.material=g_tMaterialId.Load(asint(state.r0.xyzw)).x;',
        '    sample.positionUV=asuint(g_tPosition_TexcoordY.Load(asint(state.r0.xyzw)));',
        '    sample.normalUV=asuint(g_tNormal_TexcoordX.Load(asint(state.r0.xyzw)));',
        '    GIEvaluateSecondary(state,sample);']
    kernel += [registers(x) for x in lines[last+1:]]
    return ('// GENERATED by generate-gi-secondary.py; receiver-directed secondary evaluator.\n'
        '// Same native reconstruction, no spatial/temporal reuse enabled here.\n'+resources+'\n'+
        '\n'.join(declarations)+'\n#ifndef GI_SECONDARY_LIBRARY_ONLY\n'+'\n'.join(kernel)+'\n#endif\n')

if __name__=='__main__':
    source=(ROOT/'src/shaders/gi_evaluator_reference.generated.hlsl').read_text()
    target=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'src/shaders/gi_evaluator_secondary.generated.hlsl'
    target.write_text(generate(source));print('Generated',target)
