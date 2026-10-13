#include "../src/pt_native_view.h"
#include <cassert>
#include <cstdio>
#include <limits>
static void Write(PTNativeView::View& v,unsigned offset,float value){std::memcpy(v.bytes.data()+offset,&value,4);}
int main(){
    PTNativeView::View a;Write(a,8,1.0f/2560);Write(a,12,1.0f/1440);
    for(unsigned base:{32u,96u,160u,224u})for(unsigned n=0;n<4;++n)Write(a,base+n*20,1);
    const std::uint32_t frame=45;std::memcpy(a.bytes.data()+1112,&frame,4);
    assert(a.Decode()&&a.width==2560&&a.height==1440); // Unused screen/output fields may be zero.
    auto b=a;Write(b,0,std::numeric_limits<float>::quiet_NaN());Write(b,16,3840);Write(b,20,2160);
    assert(b.Decode()&&a.Same(b));b.bytes[1000]^=1;assert(b.Decode()&&a.Same(b));
    b=a;Write(b,80,1);Write(b,144,-1);assert(b.Decode()&&!a.Same(b));
    b=a;b.bytes[1112]^=1;assert(b.Decode()&&!a.Same(b));
    b=a;Write(b,8,0);assert(!b.Decode());b=a;Write(b,12,1.0f/1440.5f);assert(!b.Decode());
    b=a;Write(b,32,std::numeric_limits<float>::quiet_NaN());assert(!b.Decode()&&b.failureOffset==32);
    b=a;Write(b,32,0);assert(!b.Decode());b=a;Write(b,224,0);assert(!b.Decode());
    b=a;std::memset(b.bytes.data()+1112,0,4);assert(!b.Decode());
    puts("PASS: inverse render extents, unused fields ignored, frame/camera changes and missing transforms rejected");
}
