#include "../src/pt_glass_vtable_call.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    // Actual instructions at Steam d3d +354a5 and +358e4.
    const unsigned char native[2][6]={{0xff,0x90,0x58,2,0,0},{0xff,0x90,0x60,2,0,0}};
    const std::uintptr_t sites[2]={0x1800354a5ull,0x1800358e4ull};
    for(unsigned i=0;i<2;++i)for(const std::uintptr_t relay:{0x180010000ull,0x180100000ull}){
        unsigned char patched[6]{};assert(PTGlass::EncodeVtableCall6(native[i],sites[i],75+i,relay,patched));
        std::int32_t delta=0;std::memcpy(&delta,patched+1,4);
        assert(patched[0]==0xe8&&patched[5]==0x90);
        assert(static_cast<std::int64_t>(sites[i]+5)+delta==static_cast<std::int64_t>(relay));
        assert(!PTGlass::EncodeVtableCall6(native[i],sites[i],74+i,relay,patched));
        assert(!PTGlass::EncodeVtableCall6(native[i],sites[i],75+i,0x900000000ull,patched));
    }
    puts("PASS: exact native DXR call encodings, forward/backward relays, continuation and mismatch/range rejection.");
}
