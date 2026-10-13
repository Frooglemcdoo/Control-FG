#include "../src/pt_native_view.h"
#include <cassert>
#include <cstdio>
#include <limits>
static void Write(PTNativeView::View& v,unsigned offset,float value){std::memcpy(v.bytes.data()+offset,&value,4);}
int main(){
    PTNativeView::View a;Write(a,16,2560);Write(a,20,1440);Write(a,24,1.0f/2560);Write(a,28,1.0f/1440);
    const std::uint32_t frame=45;std::memcpy(a.bytes.data()+1112,&frame,4);assert(a.Decode());
    auto b=a;assert(b.Decode()&&a.Same(b));b.bytes[1000]^=1;assert(b.Decode()&&a.Same(b));
    b=a;b.bytes[32]^=1;assert(b.Decode()&&!a.Same(b));
    b=a;b.bytes[1112]^=1;assert(b.Decode()&&!a.Same(b));
    b=a;Write(b,16,0);assert(!b.Decode());b=a;Write(b,20,1440.5f);assert(!b.Decode());
    b=a;Write(b,28,1);assert(!b.Decode());b=a;Write(b,32,std::numeric_limits<float>::quiet_NaN());assert(!b.Decode());
    puts("PASS: per-pass view contents, frame/dimension changes and invalid camera rejection");
}
