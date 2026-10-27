#pragma once
#include <cstring>
namespace spec_component {
inline unsigned Parse(const char* value) noexcept {
    return value && !std::strcmp(value,"emission") ? 1u :
           value && !std::strcmp(value,"shading") ? 2u : 0u;
}
inline const char* Name(unsigned mode) noexcept {
    return mode==1?"emission":mode==2?"shading":"full";
}
}
