#include "../src/native_rt_append.h"
#include <array>
#include <cassert>
#include <iostream>
#include <random>
using namespace native_rt_append;
int main(){
    std::mt19937 random(219);
    for(unsigned flag=0;flag<4;++flag)for(unsigned size=0;size<32;++size){
        std::array<unsigned char,32*64+32> actual{},expected{};std::array<unsigned char,80> source{};
        for(auto& b:actual)b=static_cast<unsigned char>(random());
        expected=actual;
        for(auto& b:source)b=static_cast<unsigned char>(random());
        Array array{actual.data()+3,size,32u|(flag<<30)};
        std::memcpy(expected.data()+3+size*64,source.data()+1,64);
        auto* result=AppendIfRoom(&array,source.data()+1);
        assert(result==array.data+size*64&&array.size==size+1&&array.capacityFlags==(32u|(flag<<30)));
        assert(actual==expected);
    }
    std::array<unsigned char,256> data{},source{};Array array{data.data(),4,4};
    assert(!AppendIfRoom(&array,source.data())&&array.size==4); // native growth
    array.size=5;assert(!AppendIfRoom(&array,source.data())&&array.size==5);
    array.size=1;array.capacityFlags=4;
    for(int offset=-63;offset<64;++offset){assert(!AppendIfRoom(&array,data.data()+64+offset));assert(array.size==1);}
    assert(AppendIfRoom(&array,data.data())==data.data()+64); // adjacent is legal
    const auto size=array.size;
    assert(!AppendIfRoom(&array,&array)&&array.size==size);
    assert(!AppendIfRoom(&array,nullptr));assert(!AppendIfRoom(nullptr,source.data()));
    Array null{nullptr,0,2};assert(!AppendIfRoom(&null,source.data()));
    Array bad{nullptr,0,2};bad.data=reinterpret_cast<unsigned char*>(&bad);
    assert(!AppendIfRoom(&bad,source.data()));
    assert(Overlap(100,64,163,1)&&!Overlap(100,64,164,1));
    // Eight-byte native copy uses one temporary: overlapping source is legal.
    for(unsigned flag=0;flag<4;++flag)for(unsigned size8=0;size8<16;++size8){
        std::array<unsigned char,160> actual{},expected{};std::uint64_t value=0x123456789abcdef0ull+size8;
        Array a{actual.data()+1,size8,16u|(flag<<30)};
        std::memcpy(expected.data()+1+size8*8,&value,8);
        assert(AppendPointerIfRoom(&a,&value)==actual.data()+1+size8*8);
        assert(actual==expected&&a.size==size8+1&&a.capacityFlags==(16u|(flag<<30)));
    }
    for(int offset=-7;offset<=7;++offset){
        unsigned char actual[128],expected[128];for(unsigned i=0;i<128;++i)actual[i]=static_cast<unsigned char>(i);
        std::memcpy(expected,actual,128);std::uint64_t value;std::memcpy(&value,expected+64+offset,8);std::memcpy(expected+64,&value,8);
        Array a{actual,8,16};assert(AppendPointerIfRoom(&a,actual+64+offset)==actual+64);
        assert(a.size==9&&std::memcmp(actual,expected,128)==0);
    }
    // Source may refer to the array header: count must increment before load.
    Array headerSource{data.data(),0,4};
    assert(AppendPointerIfRoom(&headerSource,&headerSource.size)==data.data());
    std::uint32_t copiedSize=0,copiedFlags=0;std::memcpy(&copiedSize,data.data(),4);std::memcpy(&copiedFlags,data.data()+4,4);
    assert(copiedSize==1&&copiedFlags==4);
    Array full{data.data(),4,4};assert(!AppendPointerIfRoom(&full,source.data())&&full.size==4);
    Array self{nullptr,0,4};self.data=reinterpret_cast<unsigned char*>(&self);assert(!AppendPointerIfRoom(&self,source.data())&&self.size==0);
    assert(!AppendPointerIfRoom(nullptr,source.data()));assert(!AppendPointerIfRoom(&headerSource,nullptr));
    std::cout<<"PASS RT 64-byte and 8-byte appends: record bytes/order, unaligned storage, capacity flags, exact return, growth and alias delegation\n";
}
