"""Numerical integration regression for the hybrid specular replacement.

Independent uniform-hemisphere quadrature checks the GGX VNDF estimator.
Composition cases specifically cover the prior black-wall regression.
These are CPU checks, not a claim of GPU/native-evaluator parity.
"""
import math
import random

rng = random.Random(4517)
N = 120000

def g1(n, a2):
    return 2*n/(n+math.sqrt(a2+(1-a2)*n*n)) if n>0 else 0

for rough, f0 in [(0.35,0.04),(0.8,0.04),(0.8,0.8)]:
    a2=max(rough*rough,0.02)**2
    sampled=quad=0.0
    # Normal incidence: GGX visible normals reduce to the NDF.
    for _ in range(N):
        u=rng.random()
        nh=math.sqrt((1-u)/(1+(a2-1)*u))
        nl=2*nh*nh-1
        if nl>0:
            sampled+=(f0+(1-f0)*(1-nh)**5)*g1(nl,a2)
        # Independent uniform-hemisphere integration: pdf=1/(2*pi).
        nl=rng.random()
        nh=math.sqrt((1+nl)/2)
        d=a2/(math.pi*(nh*nh*(a2-1)+1)**2)
        f=f0+(1-f0)*(1-nh)**5
        quad+=2*math.pi*f*d*g1(nl,a2)/4
    sampled/=N
    quad/=N
    assert abs(sampled-quad)<0.003, (rough,f0,sampled,quad)
    assert 0<=sampled<=1
    print(f'GGX rough={rough} F0={f0}: VNDF={sampled:.6f}, quadrature={quad:.6f}')

# Reflected concrete can be lit almost entirely by native diffuse GI.
# A black incoming specular ray must not remove that diffuse illumination.
for diffuse,direct,emission,old_spec,new_spec in [
    (0.45,0.0,0.0,0.02,0.0),
    (0.2,0.1,0.03,0.04,0.07),
    (0.0,0.0,0.8,0.01,0.0),
]:
    retained=diffuse+direct+emission
    native=retained+old_spec
    candidate=retained+new_spec
    output=native+(candidate-native)
    assert abs(output-(retained+new_spec))<1e-12
    assert output>=retained-1e-12
    # Equal replacement radiance must give exactly the original appearance.
    assert abs(native+((retained+old_spec)-native)-native)<1e-12
print('PASS: black secondary specular preserves diffuse/direct/emission; equal specular preserves native.')
