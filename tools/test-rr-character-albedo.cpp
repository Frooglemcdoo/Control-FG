#include <cassert>
#include <cstdio>
#include <limits>
#include <initializer_list>
#include "../src/shaders/rr_character_albedo.hlsli"
int main() {
    assert(ControlCharacterAlbedoValid(0,0,0)); // valid black must not fall back
    assert(ControlCharacterAlbedoValid(1,1,1));
    assert(ControlCharacterAlbedoValid(0.43f,0.19f,0.11f));
    const float invalid[]{-1,-0.00001f,1.0001f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()};
    for(float v:invalid) {
        assert(!ControlCharacterAlbedoValid(v,0.5f,0.5f));
        assert(!ControlCharacterAlbedoValid(0.5f,v,0.5f));
        assert(!ControlCharacterAlbedoValid(0.5f,0.5f,v));
    }
    // Partial hair is continuous; native skin (including zero-alpha black)
    // keeps its exact full-strength branch. NaN is never treated as a hair tag.
    assert(ControlHairGuideWeight(-2.0f)==0.0f);
    assert(ControlHairGuideWeight(-2.25f)==0.25f);
    assert(ControlHairGuideWeight(-2.5f)==0.5f);
    assert(ControlHairGuideWeight(-3.0f)==1.0f);
    for(float a : {0.0f,1.0f,-1.0f,-3.1f,-1.9f,
        std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()})
        assert(ControlHairGuideWeight(a)==1.0f);
    puts("PASS: shared albedo coverage rule retains black/white and rejects sentinel, invalid range, NaN and infinity in every channel");
}
