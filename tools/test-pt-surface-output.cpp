#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <cassert>
#include <cstdio>
#include <array>
#include "../src/pt_surface_output.h"
using Microsoft::WRL::ComPtr;
struct Writer {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> heap;
    static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource* texture,D3D12_CPU_DESCRIPTOR_HANDLE cpu){
        auto& w=*static_cast<Writer*>(context);
        w.device->CopyDescriptorsSimple(1,w.heap->GetCPUDescriptorHandleForHeapStart(),cpu,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        ID3D12DescriptorHeap* heaps[]={w.heap.Get()};list->SetDescriptorHeaps(1,heaps);
        const float value[]={.25f,.5f,.75f,1};
        list->ClearUnorderedAccessViewFloat(w.heap->GetGPUDescriptorHandleForHeapStart(),cpu,texture,value,0,nullptr);return true;
    }
};
struct GuardedWriter {
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12DescriptorHeap> heap;
    ComPtr<ID3D12RootSignature> root;
    ComPtr<ID3D12PipelineState> pipeline;
    static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource*,D3D12_CPU_DESCRIPTOR_HANDLE cpu){
        auto& w=*static_cast<GuardedWriter*>(context);
        w.device->CopyDescriptorsSimple(1,w.heap->GetCPUDescriptorHandleForHeapStart(),cpu,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        ID3D12DescriptorHeap* heaps[]={w.heap.Get()};list->SetDescriptorHeaps(1,heaps);
        list->SetComputeRootSignature(w.root.Get());list->SetPipelineState(w.pipeline.Get());
        list->SetComputeRootDescriptorTable(0,w.heap->GetGPUDescriptorHandleForHeapStart());list->Dispatch(4,3,1);return true;
    }
};
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3D12CommandQueue> queue,wrongQueue;D3D12_COMMAND_QUEUE_DESC qd{};
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&wrongQueue))));
    ComPtr<ID3D12Fence> gate,done;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    assert(SUCCEEDED(queue->Wait(gate.Get(),1))); // Everything below remains in flight until CPU release.
    std::shared_ptr<PTSurfaceComposition::Output> output;
    assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),4,3,output)));
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=6144;rd.Height=1;
    rd.DepthOrArraySize=rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> readback;
    assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback))));
    std::array<ComPtr<ID3D12CommandAllocator>,6> allocators;
    std::array<ComPtr<ID3D12GraphicsCommandList>,6> lists;
    const auto capture=[&](unsigned index){
        assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocators[index]))));
        assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocators[index].Get(),nullptr,IID_PPV_ARGS(&lists[index]))));
        auto* list=lists[index].Get();D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition={output->Texture(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
            static_cast<D3D12_RESOURCE_STATES>(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),D3D12_RESOURCE_STATE_COPY_SOURCE};
        list->ResourceBarrier(1,&barrier);
        D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint.Offset=1024ull*index;dst.PlacedFootprint.Footprint={DXGI_FORMAT_R32G32B32A32_FLOAT,4,3,1,256};
        src.pResource=output->Texture();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
        std::swap(barrier.Transition.StateBefore,barrier.Transition.StateAfter);list->ResourceBarrier(1,&barrier);
        assert(SUCCEEDED(list->Close()));ID3D12CommandList* submitted[]={list};queue->ExecuteCommandLists(1,submitted);
    };
    capture(0);
    auto writer=std::make_shared<Writer>();writer->device=device;
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.NumDescriptors=1;hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&writer->heap))));
    std::weak_ptr<Writer> retained=writer;
    assert(output->Record(writer.get(),&Writer::Encode,writer));
    assert(!output->Produce(wrongQueue.Get()));
    assert(output->Produce(queue.Get()));assert(!output->Produce(queue.Get()));
    writer.reset();assert(!retained.expired());capture(1);
    assert(!output->Invalidate(wrongQueue.Get()));
    assert(output->Invalidate(queue.Get()));assert(output->Invalidate(queue.Get()));
    assert(!output->Produce(queue.Get()));capture(2);
    std::shared_ptr<PTSurfaceComposition::Output> cancelled;
    assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),4,3,cancelled)));
    auto lateWriter=std::make_shared<Writer>();lateWriter->device=device;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&lateWriter->heap))));
    assert(cancelled->Record(lateWriter.get(),&Writer::Encode,lateWriter));
    assert(cancelled->Invalidate(queue.Get()));assert(!cancelled->Produce(queue.Get()));
    // Inline producer is gated entirely by ordered GPU clears. Replay executes
    // the same recorded shader, but sees zero and must select the native result.
    auto original=std::move(output);
    assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),4,3,output)));
    auto guarded=std::make_shared<GuardedWriter>();guarded->device=device;
    assert(SUCCEEDED(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&guarded->heap))));
    D3D12_DESCRIPTOR_RANGE uav{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,0};
    D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;param.DescriptorTable={1,&uav};
    D3D12_ROOT_SIGNATURE_DESC rootDesc{};rootDesc.NumParameters=1;rootDesc.pParameters=&param;
    ComPtr<ID3DBlob> blob,error,shader;
    assert(SUCCEEDED(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error)));
    assert(SUCCEEDED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&guarded->root))));
    const char source[]="RWTexture2D<float4> output:register(u0);[numthreads(1,1,1)]void main(uint3 id:SV_DispatchThreadID){[branch]if(output[id.xy].a!=1){output[id.xy]=0;return;}output[id.xy]=float4(.25,.5,.75,1);}";
    assert(SUCCEEDED(D3DCompile(source,sizeof(source)-1,nullptr,nullptr,nullptr,"main","cs_5_1",D3DCOMPILE_ENABLE_STRICTNESS,0,&shader,&error)));
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=guarded->root.Get();pd.CS={shader->GetBufferPointer(),shader->GetBufferSize()};
    assert(SUCCEEDED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&guarded->pipeline))));
    ComPtr<ID3D12CommandAllocator> nativeAllocator;ComPtr<ID3D12GraphicsCommandList> nativeList;
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&nativeAllocator))));
    assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,nativeAllocator.Get(),nullptr,IID_PPV_ARGS(&nativeList))));
    assert(output->RecordInline(nativeList.Get(),guarded.get(),&GuardedWriter::Encode,guarded));
    assert(SUCCEEDED(nativeList->Close()));ID3D12CommandList* inlineLists[]={nativeList.Get()};
    assert(!output->Arm(wrongQueue.Get()));assert(!output->Produce(queue.Get()));
    queue->ExecuteCommandLists(1,inlineLists);assert(output->NativeSubmitted(queue.Get()));capture(3);
    assert(output->Arm(queue.Get()));assert(!output->Arm(queue.Get()));
    queue->ExecuteCommandLists(1,inlineLists);assert(output->NativeSubmitted(queue.Get()));capture(4);
    assert(output->Invalidate(queue.Get()));assert(!output->Arm(queue.Get()));
    queue->ExecuteCommandLists(1,inlineLists);capture(5);assert(output->NativeSubmitted(queue.Get()));
    std::weak_ptr<GuardedWriter> inlineRetained=guarded;guarded.reset();output.reset();
    PTSurfaceComposition::Output::Poll();assert(!inlineRetained.expired());
    assert(SUCCEEDED(queue->Signal(done.Get(),1)));assert(done->GetCompletedValue()==0);
    assert(SUCCEEDED(gate->Signal(1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    assert(SUCCEEDED(done->SetEventOnCompletion(1,event)));assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    unsigned char* bytes=nullptr;const D3D12_RANGE range{0,6144};assert(SUCCEEDED(readback->Map(0,&range,reinterpret_cast<void**>(&bytes))));
    for(unsigned snapshot=0;snapshot<6;++snapshot)for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x){
        const auto* value=reinterpret_cast<const float*>(bytes+snapshot*1024+y*256+x*16);
        for(unsigned c=0;c<4;++c)assert(value[c]==((snapshot==1||snapshot==4)?float(c+1)*.25f:0));
    }
    const D3D12_RANGE empty{};readback->Unmap(0,&empty);
    original.reset();cancelled.reset();PTSurfaceComposition::Output::Poll();assert(inlineRetained.expired());assert(PTSurfaceComposition::Output::ResidentBytes()==0);assert(retained.expired());assert(SUCCEEDED(device->GetDeviceRemovedReason()));
    puts("PASS: GPU-zero initialization, one-shot producer, queued replay clear, wrong-queue/late-producer rejection inline arm/dispatch/replay guard, bounded deferred retirement and fence-owned dependencies");
}
