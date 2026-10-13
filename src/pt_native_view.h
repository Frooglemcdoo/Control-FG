#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <cmath>
// sys_constants b0: reviewed native byte offsets. Compare camera contents, not
// upload addresses: Control allocates distinct slices for each rendering pass.
namespace PTNativeView {
enum class Pass {opaque,primary,environment,surface};
struct View {
    std::array<unsigned char,1136> bytes{};
    std::array<unsigned char,1136> bound{};
    Pass pass=Pass::opaque;
    std::uint64_t engineFrame=0;
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
        for(unsigned n=8;n<72;++n){
            // World transform .w lanes and ViewToClip are not read by our shader.
            if((n<40&&n%4==3)||(n>=40&&n<56))continue;
            if(!std::isfinite(values[n])){failureOffset=n*4;return false;}
        }
        // Private rays need BOTH affine world transforms even when a native
        // shader does not reference them. Ignore unused .w padding lanes.
        failure="world_inverse";failureOffset=32;
        for(unsigned row=0;row<3;++row)for(unsigned col=0;col<3;++col){
            double sum=0;
            for(unsigned k=0;k<3;++k)sum+=double(values[8+row*4+k])*values[24+k*4+col];
            if(std::abs(sum-(row==col?1.0:0.0))>0.01)return false;
        }
        for(unsigned col=0;col<3;++col){
            double sum=values[36+col];
            for(unsigned k=0;k<3;++k)sum+=double(values[20+k])*values[24+k*4+col];
            if(std::abs(sum)>0.01)return false;
        }
        // ClipToView is consumed; ViewToClip need not be populated. Check the
        // consumed matrix is nonsingular instead of requiring its unused pair.
        failure="projection_singular";failureOffset=224;
        double projection[4][4]{};
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<4;++col)projection[row][col]=values[56+row*4+col];
        for(unsigned col=0;col<4;++col){
            unsigned pivot=col;
            for(unsigned row=col+1;row<4;++row)if(std::abs(projection[row][col])>std::abs(projection[pivot][col]))pivot=row;
            if(std::abs(projection[pivot][col])<1e-12)return false;
            for(unsigned k=0;k<4;++k){const double temp=projection[col][k];projection[col][k]=projection[pivot][k];projection[pivot][k]=temp;}
            for(unsigned row=col+1;row<4;++row){const double factor=projection[row][col]/projection[col][col];
                for(unsigned k=col;k<4;++k)projection[row][k]-=factor*projection[col][k];}
        }
        failure="frame_zero";failureOffset=1112;
        std::memcpy(&frame,bytes.data()+1112,4);if(!frame)return false;
        failure="none";failureOffset=0;return true;
    }
    unsigned Mismatch(const std::array<unsigned char,1136>& other)const {
        const auto same=[&](unsigned offset,unsigned count){return !std::memcmp(bound.data()+offset,other.data()+offset,count);};
        if(pass==Pass::environment){if(!same(0,8))return 0;}
        else if(!same(8,8))return 8;
        if(pass==Pass::surface||pass==Pass::opaque||pass==Pass::environment){
            const unsigned first=pass==Pass::surface?96:32;
            for(unsigned offset=first;offset<160;offset+=16)if(!same(offset,12))return offset;
        }
        if(pass!=Pass::surface&&!same(224,64))return 224;
        if((pass==Pass::primary||pass==Pass::opaque)&&!same(1112,4))return 1112;
        return UINT32_MAX;
    }
    bool MatchesBound(const std::array<unsigned char,1136>& other)const {return Mismatch(other)==UINT32_MAX;}
    bool MatchesSurface(const std::array<unsigned char,1136>& raw)const {
        // Surface shaders consume inverse extent and affine ViewToWorld, not a
        // complete primary camera. Compare against the retained producer camera.
        View surface=*this;surface.pass=Pass::surface;surface.bound=bytes;
        return surface.MatchesBound(raw);
    }
    bool Same(const View& other)const {
        if(engineFrame!=other.engineFrame||!frame||frame!=other.frame||width!=other.width||height!=other.height||
           std::memcmp(bytes.data()+8,other.bytes.data()+8,8)||
           std::memcmp(bytes.data()+224,other.bytes.data()+224,64))return false;
        for(unsigned offset=32;offset<160;offset+=16)
            if(std::memcmp(bytes.data()+offset,other.bytes.data()+offset,12))return false;
        return true;
    }
};
}
