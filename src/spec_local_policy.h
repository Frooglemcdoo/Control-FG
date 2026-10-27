#pragma once
#include <cstring>
namespace spec_local {
inline unsigned Parse(const char* value) noexcept {
    if(!value)return 0;
    if(!std::strcmp(value,"other"))return 1;
    if(!std::strcmp(value,"local"))return 2;
    return 0;
}
inline const char* Name(unsigned mode) noexcept {return mode==1?"other":mode==2?"local":"full";}
}
