#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_resource_lease_probe.h"
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<IDXGIFactory4> f;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&f))));
    ComPtr<IDXGIAdapter> a;assert(SUCCEEDED(f->EnumWarpAdapter(IID_PPV_ARGS(&a))));
    ComPtr<ID3D12Device> d;assert(SUCCEEDED(D3D12CreateDevice(a.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&d))));
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=65536;rd.Height=1;
    rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> resource;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    PTResourceLeaseProbe::Probe probe;
    auto invalid=probe.Before(resource.Get(),resource->GetGPUVirtualAddress()+1,65536);
    assert(probe.After(invalid).status==3&&probe.HeldBytes()==0);
    auto lease=probe.Before(resource.Get(),resource->GetGPUVirtualAddress(),65536);
    assert(lease.resource&&probe.HeldBytes()==65536);
    resource.Reset(); // Drop the original owner's final reference.
    assert(probe.After(lease).status==5&&probe.HeldBytes()==0);
    assert(probe.After(lease).status==0); // Cannot double-release a completed lease.
    auto missing=probe.Before(nullptr,0,0);assert(probe.After(missing).status==0);
    PTResourceLeaseProbe::Probe budget;
    rd.Width=PTResourceLeaseProbe::Probe::MaximumResource;
    assert(SUCCEEDED(d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&resource))));
    PTResourceLeaseProbe::Lease held[9]{};
    for(auto& l:held)l=budget.Before(resource.Get(),resource->GetGPUVirtualAddress(),rd.Width);
    assert(budget.HeldBytes()==PTResourceLeaseProbe::Probe::MaximumHeld&&held[8].status==4);
    resource.Reset();for(auto& l:held)budget.After(l);assert(budget.HeldBytes()==0);
    std::puts("PASS M9 WARP: owned reference survives native-owner release; address mismatch, byte budget and duplicate cleanup rejected.");
}
