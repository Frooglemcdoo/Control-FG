#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_surface_output.h"
#include "../src/pt_surface_reconstruction.h"
#include "../src/pt_primary_metrics.h"
#include "pt-test-shader-includes.h"
using Microsoft::WRL::ComPtr;
using PTSurfaceComposition::Output;
static constexpr auto readState=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
struct Run {
    PTSurfaceComposition::ReconstructionPass pass;PTPrimaryMetrics::Counters metrics;
    static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource* output,D3D12_CPU_DESCRIPTOR_HANDLE){auto& r=*static_cast<Run*>(context);r.metrics.Begin(list,{});r.pass.Record(list,output,r.metrics.Buffer());r.metrics.End(list);return true;}
};
int main(){
    setvbuf(stdout,nullptr,_IONBF,0);
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp))));assert(SUCCEEDED(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))));
    PTTestShaderIncludes includes;
    ComPtr<ID3DBlob> shader,error;HRESULT hr=D3DCompileFromFile(L"src/shaders/pt_surface_reconstruct.hlsl",nullptr,&includes,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error);
    if(FAILED(hr)&&error)puts(static_cast<const char*>(error->GetBufferPointer()));assert(SUCCEEDED(hr));
    auto pipeline=std::make_shared<PTSurfaceComposition::ReconstructionPipeline>();assert(SUCCEEDED(pipeline->Initialize(device.Get(),shader->GetBufferPointer(),shader->GetBufferSize())));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=8192;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;ComPtr<ID3D12Resource> upload,readback;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload))));hp.Type=D3D12_HEAP_TYPE_READBACK;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))));
    PTNativeView::View view;view.width=view.height=8;view.frame=1;
    std::array<float,72> camera{};camera[2]=camera[3]=.125f;
    for(UINT i=0;i<3;++i){camera[8+i*4+i]=1;camera[24+i*4+i]=1;}for(UINT i=0;i<4;++i)camera[56+i*4+i]=1;std::memcpy(view.bytes.data(),camera.data(),288);
    PTSurfaceComposition::ReconstructionConstants c;auto previousView=view;view.frame=2;
    assert(PTSurfaceComposition::ReconstructConstants(view,&previousView,c)&&c.useHistory==1);
    auto changed=previousView;changed.width=16;assert(PTSurfaceComposition::ReconstructConstants(view,&changed,c)&&c.useHistory==0);
    changed=previousView;changed.frame=0;assert(PTSurfaceComposition::ReconstructConstants(view,&changed,c)&&c.useHistory==0);
    changed=previousView;float distant=4;std::memcpy(changed.bytes.data()+144,&distant,4);assert(PTSurfaceComposition::ReconstructConstants(view,&changed,c)&&c.useHistory==0);
    changed=previousView;std::memset(changed.bytes.data()+224,0,64);assert(PTSurfaceComposition::ReconstructConstants(view,&changed,c)&&c.useHistory==0);
    std::shared_ptr<Output> previous;
    for(UINT iteration=0;iteration<9;++iteration){
        view.frame=iteration+1;view.engineFrame=iteration+1;
        std::shared_ptr<Output> raw,output;assert(SUCCEEDED(Output::Create(queue.Get(),8,8,raw,true,true)));assert(SUCCEEDED(Output::Create(queue.Get(),8,8,output,true,true,32)));assert(raw->Address()&&output->Address());
        std::array<ComPtr<ID3D12Resource>,3> guides;hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=td.Height=8;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=3;ComPtr<ID3D12DescriptorHeap> descriptors;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&descriptors))));
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,3> handles{};const auto step=device->GetDescriptorHandleIncrementSize(hd.Type);
        for(UINT i=0;i<3;++i){assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&guides[i]))));
            handles[i]=descriptors->GetCPUDescriptorHandleForHeapStart();handles[i].ptr+=SIZE_T(i)*step;
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=td.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;device->CreateShaderResourceView(guides[i].Get(),&srv,handles[i]);}
        void* mapped=nullptr;const D3D12_RANGE none{};assert(SUCCEEDED(upload->Map(0,&none,&mapped)));std::memset(mapped,0,8192);
        for(UINT y=0;y<8;++y)for(UINT x=0;x<8;++x){
            const float normal[]={.5f,.5f,.5f,0},material[]={0,0,0,iteration==2?1.0f:0.0f},depth[]={iteration==3?.7f:.5f,0,0,0};
            const float radiance=(iteration==0||iteration==6)?1.0f:(iteration>=7?0.0f:((x+y)%2?2.0f:0.0f));const float pixel[]={radiance,radiance,radiance,(x==0&&y==0)?0.0f:1.0f};
            auto* bytes=static_cast<unsigned char*>(mapped);std::memcpy(bytes+y*256+x*16,normal,16);std::memcpy(bytes+2048+y*256+x*16,material,16);std::memcpy(bytes+4096+y*256+x*16,depth,16);std::memcpy(bytes+6144+(y*8+x)*16,pixel,16);
        }upload->Unmap(0,nullptr);
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
        for(UINT i=0;i<3;++i){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint.Offset=2048ull*i;src.PlacedFootprint.Footprint={td.Format,8,8,1,256};dst.pResource=guides[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={guides[i].Get(),0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);}
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={raw->Texture(),0,readState,D3D12_RESOURCE_STATE_COPY_DEST};list->ResourceBarrier(1,&b);list->CopyBufferRegion(raw->Texture(),0,upload.Get(),6144,1024);std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);
        auto run=std::make_shared<Run>();assert(run->metrics.Initialize(device.Get()));
        assert(run->pass.Initialize(device.Get(),pipeline,view,previous?&previousView:nullptr,raw->Texture(),previous?previous->Texture():nullptr,handles));
        // Zero admission is explicitly initialized through the cancellation copy.
        assert(run->metrics.SetAdmission(queue.Get(),true,iteration==5));if(iteration==4)assert(run->metrics.SetAdmission(queue.Get(),false));
        assert(output->RecordInline(list.Get(),run.get(),&Run::Encode,run));assert(output->AdmitInline());
        b.Transition={output->Texture(),0,readState,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);list->CopyBufferRegion(readback.Get(),0,output->Texture(),0,2048);std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);
        assert(SUCCEEDED(list->Close()));ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);assert(output->NativeSubmitted(queue.Get()));assert(raw->NativeSubmitted(queue.Get()));if(previous)assert(previous->NativeSubmitted(queue.Get()));
        assert(SUCCEEDED(queue->Signal(fence.Get(),iteration+1)));assert(SUCCEEDED(fence->SetEventOnCompletion(iteration+1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);
        const D3D12_RANGE read{0,2048};assert(SUCCEEDED(readback->Map(0,&read,&mapped)));
        double errorSum=0;
        for(UINT i=0;i<64;++i){const auto* pixel=static_cast<const float*>(mapped)+i*8;
            if(i==0||iteration==4||iteration==5){for(UINT channel=0;channel<8;++channel)assert(pixel[channel]==0);continue;}
            assert(pixel[3]==1&&std::isfinite(pixel[0])&&pixel[0]>=0&&pixel[0]<=2);
            assert(std::abs(pixel[7]-(iteration==1||iteration==7?2.0f:(iteration==8?3.0f:1.0f)))<1e-5f);
            if(iteration==0||iteration==6)assert(std::abs(pixel[0]-1)<1e-5f);
            if(iteration==7)assert(std::abs(pixel[0]-.5f)<1e-5f);
            if(iteration==8)assert(std::abs(pixel[0]-1.0f/3.0f)<1e-5f);
            errorSum+=std::abs(pixel[0]-1);
        }
        if(iteration==1)assert(errorSum/63<.15); // Checker noise was +/-1 before reconstruction.
        readback->Unmap(0,&none);assert(output->Completed());previous=output;previousView=view;
    }
    previous.reset();Output::TrimIdle();CloseHandle(event);assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: raw-buffer initialization, spatial noise reduction, temporal accumulation, material/depth rejection, resolution/cut reset, zero coverage and rejected/probe admission");
}
