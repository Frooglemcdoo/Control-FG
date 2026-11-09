#pragma once
// Bounded G3: source copies in the verified native Direct list, followed by
// owned compute/readback after that list's native Present submission boundary.
// This never supplies RR guides to NGX or changes the game denoiser selection.
#include "rr_guide_inputs.h"
#include "rr_guide_shader.h"
#include "rr_guide_export.h"
#include "rr_part1_copy_policy.h"

// Implemented by rr_albedo_capture.h after this header. The albedo gate owns
// its bounded same-frame wait and terminal normal-only fallback policy.
struct RRGuideJob;
static bool RRAlbedoAllowGuideCapture(unsigned long long engineFrame) noexcept;
static bool RRAlbedoCapturePending() noexcept;
static bool RRAlbedoCopyForGuide(RRGuideJob* job, const char** reason) noexcept;
static void RRAlbedoFillExportView(RRGuideExportView* view) noexcept;
static void RRAlbedoUnmapExports() noexcept;

enum class RRGuideStage { Waiting, Preparing, Prepared, Copied, Pending, Writing, Finished, Stopped };
static SRWLOCK rrGuideLock = SRWLOCK_INIT;
static std::atomic<bool> rrGuideInactive{true};
static RRGuideStage rrGuideStage = RRGuideStage::Waiting; // protected by rrGuideLock
static unsigned int rrGuideAttempts = 0;
static unsigned long long rrGuideLastAttempt = 0;
static constexpr UINT64 kRRGuideMaxPixels = 8388608;
static std::atomic<std::uint64_t> rrGuideHighResolutionExtent{0};

// Serializes all Present/capture access. The worker only retires a job under
// this lock; no Present reader can observe a job while it is being deleted.
struct RRGuideEntry {
    DWORD lastError = GetLastError();
    bool locked = TryAcquireSRWLockExclusive(&rrGuideLock) != FALSE;
    ~RRGuideEntry() { if (locked) ReleaseSRWLockExclusive(&rrGuideLock); SetLastError(lastError); }
};
template<class T> static void RRGuideRelease(T*& p) noexcept {
    if (p) { p->Release(); p = nullptr; }
}
struct RRGuideJob {
    RRGuideInputSnapshot input{};
    ID3D12Device* device = nullptr;
    ID3D12CommandQueue* queue = nullptr; // retained unwrapped Direct queue
    ID3D12CommandAllocator* allocator = nullptr;
    ID3D12GraphicsCommandList* list = nullptr;
    ID3D12RootSignature* root = nullptr;
    ID3D12PipelineState* pipeline = nullptr;
    ID3D12DescriptorHeap* heap = nullptr;
    ID3D12Fence* fence = nullptr;
    ID3D12Resource* textures[3]{};
    ID3D12Resource* readbacks[3]{};
    ID3D12Resource* part1Readback=nullptr;
    ID3D12Resource* part1Source=nullptr; // owned reference, released only on safe job retirement
    UINT64 part1Bytes=0;
    bool part1Copied=false;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[3]{};
    UINT64 readbackBytes[3]{};
    UINT width = 0, height = 0;
    UINT preparationAttempt = 0;
    bool retireOnly = false;
    unsigned long long submittedPresent = 0;
    bool slowFenceLogged = false;
    ~RRGuideJob() {
        RRGuideRelease(part1Readback); RRGuideRelease(part1Source);
        for (auto& p : readbacks) RRGuideRelease(p);
        for (auto& p : textures) RRGuideRelease(p);
        RRGuideRelease(list); RRGuideRelease(allocator);
        RRGuideRelease(heap); RRGuideRelease(pipeline); RRGuideRelease(root);
        RRGuideRelease(fence); RRGuideRelease(queue); RRGuideRelease(device);
    }
};
#include "rr_reflectance_gpu.h"
static RRGuideJob* rrGuideJob = nullptr;

static long long RRGuideQpc() noexcept {
    LARGE_INTEGER value{};
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}
static double RRGuideElapsedMs(long long start) noexcept {
    LARGE_INTEGER qpcFrequency{};
    const long long end = RRGuideQpc();
    if (!start || end < start || !QueryPerformanceFrequency(&qpcFrequency) || qpcFrequency.QuadPart <= 0) return -1.0;
    return 1000.0 * static_cast<double>(end - start) / static_cast<double>(qpcFrequency.QuadPart);
}

