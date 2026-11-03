#include "../src/gi_reuse/instance_sample_validation.h"
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
 unsigned char data[128]{};float matrix[12]={1,0,0,10,0,1,0,20,0,0,1,30};std::memcpy(data,matrix,48);
 std::uint32_t a=0xffabcdef,b=0x02123456;std::uint64_t blas=4096;
 std::memcpy(data+48,&a,4);std::memcpy(data+52,&b,4);std::memcpy(data+56,&blas,8);
 gi_reuse::InstanceSampleSummary s;assert(gi_reuse::ValidateInstanceSample(data,128,s));
 assert(s.count==2&&s.masked==1&&s.nullBLAS==1&&!s.badAlignment&&!s.nonfinite&&!s.unknownFlags);
 assert(s.first[0].id==0xabcdef&&s.first[0].mask==255&&s.first[0].hitGroup==0x123456&&s.first[0].flags==2&&s.first[0].blas==4096&&s.first[0].transform[7]==20);
 assert(!gi_reuse::ValidateInstanceSample(data,127,s));blas=4097;std::memcpy(data+56,&blas,8);
 float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(data,&nan,4);data[55]=128;
 assert(gi_reuse::ValidateInstanceSample(data,64,s)&&s.badAlignment==1&&s.nonfinite==1&&s.unknownFlags==1);
 std::puts("PASS instance arrays: packed IDs/masks/offsets/flags, transforms, null/masked instances and malformed evidence");
}
