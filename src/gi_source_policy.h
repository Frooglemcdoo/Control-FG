#pragma once
#include <cstring>
namespace gi_source {
inline unsigned Parse(const char* text) noexcept {
 if(!text)return 0;
 if(!std::strcmp(text,"unshadowed_spot"))return 4;
 if(!std::strcmp(text,"no_point"))return 1;
 if(!std::strcmp(text,"no_spot"))return 2;
 if(!std::strcmp(text,"no_local"))return 3;
 return 0;
}
inline const char* Name(unsigned mode) noexcept {
 return mode==4?"unshadowed_spot":mode==1?"no_point":mode==2?"no_spot":mode==3?"no_local":"full";
}
}
