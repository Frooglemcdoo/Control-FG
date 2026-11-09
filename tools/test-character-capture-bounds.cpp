#include "../src/pt_character_capture_bounds.h"
#include <cassert>
#include <cstdio>
int main(){
    using namespace PTCharacterCapture;
    std::size_t bytes=123;
    assert(MappedBytes(0x10000,192,0x10080,bytes)&&bytes==128);
    assert(MappedBytes(0x10000,192,0,bytes)&&bytes==192);
    assert(!MappedBytes(0x10000,192,0xffff,bytes)&&bytes==0);
    assert(!MappedBytes(0x10000,192,0x100c1,bytes)&&bytes==0);
    assert(!MappedBytes(UINTPTR_MAX-31,64,0,bytes));
    assert(!MappedBytes(0x10000,MaxRecordBytes+1,0,bytes));
    assert(!MappedBytes(0x10000,0,0,bytes));
    assert(!MappedBytes(0x10000,192,0x10000,bytes));
    for(unsigned n=1;n<20000;++n){
        const unsigned length=n*144;
        assert(RigBytes((length+63)&~63u)==length);
    }
    assert(!RigBytes(64));assert(!RigBytes(144));
    assert(!RigBytes(0));assert(!RigBytes(MaxRecordBytes+64));
    puts("Character capture bounds: valid spans, overflow, limits, empty and rig-padding cases passed.");
}
