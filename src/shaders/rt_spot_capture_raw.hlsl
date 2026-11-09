// R201: read the completed gameplay ray outputs, before temporal/spatial filtering.
// Hash-verified R97 bindings: u0 = spot/point hit distance, u1 = visibility.
RWTexture2D<float2> nativeHitDistance : register(u0);
RWTexture2D<float2> nativeVisibility : register(u1);
RWByteAddressBuffer captureInputs : register(u2);
cbuffer CaptureLayout : register(b4) { uint width, height, frameTag, grid, mapEnabled; }
[numthreads(8,8,1)]
void captureRawShadow(uint3 id : SV_DispatchThreadID) {
    if (id.x >= 64 || id.y >= 64 || grid != 64) return;
    uint index = id.y * 64 + id.x;
    uint2 pixel = (2 * id.xy + 1) * uint2(width, height) / 128;
    uint4 identity = captureInputs.Load4(index * 48 + 28); // light, pixel x/y, frame
    uint at = 4096 * (48 + 128) + index * 32;
    captureInputs.Store4(at, uint4(identity.yz, identity.w, identity.x));
    uint w0, h0, w1, h1;
    nativeHitDistance.GetDimensions(w0, h0);
    nativeVisibility.GetDimensions(w1, h1);
    float4 value = asfloat(uint4(0x7fc00000,0x7fc00000,0x7fc00000,0x7fc00000));
    if (w0 == width && h0 == height && w1 == width && h1 == height &&
        all(identity.yz == pixel) && identity.w == frameTag)
        value = float4(nativeVisibility[pixel], nativeHitDistance[pixel]);
    captureInputs.Store4(at + 16, asuint(value));
}
