#include "../../src/rr_part1_copy_policy.h"
#include <cassert>
#include <iostream>
#include <vector>
using namespace control_rr_part1;
int main(){
    for(std::uint64_t n=1;n<=65536;++n)assert(CopyLayoutAllowed(n*8,8,0xC0,1,2));
    for(auto bytes:{0ull,1ull,7ull,9ull,524289ull,static_cast<unsigned long long>(UINT64_MAX)})
        assert(!CopyLayoutAllowed(bytes,8,0xC0,1,2));
    assert(!CopyLayoutAllowed(8,4,0xC0,1,2));
    for(auto state:{0u,0x40u,0x80u,0x400u,0x800u,0x8C0u})assert(!CopyLayoutAllowed(8,8,state,1,2));
    assert(!CopyLayoutAllowed(8,8,0xC0,2,2));assert(!CopyLayoutAllowed(8,8,0xC0,1,1));
    for(unsigned mask=0;mask<8;++mask){
        std::vector<CopyOp> calls;CopyProgress p{};
        const auto emit=[&](CopyOp op){calls.push_back(op);return !(mask&(1u<<unsigned(op)));};
        const bool okay=RecordCopy(emit,p);
        assert(p.started && okay==(mask==0));
        assert(calls[0]==CopyOp::ToCopySource);
        if(mask&1){assert(calls.size()==1 && !p.restoreAttempted && !p.initial);}
        else {assert(calls.size()==3 && calls[1]==CopyOp::CopyBytes && calls[2]==CopyOp::Restore);
            assert(p.initial && p.copy==!(mask&2) && p.restoreAttempted && p.restored==!(mask&4));}
        assert(!CanMap(okay,0));assert(CanMap(okay,1)==okay);assert(!CanMap(okay,UINT64_MAX));
    }
    std::cout<<"PASS: 65536 table sizes, layout gates, all 8 recording outcomes, restoration and fence gates\n";
}
