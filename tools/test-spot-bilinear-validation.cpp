#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <vector>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <stdexcept>
#include <string>
#include <iostream>
#include "fixtures/spot-bilinear-validation.h"
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) { if(FAILED(hr)) throw std::runtime_error("Shader validation failed: " + std::to_string(static_cast<unsigned long>(hr))); }
int main() try {
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) debug->EnableDebugLayer();
    ComPtr<ID3DBlob> disassembly;
    Check(D3DDisassemble(kSpotBilinearValidationShader,sizeof(kSpotBilinearValidationShader),0,nullptr,&disassembly));
    std::cout<<"PASS: emitted instructions disassemble\n";
    ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
    ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0]={D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1,12,0,0};
    ranges[1]={D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,1};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameter.DescriptorTable={2,ranges};parameter.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC desc{};desc.NumParameters=1;desc.pParameters=&parameter;
    ComPtr<ID3DBlob> serialized,errors;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&serialized,&errors));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,serialized->GetBufferPointer(),serialized->GetBufferSize(),IID_PPV_ARGS(&root)));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipeline{};pipeline.pRootSignature=root.Get();pipeline.CS={kSpotBilinearValidationShader,sizeof(kSpotBilinearValidationShader)};
    ComPtr<ID3D12PipelineState> pso;
    const HRESULT compiled=device->CreateComputePipelineState(&pipeline,IID_PPV_ARGS(&pso));
    if (FAILED(compiled)) {
        ComPtr<ID3D12InfoQueue> info;
        if(SUCCEEDED(device.As(&info))) {
            for(UINT64 i=0;i<info->GetNumStoredMessages();++i) {
                SIZE_T size=0;info->GetMessage(i,nullptr,&size);std::vector<unsigned char> bytes(size);
                auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
                if(SUCCEEDED(info->GetMessage(i,message,&size))) std::cerr<<message->pDescription<<'\n';
            }
        }
        std::cerr<<static_cast<const char*>(disassembly->GetBufferPointer())<<'\n';
    }
    Check(compiled);
    std::cout<<"PASS: emitted bilinear PCF block disassembles and compiles on WARP\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
