#include <windows.h>
#include <cassert>
#include <cstdio>
#include <stdexcept>
static HMODULE verifiedRenderer=nullptr,verifiedD3d=nullptr;
static constexpr char kRendererHash[]="fixture",kD3dHash[]="fixture";
static bool HashMatches(HMODULE,const char*)noexcept{return true;}
#include "../src/gi_native_render_scope.h"
#include "../src/gi_native_prepare_scope.h"
namespace s=gi_native_render_scope;
static void* expectedRoot=nullptr;static void* expectedPass=nullptr;
static unsigned calls=0,exits=0,aborts=0;static bool throwNative=false;
static unsigned preparationCalls=0,preparationEntries=0,preparationExits=0;
static void Prepare(void* root,void* argument){
 assert(root==expectedRoot&&argument==expectedPass);
 assert(preparationEntries==preparationExits+1);
 ++preparationCalls;if(throwNative)throw std::runtime_error("prepare failure");
}
static void Native(void* root,void* pass){
 assert(root==expectedRoot&&pass==expectedPass);
 assert(s::current.token&&s::current.owner==*static_cast<void**>(root)&&s::current.pass==pass&&s::current.frame==11);
 ++calls;if(throwNative)throw std::runtime_error("native failure");
}
int main(){
 auto* r=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x640000,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE));
 auto* d=static_cast<unsigned char*>(VirtualAlloc(nullptr,0x140000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
 assert(r&&d);verifiedRenderer=reinterpret_cast<HMODULE>(r);verifiedD3d=reinterpret_cast<HMODULE>(d);
 *reinterpret_cast<s::U64*>(d+0x136c68)=11;
 const unsigned char thunk[]={0x48,0x8b,0x12,0x48,0x83,0xc1,0x08,0xe9,0xb4,0xa4,0xfe,0xff};
 std::memcpy(r+0x13e860,thunk,sizeof(thunk));
 const unsigned char tail[]={0xff,0x25,0,0,0,0};std::memcpy(r+0x128d20,tail,6);
 auto* target=reinterpret_cast<void*>(&Native);std::memcpy(r+0x128d26,&target,8);
 auto** slot=reinterpret_cast<void**>(r+0x631da0);*slot=r+0x13e860;
 assert(!s::Install(nullptr,verifiedD3d));assert(s::Install(verifiedRenderer,verifiedD3d));
 assert(!s::Install(verifiedRenderer,verifiedD3d));
 void* closure[2]{nullptr,reinterpret_cast<void*>(100)};void* pass=reinterpret_cast<void*>(200);
 expectedRoot=closure+1;expectedPass=pass;
 s::leaving=+[](const s::Scope& scope,bool returned)noexcept{
  assert(scope.token==s::current.token&&scope.owner==reinterpret_cast<void*>(100));++exits;if(!returned)++aborts;
 };
 s::current={77,8,reinterpret_cast<void*>(300),reinterpret_cast<void*>(400)};
 reinterpret_cast<s::Native>(*slot)(closure,&pass);
 assert(calls==1&&exits==1&&!aborts&&s::current.token==77&&s::current.frame==8);
 throwNative=true;
 try{reinterpret_cast<s::Native>(*slot)(closure,&pass);assert(false);}catch(const std::runtime_error&){}
 assert(calls==2&&exits==2&&aborts==1&&s::current.token==77&&s::current.owner==reinterpret_cast<void*>(300));
 namespace prep=gi_native_prepare_scope;
 const unsigned char prepareThunk[]={0x48,0x83,0xc1,0x08,0xe9,0xf7,0x5e,0xfe,0xff};
 std::memcpy(r+0x13de60,prepareThunk,sizeof(prepareThunk));
 std::memcpy(r+0x123d60,tail,6);target=reinterpret_cast<void*>(&Prepare);std::memcpy(r+0x123d66,&target,8);
 auto** prepareSlot=reinterpret_cast<void**>(r+0x632270);*prepareSlot=r+0x13de60;
 assert(prep::Install(verifiedRenderer,verifiedD3d));assert(!prep::Install(verifiedRenderer,verifiedD3d));
 prep::entering=+[](const prep::Scope& scope)noexcept{
  assert(scope.token&&scope.frame==11&&scope.owner==reinterpret_cast<void*>(100)&&scope.pass==reinterpret_cast<void*>(200));
  ++preparationEntries;
 };
 prep::leaving=+[](const prep::Scope& scope,bool returned)noexcept{
  assert(scope.token&&returned==!throwNative);++preparationExits;
 };
 void* prepareClosure[6]{};prepareClosure[5]=reinterpret_cast<void*>(100);
 expectedRoot=prepareClosure+1;expectedPass=&pass;throwNative=false;
 reinterpret_cast<s::Native>(*prepareSlot)(prepareClosure,&pass);
 throwNative=true;
 try{reinterpret_cast<s::Native>(*prepareSlot)(prepareClosure,&pass);assert(false);}catch(const std::runtime_error&){}
 assert(preparationCalls==2&&preparationEntries==2&&preparationExits==2&&s::current.token==77);
 *prepareSlot=r+0x13de60;prep::original=nullptr;prep::d3d=nullptr;prep::entering=nullptr;prep::leaving=nullptr;
 s::Scope read{};assert(!s::Read(reinterpret_cast<void*>(1),&pass,&read)&&!read.token&&!read.owner);
 s::serial=UINT64_MAX;assert(s::Next()==0&&s::serial==UINT64_MAX);
 *slot=r+0x13e860;s::original=nullptr;s::d3d=nullptr;s::leaving=nullptr;
 VirtualFree(r,0,MEM_RELEASE);VirtualFree(d,0,MEM_RELEASE);
 std::puts("PASS native GI render scope: exact render/prepare thunk forwarding, preparation entry before native work, full callback scope, normal/exception cleanup and prior TLS restoration.");
}
