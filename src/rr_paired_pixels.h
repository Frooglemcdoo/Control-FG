#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
namespace rr_pair {
enum Plane {Raw,Temporal,Spatial,Scene,RRInput,Mask,Depth,Normal,Motion,Remap,RROut,PlaneCount};
using RGB=std::array<float,3>;
struct XY {int x,y;};
struct Grid {
    const unsigned char* base=nullptr;
    unsigned w=0,h=0,pitch=0,bpp=0,format=0;
    const unsigned char* At(XY p) const {
        return base&&unsigned(p.x)<w&&unsigned(p.y)<h?base+size_t(p.y)*pitch+size_t(p.x)*bpp:nullptr;
    }
};
template<class T> inline T Read(const void* p){T v{};std::memcpy(&v,p,sizeof(v));return v;}
inline float SmallFloat(unsigned v,unsigned m){
    const unsigned e=v>>m,f=v&((1u<<m)-1);
    if(e==31)return f?std::numeric_limits<float>::quiet_NaN():std::numeric_limits<float>::infinity();
    return std::ldexp(float(e?(1u<<m)+f:f),(e?int(e)-15:-14)-int(m));
}
inline float Half(unsigned v){return (v&32768?-1.f:1.f)*SmallFloat(v&32767,10);}
inline RGB Color(const Grid& g,XY p){
    auto a=g.At(p);if(!a)return {NAN,NAN,NAN};
    if(g.format==26){
        const auto u=Read<uint32_t>(a);
        return {SmallFloat(u&2047,6),SmallFloat((u>>11)&2047,6),SmallFloat(u>>22,5)};
    }
    return {Half(Read<uint16_t>(a)),Half(Read<uint16_t>(a+2)),Half(Read<uint16_t>(a+4))};
}
inline unsigned Word(const unsigned char* a){const auto v=Read<uint16_t>(a);return ((v&32767)-1024)|((v>>1)&16384);}
struct FrameView {
    std::array<Grid,PlaneCount> g{};
    uint64_t number=0,session=0,view=0,epoch=0;
    unsigned w=0,h=0,warm=0,passes=0;float jitterX=0,jitterY=0,scaleX=0,scaleY=0;
    bool reset=false;
    XY Source(XY p) const{
        auto a=g[Remap].At({p.x/2,p.y/2});if(!a)return {-1,-1};
        unsigned bits=Read<uint32_t>(a)>>((p.x&1)*8+(p.y&1)*16);
        return {(p.x&~1)+int(bits&1),(p.y&~1)+int((bits>>1)&1)};
    }
    RGB Value(Plane k,XY p) const {
        if(k<=Spatial)p=Source(p);
        if(k==RROut)p={int((p.x+.5)*g[k].w/w),int((p.y+.5)*g[k].h/h)};
        return Color(g[k],p);
    }
    bool Visible(XY p) const{
        auto m=g[Mask].At(p),d=g[Depth].At(p);
        if(!m||!d||!(Half(Read<uint16_t>(m+6))>.5f))return false;
        return (Word(m)|(Word(m+2)<<15))==Read<uint32_t>(d);
    }
};
inline bool Consecutive(const FrameView& a,const FrameView& b){
    return a.session==b.session&&a.number+1==b.number&&a.view==b.view&&a.epoch==b.epoch&&
           a.w==b.w&&a.h==b.h&&a.scaleX==b.scaleX&&a.scaleY==b.scaleY&&!b.reset;
}
inline RGB UnitNormal(const unsigned char* n){
    float x=1.3f*(2.f*((n[0]<<3)|((n[3]>>1)&7))/2047.f-1);
    float y=1.3f*(2.f*((n[1]<<4)|(n[3]>>4))/4095.f-1),d=1+x*x+y*y;
    return {2*x/d,2*y/d,(x*x+y*y-1)/d};
}
inline bool MatchPrevious(const FrameView& now,XY p,const FrameView& old,XY& result){
    auto m=now.g[Mask].At(p),v=now.g[Motion].At(p),n=now.g[Normal].At(p);
    if(!v||!m||!n||!now.Visible(p))return false;
    bool fp32=now.g[Motion].format==16;
    float x=p.x+(fp32?Read<float>(v):Half(Read<uint16_t>(v)))*now.scaleX+old.jitterX-now.jitterX;
    float y=p.y+(fp32?Read<float>(v+4):Half(Read<uint16_t>(v+2)))*now.scaleY+old.jitterY-now.jitterY;
    if(!(x>=-.5f&&y>=-.5f&&x<old.w-.5f&&y<old.h-.5f))return false;
    float z=Read<float>(now.g[Depth].At(p)),best=1.5f*1.5f;bool found=false;
    auto unit=UnitNormal(n);
    for(int j=int(std::floor(y));j<=int(std::ceil(y));++j)for(int i=int(std::floor(x));i<=int(std::ceil(x));++i){
        XY q{i,j};if(!old.Visible(q))continue;
        auto om=old.g[Mask].At(q);if(Read<uint16_t>(om+4)!=Read<uint16_t>(m+4))continue;
        float dz=std::abs(Read<float>(old.g[Depth].At(q))-z);
        if(!(dz<=std::max(std::abs(z)*.01f,1.e-5f)))continue;
        auto on=UnitNormal(old.g[Normal].At(q));
        if(on[0]*unit[0]+on[1]*unit[1]+on[2]*unit[2]<.97f)continue;
        float d=(i-x)*(i-x)+(j-y)*(j-y);if(d>=best)continue;
        best=d;result=q;found=true;
    }
    return found;
}
struct Event {XY positions[3]{};float score=0;Plane stage=Raw;unsigned channel=0;};
// Use a per-channel temporal impulse. Branch disagreement and chroma are never requirements.
inline void Score(const RGB& a,const RGB& b,const RGB& c,Plane k,Event& event){
    for(unsigned ch=0;ch<3;++ch){
        if(!std::isfinite(a[ch])||!std::isfinite(b[ch])||!std::isfinite(c[ch]))continue;
        float base=std::max(.0002f,std::max(a[ch],c[ch]));
        if(b[ch]-base<.0008f)continue;
        float ratio=b[ch]/base;
        if(ratio>=3&&ratio>event.score){event.score=ratio;event.stage=k;event.channel=ch;}
    }
}
inline Event Find(const FrameView& a,const FrameView& b,const FrameView& c){
    Event best{};
    if(!Consecutive(a,b)||!Consecutive(b,c)||a.warm<4)return best;
    for(unsigned y=0;y<c.h;++y)for(unsigned x=0;x<c.w;++x){
        XY cp{int(x),int(y)},bp{},ap{};
        if(!MatchPrevious(c,cp,b,bp)||!MatchPrevious(b,bp,a,ap))continue;
        Event next{};next.positions[0]=ap;next.positions[1]=bp;next.positions[2]=cp;
        for(Plane k:{Raw,Spatial,Scene,RRInput,RROut})
            Score(a.Value(k,ap),b.Value(k,bp),c.Value(k,cp),k,next);
        if(next.score>best.score)best=next;
    }
    return best;
}
}