// A stopped job is deliberately retained after any recording/submission
// uncertainty. Neither native nor owned lists may reference freed resources.
static void RRGuideStop(const char* reason, HRESULT hr) noexcept {
    Log("RR_GUIDE_G3_STOP reason=%s hr=0x%08lX allocations_retained=%u",
        reason, static_cast<unsigned long>(hr), unsigned(rrGuideJob != nullptr));
    rrGuideStage = RRGuideStage::Stopped;
    rrGuideInactive.store(true, std::memory_order_release);
}
static void RRGuideArm(bool ready) noexcept {
    AcquireSRWLockExclusive(&rrGuideLock);
    rrGuideInactive.store(!ready, std::memory_order_release);
    Log("RR_GUIDE_G3_READY ready=%u max_captures=1 max_attempts=8 max_pixels=%llu async_preparation=1 rr_evaluate=0 native_denoiser=untouched",
        unsigned(ready), kRRGuideMaxPixels);
    ReleaseSRWLockExclusive(&rrGuideLock);
}
static bool RRGuideCameraValid(const CameraSnapshot& camera) noexcept {
    for (unsigned int i = 0; i < 12; ++i)
        if (!std::isfinite(camera.viewToWorld[i])) return false;
    for (unsigned int i = 0; i < 3; ++i) for (unsigned int j = 0; j < 3; ++j) {
        double dot = 0;
        for (unsigned int k = 0; k < 3; ++k)
            dot += camera.viewToWorld[i * 3 + k] * camera.viewToWorld[j * 3 + k];
        if (std::fabs(dot - (i == j ? 1.0 : 0.0)) > 0.002) return false;
    }
    return true;
}
static D3D12_RESOURCE_BARRIER RRGuideBarrier(ID3D12Resource* p,
        D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) noexcept {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    b.Transition.pResource = p; b.Transition.Subresource = 0;
    b.Transition.StateBefore = before; b.Transition.StateAfter = after;
    return b;
}
static HRESULT RRGuideMakeTexture(RRGuideJob* job, UINT index, DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initial) noexcept {
    D3D12_HEAP_PROPERTIES hp{}; hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    hp.CreationNodeMask = hp.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC d{};
    d.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    d.Width = job->width; d.Height = job->height;
    d.DepthOrArraySize = d.MipLevels = 1; d.SampleDesc.Count = 1;
    d.Format = format; d.Flags = flags;
    HRESULT hr = job->device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE,
        &d, initial, nullptr, __uuidof(ID3D12Resource), reinterpret_cast<void**>(&job->textures[index]));
    if (FAILED(hr)) return hr;
    UINT rows = 0; UINT64 rowBytes = 0;
    job->device->GetCopyableFootprints(&d, 0, 1, 0, &job->footprints[index],
        &rows, &rowBytes, &job->readbackBytes[index]);
    if (rows != job->height || !rowBytes || job->readbackBytes[index] == UINT64_MAX ||
        job->readbackBytes[index] > kRRGuideMaxPixels * 16 + 2097152) return E_INVALIDARG;
    hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC buffer{};
    buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    buffer.Width = job->readbackBytes[index]; buffer.Height = 1;
    buffer.DepthOrArraySize = buffer.MipLevels = 1; buffer.SampleDesc.Count = 1;
    buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return job->device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE,
        &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, __uuidof(ID3D12Resource),
        reinterpret_cast<void**>(&job->readbacks[index]));
}
static HRESULT RRGuideMakePipeline(RRGuideJob* job) noexcept {
    const long long resourceStart = RRGuideQpc();
    HRESULT hr = RRGuideMakeTexture(job, 0, DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    if (SUCCEEDED(hr)) hr = RRGuideMakeTexture(job, 1, DXGI_FORMAT_R8G8B8A8_UNORM,
        D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COPY_DEST);
    if (SUCCEEDED(hr)) hr = RRGuideMakeTexture(job, 2, DXGI_FORMAT_R32G32B32A32_FLOAT,
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Log("RR_GUIDE_G3_PREPARE_TIMING stage=resources ms=%.3f hr=0x%08lX width=%u height=%u thread=%lu",
        RRGuideElapsedMs(resourceStart), static_cast<unsigned long>(hr), job->width, job->height, GetCurrentThreadId());
    if (FAILED(hr)) return hr;
    const long long bindingsStart = RRGuideQpc();
    D3D12_DESCRIPTOR_HEAP_DESC hd{};
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    hd.NumDescriptors = 3; hd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    hr = job->device->CreateDescriptorHeap(&hd, __uuidof(ID3D12DescriptorHeap), reinterpret_cast<void**>(&job->heap));
    if (FAILED(hr)) return hr;
    auto cpu = job->heap->GetCPUDescriptorHandleForHeapStart();
    const UINT stride = job->device->GetDescriptorHandleIncrementSize(hd.Type);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
    srv.Format = DXGI_FORMAT_R8G8B8A8_UNORM; srv.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srv.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srv.Texture2D.MipLevels = 1;
    for (UINT i = 0; i < 2; ++i) {
        job->device->CreateShaderResourceView(job->textures[i], &srv, cpu); cpu.ptr += stride;
    }
    D3D12_UNORDERED_ACCESS_VIEW_DESC uav{};
    uav.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; uav.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    job->device->CreateUnorderedAccessView(job->textures[2], nullptr, &uav, cpu);
    D3D12_DESCRIPTOR_RANGE ranges[2]{};
    ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; ranges[0].NumDescriptors = 2;
    ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; ranges[1].NumDescriptors = 1;
    ranges[1].OffsetInDescriptorsFromTableStart = 2;
    D3D12_ROOT_PARAMETER params[2]{};
    params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    params[0].DescriptorTable.NumDescriptorRanges = 2; params[0].DescriptorTable.pDescriptorRanges = ranges;
    params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    params[1].Constants.Num32BitValues = 16;
    D3D12_ROOT_SIGNATURE_DESC rd{}; rd.NumParameters = 2; rd.pParameters = params;
    HMODULE d3d12 = GetModuleHandleW(L"d3d12.dll");
    auto serialize = d3d12 ? reinterpret_cast<D3D12SerializeRootSignatureDynamicFn>(
        GetProcAddress(d3d12, "D3D12SerializeRootSignature")) : nullptr;
    if (!serialize) return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    ID3DBlob* blob = nullptr; ID3DBlob* errors = nullptr;
    hr = serialize(&rd, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errors);
    if (SUCCEEDED(hr) && blob) hr = job->device->CreateRootSignature(0,
        blob->GetBufferPointer(), blob->GetBufferSize(), __uuidof(ID3D12RootSignature), reinterpret_cast<void**>(&job->root));
    else if (SUCCEEDED(hr)) hr = E_UNEXPECTED;
    RRGuideRelease(errors); RRGuideRelease(blob);
    Log("RR_GUIDE_G3_PREPARE_TIMING stage=descriptors_and_root_signature ms=%.3f hr=0x%08lX",
        RRGuideElapsedMs(bindingsStart), static_cast<unsigned long>(hr));
    if (FAILED(hr)) return hr;
    D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};
    pd.pRootSignature = job->root; pd.CS = {kRRPrimaryGuideShader, sizeof(kRRPrimaryGuideShader)};
    const long long pipelineStart = RRGuideQpc();
    hr = job->device->CreateComputePipelineState(&pd, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&job->pipeline));
    Log("RR_GUIDE_G3_PREPARE_TIMING stage=compute_pipeline ms=%.3f hr=0x%08lX",
        RRGuideElapsedMs(pipelineStart), static_cast<unsigned long>(hr));
    const long long commandsStart = RRGuideQpc();
    if (SUCCEEDED(hr)) hr = job->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        __uuidof(ID3D12CommandAllocator), reinterpret_cast<void**>(&job->allocator));
    if (SUCCEEDED(hr)) hr = job->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        job->allocator, job->pipeline, __uuidof(ID3D12GraphicsCommandList), reinterpret_cast<void**>(&job->list));
    if (SUCCEEDED(hr)) hr = job->device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
        __uuidof(ID3D12Fence), reinterpret_cast<void**>(&job->fence));
    if(SUCCEEDED(hr)) {
        D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;
        D3D12_RESOURCE_DESC bd{};bd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        bd.Width=control_rr_part1::MaxReadbackBytes;bd.Height=1;bd.DepthOrArraySize=1;
        bd.MipLevels=1;bd.SampleDesc.Count=1;bd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        hr=job->device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&bd,
            D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&job->part1Readback));
    }
    Log("RR_GUIDE_G3_PREPARE_TIMING stage=command_objects ms=%.3f hr=0x%08lX",
        RRGuideElapsedMs(commandsStart), static_cast<unsigned long>(hr));
    return hr;
}

