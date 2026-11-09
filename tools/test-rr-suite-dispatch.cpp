#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <new>
#include <cassert>
#include <cstdio>
#define CONTROL_RR_F9_SUITE
#include "../src/rr_suite_pipelines.h"
static ID3D12PipelineState* current=nullptr;
static ID3D12PipelineState* expected=nullptr;
static unsigned sets=0,dispatchCount=0;
static void Set(ID3D12GraphicsCommandList*,ID3D12PipelineState* p){current=p;++sets;}
static void Dispatch(ID3D12GraphicsCommandList*,UINT x,UINT y,UINT z){assert(current==expected&&x==3&&y==4&&z==1);++dispatchCount;}
int main(){
 using namespace rr_suite;
 Bundle bundle{};bundle.native=reinterpret_cast<ID3D12PipelineState*>(0x1000);
 for(unsigned i=0;i<6;++i)bundle.variants[i]=reinterpret_cast<ID3D12PipelineState*>(0x2000+i*16);
 bundles[0]=&bundle;auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(0x3000);
 for(unsigned n=1;n<=count;++n){requested=n;Latch(n);Observe(list,bundle.native);current=bundle.native;expected=bundle.variants[steps[n-1].variant];
  assert(Run(list,3,4,1,&Set,&Dispatch));assert(current==bundle.native);assert(dispatchStep==n&&dispatchFrame==n);
  assert(!Run(reinterpret_cast<ID3D12GraphicsCommandList*>(0x4000),3,4,1,&Set,&Dispatch));
  Observe(list,reinterpret_cast<ID3D12PipelineState*>(0x5000));assert(!Run(list,3,4,1,&Set,&Dispatch));
  Observe(list,bundle.native);frame=n+1;assert(!Run(list,3,4,1,&Set,&Dispatch));
 }
 assert(sets==16&&dispatchCount==8);requested=0;Latch(10);Observe(list,bundle.native);assert(!Run(list,3,4,1,&Set,&Dispatch));
 puts("PASS: eight dispatch variants, exact PSO restoration, unrelated PSO/list/stale-frame rejection and inactive bypass");
}
