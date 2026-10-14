#include "../src/pt_scene_descriptors.h"
#include <cassert>
#include <cstdio>
#include <memory>
#include <random>
#include <thread>
int main(){
    using namespace PTDescriptorInterest;
    auto isolated=std::make_unique<Filter>();
    assert(!isolated->NeedsSimpleCopy(64,128));
    assert(isolated->NeedsSimpleCopy(0,128));
    isolated->Mark(128);assert(isolated->NeedsSimpleCopy(64,128));
    assert(isolated->NeedsSimpleCopy(128,256));
    isolated->IncludeMirror(4096,8192);
    assert(isolated->NeedsSimpleCopy(4096,256)&&isolated->NeedsSimpleCopy(8191,256));
    isolated->IncludeMirror(16384,32768);
    assert(isolated->NeedsSimpleCopy(12000,256)); // Conservative enclosing gap.
    std::vector<std::thread> workers;
    for(unsigned t=0;t<8;++t)workers.emplace_back([&,t]{
        for(U64 i=1;i<=20000;++i){const U64 a=(i*8+t)*32;isolated->Mark(a);assert(isolated->Contains(a));}
    });
    for(auto& worker:workers)worker.join();
    for(U64 i=1;i<=20000;++i)for(U64 t=0;t<8;++t)assert(isolated->Contains((i*8+t)*32));
    // Differential history: AS creation, chained copies, unknown overwrites,
    // address reuse and bulk copies. The reference never uses the filter.
    PTSceneDescriptors::Index index(32);
    std::map<U64,PTSceneDescriptors::Acceleration> expected;
    std::mt19937 random(42);unsigned skipped=0;
    for(unsigned n=0;n<100000;++n){
        const U64 dst=0x100000000ull+U64(random()%4096)*32;
        const U64 src=0x100000000ull+U64(random()%4096)*32;
        const auto action=random()%10;
        if(action==0){
            const PTSceneDescriptors::Acceleration value{0x10000,n+1,256};
            assert(index.Write(dst,value));expected[dst]=value;
        }else if(action==1){assert(index.Write(dst));expected.erase(dst);}
        else {
            const auto it=expected.find(src);
            const auto value=it==expected.end()?PTSceneDescriptors::Acceleration{}:it->second;
            if(action==2)assert(index.Copy({{dst,1}},{{src,1}}));
            else if(filter.NeedsSimpleCopy(dst,src))assert(index.CopySimple(dst,src,1));
            else {++skipped;assert(!value.token&&expected.find(dst)==expected.end());}
            if(value.token)expected[dst]=value;else expected.erase(dst);
        }
        const auto found=expected.find(dst);
        assert(index.Read(dst).token==(found==expected.end()?0:found->second.token));
    }
    assert(skipped>1000);
    for(const auto& entry:expected)assert(index.Read(entry.first).token==entry.second.token);
    std::printf("PASS: concurrent monotonic interest, mirror bounds and 100000 reference operations (%u safely skipped)\n",skipped);
}
