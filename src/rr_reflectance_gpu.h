#pragma once
#include "rr_reflectance_compare.h"
#include "../build/rr_reflectance_compiled.h"
// Dedicated capture worker only. Immutable same-frame bytes are uploaded once;
// this is GPU implementation validation, not the future per-frame RR lifecycle.
struct RRReflectanceGpu {
    ID3D12Device* device=nullptr;ID3D12CommandQueue* queue=nullptr;
    ID3D12CommandAllocator* allocator=nullptr;ID3D12GraphicsCommandList* list=nullptr;
    ID3D12RootSignature* root=nullptr;ID3D12PipelineState* pipeline=nullptr;
    ID3D12Resource* upload=nullptr;ID3D12Resource* output=nullptr;ID3D12Resource* readback=nullptr;
    ID3D12Fence* fence=nullptr;HANDLE event=nullptr;void* mapped=nullptr;
    bool uncertain=false;const char* stage="resources";
    ~RRReflectanceGpu() {
        if(mapped) {D3D12_RANGE none{0,0};readback->Unmap(0,&none);}
        if(event) CloseHandle(event);
        RRGuideRelease(list);RRGuideRelease(allocator);RRGuideRelease(upload);
        RRGuideRelease(output);RRGuideRelease(readback);RRGuideRelease(pipeline);
        RRGuideRelease(root);RRGuideRelease(fence);RRGuideRelease(queue);RRGuideRelease(device);
    }
};
static void RRReflectanceGpuRetire(RRReflectanceGpu*& p) noexcept {
    if(p && !p->uncertain) delete p;
    // A timed-out/ambiguous submission retains every referenced object. Never
    // release memory a queued GPU command might still access.
    else if(p) Log("RR_GUIDE_G12_GPU_RETAIN reason=submission_or_fence_uncertainty");
    p=nullptr;
}
static HRESULT RRReflectanceBuffer(RRReflectanceGpu* c,UINT64 bytes,D3D12_HEAP_TYPE heap,
    D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags,ID3D12Resource** out) noexcept {
    D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;hp.CreationNodeMask=hp.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
    d.Width=bytes;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;
    d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;d.Flags=flags;
    return c->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,
        __uuidof(ID3D12Resource),reinterpret_cast<void**>(out));
}
static bool RRReflectanceGpuRun(RRGuideJob* job,RRGuideExportView* v,RRReflectanceGpu** owner) noexcept {
    using namespace control_rr_reflectance;
    float sx=0,sy=0;
    if(!v->part1Data || !v->part1Bytes || v->part1Bytes%8 || v->part1Bytes>524288 ||
       !v->additionalImageCount || !v->additionalImages ||
       v->additionalImages[0].semantic!=RRGuideImageSemantic::NativeAlbedoTarget0 ||
       v->additionalImages[0].sourceFrame!=v->engineFrame ||
       !SupportedProjection(v->camera.viewToClip,&sx,&sy)) {
        Log("RR_GUIDE_G12_GPU_SKIP reason=part1_albedo_or_projection_unavailable");return false;
    }
    const auto& albedo=v->additionalImages[0];
    const UINT64 pixels=UINT64(v->width)*v->height;
    if(!pixels || pixels>8388608 || albedo.width!=v->width || albedo.height!=v->height ||
       !albedo.data || albedo.rowPitch<SIZE_T(v->width)*8 ||
       v->gbuffer1RowPitch<SIZE_T(v->width)*4 || v->gbuffer2RowPitch<SIZE_T(v->width)*4) return false;
    const UINT64 inputBytes=pixels*16+v->part1Bytes,outputBytes=pixels*24;
    auto* c=new(std::nothrow) RRReflectanceGpu;
    if(!c) return false;
    *owner=c;c->device=job->device;c->device->AddRef();c->queue=job->queue;c->queue->AddRef();
    HRESULT hr=RRReflectanceBuffer(c,inputBytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,
        D3D12_RESOURCE_FLAG_NONE,&c->upload);
    if(SUCCEEDED(hr)) hr=RRReflectanceBuffer(c,outputBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,&c->output);
    if(SUCCEEDED(hr)) hr=RRReflectanceBuffer(c,outputBytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_FLAG_NONE,&c->readback);
    if(FAILED(hr)) {Log("RR_GUIDE_G12_GPU_HRESULT stage=%s hr=0x%08lX",c->stage,static_cast<unsigned long>(hr));return false;}
    c->stage="upload_map";
    void* upload=nullptr;D3D12_RANGE noRead{0,0};hr=c->upload->Map(0,&noRead,&upload);
    if(FAILED(hr) || !upload) return false;
    auto* dst=static_cast<unsigned char*>(upload);
    for(UINT y=0;y<v->height;++y) {
        memcpy(dst+SIZE_T(y)*v->width*4,static_cast<const unsigned char*>(v->gbuffer1)+SIZE_T(y)*v->gbuffer1RowPitch,SIZE_T(v->width)*4);
        memcpy(dst+SIZE_T(pixels)*4+SIZE_T(y)*v->width*4,static_cast<const unsigned char*>(v->gbuffer2)+SIZE_T(y)*v->gbuffer2RowPitch,SIZE_T(v->width)*4);
        memcpy(dst+SIZE_T(pixels)*8+SIZE_T(y)*v->width*8,static_cast<const unsigned char*>(albedo.data)+SIZE_T(y)*albedo.rowPitch,SIZE_T(v->width)*8);
    }
    memcpy(dst+SIZE_T(pixels)*16,v->part1Data,v->part1Bytes);
    D3D12_RANGE written{0,SIZE_T(inputBytes)};c->upload->Unmap(0,&written);
    c->stage="root_signature";
    D3D12_ROOT_PARAMETER params[3]{};
    params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
    params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;
    params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[2].Constants.Num32BitValues=6;
    D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=3;rd.pParameters=params;
    auto serialize=reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(GetProcAddress(GetModuleHandleW(L"d3d12.dll"),"D3D12SerializeRootSignature"));
    if(!serialize) return false;
    ID3DBlob* blob=nullptr;ID3DBlob* errors=nullptr;
    hr=serialize(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&errors);
    if(SUCCEEDED(hr) && blob) hr=c->device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&c->root));
    else if(SUCCEEDED(hr)) hr=E_UNEXPECTED;
    RRGuideRelease(blob);RRGuideRelease(errors);if(FAILED(hr)) {Log("RR_GUIDE_G12_GPU_HRESULT stage=%s hr=0x%08lX",c->stage,static_cast<unsigned long>(hr));return false;}
    c->stage="pipeline_and_command_objects";
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=c->root;pd.CS={kRRReflectanceShader,sizeof(kRRReflectanceShader)};
    hr=c->device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&c->pipeline));
    if(SUCCEEDED(hr)) hr=c->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&c->allocator));
    if(SUCCEEDED(hr)) hr=c->device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,c->allocator,c->pipeline,IID_PPV_ARGS(&c->list));
    if(SUCCEEDED(hr)) hr=c->device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&c->fence));
    if(FAILED(hr)) {Log("RR_GUIDE_G12_GPU_HRESULT stage=%s hr=0x%08lX",c->stage,static_cast<unsigned long>(hr));return false;}
    c->stage="event_and_recording";
    c->event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!c->event) return false;
    struct Constants {UINT width,height,records,reserved;float sx,sy;} constants{v->width,v->height,UINT(v->part1Bytes/8),0,sx,sy};
    static_assert(sizeof(Constants)==24,"Reflectance root constants ABI");
    c->list->SetComputeRootSignature(c->root);
    c->list->SetComputeRootShaderResourceView(0,c->upload->GetGPUVirtualAddress());
    c->list->SetComputeRootUnorderedAccessView(1,c->output->GetGPUVirtualAddress());
    c->list->SetComputeRoot32BitConstants(2,6,&constants,0);
    c->list->Dispatch((v->width+7)/8,(v->height+7)/8,1);
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource=c->output;barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore=D3D12_RESOURCE_STATE_UNORDERED_ACCESS;barrier.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;
    c->list->ResourceBarrier(1,&barrier);c->list->CopyBufferRegion(c->readback,0,c->output,0,outputBytes);
    hr=c->list->Close();if(FAILED(hr)) {Log("RR_GUIDE_G12_GPU_HRESULT stage=%s hr=0x%08lX",c->stage,static_cast<unsigned long>(hr));return false;}
    c->stage="submission_and_fence";
    c->uncertain=true;ID3D12CommandList* lists[]={c->list};c->queue->ExecuteCommandLists(1,lists);
    hr=c->queue->Signal(c->fence,1);if(FAILED(hr)) {Log("RR_GUIDE_G12_GPU_HRESULT stage=%s hr=0x%08lX",c->stage,static_cast<unsigned long>(hr));return false;}
    UINT64 complete=c->fence->GetCompletedValue();
    if(complete==UINT64_MAX) return false;
    if(complete<1) {
        if(FAILED(c->fence->SetEventOnCompletion(1,c->event))) return false;
        if(WaitForSingleObject(c->event,30000)!=WAIT_OBJECT_0) return false;
        complete=c->fence->GetCompletedValue();if(complete<1 || complete==UINT64_MAX) return false;
    }
    c->uncertain=false;
    c->stage="readback_map";
    D3D12_RANGE range{0,SIZE_T(outputBytes)};hr=c->readback->Map(0,&range,&c->mapped);
    if(FAILED(hr) || !c->mapped) return false;
    c->stage="full_image_comparison";
    const CandidateSource source{v->width,v->height,
        static_cast<const unsigned char*>(v->gbuffer1),static_cast<const unsigned char*>(v->gbuffer2),
        static_cast<const unsigned char*>(albedo.data),static_cast<const unsigned char*>(v->part1Data),
        v->gbuffer1RowPitch,v->gbuffer2RowPitch,albedo.rowPitch,v->part1Bytes,sx,sy};
    const auto comparison=CompareCandidate(source,c->mapped,SIZE_T(outputBytes));
    const UINT64 errorsCount=comparison.diffuseErrors+comparison.specularErrors+(comparison.layout?0:1);
    const double maximumError=comparison.maximumError;
    Log("RR_GUIDE_G12_GPU_COMPARE frame=%llu pixels=%llu errors=%llu max_absolute_error=%.9g diffuse_rgb_exact=%u source=immutable_same_frame_capture view=unjittered_pixel_center coverage=unmasked rr_eval=disabled",
        v->engineFrame,pixels,errorsCount,maximumError,unsigned(comparison.layout && comparison.diffuseErrors==0));
    if(errorsCount) return false;
    v->reflectanceData=c->mapped;v->reflectanceBytes=SIZE_T(outputBytes);v->reflectanceMaxError=maximumError;
    return true;
}
