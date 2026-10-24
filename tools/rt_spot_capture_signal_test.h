#pragma once
// Exercise the real embedded sampler against patterned native-layout textures.
static void TestCompositionSignal(ID3D12Device* device) {
    namespace gpu=native_lighting::rt_spot_capture_gpu;
    gpu::Resources resources;Check(resources.Initialize(device));
    D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_CBV,5,4,0,0},
        {D3D12_DESCRIPTOR_RANGE_TYPE_SRV,36,0,0,5},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,8,0,0,41}};
    D3D12_ROOT_PARAMETER parameter{};parameter.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;parameter.DescriptorTable={3,ranges};
    D3D12_ROOT_SIGNATURE_DESC rootDesc{1,&parameter,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
    ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rootDesc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
    ComPtr<ID3D12RootSignature> root;Check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
    const char* fixture=R"(
        RWTexture2D<float4> diffuse : register(u0);
        RWTexture2D<float4> specular : register(u1);
        RWTexture2D<uint2> ids : register(u3);
        RWTexture2D<float4> visibility : register(u4);
        [numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID) {
            ids[p.xy]=uint2(p.x+100,p.y+200);
            visibility[p.xy]=float4((p.x%2)*.5,.75,2,3);
            diffuse[p.xy]=float4(p.x+8,p.y+4,2,1);
            specular[p.xy]=float4(p.x+16,p.y+8,4,1);
        })";
    ComPtr<ID3DBlob> shader;Check(D3DCompile(fixture,strlen(fixture),nullptr,nullptr,nullptr,"main","cs_5_1",0,0,&shader,&error));
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};desc.pRootSignature=root.Get();
    ComPtr<ID3D12PipelineState> initialize,sample;
    desc.CS={shader->GetBufferPointer(),shader->GetBufferSize()};Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&initialize)));
    desc.CS={gpu::kSpotCaptureSignalShader,sizeof(gpu::kSpotCaptureSignalShader)};Check(device->CreateComputePipelineState(&desc,IID_PPV_ARGS(&sample)));
    ComPtr<ID3D12Resource> textures[4];
    for(UINT i=0;i<4;++i) {
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
        D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=128;td.Height=64;
        td.DepthOrArraySize=td.MipLevels=td.SampleDesc.Count=1;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        td.Format=i==2?DXGI_FORMAT_R32G32_UINT:DXGI_FORMAT_R32G32B32A32_FLOAT;
        Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&textures[i])));
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};uav.Format=td.Format;uav.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
        device->CreateUnorderedAccessView(textures[i].Get(),nullptr,&uav,resources.At(41+i+(i>=2?1:0)));
        if(i>=2) {
            D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.Format=td.Format;srv.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;
            srv.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;srv.Texture2D.MipLevels=1;
            device->CreateShaderResourceView(textures[i].Get(),&srv,resources.At(19+i-2));
        }
    }
    void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(resources.upload->Map(0,&empty,&mapped));
    const UINT controls[]={128,64,123,64};memcpy(mapped,controls,sizeof(controls));resources.upload->Unmap(0,nullptr);
    D3D12_CONSTANT_BUFFER_VIEW_DESC cb{resources.upload->GetGPUVirtualAddress(),256};device->CreateConstantBufferView(&cb,resources.At(0));
    resources.RawUav(resources.signal.Get(),gpu::kSignalBytes,43);
    D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;hd.NumDescriptors=49;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ComPtr<ID3D12DescriptorHeap> heap;Check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));
    for(UINT slot:{0u,19u,20u,41u,42u,43u,44u,45u}) {
        auto dst=heap->GetCPUDescriptorHandleForHeapStart();dst.ptr+=SIZE_T(slot)*resources.step;
        device->CopyDescriptorsSimple(1,dst,resources.At(slot),hd.Type);
    }
    D3D12_COMMAND_QUEUE_DESC qd{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
    ID3D12DescriptorHeap* heaps[]={heap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetComputeRootSignature(root.Get());list->SetComputeRootDescriptorTable(0,heap->GetGPUDescriptorHandleForHeapStart());
    list->SetPipelineState(initialize.Get());list->Dispatch(16,8,1);gpu::UavBarrier(list.Get());
    for(UINT i=2;i<4;++i)gpu::Transition(list.Get(),textures[i].Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    list->SetPipelineState(sample.Get());list->Dispatch(8,8,1);
    gpu::Transition(list.Get(),resources.signal.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
    list->CopyBufferRegion(resources.readback.Get(),0,resources.signal.Get(),0,gpu::kSignalBytes);
    Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(resources.fence.Get(),1));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("signal test event");
    Check(resources.fence->SetEventOnCompletion(1,event));const auto wait=WaitForSingleObject(event,10000);CloseHandle(event);
    if(wait!=WAIT_OBJECT_0)throw std::runtime_error("signal GPU test timeout");
    D3D12_RANGE read{0,gpu::kSignalBytes};Check(resources.readback->Map(0,&read,&mapped));
    bool valid=true;
    for(UINT i=0;i<4096;++i) {
        UINT ids[4]{};float values[12]{};auto* bytes=static_cast<unsigned char*>(mapped)+i*64;
        memcpy(ids,bytes,16);memcpy(values,bytes+16,48);const UINT x=2*(i%64)+1,y=i/64;
        valid&=ids[0]==x && ids[1]==y && ids[2]==x+100 && ids[3]==y+200;
        const float expected[]={.5f,.75f,2,3,float(x+8),float(y+4),2,1,float(x+16),float(y+8),4,1};
        valid&=memcmp(values,expected,sizeof(values))==0;
    }
    resources.readback->Unmap(0,&empty);
    if(!valid)throw std::runtime_error("composition signal differs from GPU fixture");
    std::cout<<"PASS composition signal: 4096 exact grid pixels, light IDs, visibility and unclamped HDR diffuse/specular\n";
}
