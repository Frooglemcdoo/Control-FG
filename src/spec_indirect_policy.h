#pragma once
#include <cstring>
namespace spec_indirect {
inline unsigned Parse(const char* value) noexcept {
    if(!value)return 0;
    if(!std::strcmp(value,"diffuse"))return 1;
    if(!std::strcmp(value,"environment"))return 2;
    if(!std::strcmp(value,"volume"))return 3;
    if(!std::strcmp(value,"cube"))return 4;
    return 0;
}
inline const char* Name(unsigned mode) noexcept {return mode==1?"diffuse":mode==2?"environment":mode==3?"volume":mode==4?"cube":"full";}
inline unsigned Gain(unsigned mode) noexcept {return mode>=2&&mode<=4?8u:1u;}
}
