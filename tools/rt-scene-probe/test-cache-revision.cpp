#include <windows.h>
#include <array>
#include <cassert>
#include <cstdio>
#include "../../src/rt_cache_revision.h"
#include "../../src/rt_cache_revision_patch.h"
using namespace rt_cache_revision;
static Revisions revisions;
using Create=void*(*)(void*,void*,std::int32_t,std::int32_t,unsigned char,void*);
using Notify=void(*)(void*);
static Create createOriginal;
static Notify notifyOriginal;
static unsigned seen=0;
static void* CreateTail(void* a,void* b,std::int32_t c,std::int32_t d,unsigned char e,void* f){assert(a==reinterpret_cast<void*>(0x1230)&&b==reinterpret_cast<void*>(0x4560)&&c==-17&&d==43&&e==1&&f==reinterpret_cast<void*>(0x7890));++seen;return f;}
static void* CreateHook(void* a,void* b,std::int32_t c,std::int32_t d,unsigned char e,void* f){revisions.ProviderCreated(a);return createOriginal(a,b,c,d,e,f);}
static void NotifyTail(void* a){assert(a==reinterpret_cast<void*>(0x1230));++seen;}
static void NotifyHook(void* a){revisions.ProviderDestroyed(a);notifyOriginal(a);}
template<std::size_t N,std::size_t U>static void TestPatch(const unsigned char (&entry)[N],const unsigned char (&unwind)[U],unsigned kind){
 auto* code=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));assert(code);
 std::memcpy(code,entry,N);unsigned end=static_cast<unsigned>(N);
 if(kind==1){const unsigned char undo[]={0x48,0x83,0xc4,0x40,0x5f};std::memcpy(code+end,undo,sizeof(undo));end+=sizeof(undo);}
 if(kind==2){const unsigned char undo[]={0x48,0x83,0xc4,0x30,0x5f,0x5e,0x5d};std::memcpy(code+end,undo,sizeof(undo));end+=sizeof(undo);}
 AbsoluteJump(code+end,reinterpret_cast<std::uintptr_t>(kind?reinterpret_cast<void*>(&NotifyTail):reinterpret_cast<void*>(&CreateTail)));
 std::memcpy(code+0x80,entry,N);ResumeJump(code+0x80+N,reinterpret_cast<std::uintptr_t>(code+N));std::memcpy(code+0x200,unwind,U);
 AbsoluteJump(code+0x100,reinterpret_cast<std::uintptr_t>(kind?reinterpret_cast<void*>(&NotifyHook):reinterpret_cast<void*>(&CreateHook)));
 unsigned char patch[8]{};std::memset(patch,0x90,N);assert(Jump5(reinterpret_cast<std::uintptr_t>(code),reinterpret_cast<std::uintptr_t>(code+0x100),patch));std::memcpy(code,patch,N);
 createOriginal=reinterpret_cast<Create>(code+0x80);notifyOriginal=reinterpret_cast<Notify>(code+0x80);
 DWORD old=0;assert(VirtualProtect(code,4096,PAGE_EXECUTE_READ,&old));assert(FlushInstructionCache(GetCurrentProcess(),code,4096));
 const auto before=seen;
 for(unsigned i=0;i<1024;++i){
  if(kind)reinterpret_cast<Notify>(code)(reinterpret_cast<void*>(0x1230));
  else assert(reinterpret_cast<Create>(code)(reinterpret_cast<void*>(0x1230),reinterpret_cast<void*>(0x4560),-17,43,1,reinterpret_cast<void*>(0x7890))==reinterpret_cast<void*>(0x7890));
 }
 assert(seen-before==1024);
 // Unwind the exact production copied prologues, including both REX-prefixed PUSHes.
 RUNTIME_FUNCTION function{0x80,static_cast<DWORD>(0x80+N+13),0x200};
 for(unsigned pcOffset:{0u,10u}){
 std::array<DWORD64,64> stack{};auto* original=stack.data()+32;original[0]=0x12345678;
 CONTEXT context{};context.Rip=reinterpret_cast<DWORD64>(code+0x80+N+pcOffset);context.Rsp=reinterpret_cast<DWORD64>(original);
 if(kind==1){original[-1]=0x2222;context.Rsp-=72;}
 if(kind==2){original[-1]=0x3333;original[-2]=0x4444;original[-3]=0x5555;context.Rsp-=72;}
 PVOID handler=nullptr;DWORD64 frame=0;
 RtlVirtualUnwind(UNW_FLAG_NHANDLER,reinterpret_cast<DWORD64>(code),context.Rip,&function,&context,&handler,&frame,nullptr);
 assert(context.Rip==original[0]&&context.Rsp==reinterpret_cast<DWORD64>(original+1));
 if(kind==1)assert(context.Rdi==0x2222);
 if(kind==2)assert(context.Rbp==0x3333&&context.Rsi==0x4444&&context.Rdi==0x5555);
 }
 assert(VirtualFree(code,0,MEM_RELEASE));
}
int main(){
 Entry entry;Observation a{0x1230,0x4560,0,0,7,11};assert(Observe(entry,a)==Outcome::Cold);assert(Observe(entry,a)==Outcome::Unchanged);
 a.payload=12;assert(Observe(entry,a)==Outcome::Untracked);a.guard=8;assert(Observe(entry,a)==Outcome::Invalidated);a.provider=0x1240;assert(Observe(entry,a)==Outcome::Cold);
 for(unsigned i=0;i<65536;++i){revisions.MaterialChanged(static_cast<std::uint16_t>(i));revisions.MaterialChanged(static_cast<std::uint16_t>(i));assert(revisions.materials[i].load()==2);}
 assert(revisions.notifications.load()==131072);
 unsigned char jump[5]{};assert(!Jump5(0x1000,0x100000000ull,jump));assert(Jump5(0x1000,0x80001004ull,jump));assert(!Jump5(0x1000,0x80001005ull,jump));
 TestPatch(ConstructorEntry,ConstructorUnwind,0);TestPatch(DestructorEntry,DestructorUnwind,1);TestPatch(MaterialEntry,MaterialUnwind,2);
 assert(revisions.creates.load()==1024&&revisions.destroys.load()==2048);
 std::puts("PASS revision ID reuse/dirty repeats, observation mismatch classification, 3072 production entry-patch calls, six-argument forwarding, return values, and Windows virtual unwind. Reuse remains disabled.");
}
