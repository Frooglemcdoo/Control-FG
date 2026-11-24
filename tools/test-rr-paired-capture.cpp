#include "rr_paired_fixture.h"
#include <cassert>
#include <cstdio>
int main(){
    for(unsigned channel=0;channel<4;++channel)for(bool common:{false,true}){
        Fixture a(110,4),b(111,5),c(112,6);
        if(channel==3){for(unsigned k=0;k<3;++k)b.Light(k,5,common);}else b.Light(channel,5,common);
        auto e=Find(a.f,b.f,c.f);assert(e.score>3&&e.positions[0].x==4&&e.positions[1].x==5&&e.positions[2].x==6);
        assert(e.channel==(channel==3?0:channel));
        for(unsigned i=0;i<3;++i)assert(bool(e.byChannel[i].score)==(channel==3||channel==i));
        a.f.warm=0;assert(!Find(a.f,b.f,c.f).score);a.f.warm=9;
        ++c.f.number;assert(!Find(a.f,b.f,c.f).score);--c.f.number;
        auto& m=c.data[Mask][8*(6+Width*4)+4];++m;assert(!Find(a.f,b.f,c.f).score);--m;
        for(unsigned k=0;k<3;++k){a.Light(k,4,common);c.Light(k,6,common);}
        assert(!Find(a.f,b.f,c.f).score);
    }
    {
        Fixture a(1,4),b(2,5),c(3,6);b.Light(0,5,true);
        for(Fixture* fixture:{&a,&b,&c}){
            auto& bytes=fixture->data[RROut];bytes.assign(18*18*8,0);
            fixture->f.g[RROut]={bytes.data(),18,18,18*8,8,10};
        }
        assert(!Find(a.f,b.f,c.f).score); // Strong raw flash already removed from the displayed signal.
        uint16_t green=0x2c00;
        std::memcpy(b.data[RROut].data()+(7+18*8)*8+2,&green,2);
        auto e=Find(a.f,b.f,c.f);
        assert(e.score>3&&e.channel==1&&e.output[1].x==7&&e.output[1].y==8);
        assert(FirstOutput(707,1440,2160)==1060&&FirstOutput(708,1440,2160)==1062);
    }
    std::puts("PASS moving character, RGB and white, shared flash, single branch, steady highlights, gaps, cold history and other surfaces");
}
