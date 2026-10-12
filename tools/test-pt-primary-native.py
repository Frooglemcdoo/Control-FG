"""Lock the primary sampling contract to the captured native DXIL.

Checks the source against that evidence, then checks the angular support and
ray-origin geometry independently. Does not claim in-game visual parity.
"""
from pathlib import Path
import math
import re
import struct

root=Path(__file__).resolve().parents[1]
native=(root/'research/native-integration-audit/reflection-000.ll').read_text()
shader=(root/'src/shaders/pt_reflection_p2.hlsl').read_text()

def llvm_float(hex_value):
    return struct.unpack('>d',bytes.fromhex(hex_value))[0]

# Native disassembly: truncated NDF, alpha^2=roughness^4, horizon retry,
# depth-scaled origin, and zero TMin. These are not values tuned to a scene.
assert '%344 = fmul fast float %134, %134' in native
assert '%345 = fmul fast float %344, %344' in native
assert '%346 = fadd fast float %345, -1.000000e+00' in native
cut=re.search(r'%\.i1265 = fmul fast float %Frc64, 0x([A-F0-9]+)',native)
assert cut and abs(llvm_float(cut[1])-0.4)<1e-7
horizon=re.search(r'%292 = fcmp fast olt float %291, 0x([A-F0-9]+)',native)
assert horizon and abs(llvm_float(horizon[1])-0.005)<1e-8
assert '%317 = icmp ult i32 %294, 100' in native
assert 'store float 0.000000e+00, float* %137' in native
bias=re.search(r'%\.i0209 = fmul fast float %19, 0x([A-F0-9]+)',native)
assert bias and abs(llvm_float(bias[1])-0.001)<1e-9

primary=shader.split('float3 PTNativePrimaryDirection(',1)[1].split('float3 PTSampleCosineHemisphere',1)[0]
assert 'frac(u.y)*0.4f' in primary
assert '(1.0f-v)/(1.0f+v*(alpha*alpha-1.0f))' in primary
assert 'max(roughness*roughness,0.02f)' not in primary
assert 'ray.TMin=layer==0u?0.0f:0.01f' in shader
raygen=shader.split('void reflectionRayGeneration()',1)[1].split('// Exactly TWO',1)[0]
assert 'dot(firstViewDirection,normalView)<0.005f&&retry<=100u' in raygen
assert 'primaryWorld+(normalWorld+viewToCamera)*(0.001f*linearDepth)' in raygen
assert 'PTSampleGGXVNDF(firstHitNormal,viewBack,firstRoughness,secondRandom)' in raygen
assert 'PTSampleGGXVNDF(normalWorld' not in raygen

# Independent inverse GGX CDF. At normal incidence the native primary lobe
# never samples below the surface, even at roughness=1 (cos(theta_ray)>=.2).
# The old full NDF/VNDF samples below the surface with probability a^2/(1+a^2).
for rough in [0.0,0.01,0.2,0.5,0.8,1.0]:
    worst=1.0
    for i in range(10001):
        cdf=.4*i/10001
        tan2=(rough**4)*cdf/(1-cdf)
        cos_ray=(1-tan2)/(1+tan2)
        worst=min(worst,cos_ray)
    assert worst>=.2-1e-12,(rough,worst)
    if rough==0: assert worst==1.0  # no forced roughness floor on mirrors

# At shallow views the native origin pulls back toward the camera as well
# as away from the surface. A fixed normal-only 1cm bias cannot match it.
for depth in [.5,10,100]:
    normal=(0,0,1)
    to_camera=(math.sqrt(.99),0,.1)
    delta=[(n+v)*.001*depth for n,v in zip(normal,to_camera)]
    assert delta[0]>0 and delta[2]>0
    assert abs(delta[2]-.0011*depth)<1e-12
print('PASS: captured native primary lobe, horizon retries, mirror limit and origin contract; secondary VNDF unchanged')
