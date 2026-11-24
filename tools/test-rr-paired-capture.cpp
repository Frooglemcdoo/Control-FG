#include "../src/rr_paired_pixels.h"
#include <cassert>
#include <cstdio>
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
            if(!common&&k!=Scene)continue;
            std::memcpy(data[k].data()+8*(x+Width*4)+2*ch,&v,2);
        }
    }
};
int main(){
    for(unsigned channel=0;channel<4;++channel)for(bool common:{false,true}){
        Fixture a(110,4),b(111,5),c(112,6);
        if(channel==3){for(unsigned k=0;k<3;++k)b.Light(k,5,common);}else b.Light(channel,5,common);
        auto e=Find(a.f,b.f,c.f);assert(e.score>3&&e.positions[0].x==4&&e.positions[1].x==5&&e.positions[2].x==6);
        assert(e.channel==(channel==3?0:channel));
        a.f.warm=0;assert(!Find(a.f,b.f,c.f).score);a.f.warm=9;
        ++c.f.number;assert(!Find(a.f,b.f,c.f).score);--c.f.number;
        auto& m=c.data[Mask][8*(6+Width*4)+4];++m;assert(!Find(a.f,b.f,c.f).score);--m;
        for(unsigned k=0;k<3;++k){a.Light(k,4,common);c.Light(k,6,common);}
        assert(!Find(a.f,b.f,c.f).score);
    }
    std::puts("PASS moving character, RGB and white, shared flash, single branch, steady highlights, gaps, cold history and other surfaces");
}