// The preparation worker has only owned D3D interfaces and scalar dimensions.
// In particular, input stays zero until a later render-thread observation has
// re-read and validated that frame's borrowed engine resources and camera.
static DWORD WINAPI RRGuidePrepareWorker(void* value) noexcept {
    auto* job = static_cast<RRGuideJob*>(value);
    const long long started = RRGuideQpc();
    const bool retireOnly = job->retireOnly;
    const HRESULT hr = retireOnly ? S_FALSE : RRGuideMakePipeline(job);
    Log("RR_GUIDE_G3_PREPARE_COMPLETE attempt=%u retire_only=%u ms=%.3f hr=0x%08lX width=%u height=%u thread=%lu",
        job->preparationAttempt, unsigned(retireOnly), RRGuideElapsedMs(started),
        static_cast<unsigned long>(hr), job->width, job->height, GetCurrentThreadId());
    AcquireSRWLockExclusive(&rrGuideLock);
    const bool current = rrGuideJob == job && rrGuideStage == RRGuideStage::Preparing;
    if (current && SUCCEEDED(hr) && !retireOnly) {
        rrGuideStage = RRGuideStage::Prepared;
        ReleaseSRWLockExclusive(&rrGuideLock);
        return 0;
    }
    if (rrGuideJob == job) rrGuideJob = nullptr;
    if (current) {
        rrGuideStage = RRGuideStage::Waiting;
        if (rrGuideAttempts >= 8) RRGuideStop("preparation_attempt_limit", hr);
    }
    ReleaseSRWLockExclusive(&rrGuideLock);
    delete job; // Never copied or submitted; all resources are worker-owned.
    return 0;
}
static DWORD WINAPI RRGuideExportWorker(void* value) noexcept {
    auto* job = static_cast<RRGuideJob*>(value);
    void* mapped[3]{}; bool good = true;
    void* part1Mapped=nullptr;
    if(job->part1Copied) {
        if(!control_rr_part1::CanMap(job->part1Copied,job->fence->GetCompletedValue())) good=false;
        else {
            D3D12_RANGE range{0,static_cast<SIZE_T>(job->part1Bytes)};
            good=SUCCEEDED(job->part1Readback->Map(0,&range,&part1Mapped)) && part1Mapped;
        }
        Log("RR_GUIDE_G12_PART1_READBACK frame=%llu bytes=%llu fence_complete=%u map_ok=%u",
            job->input.engineFrame,job->part1Bytes,unsigned(control_rr_part1::CanMap(true,job->fence->GetCompletedValue())),unsigned(good));
    }
    for (UINT i = 0; i < 3; ++i) {
        D3D12_RANGE range{0, static_cast<SIZE_T>(job->readbackBytes[i])};
        const HRESULT hr = job->readbacks[i]->Map(0, &range, &mapped[i]);
        if (FAILED(hr) || !mapped[i]) {
            Log("RR_GUIDE_G3_EXPORT_FAILED stage=map index=%u hr=0x%08lX", i, static_cast<unsigned long>(hr));
            good = false; break;
        }
    }
    RRReflectanceGpu* reflectanceOwner=nullptr;
    wchar_t directory[32768]{};
    if (good) {
        RRGuideExportView view{};
        view.part1Data=part1Mapped;view.part1Bytes=job->part1Copied?static_cast<size_t>(job->part1Bytes):0;
        view.width = job->width; view.height = job->height;
        view.gbuffer1 = static_cast<unsigned char*>(mapped[0]) + job->footprints[0].Offset;
        view.gbuffer2 = static_cast<unsigned char*>(mapped[1]) + job->footprints[1].Offset;
        view.normalRoughness = static_cast<unsigned char*>(mapped[2]) + job->footprints[2].Offset;
        view.gbuffer1RowPitch = job->footprints[0].Footprint.RowPitch;
        view.gbuffer2RowPitch = job->footprints[1].Footprint.RowPitch;
        view.normalRoughnessRowPitch = job->footprints[2].Footprint.RowPitch;
        view.engineFrame = job->input.engineFrame; view.presentToken = job->input.presentToken;
        view.camera = job->input.camera;
        RRAlbedoFillExportView(&view);
        const bool gpuOkay=RRReflectanceGpuRun(job,&view,&reflectanceOwner);
        Log("RR_GUIDE_G12_GPU_RESULT frame=%llu success=%u stage=%s rr_eval=disabled",view.engineFrame,unsigned(gpuOkay),reflectanceOwner?reflectanceOwner->stage:"preflight");
        good = gpuOkay && RRGuideExportCapture(view, directory, _countof(directory));
    }
    RRReflectanceGpuRetire(reflectanceOwner);
    RRAlbedoUnmapExports();
    D3D12_RANGE empty{0, 0};
    if(part1Mapped) job->part1Readback->Unmap(0,&empty);
    for (UINT i = 0; i < 3; ++i) if (mapped[i]) job->readbacks[i]->Unmap(0, &empty);
    Log(good ? "RR_GUIDE_G3_EXPORT_COMPLETE directory=%ls frame=%llu width=%u height=%u rr_evaluate=0" :
        "RR_GUIDE_G3_EXPORT_FAILED directory=%ls frame=%llu width=%u height=%u rr_evaluate=0",
        directory, job->input.engineFrame, job->width, job->height);
    AcquireSRWLockExclusive(&rrGuideLock);
    rrGuideJob = nullptr;
    rrGuideStage = good ? RRGuideStage::Finished : RRGuideStage::Stopped;
    rrGuideInactive.store(true, std::memory_order_release);
    ReleaseSRWLockExclusive(&rrGuideLock);
    delete job; // Fence completion precedes creation of this worker.
    return 0;
}

