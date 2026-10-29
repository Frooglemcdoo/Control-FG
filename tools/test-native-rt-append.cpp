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
    std::cout<<"PASS RT append: record bytes/order, unaligned storage, capacity flags, exact return, growth and alias delegation\n";
}
