#pragma once
#include "../src/rr_paired_pixels.h"


#include <vector>
using namespace rr_pair;
constexpr int Width=12, Height=9;
struct Fixture {
    FrameView f;
    std::array<std::vector<unsigned char>,PlaneCount> data;
    Fixture(unsigned n,int x){
        f.number=n;f.session=f.view=f.epoch=1;f.warm=9;f.w=Width;f.h=Height;f.scaleX=f.scaleY=1;
        for(unsigned i=0;i<PlaneCount;++i){
            unsigned bpp=(i==Depth||i==Normal||i==Remap||i==Motion)?4:8;
            unsigned w=i==Remap?(Width+1)/2:Width,h=i==Remap?(Height+1)/2:Height;
            data[i].resize(w*h*bpp);f.g[i]={data[i].data(),w,h,w*bpp,bpp,i==Motion?34u:10u};
        }
        int index=x+Width*4;uint16_t mask[]={0x400,0x3e00,0x412,0x3c00};
        // Encode actual raster depth using the same two 15-bit limbs as the shader.
        uint32_t z=0x3e800000u;
        auto pack=[](unsigned bits){return uint16_t((0x400u+(bits&0x3fff))|((bits&0x4000)<<1));};
        mask[0]=pack(z&32767);mask[1]=pack(z>>15);
        std::memcpy(data[Mask].data()+8*index,mask,8);
        std::memcpy(data[Depth].data()+4*index,&z,4);
        for(unsigned i=0;i<data[Remap].size();i+=4){uint32_t v=0x03020100;std::memcpy(data[Remap].data()+i,&v,4);}
        uint16_t mv=0xbc00;std::memcpy(data[Motion].data()+4*index,&mv,2);
    }
    void Light(unsigned ch,int x,bool common){uint16_t v=0x2c00;
        for(Plane k:{Raw,Temporal,Spatial,Scene,RRInput,RROut}){
            if(!common&&k!=RROut)continue;
            std::memcpy(data[k].data()+8*(x+Width*4)+2*ch,&v,2);
        }
    }
};
