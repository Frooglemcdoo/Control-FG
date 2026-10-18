#ifndef CONTROL_RR_CHARACTER_ALBEDO_HLSLI
#define CONTROL_RR_CHARACTER_ALBEDO_HLSLI
// Shared CPU/HLSL acceptance rule. Comparisons reject NaN and infinities too.
// Uncovered replay pixels have RGB=-1. Alpha is deliberately not coverage.
bool ControlCharacterAlbedoValid(float r, float g, float b) {
    return r >= 0.0f && r <= 1.0f &&
           g >= 0.0f && g <= 1.0f &&
           b >= 0.0f && b <= 1.0f;
}
#endif
