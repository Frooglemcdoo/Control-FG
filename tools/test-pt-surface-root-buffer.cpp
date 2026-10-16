#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <cassert>
#include <cstdio>
#include <cmath>
#include "../src/pt_surface_pipeline.h"
#include "../src/pt_surface_output.h"
#include "pt-native-root-fixture.h"
using Microsoft::WRL::ComPtr;
int main(){
    setvbuf(stdout,nullptr,_IONBF,0);
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp))));
    assert(SUCCEEDED(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))));
    const char psSource[]="Texture2D<float4> diffuse:register(t0);Texture2D<float4> specular:register(t1);Texture2D<float4> material:register(t2);cbuffer C:register(b0){float4 color;};float4 main(float4 p:SV_Position):SV_Target{int3 q=int3(p.xy,0);float3 light=diffuse.Load(q).rgb*(color.rgb*material.Load(q).rgb)+specular.Load(q).rgb;return float4(sqrt(max(light,0)),1);}";
    const char vsSource[]="float4 main(uint v:SV_VertexID):SV_Position{float2 p=float2((v<<1)&2,v&2);return float4(p*float2(2,-2)+float2(-1,1),0,1);}";
    ComPtr<ID3DBlob> ps,vs,error;
    assert(SUCCEEDED(D3DCompile(psSource,sizeof(psSource)-1,nullptr,nullptr,nullptr,"main","ps_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&ps,&error)));
    assert(SUCCEEDED(D3DCompile(vsSource,sizeof(vsSource)-1,nullptr,nullptr,nullptr,"main","vs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&vs,&error)));
    // The fixture compiler may add redundant resource-type opcode extensions.
    // Normalize those to the reviewed game's compact LD encoding before applying
    // the production patch; declarations continue to define the resource type.
    const auto* compiled=static_cast<const unsigned char*>(ps->GetBufferPointer());
    const UINT chunkCount=PTDxbc::Word(compiled+28);std::vector<std::vector<unsigned char>> parts;
    for(UINT n=0;n<chunkCount;++n){const UINT start=PTDxbc::Word(compiled+32+4*n),size=PTDxbc::Word(compiled+start+4);
        std::vector<unsigned char> part(compiled+start,compiled+start+8+size);
        if(PTDxbc::Word(part.data())==0x58454853u){std::vector<UINT> words(size/4);std::memcpy(words.data(),part.data()+8,size);std::vector<UINT> normalized{words[0],0};
            for(UINT i=2;i<words.size();){const UINT length=(words[i]>>24)&127;assert(length);UINT skip=0;
                if((words[i]&2047)==45){UINT extension=words[i]>>31;while(extension){++skip;extension=words[i+skip]>>31;}}
                if(skip){normalized.push_back((words[i]&0x007fffffu)|((length-skip)<<24));normalized.insert(normalized.end(),words.begin()+i+1+skip,words.begin()+i+length);}
                else normalized.insert(normalized.end(),words.begin()+i,words.begin()+i+length);i+=length;
            }
            normalized[1]=static_cast<UINT>(normalized.size());part.resize(8+normalized.size()*4);PTDxbc::Put(part.data()+4,static_cast<UINT>(normalized.size()*4));std::memcpy(part.data()+8,normalized.data(),normalized.size()*4);
        }parts.push_back(std::move(part));
    }
    std::size_t total=32+chunkCount*4;for(const auto& part:parts)total+=part.size();std::vector<unsigned char> fixtureBytes(total);
    std::memcpy(fixtureBytes.data(),compiled,32);PTDxbc::Put(fixtureBytes.data()+24,static_cast<UINT>(total));std::size_t cursor=32+chunkCount*4;
    for(UINT i=0;i<chunkCount;++i){PTDxbc::Put(fixtureBytes.data()+32+i*4,static_cast<UINT>(cursor));std::memcpy(fixtureBytes.data()+cursor,parts[i].data(),parts[i].size());cursor+=parts[i].size();}
    const auto sum=PTDxbc::Checksum(fixtureBytes.data()+20,static_cast<UINT>(total-20));std::memcpy(fixtureBytes.data()+4,sum.data(),16);
    const auto* original=fixtureBytes.data();
    PTSurfaceComposition::Profile profile{};profile.bytes=static_cast<UINT>(fixtureBytes.size());std::memcpy(profile.checksum.data(),original+4,16);
    const UINT chunks=PTDxbc::Word(original+28);const unsigned char* code=nullptr;UINT words=0;
    for(UINT i=0;i<chunks;++i){const UINT at=PTDxbc::Word(original+32+i*4);if(PTDxbc::Word(original+at)==0x58454853u){code=original+at+8;words=PTDxbc::Word(original+at+4)/4;}}
    assert(code);profile.resource=3;
    for(UINT i=2;i<words;){const UINT op=PTDxbc::Word(code+i*4),length=(op>>24)&127;assert(length);
        if((op&2047)==104)profile.temps=i;
        if(op==0x0800002du&&PTDxbc::Word(code+(i+5)*4)>>12==0x207u&&PTDxbc::Word(code+(i+6)*4)==0&&PTDxbc::Word(code+(i+7)*4)==0)profile.load=i;
        if(op==0x09000032u&&PTDxbc::Word(code+(i+1)*4)==0x00100072u&&PTDxbc::Word(code+(i+2)*4)==0)profile.combine=i;
        i+=length;
    }
    if(!profile.load||!profile.combine||!profile.temps){ComPtr<ID3DBlob> dis;D3DDisassemble(original,fixtureBytes.size(),0,nullptr,&dis);if(dis)puts(static_cast<const char*>(dis->GetBufferPointer()));}
    assert(profile.load&&profile.combine&&profile.temps);
    std::vector<unsigned char> composed;assert(PTSurfaceComposition::PatchProfile(original,fixtureBytes.size(),&profile,composed));
    PTNativeRootFixture fixture(true);ComPtr<ID3DBlob> blob;ComPtr<ID3D12RootSignature> nativeRoot,ptRoot;
    assert(SUCCEEDED(D3D12SerializeRootSignature(&fixture.desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error)));
    assert(SUCCEEDED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&nativeRoot))));
    std::vector<D3D12_ROOT_PARAMETER> params(fixture.parameters,fixture.parameters+18);params.resize(21);
    params[18].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[18].Descriptor={0,2};params[18].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    params[19].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[19].Constants={0,2,2};params[19].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    params[20].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[20].Descriptor={1,2};params[20].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
    auto desc=fixture.desc;desc.NumParameters=21;desc.pParameters=params.data();blob.Reset();
    assert(SUCCEEDED(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error)));
    assert(SUCCEEDED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&ptRoot))));
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};pd.pRootSignature=nativeRoot.Get();pd.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pd.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    pd.BlendState.RenderTarget[0].RenderTargetWriteMask=15;pd.SampleMask=UINT_MAX;pd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;
    pd.RasterizerState.DepthClipEnable=TRUE;pd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pd.NumRenderTargets=1;pd.RTVFormats[0]=DXGI_FORMAT_R32G32B32A32_FLOAT;pd.SampleDesc.Count=1;
    ComPtr<ID3D12PipelineState> nativePso,ptPso;assert(SUCCEEDED(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&nativePso))));
    pd.pRootSignature=ptRoot.Get();pd.PS={composed.data(),composed.size()};assert(SUCCEEDED(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&ptPso))));
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=4096;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> data;assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&data))));
    void* mapped=nullptr;const D3D12_RANGE none{};assert(SUCCEEDED(data->Map(0,&none,&mapped)));std::memset(mapped,0,4096);
    auto* values=static_cast<float*>(mapped);for(UINT y=0;y<8;++y)for(UINT x=0;x<8;++x){const UINT at=(y*8+x)*8;values[at]=.25f;values[at+1]=.5f;values[at+2]=.75f;values[at+3]=(x+y)%2?1.0f:0.0f;}
    static_cast<UINT*>(mapped)[896]=1;values[768]=values[769]=values[770]=2;data->Unmap(0,nullptr);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;ComPtr<ID3D12DescriptorHeap> heap;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap))));const UINT step=device->GetDescriptorHandleIncrementSize(hd.Type);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
    // Null native SRVs return black. Covered PT pixels return their raw-buffer
    // values; uncovered pixels must stay native black, including row boundaries.
    auto cpu=heap->GetCPUDescriptorHandleForHeapStart();cpu.ptr+=5ull*step;device->CreateShaderResourceView(nullptr,&srv,cpu);cpu.ptr+=step;device->CreateShaderResourceView(nullptr,&srv,cpu);cpu.ptr+=step;device->CreateShaderResourceView(nullptr,&srv,cpu);
    hp.Type=D3D12_HEAP_TYPE_DEFAULT;D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=8;td.Height=8;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=srv.Format;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    ComPtr<ID3D12Resource> target;assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_RENDER_TARGET,nullptr,IID_PPV_ARGS(&target))));
    hd={};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=1;ComPtr<ID3D12DescriptorHeap> rtv;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtv))));device->CreateRenderTargetView(target.Get(),nullptr,rtv->GetCPUDescriptorHandleForHeapStart());
    hp.Type=D3D12_HEAP_TYPE_READBACK;bd.Width=12288;ComPtr<ID3D12Resource> readback;assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    std::shared_ptr<PTSurfaceComposition::Output> acceptedGuard,rejectedGuard;
    assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),1,1,acceptedGuard,true,true)));
    assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),1,1,rejectedGuard,true,true)));
    assert(acceptedGuard->EnableConsumer(queue.Get())&&rejectedGuard->EnableConsumer(queue.Get()));
    assert(rejectedGuard->Invalidate(queue.Get()));assert(!rejectedGuard->EnableConsumer(queue.Get()));
    ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
    ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);auto handle=rtv->GetCPUDescriptorHandleForHeapStart();list->OMSetRenderTargets(1,&handle,FALSE,nullptr);
    D3D12_VIEWPORT viewport{0,0,8,8,0,1};D3D12_RECT rect{0,0,8,8};list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&rect);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    for(UINT draw=0;draw<200;++draw){list->SetGraphicsRootSignature(ptRoot.Get());list->SetGraphicsRootDescriptorTable(1,heap->GetGPUDescriptorHandleForHeapStart());list->SetGraphicsRootConstantBufferView(12,data->GetGPUVirtualAddress()+3072);list->SetGraphicsRootShaderResourceView(18,data->GetGPUVirtualAddress());list->SetGraphicsRootShaderResourceView(20,acceptedGuard->Address());const UINT extent[]={8,8};list->SetGraphicsRoot32BitConstants(19,2,extent,0);list->SetPipelineState(ptPso.Get());list->DrawInstanced(3,1,0,0);}
    const auto capture=[&](UINT offset){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={target.Get(),0,D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
        D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=target.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint.Offset=offset;dst.PlacedFootprint.Footprint={srv.Format,8,8,1,256};list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);};
    capture(0);
    list->SetGraphicsRootSignature(nativeRoot.Get());list->SetGraphicsRootDescriptorTable(1,heap->GetGPUDescriptorHandleForHeapStart());list->SetGraphicsRootConstantBufferView(12,data->GetGPUVirtualAddress()+3072);list->SetPipelineState(nativePso.Get());list->DrawInstanced(3,1,0,0);capture(2048);
    list->SetGraphicsRootSignature(ptRoot.Get());list->SetGraphicsRootDescriptorTable(1,heap->GetGPUDescriptorHandleForHeapStart());list->SetGraphicsRootConstantBufferView(12,data->GetGPUVirtualAddress()+3072);list->SetGraphicsRootShaderResourceView(18,data->GetGPUVirtualAddress());list->SetGraphicsRootShaderResourceView(20,acceptedGuard->Address());const UINT cropped[]={8,4};list->SetGraphicsRoot32BitConstants(19,2,cropped,0);list->SetPipelineState(ptPso.Get());list->DrawInstanced(3,1,0,0);capture(4096);
    // A rejected consumer uses native black, then another consumer still reads
    // the unchanged shared PT buffer. No global radiance clear is permitted.
    const UINT full[]={8,8};list->SetGraphicsRoot32BitConstants(19,2,full,0);
    list->SetGraphicsRootShaderResourceView(20,rejectedGuard->Address());
    list->DrawInstanced(3,1,0,0);capture(6144);
    list->SetGraphicsRootShaderResourceView(20,acceptedGuard->Address());
    list->DrawInstanced(3,1,0,0);capture(8192);
    assert(SUCCEEDED(list->Close()));ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);assert(acceptedGuard->NativeSubmitted(queue.Get()));assert(rejectedGuard->NativeSubmitted(queue.Get()));
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));assert(SUCCEEDED(queue->Signal(fence.Get(),1)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(SUCCEEDED(fence->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    const D3D12_RANGE read{0,10240};assert(SUCCEEDED(readback->Map(0,&read,&mapped)));
    for(UINT frame=0;frame<5;++frame)for(UINT y=0;y<8;++y)for(UINT x=0;x<8;++x){const auto* pixel=reinterpret_cast<const float*>(static_cast<unsigned char*>(mapped)+frame*2048+y*256+x*16);for(UINT c=0;c<4;++c){const float pt[]={.25f,.5f,.75f};const float expected=c==3?1.0f:(((frame==0||frame==4||(frame==2&&y<4))&&(x+y)%2)?std::sqrt(pt[c]):0.0f);assert(std::abs(pixel[c]-expected)<.0001f);}}
    readback->Unmap(0,&none);assert(SUCCEEDED(device->GetDeviceRemovedReason()));puts("PASS: patched DXBC root-buffer GPU read, 200 draws, coverage selection, row pitch, native-root restoration, and rejected consumer isolation without clearing shared PT output");
}
