// Generated from the tested rr_reflectance.h. Do not edit coefficients here.
float3 EnvBRDFApprox2(float3 f0, float alpha, float nDotV) {
    const float v = abs(nDotV), v2 = v*v, v3 = v*v2;
    const float a = alpha, a3 = a*a*a;
    const float biasNumerator = (0.99044f - 1.28514f*v) + a*(1.29678f - 0.755907f*v);
    const float biasDenominator = (1.0f + 2.92338f*v + 59.4188f*v3)
        + a*(20.3225f - 27.0302f*v + 222.592f*v3)
        + a3*(121.563f + 626.13f*v + 316.627f*v3);
    const float scaleNumerator = (0.0365463f + 3.32707f*v) + a*(9.0632f - 9.04756f*v);
    const float scaleDenominator = (1.0f + 3.59685f*v2 - 1.36772f*v3)
        + a*(9.04401f - 16.3174f*v2 + 9.22949f*v3)
        + a3*(5.56589f + 19.7886f*v2 - 20.2123f*v3);
    const float zeroWeight = max(0.0f, min(1.0f, f0.y * 50.0f));
    const float bias = max(0.0f, (biasNumerator / biasDenominator) * zeroWeight);
    const float scale = max(0.0f, scaleNumerator / scaleDenominator);
    return float3(f0.x * scale + bias, f0.y * scale + bias, f0.z * scale + bias);
}
