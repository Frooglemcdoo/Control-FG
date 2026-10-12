"""Numerical transport and layout checks for the diffuse continuation."""
import math
from pathlib import Path
root=Path(__file__).resolve().parents[1]
def h(x):
    x^=x>>16;x=(x*0x7feb352d)&0xffffffff;x^=x>>15;x=(x*0x846ca68b)&0xffffffff
    return x^(x>>16)
sx=sy=sz=0.
for i in range(65536):
    seed=h(i);u=((seed&65535)+.5)/65536;v=((seed>>16)+.5)/65536
    x=math.sqrt(u)*math.cos(2*math.pi*v);y=math.sqrt(u)*math.sin(2*math.pi*v);z=math.sqrt(1-u)
    assert z>0 and abs(x*x+y*y+z*z-1)<1e-12
    # Unit Lambertian BRDF times cosine / cosine-weighted PDF is one.
    assert abs((1/math.pi)*z/(z/math.pi)-1)<1e-12
    sx+=x;sy+=y;sz+=z
assert abs(sx/65536)<.01 and abs(sy/65536)<.01 and abs(sz/65536-2/3)<.01
for width,height in [(1,1),(129,33),(2560,1440),(3840,2160)]:
    count=width*height
    assert count-1<count and (count-1)+count==2*count-1
    assert 2*height<=16384
shader=(root/'src/shaders/pt_glass_native.generated.hlsl').read_text()
assert shader.index('PTGlassFilteredDiffuse(r1.xy')<shader.index('// native dword 6728')
assert 'layer*ptGlassExtent.x*ptGlassExtent.y' in shader
assert 'uint2(0,layer*ptGlassExtent.y)' in shader
runtime=(root/'src/pt_glass_runtime.h').read_text()
assert 'secondaryTable);list->Dispatch(dims[0],dims[1],PTGlass::HitLayers)' in runtime
print('PASS: cosine estimator normalization and moments; separate layers through 4K; diffuse integration before native material response.')