// The input snapshot borrows game interfaces. Contain a stale-interface fault
// while acquiring owned device/queue references, before recording any copies.
static HRESULT RRGuideAcquireDeviceQueueImpl(RRGuideJob* job, const RRGuideInputSnapshot* input,
        const char** reason) noexcept {
    ID3D12Device* queueDevice = nullptr;
    HRESULT hr = E_FAIL;
    __try {
        *reason = "device_unavailable";
        hr = input->gbuffer1.resource->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&job->device));
        if (FAILED(hr)) __leave;
        if (!job->device) { hr = E_UNEXPECTED; __leave; }
        *reason = "native_direct_queue_mismatch";
        void* native = nullptr;
        const auto result = slGetNativeInterfaceApi ? slGetNativeInterfaceApi(input->queue, &native) : sl::Result::eErrorInvalidParameter;
        job->queue = static_cast<ID3D12CommandQueue*>(native);
        if (result != sl::Result::eOk || !job->queue ||
            job->queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT) { hr = E_NOINTERFACE; __leave; }
        hr = job->queue->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&queueDevice));
        if (SUCCEEDED(hr) && queueDevice != job->device) hr = E_NOINTERFACE;
    } __finally {
        if (queueDevice) queueDevice->Release();
    }
    return hr;
}
static HRESULT RRGuideAcquireDeviceQueue(RRGuideJob* job, const RRGuideInputSnapshot* input,
        const char** reason, DWORD* fault) noexcept {
    __try { return RRGuideAcquireDeviceQueueImpl(job, input, reason); }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *fault = GetExceptionCode(); *reason = "borrowed_interface_exception"; return E_FAIL;
    }
}

