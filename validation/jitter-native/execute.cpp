#include <sys/mman.h>
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
#include "native-multiply.h"
int main(){
 void* memory=mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);assert(memory!=MAP_FAILED);
 std::memcpy(memory,nativeMultiply,sizeof(nativeMultiply));assert(mprotect(memory,4096,PROT_READ|PROT_EXEC)==0);
 using Fn=void(__attribute__((ms_abi)) *)(double*,const double*);auto fn=reinterpret_cast<Fn>(memory);
 unsigned seed=42;auto random=[&](){seed=seed*1664525+1013904223;return double(int(seed%10000)-5000)/1000;};
 for(int test=0;test<10000;++test){
  alignas(16) double a[16],b[16],expected[16]{};
  for(auto& v:a){v=random();}for(auto& v:b){v=random();}
  for(int row=0;row<4;++row)for(int col=0;col<4;++col)for(int k=0;k<4;++k)expected[4*row+col]+=a[4*row+k]*b[4*k+col];
  fn(a,b);for(int i=0;i<16;++i)assert(std::fabs(a[i]-expected[i])<1e-10);
 }
 alignas(16) double p[16]={1.25,0,0,0,0,2,0,0,0,0,1.001,1,0,0,-.1,0};
 double t[16]={1,0,0,0,0,1,0,0,0,0,1,0,.00025,-.0005,0,1};fn(p,t);
 assert(p[8]==t[12]&&p[9]==t[13]&&p[0]==1.25&&p[5]==2);
 assert(munmap(memory,4096)==0);
 puts("PASS: exact Control matrix instructions executed under Microsoft x64 ABI; 10000 independent matrix comparisons; projection posttranslation adds jitter to m8/m9");
}
