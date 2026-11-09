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
static unsigned releases;
static std::uintptr_t Read(unsigned rva){std::uintptr_t x;std::memcpy(&x,image+rva,8);return x;}
static void Write(unsigned rva,std::uintptr_t x){std::memcpy(image+rva,&x,8);}
static void ABI Release(void* pool,void* texture){
 assert(reinterpret_cast<std::uintptr_t>(pool)==0x12340000);
 assert(reinterpret_cast<std::uintptr_t>(texture)==0x11110000);
 assert(Read(0x90eac8)==0);++releases;
}
int main(int argc,char**argv){
 assert(argc==2);std::ifstream f(argv[1],std::ios::binary);
 std::vector<unsigned char> raw((std::istreambuf_iterator<char>(f)),{});assert(!raw.empty());
 image=static_cast<unsigned char*>(mmap(nullptr,raw.size(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
 assert(image!=MAP_FAILED);std::memcpy(image,raw.data(),raw.size());
 const unsigned char tail[]{0xff,0x25,0,0,0,0};void* release=reinterpret_cast<void*>(&Release);
 std::memcpy(image+0xa9e00,tail,6);std::memcpy(image+0xa9e06,&release,8);
 // Enter the exact native decision with aligned Win64 shadow space. Only the
 // two exits are replaced; the option getter and history-detach routine run.
 const unsigned char enter[]{0x48,0x83,0xec,0x28,0xe9};
 std::memcpy(image+0x1000,enter,5);const std::int32_t delta=0x12cd3e - 0x1009;
 std::memcpy(image+0x1005,&delta,4);
 unsigned char leave[]{0xb8,0,0,0,0,0x48,0x83,0xc4,0x28,0xc3};
 std::memcpy(image+0x12cd5f,leave,sizeof(leave));leave[1]=1;
 std::memcpy(image+0x12cf4b,leave,sizeof(leave));
 for(unsigned page:{0x1000u,0x13000u,0xa9000u,0x12c000u})assert(mprotect(image+page,4096,PROT_READ|PROT_EXEC)==0);
 using Branch=unsigned(ABI *)();const auto branch=reinterpret_cast<Branch>(image+0x1000);
 Write(0x8028d8,0x12340000);
 for(unsigned enabled=0;enabled<2;++enabled)for(unsigned populated=0;populated<2;++populated){
  image[0x914620]=static_cast<unsigned char>(enabled);Write(0x90eac8,populated?0x11110000:0);releases=0;
  assert(branch()==enabled);assert(releases==enabled*populated);
  assert(Read(0x90eac8)==(enabled?0:populated?0x11110000:0));
  assert(branch()==enabled&&releases==enabled*populated);
 }
 munmap(image,raw.size());
 puts("PASS exact native GI RR/SR branch, option getter, detach-before-release, empty/populated history and repeated bypass");
}
