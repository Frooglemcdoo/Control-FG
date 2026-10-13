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
    const char* failure="undecoded";
    unsigned failureOffset=0;
    static bool Extent(float inverse,std::uint32_t& extent){
        if(!std::isfinite(inverse)||inverse<=0)return false;
        const double pixels=std::round(1.0/double(inverse));
        if(pixels<1||pixels>16384||std::abs(pixels*inverse-1)>1e-6)return false;
        extent=static_cast<std::uint32_t>(pixels);return true;
    }
    bool Decode(){
        frame=width=height=0;failure="inverse_extent";failureOffset=8;
        float values[72]{};std::memcpy(values,bytes.data(),sizeof(values));
        // RDEF and instructions use g_vInvScreenRes at +8. g_vScreenRes.xy
        // and the output resolution are not reliable per-pass identity fields.
        if(!Extent(values[2],width)||!Extent(values[3],height))return false;
        failure="camera_nonfinite";
        for(unsigned n=8;n<72;++n)if(!std::isfinite(values[n])){failureOffset=n*4;return false;}
        // Private rays need BOTH world transforms even when a native shader
        // does not reference them. Never admit zero/unpopulated camera bytes.
        failure="world_inverse";failureOffset=32;
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
            double sum=0;
            for(unsigned k=0;k<4;++k)sum+=double(values[8+row*4+k])*values[24+k*4+col];
            if(std::abs(sum-(row==col?1.0:0.0))>0.01)return false;
        }
        failure="projection_inverse";failureOffset=160;
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col){
            double sum=0;
            for(unsigned k=0;k<4;++k)sum+=double(values[40+row*4+k])*values[56+k*4+col];
            if(std::abs(sum-(row==col?1.0:0.0))>0.01)return false;
        }
        failure="frame_zero";failureOffset=1112;
        std::memcpy(&frame,bytes.data()+1112,4);if(!frame)return false;
        failure="none";failureOffset=0;return true;
    }
    bool Same(const View& other)const {
        return frame&&frame==other.frame&&width==other.width&&height==other.height&&
            !std::memcmp(bytes.data()+8,other.bytes.data()+8,8)&&
            !std::memcmp(bytes.data()+32,other.bytes.data()+32,256);
    }
};
}
