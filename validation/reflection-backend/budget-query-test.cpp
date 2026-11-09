// Host COM mocks execute the production QueryBudget method extracted by run.py.
// C++ exceptions stand in for faults; this does not validate Windows SEH.
#include "rr_memory_budget.h"
#include <cassert>
#include <iostream>
using Address=std::uintptr_t;using HRESULT=int;using DWORD=unsigned long;
using REFIID=int;constexpr HRESULT S_OK=0,E_NOINTERFACE=-1,E_FAIL=-2;
#define WINAPI
#define SUCCEEDED(hr) ((hr)>=0)
#define __uuidof(T) IID_##T
#define IID_PPV_ARGS(p) IID_IDXGIAdapter3,reinterpret_cast<void**>(p)
#define __try try
#define __except(x) catch(...)
static DWORD GetExceptionCode(){return 0xBAD;}
constexpr int IID_IDXGIFactory4=4,IID_IDXGIAdapter3=3,DXGI_MEMORY_SEGMENT_GROUP_LOCAL=0;
struct DXGI_QUERY_VIDEO_MEMORY_INFO {std::uint64_t Budget=0,CurrentUsage=0;};
static unsigned failure=0,throwAt=0,factoryReleases=0,adapterReleases=0,queries=0;
static bool nullFactory=false,nullAdapter=false;
static void Fault(unsigned point){if(throwAt==point)throw 1;}
struct ID3D12Device {int GetAdapterLuid(){Fault(2);return 731;}};
struct IDXGIAdapter3 {
 HRESULT QueryVideoMemoryInfo(unsigned node,int segment,DXGI_QUERY_VIDEO_MEMORY_INFO* out){
  ++queries;assert(node==0&&segment==DXGI_MEMORY_SEGMENT_GROUP_LOCAL);Fault(4);
  *out={32ull<<30,8ull<<30};return failure==4?E_FAIL:S_OK;
 }
 void Release(){++adapterReleases;Fault(5);}
} adapter;
struct IDXGIFactory4 {
 HRESULT EnumAdapterByLuid(int luid,int iid,void** out){assert(luid==731&&iid==IID_IDXGIAdapter3);Fault(3);
  if(failure==3)return E_FAIL;
  *out=nullAdapter?nullptr:&adapter;return S_OK;}
 void Release(){++factoryReleases;Fault(6);}
} factory;
static HRESULT Factory(REFIID iid,void** out){assert(iid==IID_IDXGIFactory4);Fault(1);
 if(failure==1)return E_FAIL;
 *out=nullFactory?nullptr:&factory;return S_OK;}
struct Api {
 using CreateFactory=HRESULT (WINAPI*)(REFIID,void**);
 CreateFactory createFactory=&Factory;HRESULT lastBudgetResult=S_OK;DWORD lastBudgetException=0;
#include "budget-query.inc"
};
static void Reset(){failure=throwAt=factoryReleases=adapterReleases=queries=0;nullFactory=nullAdapter=false;}
int main(){ID3D12Device device;const auto address=reinterpret_cast<Address>(&device);Api api;
 control_rr::MemoryBudget result{};
 assert(api.QueryBudget(address,result)&&result.budget==(32ull<<30)&&result.usage==(8ull<<30));
 assert(factoryReleases==1&&adapterReleases==1&&queries==1);
 for(unsigned stage:{1u,3u,4u}){Reset();failure=stage;result={9,9};assert(!api.QueryBudget(address,result));
  assert(!result.budget&&!result.usage&&api.lastBudgetResult==E_FAIL);
  assert(factoryReleases==(stage>1?1u:0u)&&adapterReleases==(stage>3?1u:0u));}
 for(unsigned stage=1;stage<=6;++stage){Reset();throwAt=stage;result={9,9};assert(!api.QueryBudget(address,result));
  assert(!result.budget&&!result.usage&&api.lastBudgetException==0xBAD);
  assert(factoryReleases==(stage>1?1u:0u)&&adapterReleases==(stage>3?1u:0u));}
 Reset();nullFactory=true;assert(!api.QueryBudget(address,result)&&!factoryReleases&&!adapterReleases);
 Reset();nullAdapter=true;assert(!api.QueryBudget(address,result)&&factoryReleases==1&&!adapterReleases);
 Reset();api.createFactory=nullptr;assert(!api.QueryBudget(address,result));
 Reset();api.createFactory=&Factory;assert(!api.QueryBudget(0,result)&&!queries);
 std::cout<<"PASS actual budget query: rendering adapter LUID, local node zero, COM cleanup, unavailable/null interfaces and injected failures\n";
}
