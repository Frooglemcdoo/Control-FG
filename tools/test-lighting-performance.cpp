#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <stdexcept>
static std::atomic<unsigned long long> presentCount{16};
namespace control_rr_diagnostic { inline std::atomic<bool> lastSelected{true};inline std::atomic<unsigned> lastEffects{43}; }
namespace native_lighting {
struct Selection { unsigned long long Read() const{return 7;} };inline Selection selection;
#include "../src/lighting_performance.h"
}
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr){if(FAILED(hr))throw std::runtime_error("D3D call failed");}
static void Require(bool value){if(!value)throw std::runtime_error("Performance capture contract failed");}
int main(){try{
    namespace p=native_lighting::performance;
    ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    p::RememberDevice(device.Get());Require(p::Prepare());
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};Check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    Require(p::Begin(list.Get(),p::Stage::ShadowRays,100,100,1)==p::kInvalid);
    p::capture=1;p::enabled=true;presentCount=17;
    Require(p::Begin(list.Get(),p::Stage::ShadowRays,100,100,1)==p::kInvalid);
    presentCount=p::kSampleInterval;
    char shader[65]{};for(unsigned i=0;i<64;++i)shader[i]='a';
    p::Bind(list.Get(),p::Stage::OtherCompute,shader);
    Require(p::ComputeStage(list.Get())==p::Stage::OtherCompute);
    Require(p::ComputeShader(list.Get())[0]=='a' && p::ComputeShader(nullptr)==nullptr);
    p::Bind(list.Get(),p::Stage::None);Require(p::ComputeShader(list.Get())[0]==0);
    D3D12_DISPATCH_RAYS_DESC rayDesc{};
    rayDesc.RayGenerationShaderRecord={0x10000,64};
    rayDesc.HitGroupTable={0x20000,960,32};
    rayDesc.MissShaderTable={0x30000,64,32};
    unsigned ticket=p::Begin(list.Get(),p::Stage::ShadowRays,100,100,1,960,shader,&rayDesc);Require(ticket!=p::kInvalid);p::End(ticket);Require(p::samples[ticket].sourceShader[63]=='a');
    rayDesc.RayGenerationShaderRecord.StartAddress=0; // metadata copied, no pointer retained
    Require(p::samples[ticket].raygenStart==0x10000 && p::samples[ticket].raygenBytes==64);
    Require(p::samples[ticket].hitStart==0x20000 && p::samples[ticket].hitStride==32);
    Require(p::samples[ticket].missStart==0x30000 && p::samples[ticket].missStride==32);
    Require(p::samples[ticket].recordIndex==ticket);
    unsigned rows=0;
    auto emit=[&](const p::Sample& s,double ms){Require(s.rr && s.effects==43 && s.x==100 && s.tableBytes==960 && ms>=0);++rows;};
    Require(p::Poll(1,emit)==1 && !rows); // recorded is not GPU completion
    Check(list->Close());ID3D12CommandList* lists[]={list.Get()};
    auto value=p::BeforeExecute(queue.Get(),1,lists);Require(value!=0);
    queue->ExecuteCommandLists(1,lists);p::AfterExecute(queue.Get(),value);
    const auto deadline=GetTickCount64()+10000;
    while(p::Poll(1,emit) && GetTickCount64()<deadline)Sleep(1);
    Require(rows==1 && !p::fault.load());p::Poll(1,emit);Require(rows==1);
    Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));p::Reset(list.Get());
    Require(!p::samples[ticket].current);
    auto cancelled=p::Begin(list.Get(),p::Stage::History,1,1,1);p::End(cancelled);
    Check(list->Close());Check(list->Reset(allocator.Get(),nullptr));p::Reset(list.Get());
    Require(p::samples[cancelled].state==p::State::Discarded);
    auto next=p::Begin(list.Get(),p::Stage::ShadowRays,100,100,1,960);p::End(next);Require(next>cancelled);Require(p::samples[next].raygenStart==0 && p::samples[next].recordIndex==next);
    Check(list->Close());value=p::BeforeExecute(queue.Get(),1,lists);Require(value!=0);
    queue->ExecuteCommandLists(1,lists);p::AfterExecute(queue.Get(),value);
    const auto second=GetTickCount64()+10000;
    while(p::Poll(1,emit) && GetTickCount64()<second)Sleep(1);
    Require(rows==2 && !p::fault.load());
    // Reusing an unreset list is not silently treated as a fresh measurement.
    Require(p::BeforeExecute(queue.Get(),1,lists)==0 && p::fault.load());
    p::enabled=false;
    Require(p::Begin(list.Get(),p::Stage::ShadowRays,100,100,1)==p::kInvalid);
    puts("PASS timestamp sampling, exact submission, fence retirement, reset/discard, unique indices, repeated-list rejection and disabled fast path");return 0;
}catch(const std::exception& e){puts(e.what());return 1;}}
