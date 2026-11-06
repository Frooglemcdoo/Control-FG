// R345: replace the native participating-media integration dispatch.
// Keep the native grid, light inputs, density, atmosphere and output contract.
// MODE 0: recovered native reference; 1: four-column quadrature; 2: fog isolation.
#ifndef MODE
#define MODE 1
#endif
cbuffer System : register(b0) { float4 sys[55]; }
cbuffer Volume : register(b1) { float4 vol[25]; }
cbuffer Atmosphere : register(b2) { float4 atmosphere[12]; }
Texture3D<float4> lighting[2] : register(t0);
Texture3D<float4> density : register(t2);
RWTexture3D<float4> media : register(u0);
SamplerState linearClamp : register(s6, space1);

float VerticalDirection(float2 uv) {
    float2 clip = uv * float2(2, -2) + float2(-1, 1);
    float4 p = sys[23] * clip.y;
    p = mad(sys[22], clip.x, p);
    p += sys[24]; p += sys[25];
    float3 direction = p.xyz / p.w;
    return direction.y * rsqrt(dot(direction, direction));
}

[numthreads(8,8,1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint3 dimensions = asuint(vol[18].xyz);
    if (any(id.xy >= dimensions.xy)) return;
#if MODE == 2
    for (uint z = 0; z < dimensions.z; ++z) media[uint3(id.xy,z)] = float4(0,0,0,1);
#else
    float2 center = (float2(id.xy) + .5) * vol[21].xy;
    const uint taps = MODE == 1 ? 4 : 1;
    float3 sumLight[4], transmission[4];
    float vertical[4];
    [unroll] for (uint t = 0; t < taps; ++t) {
        float2 offset = MODE == 1 ? float2((t & 1) ? .5 : -.5, (t & 2) ? .5 : -.5) : 0;
        float2 uv = clamp(center + offset * vol[21].xy, .5 * vol[21].xy, 1 - .5 * vol[21].xy);
        vertical[t] = VerticalDirection(uv);
        sumLight[t] = 0; transmission[t] = 1;
    }
    float nearZ = vol[24].x, farZ = vol[24].y;
    float depthMinusOne = float(dimensions.z) - 1;
    float depthScale = 1 - vol[23].z;
    float depthLog = log2(vol[24].z);
    float fadeStart = atmosphere[9].w - atmosphere[10].x;
    float fadeEnd = atmosphere[9].w + atmosphere[10].x;
    for (uint z = 0; z < dimensions.z; ++z) {
        float4 accumulated = 0;
        [unroll] for (uint t = 0; t < taps; ++t) {
            float2 offset = MODE == 1 ? float2((t & 1) ? .5 : -.5, (t & 2) ? .5 : -.5) : 0;
            float2 uv = clamp(center + offset * vol[21].xy, .5 * vol[21].xy, 1 - .5 * vol[21].xy);
#if MODE == 1
            // Density is an optical-depth contribution per native depth slice.
            // Interpolate only across XY, preserving exact native depth slices.
            float3 optical = density.SampleLevel(linearClamp, float3(uv,(z+.5)/float(dimensions.z)),0).xyz;
#else
            float3 optical = density.Load(int4(id.xy,z,0)).xyz;
#endif
            float lengthZ = farZ - nearZ;
            float height = nearZ * vertical[t] + sys[54].y;
            float integral = lengthZ * exp2(-(height-atmosphere[7].y)*atmosphere[7].z*1.44269502);
            float extinctionSlope = vertical[t] * atmosphere[7].z * lengthZ;
            if (abs(extinctionSlope) > .01)
                integral *= (1-exp2(-extinctionSlope*1.44269502))/extinctionSlope;
            integral = max(integral * atmosphere[7].w, 0);
            float fade = saturate(((nearZ+farZ)*.5-fadeStart)/(fadeEnd-fadeStart));
            fade = fade*fade*(3-2*fade);
            fade = mad(fade, 1-atmosphere[10].y, atmosphere[10].y);
            optical += (atmosphere[4].x*lengthZ + integral)*fade;
            float3 lit = lighting[0].SampleLevel(linearClamp,float3(uv,(z+.5)*vol[23].w),0).xyz;
            float3 stepTransmission = exp2(optical * -1.44269502);
            sumLight[t] += lit * (1-stepTransmission) * transmission[t] * atmosphere[11].xyz;
            transmission[t] *= stepTransmission;
            accumulated += float4(sumLight[t], dot(transmission[t],float3(.212599993,.715200007,.0722000003)));
        }
        // Average integrated radiance and transmittance, never density before exp.
        media[uint3(id.xy,z)] = accumulated / float(taps);
        nearZ = exp2(mad((z+.5)/depthMinusOne,depthScale,vol[23].z)*depthLog);
        farZ = exp2(mad((z+1.5)/depthMinusOne,depthScale,vol[23].z)*depthLog);
    }
#endif
}
