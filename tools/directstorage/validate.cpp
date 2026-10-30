#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dstorage.h>
#include <wrl/client.h>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr,const char* step){if(FAILED(hr)){printf("FAIL step=%s hr=0x%08lx\n",step,static_cast<unsigned long>(hr));fflush(stdout);ExitProcess(1);}}
static void Wait(HANDLE event,const char* step){if(WaitForSingleObject(event,30000)!=WAIT_OBJECT_0){printf("FAIL step=%s timeout_or_wait_error=1\n",step);fflush(stdout);ExitProcess(2);}}
static ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,UINT64 bytes,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
    D3D12_HEAP_PROPERTIES heap{};heap.Type=type;
    D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> resource;Check(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&resource)),"create_buffer");return resource;
}
int wmain(){
    puts("Control-FG DirectStorage 1.3 foundation validation. Synthetic payload; no game files modified. Not an FPS benchmark.");
    ComPtr<IDXGIFactory6> dxgi;Check(CreateDXGIFactory2(0,IID_PPV_ARGS(&dxgi)),"dxgi_factory");
    ComPtr<IDXGIAdapter1> adapter;ComPtr<ID3D12Device> device;
    for(UINT i=0;dxgi->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter))!=DXGI_ERROR_NOT_FOUND;++i){
        DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);
        if(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))))break;
        adapter.Reset();
    }
    if(!device){Check(dxgi->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"warp_adapter");Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)),"warp_device");}
    DXGI_ADAPTER_DESC1 adapterDesc{};Check(adapter->GetDesc1(&adapterDesc),"adapter_desc");
    printf("adapter_vendor=0x%x adapter_device=0x%x software=%u\n",adapterDesc.VendorId,adapterDesc.DeviceId,unsigned((adapterDesc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)!=0));
    ComPtr<IDStorageFactory> factory;Check(DStorageGetFactory(IID_PPV_ARGS(&factory)),"storage_factory");
    ComPtr<IDStorageCompressionCodec> codec;Check(DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE,0,IID_PPV_ARGS(&codec)),"gdeflate_codec");
    constexpr UINT32 chunkSize=256*1024,chunkCount=16,total=chunkSize*chunkCount;
    std::vector<unsigned char> expected(total),packed;std::vector<UINT64> offsets;std::vector<UINT32> lengths;
    unsigned state=0x12345678;
    for(UINT32 i=0;i<total;++i){state^=state<<13;state^=state>>17;state^=state<<5;expected[i]=static_cast<unsigned char>((i/chunkSize)%2?state:(i/32)%251);}
    for(UINT32 i=0;i<chunkCount;++i){
        std::vector<unsigned char> compressed(codec->CompressBufferBound(chunkSize));size_t written=0;
        Check(codec->CompressBuffer(expected.data()+size_t(i)*chunkSize,chunkSize,DSTORAGE_COMPRESSION_DEFAULT,compressed.data(),compressed.size(),&written),"compress");
        // Independent chunks with padding; exercise nonzero file offsets and mixed compression ratios.
        packed.resize((packed.size()+4095)&~size_t(4095));offsets.push_back(packed.size());lengths.push_back(static_cast<UINT32>(written));
        packed.insert(packed.end(),compressed.begin(),compressed.begin()+written);
    }
    wchar_t tempDir[MAX_PATH]{},path[MAX_PATH]{};
    if(!GetTempPathW(MAX_PATH,tempDir)||!GetTempFileNameW(tempDir,L"cfd",0,path)){puts("FAIL temp_file");return 1;}
    HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){puts("FAIL create_payload");DeleteFileW(path);return 1;}
    DWORD written=0;bool wrote=WriteFile(file,packed.data(),static_cast<DWORD>(packed.size()),&written,nullptr)&&written==packed.size();CloseHandle(file);
    if(!wrote){puts("FAIL write_payload");DeleteFileW(path);return 1;}
    ComPtr<IDStorageFile> source;Check(factory->OpenFile(path,IID_PPV_ARGS(&source)),"storage_open");
    auto gpu=Buffer(device.Get(),total,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
    auto readback=Buffer(device.Get(),total,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    DSTORAGE_QUEUE_DESC q{};q.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;q.Capacity=128;q.Priority=DSTORAGE_PRIORITY_NORMAL;q.Name="ControlFG-foundation";q.Device=device.Get();
    ComPtr<IDStorageQueue2> queue;Check(factory->CreateQueue(&q,IID_PPV_ARGS(&queue)),"storage_queue");
    printf("gdeflate_support_flags=0x%x chunks=%u uncompressed_bytes=%u stored_bytes=%zu\n",unsigned(queue->GetCompressionSupport(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)),chunkCount,total,packed.size());
    ComPtr<IDStorageStatusArray> status;Check(factory->CreateStatusArray(1,"validation",IID_PPV_ARGS(&status)),"status_array");
    ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"fence");
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event){puts("FAIL event");return 1;}
    for(UINT32 i=0;i<chunkCount;++i){
        DSTORAGE_REQUEST request{};request.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;request.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_BUFFER;request.Options.CompressionFormat=DSTORAGE_COMPRESSION_FORMAT_GDEFLATE;
        request.Source.File={source.Get(),offsets[i],lengths[i]};request.Destination.Buffer={gpu.Get(),UINT64(i)*chunkSize,chunkSize};request.UncompressedSize=chunkSize;
        queue->EnqueueRequest(&request);
    }
    queue->EnqueueStatus(status.Get(),0);queue->EnqueueSignal(fence.Get(),1);queue->Submit();
    Check(fence->SetEventOnCompletion(1,event),"storage_completion");Wait(event,"storage_completion");Check(status->GetHResult(0),"storage_requests");
    ComPtr<ID3D12CommandQueue> copy;D3D12_COMMAND_QUEUE_DESC copyDesc{};copyDesc.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(device->CreateCommandQueue(&copyDesc,IID_PPV_ARGS(&copy)),"copy_queue");
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"command_list");
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition.pResource=gpu.Get();barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_COMMON;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    list->ResourceBarrier(1,&barrier);list->CopyBufferRegion(readback.Get(),0,gpu.Get(),0,total);Check(list->Close(),"close_list");
    ID3D12CommandList* lists[]={list.Get()};copy->ExecuteCommandLists(1,lists);Check(copy->Signal(fence.Get(),2),"copy_signal");Check(fence->SetEventOnCompletion(2,event),"copy_completion");Wait(event,"copy_completion");
    void* bytes=nullptr;D3D12_RANGE range{0,total};Check(readback->Map(0,&range,&bytes),"readback_map");bool equal=memcmp(bytes,expected.data(),total)==0;D3D12_RANGE noWrites{0,0};readback->Unmap(0,&noWrites);
    queue->Close();queue.Reset();source.Reset();CloseHandle(event);DeleteFileW(path);
    printf("%s file_to_gpu_gdeflate_byte_equality=%u native_game_integration=0\n",equal?"PASS":"FAIL",unsigned(equal));return equal?0:1;
}