// Reflection remains the preparation/fallback opportunity. A completed native
// join also gets explicit same-frame opportunities; those may only consume an
// already prepared job and never queue allocations from the new boundaries.
static void RRGuideTryCaptureAtBoundary(unsigned long long aaCall,
        bool joinedOnly, const char* boundary) noexcept {
    if (rrGuideInactive.load(std::memory_order_acquire) || aaCall < 120) return;
    RRGuideEntry entry;
    if (!entry.locked) return;
    if (joinedOnly && (rrGuideStage != RRGuideStage::Prepared || !RRAlbedoCapturePending())) return;
    if ((rrGuideStage != RRGuideStage::Waiting && rrGuideStage != RRGuideStage::Prepared) ||
        (aaCall < rrGuideLastAttempt + 120 &&
            !(rrGuideStage == RRGuideStage::Prepared && RRAlbedoCapturePending()))) return;
    if (rrGuideAttempts >= 8) { RRGuideStop("attempt_limit", E_FAIL); return; }
    rrGuideLastAttempt = aaCall; const UINT attempt = ++rrGuideAttempts;
    const long long captureStart = RRGuideQpc();
    if (joinedOnly) Log("RR_GUIDE_G5_PAIRED_OPPORTUNITY boundary=%s attempt=%u aa=%llu", boundary, attempt, aaCall);
    RRGuideInputSnapshot input{};
    const char* reason = "inputs_unavailable";
    HRESULT hr = S_FALSE;
    DWORD preparationFault = 0;
    bool ready = RRGuideReadInputs(&input, &reason);
    preparationFault = input.fault;
    if (ready) {
        const auto& d = input.gbuffer1.desc;
        ready = d.Width && d.Height && d.Width <= 8192 && d.Height <= 8192 &&
            RRGuideCameraValid(input.camera);
        reason = "dimensions_or_camera_invalid";
        const auto highResolution=ready?control_rr::HighResolutionExtent(d.Width,d.Height):0;
        if(highResolution&&rrGuideStage==RRGuideStage::Waiting){
            // No diagnostic owner/commands exist yet. Let recurring native
            // replay start independently and retain all its shader/view checks.
            rrGuideHighResolutionExtent.store(highResolution,std::memory_order_release);
            rrGuideStage=RRGuideStage::Finished;rrGuideInactive.store(true,std::memory_order_release);
            Log("RR_GUIDE_DIAGNOSTIC_SKIPPED reason=diagnostic_size_limit width=%llu height=%u recurring_replay=independent",d.Width,d.Height);
            return;
        }
        ready=ready&&d.Width*d.Height<=kRRGuideMaxPixels;
    }
    // Acquiring references is the only device work on this path. No resource,
    // root-signature or pipeline creation happens on the game's render thread.
    RRGuideJob current;
    if (ready) {
        hr = RRGuideAcquireDeviceQueue(&current, &input, &reason, &preparationFault);
        ready = SUCCEEDED(hr);
    }
    if (!ready) {
        Log("RR_GUIDE_G3_CAPTURE_SKIP attempt=%u aa=%llu reason=%s hr=0x%08lX exception=0x%08lX ms=%.3f",
            attempt, aaCall, reason, static_cast<unsigned long>(hr), preparationFault, RRGuideElapsedMs(captureStart));
        if (attempt >= 8) RRGuideStop("attempt_limit", hr);
        return;
    }
    if (rrGuideStage == RRGuideStage::Waiting) {
        auto* prepared = new (std::nothrow) RRGuideJob;
        if (!prepared) {
            Log("RR_GUIDE_G3_CAPTURE_SKIP attempt=%u aa=%llu reason=allocation_failed", attempt, aaCall);
            if (attempt >= 8) RRGuideStop("attempt_limit", E_OUTOFMEMORY);
            return;
        }
        prepared->width = static_cast<UINT>(input.gbuffer1.desc.Width);
        prepared->height = input.gbuffer1.desc.Height;
        prepared->preparationAttempt = attempt;
        prepared->device = current.device; current.device = nullptr;
        prepared->queue = current.queue; current.queue = nullptr;
        rrGuideJob = prepared; rrGuideStage = RRGuideStage::Preparing;
        HANDLE worker = CreateThread(nullptr, 0, &RRGuidePrepareWorker, prepared, 0, nullptr);
        if (worker) {
            CloseHandle(worker);
            Log("RR_GUIDE_G3_PREPARE_QUEUED attempt=%u aa=%llu width=%u height=%u render_ms=%.3f borrowed_inputs_retained=0",
                attempt, aaCall, prepared->width, prepared->height, RRGuideElapsedMs(captureStart));
        } else {
            const DWORD error = GetLastError();
            rrGuideJob = nullptr; rrGuideStage = RRGuideStage::Waiting;
            delete prepared; // Contains two owned references only; worker never ran.
            Log("RR_GUIDE_G3_CAPTURE_SKIP attempt=%u aa=%llu reason=prepare_worker_unavailable error=%lu", attempt, aaCall, error);
            if (attempt >= 8) RRGuideStop("attempt_limit", HRESULT_FROM_WIN32(error));
        }
        return;
    }
    auto* job = rrGuideJob;
    if (!job) { RRGuideStop("prepared_job_missing", E_UNEXPECTED); return; }
    if (job->width != input.gbuffer1.desc.Width || job->height != input.gbuffer1.desc.Height ||
        job->device != current.device || job->queue != current.queue) {
        // A resize or device/queue replacement invalidates this unused job.
        // Retire its allocations on a worker; obtain fresh inputs next attempt.
        job->retireOnly = true; rrGuideStage = RRGuideStage::Preparing;
        HANDLE worker = CreateThread(nullptr, 0, &RRGuidePrepareWorker, job, 0, nullptr);
        if (worker) {
            CloseHandle(worker);
            Log("RR_GUIDE_G3_PREPARE_DISCARD attempt=%u aa=%llu reason=dimensions_device_or_queue_changed", attempt, aaCall);
        } else {
            RRGuideStop("retirement_worker_unavailable", HRESULT_FROM_WIN32(GetLastError()));
        }
        return;
    }
    // This is the first time a prepared job receives any borrowed engine state.
    // CopyInputs rechecks identity, frame, dimensions and states under its locks.
    if (!RRAlbedoAllowGuideCapture(input.engineFrame)) {
        --rrGuideAttempts; // Waiting for the paired guide is not a failed capture.
        return;
    }
    job->input = input;
    bool commandsStarted = false; DWORD recordingFault = 0;
    const long long copyStart = RRGuideQpc();
    ready = RRGuideCopyInputs(&job->input, job->textures[0], job->textures[1],
        &reason, &commandsStarted, &recordingFault);
    if (ready) ready = RRAlbedoCopyForGuide(job, &reason);
    Log("RR_GUIDE_G3_CAPTURE_TIMING attempt=%u aa=%llu copy_ms=%.3f render_ms=%.3f commands_started=%u",
        attempt, aaCall, RRGuideElapsedMs(copyStart), RRGuideElapsedMs(captureStart), unsigned(commandsStarted));
    if (!ready) {
        if (commandsStarted) {
            rrGuideJob = job;
            Log("RR_GUIDE_G3_CAPTURE_SKIP attempt=%u aa=%llu reason=%s exception=0x%08lX commands_started=1",
                attempt, aaCall, reason, recordingFault);
            RRGuideStop("source_recording_fault", E_FAIL);
        } else {
            job->input = {}; // The prepared resources remain unused and reusable.
            Log("RR_GUIDE_G3_CAPTURE_SKIP attempt=%u aa=%llu reason=%s hr=0x%08lX exception=0x%08lX",
                attempt, aaCall, reason, static_cast<unsigned long>(hr), preparationFault ? preparationFault : recordingFault);
            if (attempt >= 8) RRGuideStop("attempt_limit", hr);
        }
        return;
    }
    rrGuideStage = RRGuideStage::Copied;
    Log("RR_GUIDE_G3_INPUTS_COPIED aa=%llu frame=%llu present=%llu width=%u height=%u source_states_restored=1 shader_sha256=%s",
        aaCall, job->input.engineFrame, job->input.presentToken, job->width, job->height, kRRPrimaryGuideShaderSha256);
}

