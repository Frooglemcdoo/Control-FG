// Included in ControlNrdRuntime to reuse its command-list leases and native
// descriptor allocator. The character pass has no denoiser history.
static const char* RecordCharacterBlend(const control_rr_reflection::Context& ctx) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    Mute mute;
    if(!rrDiffuse || rrDiffuseStage.load(std::memory_order_acquire)!=RRDiffuseStage::Joined ||
       rrDiffuse->frame!=ctx.frame || rrDiffuse->cameraView!=ctx.view ||
       reinterpret_cast<UINT64>(rrDiffuse->queue)!=ctx.queue) return "mask_frame_or_view";
    const UINT w=rrDiffuse->width, h=rrDiffuse->height;
    auto* mask=RRDiffuseCharacterCurrent(ctx.frame,w,h);
    if(!mask) return "no_complete_mask";
    PTGlassRuntime::Context c;gi_reuse::ComputeBindingSnapshot binding;UINT64 generation=0;
    if(!Bindings(c,binding,generation) || !binding.Ready() || reinterpret_cast<UINT64>(c.list)!=ctx.list ||
       reinterpret_cast<UINT64>(c.queue)!=ctx.queue) return "command_bindings";
    Poll();
    control_nrd_native::Texture raw{},out{},depth{};
    if(!NamedTexture("g_DLF_rwtColorSource",raw) || !NamedTexture("g_DLF_rwtColorTarget",out) ||
       !NamedTexture("g_tClipDepth",depth,true)) return "gi_or_depth_provider";
    if(raw.desc.Format!=DXGI_FORMAT_R16G16B16A16_FLOAT || out.desc.Format!=raw.desc.Format ||
       (depth.desc.Format!=DXGI_FORMAT_R32_TYPELESS && depth.desc.Format!=DXGI_FORMAT_R32_FLOAT) ||
       (depth.desc.Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ||
       out.state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS || !(out.desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
        return "gi_or_depth_format";
    RRGuideInputTexture g1{},g2{};const char* reason=nullptr;DWORD fault=0;
    if(!RRGuideReadTexture(kRRGuideGBuffer1ShaderRva,&g1,&reason,&fault) ||
       !RRGuideReadTexture(kRRGuideGBuffer2ShaderRva,&g2,&reason,&fault)) return "primary_gbuffer";
    ID3D12Resource* resources[]={raw.resource,mask,depth.resource,g1.resource,g2.resource,out.resource};
    const DXGI_FORMAT formats[]={raw.desc.Format,DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R32_FLOAT,
                                 g1.desc.Format,g2.desc.Format,out.desc.Format};
    ID3D12Device* device=PTGlassRuntime::device.Get();
    for(unsigned i=0;i<6;++i){
        const auto d=resources[i]->GetDesc();
        if(d.Width!=w || d.Height!=h || d.DepthOrArraySize!=1 || !d.MipLevels ||
           d.SampleDesc.Count!=1 || (i==1 && d.Format!=formats[i])) return "extent";
        for(unsigned j=0;j<i;++j) if(resources[i]==resources[j])return "resource_alias";
        ComPtr<ID3D12Device> owner;
        if(FAILED(resources[i]->GetDevice(IID_PPV_ARGS(&owner))) || owner.Get()!=device)return "device";
    }
    static ComPtr<ID3D12RootSignature> root;
    static ComPtr<ID3D12PipelineState> pipeline;
    if(!pipeline){
        D3D12_DESCRIPTOR_RANGE ranges[]={{D3D12_DESCRIPTOR_RANGE_TYPE_SRV,5,0,0,0},{D3D12_DESCRIPTOR_RANGE_TYPE_UAV,1,0,0,5}};
        D3D12_ROOT_PARAMETER table{};table.ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;table.DescriptorTable={2,ranges};
        D3D12_ROOT_SIGNATURE_DESC desc{1,&table,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ComPtr<ID3DBlob> blob,error;ComPtr<ID3D12RootSignature> prepared;
        if(FAILED(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error)) ||
           FAILED(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&prepared))))return "root_signature";
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=prepared.Get();pd.CS={kGICharacterBlend,sizeof(kGICharacterBlend)};
        if(FAILED(device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pipeline))))return "pipeline";
        root=std::move(prepared);
    }
    auto* rec=GetRecording(c.rays,generation);
    if(!rec || rec->characterTextures[0])return "recording_busy";
    PTGlass::DescriptorRange range{};
    if(!PTGlassRuntime::FreshCount(c,6,&range,&reason))return reason?reason:"descriptor_range";
    auto cpu=range.heap->GetCPUDescriptorHandleForHeapStart();auto gpu=range.heap->GetGPUDescriptorHandleForHeapStart();
    const UINT step=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpu.ptr+=SIZE_T(step)*range.first;gpu.ptr+=UINT64(step)*range.first;
    for(unsigned i=0;i<5;++i){
        D3D12_SHADER_RESOURCE_VIEW_DESC d{};d.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;d.Format=formats[i];
        d.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;d.Texture2D.MipLevels=1;
        device->CreateShaderResourceView(resources[i],&d,cpu);cpu.ptr+=step;
    }
    D3D12_UNORDERED_ACCESS_VIEW_DESC ud{};ud.Format=formats[5];ud.ViewDimension=D3D12_UAV_DIMENSION_TEXTURE2D;
    device->CreateUnorderedAccessView(out.resource,nullptr,&ud,cpu);
    for(unsigned i=0;i<6;++i)rec->characterTextures[i]=resources[i];
    const D3D12_RESOURCE_STATES states[]={raw.state,D3D12_RESOURCE_STATE_RENDER_TARGET,depth.state,
        D3D12_RESOURCE_STATES(g1.trackedState),D3D12_RESOURCE_STATES(g2.trackedState)};
    D3D12_RESOURCE_BARRIER barriers[5]{};UINT n=0;
    for(unsigned i=0;i<5;++i){
        if(states[i]&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)continue;
        auto& b=barriers[n++];b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        b.Transition={resources[i],0,states[i],D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE};
    }
    D3D12_RESOURCE_BARRIER order{};order.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;order.UAV.pResource=out.resource;
    c.list->ResourceBarrier(1,&order);if(n)c.list->ResourceBarrier(n,barriers);
    c.list->SetComputeRootSignature(root.Get());c.list->SetPipelineState(pipeline.Get());
    c.list->SetComputeRootDescriptorTable(0,gpu);c.list->Dispatch((w+7)/8,(h+7)/8,1);
    for(UINT i=0;i<n;++i)std::swap(barriers[i].Transition.StateBefore,barriers[i].Transition.StateAfter);
    if(n)c.list->ResourceBarrier(n,barriers);c.list->ResourceBarrier(1,&order);
    binding.Restore(c.rays);
    return "recorded";
}
