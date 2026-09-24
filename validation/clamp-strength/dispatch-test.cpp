#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <cassert>
#include <cstdio>
static void Log(const char*,...) noexcept {}
#include "../../src/rr_clamp_strength.h"
using namespace control_rr_clamp;
static ID3D12PipelineState* current=nullptr;
static ID3D12PipelineState* used=nullptr;
static unsigned calls=0;
static bool raiseFault=false,observe=true,wrong=false;
static void STDMETHODCALLTYPE FakeSet(ID3D12GraphicsCommandList*,ID3D12PipelineState* p){current=p;}
static void STDMETHODCALLTYPE FakeDispatch(ID3D12GraphicsCommandList*,UINT,UINT,UINT){++calls;used=current;if(raiseFault)RaiseException(0xe0421234u,0,0,nullptr);}
struct FakeList {void** table;};
static ID3D12GraphicsCommandList* testList=nullptr;
static ID3D12PipelineState* native=reinterpret_cast<ID3D12PipelineState*>(0x1000);
static ID3D12PipelineState* variant=reinterpret_cast<ID3D12PipelineState*>(0x2000);
static void Native(void*,const void*){
 auto** table=*reinterpret_cast<void***>(testList);
 if(observe)reinterpret_cast<SetFn>(table[25])(testList,wrong?reinterpret_cast<ID3D12PipelineState*>(0x3000):native);
 reinterpret_cast<DispatchFn>(table[14])(testList,1,1,1);
}
int main(){
 void* table[26]{};table[25]=reinterpret_cast<void*>(&FakeSet);table[14]=reinterpret_cast<void*>(&FakeDispatch);
 FakeList list{table};testList=reinterpret_cast<ID3D12GraphicsCommandList*>(&list);
 Bundle b{};b.native=native;b.variants[Variant(50)]=variant;bundles[0].store(&b);
 assert(Invoke(&Native,nullptr,nullptr,testList,50));assert(calls==1&&used==variant&&current==native&&effective.load()==50&&active==nullptr);
 calls=0;assert(Invoke(&Native,nullptr,nullptr,testList,100));assert(calls==1&&used==native&&current==native&&effective.load()==100);
 observe=false;calls=0;assert(Invoke(&Native,nullptr,nullptr,testList,50));assert(calls==1&&used==native&&!applied.load());observe=true;
 wrong=true;calls=0;assert(Invoke(&Native,nullptr,nullptr,testList,50));assert(calls==1&&used!=variant&&!applied.load());wrong=false;
 calls=0;assert(Invoke(&Native,nullptr,nullptr,testList,75));assert(calls==1&&used==native&&!applied.load());
 raiseFault=true;bool caught=false;
 __try {Invoke(&Native,nullptr,nullptr,testList,50);} __except(EXCEPTION_EXECUTE_HANDLER){caught=true;}
 assert(caught&&current==native&&active==nullptr);raiseFault=false;
 calls=0;Native(nullptr,nullptr);assert(calls==1&&used==native&&active==nullptr);
 bundles[0].store(nullptr);
 std::puts("PASS CS3 actual dispatch adapter: scoped replacement once, exact PSO restoration, native/unobserved/wrong/missing fallback, SEH restoration and RR-off passthrough");
}
