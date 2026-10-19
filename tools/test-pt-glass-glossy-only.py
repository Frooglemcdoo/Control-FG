"""Prevent accidental reintroduction of the removed diffuse/diagnostic path."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
ray=(r/'src/shaders/pt_glass_raygen.hlsl').read_text()
lighting=(r/'src/shaders/pt_glass_native.generated.hlsl').read_text()
resources=(r/'src/pt_glass_resources.h').read_text()
runtime=(r/'src/pt_glass_runtime.h').read_text()
ui=(r/'src/fg_overlay.h').read_text()
assert ray.count('TraceRay(')==2 # Native primary plus glossy continuation.
for name,text in [('ray',ray),('lighting',lighting),('resources',resources),('runtime',runtime)]:
 for token in ['DiffuseDirection','PTGlassApplyDiffuse','ptGlassHistory','ptGlassDiagnostics','diagnosticReadback','HitLayers']:
  assert token not in text,(name,token)
assert 'desc[1].Height=height;' in resources
assert 'hd.NumDescriptors=5;' in resources
assert 'pass.mode==0' in runtime and 'GLASS ON' in ui and 'GLASS OFF' in ui
assert 'GLASS: GLOSSY + DIFFUSE' not in ui
assert 'g_rwtRadiance[r1.xy]=0.0f;' in lighting
assert 'float4(max(asfloat(r0.xyz),0.0f),1.0f)' in lighting
assert 'traced.w>0.5f && all(isfinite(traced.xyz))' in lighting
assert 'native dword 6728' in lighting
print('PASS: one glossy continuation; no added diffuse/history/diagnostic work; two-state UI; invalid secondary lighting retains native specular.')
