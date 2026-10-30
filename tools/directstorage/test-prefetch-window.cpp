#include "../../src/directstorage_prefetch_window.h"
#include <cassert>
#include <cstdio>
#include <cstdint>
int main(){
    using namespace control_storage;
    constexpr unsigned maximum=2*1024*1024;
    auto w=RequestWindow(713,1367,6ull*maximum,maximum);
    assert(w.offset==0&&w.size==65536);
    w=RequestWindow(maximum-10,20,6ull*maximum,maximum);
    assert(w.offset==maximum-65536&&w.size==131072);
    w=RequestWindow(713,maximum,6ull*maximum,maximum);
    assert(w.offset==713&&w.size==maximum);
    assert(!RequestWindow(0,0,100,maximum).size);
    assert(!RequestWindow(0,maximum+1,UINT64_MAX,maximum).size);
    assert(!RequestWindow(UINT64_MAX,1,UINT64_MAX,maximum).size);
    std::uint64_t random=234;
    for(unsigned i=0;i<1000000;++i){
        random^=random<<13;random^=random>>7;random^=random<<17;
        const auto offset=random;const unsigned count=static_cast<unsigned>(random%maximum)+1;
        const auto fileSize=offset>UINT64_MAX-count?UINT64_MAX:offset+count+(UINT64_MAX-offset-count)/2;
        w=RequestWindow(offset,count,fileSize,maximum);
        if(count>fileSize-offset){assert(!w.size);continue;}
        assert(w.size&&w.size<=maximum&&w.offset<=offset);
        assert(offset-w.offset<=w.size&&count<=w.size-(offset-w.offset));
        assert(w.size<=fileSize-w.offset);
        assert(w.size<=static_cast<std::uint64_t>(count)+2*PrefetchGranularity-2);
    }
    puts("PASS R234 prefetch windows: request coverage, bounded size, EOF, overflow and one million randomized ranges");
}
