#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
// sys_constants b0: reviewed native byte offsets. Compare camera contents, not
// upload addresses: Control allocates distinct slices for each rendering pass.
namespace PTNativeView {
struct View {
    std::array<unsigned char,1136> bytes{};
    std::uint32_t frame=0,width=0,height=0;
    bool Decode(){
        frame=width=height=0;float values[72]{};std::memcpy(values,bytes.data(),sizeof(values));
        for(float v:values)if(!std::isfinite(v))return false;
        if(values[4]<1||values[4]>16384||values[5]<1||values[5]>16384||
           std::floor(values[4])!=values[4]||std::floor(values[5])!=values[5]||
           std::abs(values[4]*values[6]-1)>1e-5f||std::abs(values[5]*values[7]-1)>1e-5f)return false;
        std::memcpy(&frame,bytes.data()+1112,4);if(!frame)return false;
        width=static_cast<std::uint32_t>(values[4]);height=static_cast<std::uint32_t>(values[5]);return true;
    }
    bool Same(const View& other)const {
        return frame&&frame==other.frame&&width==other.width&&height==other.height&&
            !std::memcmp(bytes.data(),other.bytes.data(),288);
    }
};
}
