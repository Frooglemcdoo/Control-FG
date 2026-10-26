#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
#include <iterator>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if(FAILED(hr)) throw std::runtime_error("Validation HRESULT " + std::to_string(static_cast<unsigned long>(hr))); }
#include "rt_spot_debug_warp_test.h"
int main() try {
    ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    D3D12_DESCRIPTOR_RANGE ranges[] = {
        {D3D12_DESCRIPTOR_RANGE_TYPE_CBV,16,0,0,0},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,32,0,0,16},
        {D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,48}};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable={3,ranges};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_STATIC_SAMPLER_DESC samplers[16]{};
    for(UINT i=0;i<16;++i) {
        auto& s=samplers[i];s.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        s.AddressU=s.AddressV=s.AddressW=D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        s.MaxAnisotropy=1;s.ComparisonFunc=D3D12_COMPARISON_FUNC_ALWAYS;s.MaxLOD=D3D12_FLOAT32_MAX;
        s.ShaderRegister=i;s.RegisterSpace=1;s.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    }
    D3D12_ROOT_SIGNATURE_DESC desc{1,&parameter,16,samplers,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    for(const auto* name:{"build/rt_spot_selector_map_priority.dxbc","build/rt_spot_corrected_open.dxbc","build/rt_spot_guarded_history.dxbc","build/rt_spot_corrected_no_ao.dxbc","build/rt_spot_selector_corrected_no_ao.dxbc","build/rt_spot_bent_normal_off.dxbc","build/rt_spot_selector_bent_normal_off.dxbc","build/rt_spot_local_maps_off.dxbc","build/rt_spot_selector_local_maps_off.dxbc","build/rt_spot_contact_off.dxbc","build/rt_spot_screen_occlusion_off.dxbc","build/rt_spot_selector_occlusion_off.dxbc","build/rt_spot_band_colors.dxbc","build/rt_spot_open.dxbc","build/rt_spot_selected_off.dxbc","build/rt_spot_selection.dxbc","build/rt_spot_visibility.dxbc","build/rt_spot_light_ids.dxbc","build/rt_spot_raw_history.dxbc","build/rt_spot_raw_spatial0.dxbc","build/rt_spot_raw_spatial1.dxbc","build/rt_spot_light.dxbc","build/rt_spot_reset.dxbc","build/rt_spot_history.dxbc","build/rt_spot_spatial0.dxbc","build/rt_spot_spatial1.dxbc"}) {
        std::ifstream file(name,std::ios::binary);if(!file)throw std::runtime_error(name);
        std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
        ComPtr<ID3DBlob> assembly;Check(D3DDisassemble(bytes.data(),bytes.size(),0,nullptr,&assembly));
        D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();pipeline.CS={bytes.data(),bytes.size()};
        ComPtr<ID3D12PipelineState> pso;Check(device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&pso)));
        if (std::string(name)=="build/rt_spot_band_colors.dxbc" || std::string(name)=="build/rt_spot_visibility.dxbc" || std::string(name)=="build/rt_spot_light_ids.dxbc")
            TestSpotDebugView(device.Get(),root.Get(),pso.Get(),std::string(name)=="build/rt_spot_light_ids.dxbc",std::string(name)=="build/rt_spot_band_colors.dxbc");
        std::cout<<"PASS: disassembly and WARP pipeline validation: "<<name<<'\n';
    }
    return 0;
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n';return 1; }
