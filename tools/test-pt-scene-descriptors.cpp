#include "../src/pt_scene_descriptors.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace PTSceneDescriptors;
int main(){
    Index index(32);const char* reason=nullptr;
    assert(!index.Read(1000,&reason).token&&std::string(reason)=="descriptor_missing");
    assert(index.Write(1000,{0x10000,7,256}));
    assert(index.Copy({{2000,4}},{{968,4}}));
    assert(index.Read(2032).token==7&&index.Read(2000).token==0);
    // Split destinations and sources retain exact sequence positions.
    assert(index.Write(3000,{0x20000,8,512}));
    assert(index.Copy({{4000,1},{5000,3}},{{968,2},{3000,2}}));
    assert(index.Read(4000).token==0&&index.Read(5000).token==7&&index.Read(5032).token==8);
    assert(index.Write(5000));assert(index.Read(5000).token==0); // Overwrite with a non-AS descriptor.
    assert(index.Copy({{5032,1}},{{9000,1}}));assert(index.Read(5032).token==0);
    assert(index.Invalidate({2000,4}));assert(index.Read(2032).token==0); // Heap range reuse.
    assert(index.Write(1000,{0x10000,9,256}));assert(index.Read(1000).token==9);
    Index sparse(32);assert(sparse.Write(1000,{0x10000,9,256}));
    assert(sparse.Copy({{64000000,1000000}},{{1000,1000000}})); // Sparse copy, no million-row temporary.
    assert(sparse.Read(64000000).token==9);
    assert(!index.Copy({{7000,2}},{{1000,1}}));assert(index.Read(1000).token==0); // Invalid transaction fails closed.
    assert(!index.Write(1000,{0x10000,10,256}));
    assert(!index.Read(1000,&reason).token&&std::string(reason)=="descriptor_copy_count");
    const auto stats=index.Statistics();assert(!stats.healthy&&std::string(stats.failure)=="descriptor_copy_count"&&stats.asWrites&&stats.copies);
    Index alignment(32);assert(alignment.Write(1001,{0x10000,10,256}));
    assert(!alignment.Copy({{2000,1}},{{1000,1}}));
    assert(std::string(alignment.Statistics().failure)=="descriptor_copy_alignment");
    Index overflow(32);assert(!overflow.Invalidate({UINT64_MAX-16,1}));
    std::puts("PASS AS descriptor identities: sparse copies, split ranges, overwrite, heap reuse, generation preservation and fail-closed bounds.");
}
