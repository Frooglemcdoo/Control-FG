#include "../../src/rr_evaluation_tail.h"
#include <cassert>
#include <cstdio>
#include <sys/mman.h>
#include <unistd.h>
using namespace control_rr;
static unsigned called=0;
extern "C" std::uint32_t __attribute__((ms_abi,noinline)) target(void* list,void* feature,void* parameters,void* callback) {
    assert(list==reinterpret_cast<void*>(0x1111) && feature==reinterpret_cast<void*>(0x2222));
    assert(parameters==reinterpret_cast<void*>(0x3333) && callback==reinterpret_cast<void*>(0x4444));
    ++called;return 0xbad00007u;
}
int main() {
    Jump5 original{},patch{};Tail14 tail{};
    assert(RelativeJump(0x1000,UINT64_C(0x1005)+INT32_MAX,patch));
    assert(!RelativeJump(0x1000,0x1006+UINT64_C(2147483647),patch));
    assert(RelativeJump(0x80000000,5,patch));assert(!RelativeJump(UINT64_MAX,0,patch));
    assert(AbsoluteTail(reinterpret_cast<std::uintptr_t>(&target),tail));
    void* page=mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(page!=MAP_FAILED);
    const auto addr=reinterpret_cast<std::uintptr_t>(page);
    assert(RelativeJump(addr,addr+64,original));assert(CheckedTailPatch(original,addr,addr+64,addr+32,patch));
    std::memcpy(page,patch.data(),patch.size());std::memcpy(static_cast<unsigned char*>(page)+32,tail.data(),tail.size());
    assert(mprotect(page,4096,PROT_READ|PROT_EXEC)==0);
    using Function=std::uint32_t(__attribute__((ms_abi)) *)(void*,void*,void*,void*);
    auto fn=reinterpret_cast<Function>(page);
    for(unsigned i=0;i<10000;++i) assert(fn(reinterpret_cast<void*>(0x1111),reinterpret_cast<void*>(0x2222),reinterpret_cast<void*>(0x3333),reinterpret_cast<void*>(0x4444))==0xbad00007u);
    assert(called==10000);assert(munmap(page,4096)==0);
    original[0]=0xe8;assert(!CheckedTailPatch(original,addr,addr+64,addr+32,patch));assert(patch==Jump5{});
    assert(!AbsoluteTail(0,tail));
    puts("PASS: executed x64 tail relay 10000 times using Microsoft ABI; four arguments and return preserved; patch mismatch/range rejection");
}
