// Execute the exact native copyRegion body with synthetic TLS/COM endpoints.
// This validates its subresource/extent/barrier arithmetic, not D3D12 execution.
#include <sys/mman.h>
#include <sys/syscall.h>
#include <asm/prctl.h>
#include <unistd.h>
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstring>
#include <cstdio>
#define ABI __attribute__((ms_abi))
using HMODULE=void*;
static void* GetProcAddress(HMODULE,const char*){return nullptr;}
#define __except(x) catch(...)
#include "../../src/rr_specular_noisy_windows.h"
#undef __except
using Int3=control_rr_specular::NativeCopyApi::Int3;
static unsigned char* image;
static std::array<unsigned char,0x90> source{},target{};
static std::array<unsigned char,0x30> commandContext{};
static std::array<void*,17> resourceVtable{},commandVtable{};
struct Object {void** table;};
static Object sourceResource{resourceVtable.data()},targetResource{resourceVtable.data()},commandList{commandVtable.data()};
static unsigned copies=0,barrierCalls=0,bridges=0;
template<class T>static T load(const void* address){T value;std::memcpy(&value,address,sizeof(value));return value;}
template<class T>static void put(void* address,T value){std::memcpy(address,&value,sizeof(value));}
static void* ABI Desc(void* self,void* output){assert(self==&sourceResource||self==&targetResource);std::memset(output,0,56);put(output,3u);return output;}
static void ABI Barriers(std::uint64_t count,const unsigned char* entries){
 assert(count==2);++barrierCalls;
 for(unsigned i=0;i<2;++i){const auto* b=entries+i*32;
  assert(load<unsigned>(b)==0&&load<void*>(b+8)==(i?static_cast<void*>(&targetResource):&sourceResource));
  assert(load<unsigned>(b+16)==UINT32_MAX);
  const unsigned normal=i?8:64,copy=i?1024:2048;
  assert(load<unsigned>(b+20)==(barrierCalls%2?normal:copy)&&load<unsigned>(b+24)==(barrierCalls%2?copy:normal));
 }
}
static void ABI CopyTexture(void* self,const unsigned char* dst,unsigned x,unsigned y,unsigned z,const unsigned char* src,const unsigned* box){
 assert(self==&commandList&&x==0&&y==0&&z==0);
 assert(load<void*>(dst)==&targetResource&&load<void*>(src)==&sourceResource);
 assert(load<unsigned>(dst+8)==0&&load<unsigned>(src+8)==0); // subresource location type
 assert(load<unsigned>(dst+16)==0&&load<unsigned>(src+16)==0); // mip zero, layer zero
 assert(box&&box[0]==0&&box[1]==0&&box[2]==0&&box[3]==2560&&box[4]==1440&&box[5]==1);
 ++copies;
}
static void ABI Cookie(std::uintptr_t value){assert(value==load<std::uintptr_t>(image+0xf6bf0));}
using NativeCopy=void(ABI *)(void*,int,int,const Int3&,void*,int,int,const Int3&,const Int3*);
static void Bridge(void* dst,int dstLayer,int dstMip,const Int3& dstOffset,void* src,int srcLayer,int srcMip,const Int3& srcOffset,const Int3* extent){
 ++bridges;assert(dst==target.data()&&src==source.data()&&dstLayer==0&&dstMip==0&&srcLayer==0&&srcMip==0&&!extent);
 reinterpret_cast<NativeCopy>(image+0x3d190)(dst,dstLayer,dstMip,dstOffset,src,srcLayer,srcMip,srcOffset,extent);
}
static void Stub(unsigned rva,void* function){const unsigned char jump[]{0xff,0x25,0,0,0,0};std::memcpy(image+rva,jump,6);put(image+rva+6,function);}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream file(argv[1],std::ios::binary);std::vector<unsigned char> raw((std::istreambuf_iterator<char>(file)),{});assert(raw.size()>0x111c28);
 image=static_cast<unsigned char*>(mmap(nullptr,raw.size(),PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));assert(image!=MAP_FAILED);std::memcpy(image,raw.data(),raw.size());
 Stub(0x3b000,reinterpret_cast<void*>(&Barriers));Stub(0x57070,reinterpret_cast<void*>(&Cookie));
 resourceVtable[10]=reinterpret_cast<void*>(&Desc);commandVtable[16]=reinterpret_cast<void*>(&CopyTexture);
 for(auto* texture:{&source,&target}){put(texture->data(),2560u);put(texture->data()+4,1440u);put(texture->data()+8,1u);}
 put(source.data()+0x88,&sourceResource);put(target.data()+0x88,&targetResource);
 put(source.data()+0x60,64u);put(target.data()+0x60,8u);put(commandContext.data()+0x18,&commandList);
 put(image+0x111c18,commandContext.data());put(image+0x111c20,std::uintptr_t(0));put(image+0x1115fc,0u);
 std::array<std::uintptr_t,16> teb{},tls{},tlsArray{};teb[11]=reinterpret_cast<std::uintptr_t>(tlsArray.data());tlsArray[0]=reinterpret_cast<std::uintptr_t>(tls.data());
 unsigned long savedGs=0;assert(syscall(SYS_arch_prctl,ARCH_GET_GS,&savedGs)==0);assert(syscall(SYS_arch_prctl,ARCH_SET_GS,teb.data())==0);
 assert(mprotect(image+0x3b000,0x1000,PROT_READ|PROT_EXEC)==0);
 assert(mprotect(image+0x3d000,0x1000,PROT_READ|PROT_EXEC)==0);
 assert(mprotect(image+0x57000,0x1000,PROT_READ|PROT_EXEC)==0);
 control_rr_specular::NativeCopyApi api;api.copy=&Bridge;
 for(unsigned sm=1;sm<=12;++sm)for(unsigned tm=1;tm<=12;++tm){
  put(source.data()+0x10,sm);put(target.data()+0x10,tm);
  assert(api.CopyNative(reinterpret_cast<std::uintptr_t>(target.data()),reinterpret_cast<std::uintptr_t>(source.data())));
 }
 assert(syscall(SYS_arch_prctl,ARCH_SET_GS,savedGs)==0);
 assert(copies==144&&bridges==144&&barrierCalls==288&&load<std::uint64_t>(commandContext.data()+8)==144);
 assert(load<unsigned>(source.data()+0x60)==64&&load<unsigned>(target.data()+0x60)==8);
 munmap(image,raw.size());
 puts("PASS exact native copyRegion plus production adapter: 144 unequal/equal mip pairs, mip-zero/layer-zero copies, full base extent, both state transitions/restorations, native command count");
}
