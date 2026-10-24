// Read-only observation of the native composition inputs and completed outputs.
// Native composition already uses typed loads on these same two UAVs.
Texture2D<uint2> selectedLights : register(t14);
Texture2D<float4> shadowVisibility : register(t15);
RWTexture2D<float4> diffuseOutput : register(u0);
RWTexture2D<float4> specularOutput : register(u1);
RWByteAddressBuffer captureSignal : register(u2);
cbuffer CaptureLayout : register(b4) { uint width, height, frameTag, grid; }
[numthreads(8,8,1)]
void captureComposition(uint3 id : SV_DispatchThreadID) {
    if (id.x >= 64 || id.y >= 64 || grid != 64) return;
    uint2 pixel = (2 * id.xy + 1) * uint2(width, height) / 128;
    uint at = (id.y * 64 + id.x) * 64;
    uint2 selected = selectedLights.Load(int3(pixel, 0));
    captureSignal.Store4(at, uint4(pixel, selected));
    captureSignal.Store4(at + 16, asuint(shadowVisibility.Load(int3(pixel, 0))));
    captureSignal.Store4(at + 32, asuint(diffuseOutput[pixel]));
    captureSignal.Store4(at + 48, asuint(specularOutput[pixel]));
}
