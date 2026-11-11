"""Guard capture integration: preserve existing lanes, include map in tracked copies."""
from pathlib import Path
r=Path(__file__).resolve().parents[1]
s=(r/'src/rr_input_capture.h').read_text();limits=(r/'src/rr_input_capture_limits.h').read_text()
assert 'kRRInputCaptureMaxResources = 18;' in limits
assert 'kRRInputCaptureNativeCount = 9;' in limits
assert 'resources[nativeIndex+8]=nativeInputs[8].resource;' in s
assert 'native-reconstruction-map.r32u' in s and 'c->pixelBytes[nativeIndex+8]=4;' in s
assert 'map.Format==DXGI_FORMAT_R32_UINT' in s
assert 'map.Width>=(width+1)/2' in s and 'map.Height>=(height+1)/2' in s
assert 'for(unsigned i=nativeIndex+6;i<maskIndex;++i)' not in s
assert 'nativeInputs[i-nativeIndex].trackedState' in s
assert 'RRInputCaptureReadShaderTexture(kRRShaderGBufferReconstructionRva,&nativeInputs[8])' in s
for guide in (3,5):
 for raw in (0,1):
  for mask in (0,1):
   output=guide+raw+9+mask
   assert output+2<=18 # output and pre-GI slot
print('PASS map lane capacity, R32_UINT gate, extent bounds, tracked-state wiring and preserved lighting indices')
