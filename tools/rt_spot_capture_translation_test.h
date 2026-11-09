#pragma once
#include <array>
static void TestGpuHitTableTranslation(ID3D12Device* device) {
    namespace gpu=native_lighting::rt_spot_capture_gpu;
    gpu::Resources resources;Check(resources.Initialize(device));
    constexpr UINT records=65, bytes=records*96, checkedBytes=bytes+96;
    ComPtr<ID3D12Resource> source,initializer;
    Check(resources.Buffer(checkedBytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,&source));
    Check(resources.Buffer(checkedBytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,&initializer));
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,0},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,41}};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={3,ranges};
    D3D12_ROOT_SIGNATURE_DESC rootDesc{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();
    ComPtr<ID3D12PipelineState> translate,clear;
    desc.CS={gpu::kSpotCaptureTranslateShader,sizeof(gpu::kSpotCaptureTranslateShader)};Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&translate)));
    desc.CS={gpu::kSpotCaptureClearShader,sizeof(gpu::kSpotCaptureClearShader)};Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&clear)));
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc{};heapDesc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;heapDesc.NumDescriptors=49;heapDesc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;Check(device->CreateDescriptorHeap(&heapDesc,IID_PPV_ARGS(&heap)));
    D3D12_CONSTANT_BUFFER_VIEW_DESC cb{resources.upload->GetGPUVirtualAddress(),256};device->CreateConstantBufferView(&cb,resources.At(0));
    resources.RawSrv(source.Get(),0,bytes,13);resources.RawUav(resources.table.Get(),gpu::kTableBytes,43);resources.RawUav(resources.status.Get(),16,44);
    for(UINT slot:{0u,13u,43u,44u}){auto dst=heap->GetCPUDescriptorHandleForHeapStart();dst.ptr+=SIZE_T(slot)*resources.step;device->CopyDescriptorsSimple(1,dst,resources.At(slot),heapDesc.Type);}
    D3D12_COMMAND_QUEUE_DESC queueDesc{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&queueDesc,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
    gpu::TableControls controls;controls.count=records;controls.bytes=bytes;
    for(UINT i=0;i<32;++i){controls.nativeIds[0][i]=static_cast<unsigned char>(i+1);controls.nativeIds[1][i]=static_cast<unsigned char>(i+51);
        controls.captureIds[0][i]=static_cast<unsigned char>(i+101);controls.captureIds[1][i]=static_cast<unsigned char>(i+151);}
    std::array<unsigned char,checkedBytes> input{},expected{},actual{};
    input.fill(0xcc);expected.fill(0xcc);
    for(UINT i=0;i<records;++i) {
        for(UINT j=32;j<96;++j)input[i*96+j]=static_cast<unsigned char>(i+j);
        if(i%3==2)memset(input.data()+i*96,0,32);else memcpy(input.data()+i*96,controls.nativeIds[i%3],32);
    }
    rt_spot_capture::IdentifierReplacement replacements[2]{};
    for(UINT i=0;i<2;++i){memcpy(replacements[i].native.data(),controls.nativeIds[i],32);memcpy(replacements[i].capture.data(),controls.captureIds[i],32);}
    // The CPU reference is independent of the GPU instruction sequence.
    if(!rt_spot_capture::TranslateHitTableBytes(input.data(),bytes,replacements,2,expected.data(),expected.size()))throw std::runtime_error("CPU fixture translation failed");
    UINT64 fenceValue=0;
    auto run=[&](UINT unknown,UINT invalid) {
        void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(source->Map(0,&empty,&mapped));memcpy(mapped,input.data(),input.size());source->Unmap(0,nullptr);
        Check(initializer->Map(0,&empty,&mapped));memset(mapped,0xcc,checkedBytes);initializer->Unmap(0,nullptr);
        Check(resources.upload->Map(0,&empty,&mapped));memcpy(mapped,&controls,sizeof(controls));resources.upload->Unmap(0,nullptr);
        Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));
        gpu::Transition(list.Get(),resources.table.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_DEST);
        list->CopyBufferRegion(resources.table.Get(),0,initializer.Get(),0,checkedBytes);
        gpu::Transition(list.Get(),resources.table.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());
        list->SetPipelineState(clear.Get());list->Dispatch(1,1,1);gpu::UavBarrier(list.Get());
        list->SetPipelineState(translate.Get());list->Dispatch(2,1,1);
        gpu::Transition(list.Get(),resources.table.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        gpu::Transition(list.Get(),resources.status.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
        list->CopyBufferRegion(resources.readback.Get(),0,resources.table.Get(),0,checkedBytes);
        list->CopyBufferRegion(resources.readback.Get(),checkedBytes,resources.status.Get(),0,16);
        gpu::Transition(list.Get(),resources.table.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        gpu::Transition(list.Get(),resources.status.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(resources.fence.Get(),++fenceValue));
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("capture test event");
        Check(resources.fence->SetEventOnCompletion(fenceValue,event));const auto wait=WaitForSingleObject(event,10000);CloseHandle(event);
        if(wait!=WAIT_OBJECT_0)throw std::runtime_error("capture GPU test timeout");
        D3D12_RANGE read{0,checkedBytes+16};Check(resources.readback->Map(0,&read,&mapped));memcpy(actual.data(),mapped,actual.size());
        UINT status[4]{};memcpy(status,static_cast<unsigned char*>(mapped)+checkedBytes,16);resources.readback->Unmap(0,&empty);
        if(actual!=expected || status[0]!=unknown || status[3]!=invalid || (!invalid && status[1]+status[2]+status[0]!=records))throw std::runtime_error("GPU capture table mismatch");
    };
    run(0,0);
    // Late unknown identifier: count it and null only its identifier; no traversal is allowed.
    memset(input.data()+64*96,0xa7,32);memset(expected.data()+64*96,0,32);run(1,0);
    controls.version=2;expected.fill(0xcc);run(0,1);
    controls.version=1;memcpy(controls.nativeIds[1],controls.nativeIds[0],32);run(0,1);
    std::cout<<"PASS GPU capture table: 65 records, partial final group, CPU-reference parity, nonzero null-record arguments, late unknown, bad layout, duplicate identifiers and untouched guard bytes\n";
}
