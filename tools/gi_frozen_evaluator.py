"""Generate disjoint native GI6 shading with frozen training/mixture weights.

Variant-zero one-ray ABI only. Runtime must validate that contract before
suppressing native rays. This does not install hooks or enable gameplay.
"""
from pathlib import Path
import argparse
from gi_single_producer import replace_once


def transform(source):
    declarations = '''
#include "gi_frozen_receiver.hlsl"
cbuffer GIFrozenPhase : register(b0,space18) {
 uint frozenWidth,frozenHeight,frozenFrame,frozenEpoch;
 uint frozenParity,frozenPhase,frozenWeighted,frozenReserved;
}
cbuffer GIFrozenRayFrame : register(b1,space18) {uint4 frozenRaySystem[71];}
RWStructuredBuffer<GITrainingRecord> frozenTraining : register(u0,space18);
StructuredBuffer<float> frozenWeights : register(t0,space18);
'''
    source = replace_once(source, '[numthreads(8,16,1)]', declarations+'\n[numthreads(8,16,1)]')
    source = replace_once(source, 'void main(uint3 id : SV_DispatchThreadID) {', '''void main(uint3 id : SV_DispatchThreadID) {
    uint2 compactId=id.xy;
    if(frozenWidth<2||(frozenWidth&1)||frozenParity>1||frozenPhase>1||frozenReserved||
       id.z||compactId.x>=frozenWidth/2||compactId.y>=frozenHeight)return;
    uint frozenIndex=compactId.y*(frozenWidth/2)+compactId.x;
    id.x=compactId.x*2+frozenParity;
    GITrainingRecord trainingRecord=(GITrainingRecord)0;
    if(frozenPhase==0)frozenTraining[frozenIndex]=trainingRecord;
    float sampleWeight=(frozenPhase==1&&frozenWeighted)?frozenWeights[frozenIndex]:1;
    // Invalid state is not admitted by the host. Never reinterpret a bad weight
    // as a fresh native draw after observing its endpoint.
    if(!isfinite(sampleWeight)||sampleWeight<0)sampleWeight=0;
    if(frozenPhase==0){
        GIFrozenReceiver receiver=GIFrozenDecodeReceiver(id.xy,
            g_tGBuffer1.Load(int3(id.xy,0)),g_tLinearDepth.Load(int3(id.xy,0)).x,
            g_tClipDepth.Load(int3(id.xy,0)).x,frozenRaySystem);
        if(receiver.valid){
            GIPoint normal=GIFrozenPoint(receiver.normal);
            trainingRecord.key=GIMakeWorldGuideKey(GIFrozenPoint(receiver.world),normal);
            GIProducerCoordinates random=GIProducerSample(id.x,id.y,frozenRaySystem[69].z,0,0,1);
            GIFrozenWorldGuide empty=(GIFrozenWorldGuide)0;
            GIFrozenRay draw=GISampleFrozenWorld(empty,normal,frozenFrame,frozenEpoch,0,random.u,random.v);
            trainingRecord.direction=draw.direction;
            if(!draw.valid)trainingRecord.key.valid=0;
        }
    }''')
    accumulate = '        radiance = mad(throughput, asfloat(receiver.r15.xyz), radiance);'
    source = replace_once(source, accumulate, '''    float3 contribution=throughput*asfloat(receiver.r15.xyz);
    if(frozenPhase==0){
        // One configured ray, including misses/volume, no bright-source selection.
        trainingRecord.energy=dot(contribution,float3(.2126f,.7152f,.0722f));
        if(!all(isfinite(contribution))||any(contribution<0))trainingRecord.energy=-1;
        frozenTraining[frozenIndex]=trainingRecord;
    }
    radiance += sampleWeight==0 ? float3(0,0,0) : contribution*sampleWeight;
    // Native hit-distance accumulation and configured sample denominator remain unchanged.''')
    source = replace_once(source, 'if (rays <= 0 ||', 'if (rays != 1 ||')
    return source


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    source = (root/'src/shaders/gi_owned_lighting.hlsl').read_text()
    Path(args.output).write_text(transform(source))
