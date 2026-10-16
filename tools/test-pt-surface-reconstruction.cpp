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
#include "../src/rr_pt_input.h"
using Microsoft::WRL::ComPtr;
using PTSurfaceComposition::Output;
static constexpr auto readState=static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
struct Run {
    PTSurfaceComposition::ReconstructionPass pass;PTPrimaryMetrics::Counters metrics;
    static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource* output,D3D12_CPU_DESCRIPTOR_HANDLE){auto& r=*static_cast<Run*>(context);r.metrics.Begin(list,{});r.pass.Record(list,output,r.metrics.Buffer());r.metrics.End(list);return true;}
};
int main(int argc,char**){
    const bool split=argc>1,freshTest=argc>2;
    control_rr::RRPTInputRoute route;assert(!route.Fresh(0)&&!route.Fresh(10));
    route.Publish(10,true);assert(route.Fresh(10)&&!route.Fresh(9)&&!route.Fresh(11));
    route.Publish(11,false);assert(!route.Fresh(10)&&!route.Fresh(11));
    route.Publish(12,true);route.Publish(0,false);assert(!route.Fresh(12));
    setvbuf(stdout,nullptr,_IONBF,0);
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> warp;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp))));assert(SUCCEEDED(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))));
    // Reproduce the former 4K capacity mismatch without a multi-GiB CI allocation.
    using Storage=PTSurfaceComposition::SignalStorage;
    constexpr UINT64 one4K=3840ull*2160*32,oldBudget=2048ull*1024*1024;
    static_assert(8*one4K<=oldBudget&&9*one4K>oldBudget,"old cap rejects ninth retained signal");
    static_assert(Storage::Fits(8*one4K,one4K),"history plus four signal/scratch pairs must fit");
    static_assert(Storage::Fits(10*one4K,one4K),"five retained frame pairs plus history must fit");
    static_assert(!Storage::Fits(Storage::MaximumBytes,one4K),"cap remains bounded");
    {
        std::vector<std::shared_ptr<Storage>> live;
        for(UINT i=0;i<11;++i){auto v=Storage::Acquire(device.Get(),2048);assert(v);
            for(const auto& old:live)assert(old->resource.Get()!=v->resource.Get());live.push_back(v);}
        auto retained=live.front();live.clear();auto another=Storage::Acquire(device.Get(),2048);
        assert(another&&another->resource.Get()!=retained->resource.Get());
    }
    puts("PASS: signal capacity covers output retention plus history; retained signal storage never aliases a new acquisition");
    PTTestShaderIncludes includes;
    ComPtr<ID3DBlob> shader,error;HRESULT hr=D3DCompileFromFile(L"src/shaders/pt_surface_reconstruct.hlsl",nullptr,&includes,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_IEEE_STRICTNESS|D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&shader,&error);
    if(FAILED(hr)&&error)puts(static_cast<const char*>(error->GetBufferPointer()));assert(SUCCEEDED(hr));
    auto pipeline=std::make_shared<PTSurfaceComposition::ReconstructionPipeline>();assert(SUCCEEDED(pipeline->Initialize(device.Get(),shader->GetBufferPointer(),shader->GetBufferSize())));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12Fence> fence;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence))));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;bd.Width=16384;bd.Height=1;bd.DepthOrArraySize=bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
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
    std::shared_ptr<Output> previous;ComPtr<ID3D12Resource> previousMaterials;std::shared_ptr<PTSurfaceComposition::SignalStorage> previousSignals;
    for(UINT iteration=0;iteration<16;++iteration){
        const bool fresh=freshTest&&iteration!=0&&iteration!=8;
        if(iteration==9||iteration==11)previous.reset();
        if(iteration==10){camera[36]=.0625f;camera[20]=-.0625f;std::memcpy(view.bytes.data(),camera.data(),288);}
        view.frame=iteration+1;view.engineFrame=iteration+1;
        std::shared_ptr<Output> raw,output;assert(SUCCEEDED(Output::Create(queue.Get(),8,8,raw,true,true,split?64:16)));assert(SUCCEEDED(Output::Create(queue.Get(),8,8,output,true,true,64)));assert(raw->Address()&&output->Address());
        std::array<ComPtr<ID3D12Resource>,3> guides;hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=td.Height=8;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;
        D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=5;ComPtr<ID3D12DescriptorHeap> descriptors;assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&descriptors))));
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE,5> handles{};const auto step=device->GetDescriptorHandleIncrementSize(hd.Type);
        for(UINT i=0;i<3;++i){assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&guides[i]))));
            handles[i]=descriptors->GetCPUDescriptorHandleForHeapStart();handles[i].ptr+=SIZE_T(i)*step;
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=td.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;device->CreateShaderResourceView(guides[i].Get(),&srv,handles[i]);}
        std::array<ComPtr<ID3D12Resource>,2> materialBuffers;
        for(UINT i=0;i<2;++i){
            auto desc=bd;desc.Width=256ull*(i?336:8);hp.Type=D3D12_HEAP_TYPE_UPLOAD;
            assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&materialBuffers[i]))));
            void* data=nullptr;D3D12_RANGE noRead{};assert(SUCCEEDED(materialBuffers[i]->Map(0,&noRead,&data)));std::memset(data,0,static_cast<size_t>(desc.Width));
            auto* words=static_cast<UINT*>(data);
            if(i&&iteration>=13)words[84+83]=4; // Record mutation independent of slot/geometry.
            if(!i&&iteration==15)words[2]=1; // Part1 mutation independent of Part3.
            materialBuffers[i]->Unmap(0,nullptr);
            handles[3+i]=descriptors->GetCPUDescriptorHandleForHeapStart();handles[3+i].ptr+=SIZE_T(3+i)*step;
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_BUFFER;srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srv.Buffer.NumElements=256;srv.Buffer.StructureByteStride=i?336:8;device->CreateShaderResourceView(materialBuffers[i].Get(),&srv,handles[3+i]);
        }
        PTMaterialHistory::Keys keys{};keys[0]=keys[1]=1;keys[255]=2;if(iteration>=14)keys[1]=3;
        void* mapped=nullptr;const D3D12_RANGE none{};assert(SUCCEEDED(upload->Map(0,&none,&mapped)));std::memset(mapped,0,16384);
        for(UINT y=0;y<8;++y)for(UINT x=0;x<8;++x){
            const float normal[]={(iteration==10||(iteration==9&&x%2==0))?.7f:.5f,.5f,.5f,0},material[]={0,0,0,iteration==2?1.0f:(iteration>=12?1.0f/255.0f:0.0f)},depth[]={iteration==3?.7f:.5f,0,0,0};
            const float radiance=(iteration==9||iteration==11)?2.0f:(iteration==0||iteration==6)?1.0f:(iteration>=7?0.0f:((x+y)%2?2.0f:0.0f));const float pixel[]={radiance,radiance,radiance,(x==0&&y==0)?0.0f:1.0f};
            auto* bytes=static_cast<unsigned char*>(mapped);std::memcpy(bytes+y*256+x*16,normal,16);std::memcpy(bytes+2048+y*256+x*16,material,16);std::memcpy(bytes+4096+y*256+x*16,depth,16);if(split){
                const float factor=x%2?.25f:1.0f;
                const float channels[]={radiance*.75f*factor,radiance*.75f*factor,radiance*.75f*factor,pixel[3],
                    radiance*.125f,radiance*.125f,radiance*.125f,1,
                    factor,factor,factor,.5f,.5f,.5f,.5f,0};
                std::memcpy(bytes+6144+(y*8+x)*64,channels,64);
            }else std::memcpy(bytes+6144+(y*8+x)*16,pixel,16);
        }upload->Unmap(0,nullptr);
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
        for(UINT i=0;i<3;++i){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint.Offset=2048ull*i;src.PlacedFootprint.Footprint={td.Format,8,8,1,256};dst.pResource=guides[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
            D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={guides[i].Get(),0,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};list->ResourceBarrier(1,&b);}
        D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={raw->Texture(),0,readState,D3D12_RESOURCE_STATE_COPY_DEST};list->ResourceBarrier(1,&b);list->CopyBufferRegion(raw->Texture(),0,upload.Get(),6144,split?4096:1024);std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);
        auto run=std::make_shared<Run>();assert(run->metrics.Initialize(device.Get()));
        assert(run->pass.Initialize(device.Get(),pipeline,view,previous?&previousView:nullptr,raw->Texture(),previous?previous->Texture():nullptr,handles,keys,previous?previousMaterials.Get():nullptr,previous?previousSignals:nullptr,fresh));
        if(freshTest)assert(!run->pass.historyEnabled);
        // Zero admission is explicitly initialized through the cancellation copy.
        assert(run->metrics.SetAdmission(queue.Get(),true,iteration==5));if(iteration==4)assert(run->metrics.SetAdmission(queue.Get(),false));
        assert(output->RecordInline(list.Get(),run.get(),&Run::Encode,run));assert(output->AdmitInline());
        b.Transition={output->Texture(),0,readState,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);list->CopyBufferRegion(readback.Get(),0,output->Texture(),0,4096);std::swap(b.Transition.StateBefore,b.Transition.StateAfter);list->ResourceBarrier(1,&b);
        assert(SUCCEEDED(list->Close()));ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);assert(output->NativeSubmitted(queue.Get()));assert(raw->NativeSubmitted(queue.Get()));if(previous)assert(previous->NativeSubmitted(queue.Get()));
        assert(SUCCEEDED(queue->Signal(fence.Get(),iteration+1)));assert(SUCCEEDED(fence->SetEventOnCompletion(iteration+1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);
        std::array<UINT,12> historyCounters{};assert(run->metrics.ReadReconstruction(historyCounters));
        const bool active=iteration!=4&&iteration!=5;
        const bool accepted=!freshTest&&(iteration==1||iteration==7||iteration==8||iteration==10||iteration==12);
        assert(historyCounters[0]==(active?1u:0u));
        assert(historyCounters[5]==(accepted?1u:0u));
        assert(historyCounters[6]==(accepted?(iteration==8?3u:2u):0u));
        if(!freshTest&&iteration==10)assert(historyCounters[9]==1); // Nearest normal fails; right tap is valid.
        if(!freshTest&&iteration==12){assert(historyCounters[7]==0&&historyCounters[10]==1&&historyCounters[11]==1);}
        if(!freshTest&&iteration>=13)assert(historyCounters[7]==1&&historyCounters[10]==0&&historyCounters[11]==0);
        const D3D12_RANGE read{0,4096};assert(SUCCEEDED(readback->Map(0,&read,&mapped)));
        double errorSum=0;
        for(UINT i=0;i<64;++i){const auto* sourcePixel=static_cast<const float*>(mapped)+i*16;
            float pixel[8];std::memcpy(pixel,sourcePixel,sizeof(pixel));
            for(UINT k=0;k<3;++k)assert(std::abs(sourcePixel[k]-sourcePixel[8+k]-sourcePixel[12+k])<1e-4f);
            if(split){const float factor=.75f*(i%2?.25f:1.0f)+.125f;for(UINT k=0;k<3;++k)pixel[k]/=factor;}
            if(i==0||iteration==4||iteration==5){for(UINT channel=0;channel<8;++channel)assert(pixel[channel]==0);continue;}
            assert(pixel[3]==1&&std::isfinite(pixel[0])&&pixel[0]>=0&&pixel[0]<=2);
            if(freshTest){
                const float expected=(iteration==9||iteration==11)?2.0f:(iteration==0||iteration==6)?1.0f:(iteration>=7?0.0f:(((i%8+i/8)%2)?2.0f:0.0f));
                for(UINT k=0;k<3;++k)assert(std::abs(pixel[k]-expected)<1e-5f);
                assert(pixel[7]==1.0f);continue;
            }
            if(iteration>=11){
                assert(std::abs(pixel[7]-(iteration==12?2.0f:1.0f))<1e-5f);
                assert(std::abs(pixel[0]-(iteration==11?2.0f:(iteration==12&&!split?1.0f:0.0f)))<1e-5f);continue;
            }
            if(iteration>=9){
                const bool reused=iteration==10&&i%8!=7;
                assert(std::abs(pixel[7]-(reused?2.0f:1.0f))<1e-5f);
                if(!split||iteration==9)assert(std::abs(pixel[0]-(iteration==9?2.0f:(reused?1.0f:0.0f)))<1e-5f);
                continue;
            }
            assert(std::abs(pixel[7]-(iteration==1||iteration==7?2.0f:(iteration==8?3.0f:1.0f)))<1e-5f);
            if(iteration==0||iteration==6)assert(std::abs(pixel[0]-1)<1e-5f);
            if(iteration==7)assert(std::abs(pixel[0]-(split?0.0f:.5f))<1e-5f);
            if(iteration==8)assert(std::abs(pixel[0]-(split?0.0f:1.0f/3.0f))<1e-5f);
            errorSum+=std::abs(pixel[0]-1);
        }
        if(!freshTest&&iteration==1)assert(errorSum/63<.15); // Checker noise was +/-1 before reconstruction.
        readback->Unmap(0,&none);assert(output->Completed());previous=output;previousView=view;previousMaterials=run->pass.materials;previousSignals=run->pass.signals;
    }
    previous.reset();previousSignals.reset();Output::TrimIdle();CloseHandle(event);assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    if(freshTest)puts("PASS: RR receives current diffuse plus specular exactly, checker samples unchanged, old light history ignored, RR-off transition discards incompatible history, coverage/admission retained");
    puts("PASS: raw-buffer initialization, spatial noise reduction, temporal accumulation, material/depth rejection, resolution/cut reset, zero coverage and rejected/probe admission, subpixel camera motion with incompatible nearest and valid neighboring history, renumbered material history retained, same-slot parameter/geometry/flag changes rejected, split lighting-off history clipping");
}
