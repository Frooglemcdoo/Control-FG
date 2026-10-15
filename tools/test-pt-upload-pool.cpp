#define NOMINMAX
#include <windows.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/pt_upload_pool.h"
#include "../src/pt_surface_output.h"
using Microsoft::WRL::ComPtr;
struct UploadUse {
    std::shared_ptr<PTUploadPool::Buffer> buffer;
    ComPtr<ID3D12Resource> readback;
    static bool Encode(void* context,ID3D12GraphicsCommandList* list,ID3D12Resource*,D3D12_CPU_DESCRIPTOR_HANDLE){
        auto& self=*static_cast<UploadUse*>(context);
        list->CopyBufferRegion(self.readback.Get(),0,self.buffer->Resource(),0,4);return true;
    }
};
static void TestChangingSizes(ID3D12Device* device){
    // Fill all idle slots with obsolete SBT capacities. Camera buffers must
    // replace an old idle entry, rather than being discarded every frame.
    std::array<std::shared_ptr<PTUploadPool::Buffer>,16> obsolete;
    for(std::size_t i=0;i<obsolete.size();++i)
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,(i+3)*65536,obsolete[i])));
    for(auto& item:obsolete)item.reset();
    std::shared_ptr<PTUploadPool::Buffer> camera,table;
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,1792,camera)));
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,3*1024*1024,table)));
    camera.reset();table.reset();const auto warm=PTUploadPool::Buffer::Statistics();
    for(unsigned frame=0;frame<128;++frame){
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,1792,camera)));
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,3*1024*1024,table)));
        camera.reset();table.reset();
    }
    auto after=PTUploadPool::Buffer::Statistics();
    assert(after.created==warm.created&&after.reused==warm.reused+256&&after.idleBytes<=32ull*1024*1024);
    // Exercise the byte ceiling as well as the slot ceiling.
    std::shared_ptr<PTUploadPool::Buffer> largeA,largeB;
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,16*1024*1024,largeA)));
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,16*1024*1024,largeB)));
    largeA.reset();largeB.reset();assert(PTUploadPool::Buffer::Statistics().idleBytes==32ull*1024*1024);
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,1792,camera)));camera.reset();
    const auto full=PTUploadPool::Buffer::Statistics();
    assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device,1792,camera)));camera.reset();
    after=PTUploadPool::Buffer::Statistics();assert(after.created==full.created&&after.idleBytes<=32ull*1024*1024);
    puts("PASS upload pool pressure: current camera/SBT sizes replace obsolete idle capacities; zero steady-state creates; 32 MiB limit retained.");
}
int main(){
    ComPtr<IDXGIFactory4> factory;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
    assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};
    assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12Fence> gate,done;
    assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&done))));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    for(unsigned pass=1;pass<=2;++pass){
        auto use=std::make_shared<UploadUse>();const UINT64 bytes=UINT64(pass)*65536;
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device.Get(),bytes,use->buffer)));
        const auto address=use->buffer->Resource()->GetGPUVirtualAddress();
        void* data=nullptr;D3D12_RANGE noRead{};
        assert(SUCCEEDED(use->buffer->Resource()->Map(0,&noRead,&data)));
        const UINT value=0x12340000u+pass;std::memcpy(data,&value,4);use->buffer->Resource()->Unmap(0,nullptr);
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=4;desc.Height=1;
        desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        assert(SUCCEEDED(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&use->readback))));
        auto readback=use->readback;
        ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
        assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator))));
        assert(SUCCEEDED(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list))));
        std::shared_ptr<PTSurfaceComposition::Output> output;
        assert(SUCCEEDED(PTSurfaceComposition::Output::Create(queue.Get(),1,1,output,true)));
        assert(output->RecordInline(list.Get(),use.get(),&UploadUse::Encode,use));
        assert(SUCCEEDED(list->Close()));assert(output->AdmitInline());
        assert(SUCCEEDED(queue->Wait(gate.Get(),pass)));
        ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
        assert(output->NativeSubmitted(queue.Get()));assert(SUCCEEDED(queue->Signal(done.Get(),pass)));
        auto recording=output;use.reset();output.reset();
        // Pass 1: completed GPU cannot recycle a still-recorded buffer.
        // Pass 2: no recording owner cannot recycle an in-flight buffer.
        if(pass==2)recording.reset();
        PTSurfaceComposition::Output::Poll();
        std::shared_ptr<PTUploadPool::Buffer> other;
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device.Get(),bytes,other)));
        assert(other->Resource()->GetGPUVirtualAddress()!=address);
        assert(SUCCEEDED(gate->Signal(pass)));
        assert(SUCCEEDED(done->SetEventOnCompletion(pass,event)));
        assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);
        assert(SUCCEEDED(readback->Map(0,nullptr,&data)));UINT observed=0;std::memcpy(&observed,data,4);
        readback->Unmap(0,&noRead);assert(observed==value);
        std::shared_ptr<PTUploadPool::Buffer> stillRecorded;
        if(pass==1){
            assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device.Get(),bytes,stillRecorded)));
            assert(stillRecorded->Resource()->GetGPUVirtualAddress()!=address);
        }
        list.Reset();allocator.Reset();recording.reset();PTSurfaceComposition::Output::Poll();
        std::shared_ptr<PTUploadPool::Buffer> recycled;
        assert(SUCCEEDED(PTUploadPool::Buffer::Acquire(device.Get(),bytes,recycled)));
        assert(recycled->Resource()->GetGPUVirtualAddress()==address);
    }
    TestChangingSizes(device.Get());
    CloseHandle(event);const auto stats=PTUploadPool::Buffer::Statistics();assert(stats.reused>=2&&stats.idleBytes<=32ull*1024*1024);
    puts("PASS: pooled uploads retain GPU contents; recycling waits for recording release AND completed retirement fence.");
}