static void RRGuideTryCapture(unsigned long long aaCall, const char* raygen) noexcept {
    if (rrGuideInactive.load(std::memory_order_acquire) ||
        !raygen || strcmp(raygen, "reflectionRayGeneration")) return;
    RRGuideTryCaptureAtBoundary(aaCall, false, "post_reflection");
}
static void RRGuideTryCaptureJoined(unsigned long long aaCall, const char* boundary) noexcept {
    if (rrGuideInactive.load(std::memory_order_acquire) || !RRAlbedoCapturePending()) return;
    RRGuideTryCaptureAtBoundary(aaCall, true, boundary);
}

static HRESULT RRGuideSubmit(RRGuideJob* job) noexcept {
    auto* list = job->list;
    // The compiled G1 shader consumes only t0. GBuffer2 is retained solely for
    // raw export and material-ID preview; if t1 becomes live, revise its states.
    D3D12_RESOURCE_BARRIER before[] = {
        RRGuideBarrier(job->textures[0], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE),
        RRGuideBarrier(job->textures[1], D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE)};
    list->ResourceBarrier(_countof(before), before);
    ID3D12DescriptorHeap* heaps[] = {job->heap};
    list->SetDescriptorHeaps(1, heaps); list->SetComputeRootSignature(job->root);
    list->SetComputeRootDescriptorTable(0, job->heap->GetGPUDescriptorHandleForHeapStart());
    struct Constants { float basis[3][4]; UINT size[2]; UINT reserved[2]; } constants{};
    static_assert(sizeof(Constants) == 64, "Guide shader constant ABI");
    for (UINT i = 0; i < 3; ++i) for (UINT j = 0; j < 3; ++j)
        constants.basis[i][j] = static_cast<float>(job->input.camera.viewToWorld[i * 3 + j]);
    constants.size[0] = job->width; constants.size[1] = job->height;
    list->SetComputeRoot32BitConstants(1, 16, &constants, 0);
    list->Dispatch((job->width + 7) / 8, (job->height + 7) / 8, 1);
    D3D12_RESOURCE_BARRIER after[] = {
        RRGuideBarrier(job->textures[0], D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE),
        RRGuideBarrier(job->textures[2], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE)};
    list->ResourceBarrier(_countof(after), after);
    for (UINT i = 0; i < 3; ++i) {
        D3D12_TEXTURE_COPY_LOCATION src{}, dst{};
        src.pResource = job->textures[i]; src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst.pResource = job->readbacks[i]; dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dst.PlacedFootprint = job->footprints[i];
        list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }
    const HRESULT hr = list->Close();
    if (FAILED(hr)) return hr;
    ID3D12CommandList* lists[] = {list}; job->queue->ExecuteCommandLists(1, lists);
    return job->queue->Signal(job->fence, 1);
}
// A POD-only SEH leaf keeps C++ unwinding and __try in different functions.
static HRESULT RRGuideSubmitSafe(RRGuideJob* job, DWORD* fault) noexcept {
    __try { return RRGuideSubmit(job); }
    __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return E_FAIL; }
}
static void RRGuideAfterPresent(unsigned long long present) noexcept {
    if (rrGuideInactive.load(std::memory_order_acquire)) return;
    RRGuideEntry entry;
    if (!entry.locked || !rrGuideJob) return;
    auto* job = rrGuideJob;
    if (rrGuideStage == RRGuideStage::Copied) {
        if (GetCurrentThreadId() != job->input.captureThreadId ||
            present != job->input.presentToken + 1 || GetHdr10BridgeDirectQueue() != job->input.queue) {
            RRGuideStop("present_submission_boundary_mismatch", E_UNEXPECTED); return;
        }
        DWORD fault = 0; const HRESULT hr = RRGuideSubmitSafe(job, &fault);
        if (FAILED(hr)) {
            Log("RR_GUIDE_G3_SUBMIT_FAULT exception=0x%08lX", fault);
            RRGuideStop("submit_or_signal_failed", hr); return;
        }
        job->submittedPresent = present; rrGuideStage = RRGuideStage::Pending;
        Log("RR_GUIDE_G3_DISPATCH_SUBMITTED present=%llu frame=%llu width=%u height=%u engine_descriptor_state_changed=0",
            present, job->input.engineFrame, job->width, job->height);
        return;
    }
    if (rrGuideStage != RRGuideStage::Pending) return;
    const UINT64 completed = job->fence->GetCompletedValue();
    if (completed == UINT64_MAX) { RRGuideStop("device_removed", DXGI_ERROR_DEVICE_REMOVED); return; }
    if (completed < 1) {
        if (!job->slowFenceLogged && present > job->submittedPresent + 600) {
            job->slowFenceLogged = true;
            Log("RR_GUIDE_G3_FENCE_PENDING present=%llu submitted=%llu cpu_wait=0", present, job->submittedPresent);
        }
        return;
    }
    rrGuideStage = RRGuideStage::Writing;
    HANDLE worker = CreateThread(nullptr, 0, &RRGuideExportWorker, job, 0, nullptr);
    if (worker) CloseHandle(worker);
    else {
        const DWORD error = GetLastError();
        delete job; rrGuideJob = nullptr; // The fence has completed.
        Log("RR_GUIDE_G3_EXPORT_FAILED stage=create_worker error=%lu", error);
        RRGuideStop("export_worker_unavailable", HRESULT_FROM_WIN32(error));
    }
}
