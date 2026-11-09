#include <sys/mman.h>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
#define ABI __attribute__((ms_abi))
static unsigned char* image;
static std::vector<std::uintptr_t> releases;
static std::uintptr_t Read(unsigned rva){std::uintptr_t value;std::memcpy(&value,image+rva,8);return value;}
static void Write(unsigned rva,std::uintptr_t value){std::memcpy(image+rva,&value,8);}
static void ABI Release(void* pool,void* texture){
 assert(reinterpret_cast<std::uintptr_t>(pool)==0x12340000);
 auto t=reinterpret_cast<std::uintptr_t>(texture);
 // The exact native code must detach each pointer BEFORE delegating release.
 if(t==0x11110000)assert(Read(0x912b18)==0);else {assert(t==0x22220000);assert(Read(0x912b20)==0);}
 releases.push_back(t);
}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream file(argv[1],std::ios::binary);std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});assert(!raw.empty());
 image=static_cast<unsigned char*>(mmap(nullptr,raw.size(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(image!=MAP_FAILED);std::memcpy(image,raw.data(),raw.size());
 unsigned char stub[6]={0xff,0x25,0,0,0,0};void* fn=reinterpret_cast<void*>(&Release);
 std::memcpy(image+0xa9e00,stub,6);std::memcpy(image+0xa9e06,&fn,8);
 // Only executable pages become RX; synthetic native data remains RW.
 assert(mprotect(image+0x13000,0x1000,PROT_READ|PROT_EXEC)==0);
 assert(mprotect(image+0xa9000,0x1000,PROT_READ|PROT_EXEC)==0);
 using Reset=void(ABI *)();auto reset=reinterpret_cast<Reset>(image+0x13e20);
 Write(0x8028d8,0x12340000);
 for(unsigned mask=0;mask<4;++mask){
  releases.clear();Write(0x912b18,(mask&1)?0x11110000:0);Write(0x912b20,(mask&2)?0x22220000:0);
  reset();assert(Read(0x912b18)==0&&Read(0x912b20)==0);
  std::vector<std::uintptr_t> expected;if(mask&1)expected.push_back(0x11110000);if(mask&2)expected.push_back(0x22220000);
  assert(releases==expected);reset();assert(releases==expected);
 }
 puts("PASS exact native reset: empty/first/second/both histories; detach before release; release order; repeated reset does not release twice");
 munmap(image,raw.size());
}
