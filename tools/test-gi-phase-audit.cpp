#define NOMINMAX
#include "../src/gi_phase_audit.h"
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <stdexcept>
#include <thread>
#include <vector>
using Microsoft::WRL::ComPtr;
static void Require(bool b){if(!b)throw std::runtime_error("phase audit contract failed");}
static void Check(HRESULT hr){Require(SUCCEEDED(hr));}
int main(){try {
 SetEnvironmentVariableA("CONTROLTOH_GI_PHASE_AUDIT","1");
 namespace a=gi_phase_audit;
 a::frameReader=[]() noexcept -> unsigned long long {return 100;};
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
 ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
 ComPtr<ID3D12CommandAllocator> alloc;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&alloc)));
 ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,alloc.Get(),nullptr,IID_PPV_ARGS(&list)));
 D3D12_COMMAND_QUEUE_DESC desc{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)));
 auto phase=[&](unsigned kind){return a::Phase(list.Get(),kind,100,1,320,90,1,nullptr,nullptr,nullptr,nullptr,false);};
 const auto ray=phase(1),shade=phase(2);Require(ray&&shade>ray);
 ID3D12CommandList* lists[]={list.Get()};auto call=a::Submit(queue.Get(),1,lists);Require(call>shade);
 Require(a::events[call].kind==4&&a::events[call].data[3]==ray);
 Require(a::events[call+1].data[3]==shade);
 // ClearState must preserve already-recorded events, Reset must discard them.
 list->ClearState(nullptr);a::Marker m{};UINT size=sizeof(m);Check(list->GetPrivateData(a::markerKey,&size,&m));Require(m.count==2);
 a::Reset(list.Get());size=sizeof(m);Check(list->GetPrivateData(a::markerKey,&size,&m));Require(m.count==0);
 auto empty=a::Submit(queue.Get(),1,lists);Require(a::next.load()==empty);
 Require(!a::PhaseWindow(99)&&!a::PhaseWindow(124)&&a::PhaseWindow(123));
 std::vector<std::thread> workers;for(unsigned i=0;i<8;++i)workers.emplace_back([] {for(unsigned j=0;j<200;++j)a::Emit(10,{j});});
 for(auto& worker:workers)worker.join();for(unsigned i=0;i<a::next.load();++i)Require(a::events[i].ready.load());
 while(a::next.load()<a::capacity)a::Emit(10,{});
 Require(a::Emit(10,{})==0&&a::dropped.load()==1&&a::stopped.load());
 Check(list->Close());puts("PASS phase audit: recording IDs, ClearState/Reset, frame bounds, concurrent publication, bounded overflow; no GPU submission");return 0;
 }catch(const std::exception& e){puts(e.what());return 1;}}
