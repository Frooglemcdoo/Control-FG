#pragma once
// Bounded, diagnostic replay of Control's native primary albedo_only technique.
// Included after rr_guide_render.h; native engine allocations stay on the
// primary thread. Borrowed draw data is used only before the matching join.

enum class RRAlbedoStage { Disabled, Waiting, Preparing, Recording, Joined, Copied, Finished, Failed };
static std::atomic<RRAlbedoStage> rrAlbedoStage{RRAlbedoStage::Disabled};
static RRAlbedoNativeAPI rrAlbedoAPI{};
using RRAlbedoCallback = void (*)(void*, void*);
using RRAlbedoLookup = void* (*)(void*, unsigned int);
static RRAlbedoCallback rrAlbedoOriginalPrepare = nullptr, rrAlbedoOriginalJoin = nullptr;
static control_rr_albedo::DrawOpaque rrAlbedoOriginalDraw = nullptr;
static RRAlbedoLookup rrAlbedoLookupShader = nullptr;
static thread_local void* rrAlbedoSerialJoinView = nullptr;

struct RRAlbedoCapture {
    control_rr_albedo::PreparedReplay replay;
    RRAlbedoShader::Session shaders;
    bool usedShaders[128]{};
    RRAlbedoNativeTarget targets[2]{};
    ID3D12Resource* readbacks[2]{};
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprints[2]{};
    UINT64 readbackBytes[2]{};
    void* mapped[2]{};
    RRGuideExportImageView exportImages[2]{};
    std::vector<RRGuideExportShaderView> exportShaders;
    std::uintptr_t renderer = 0, manager = 0, resourceTable = 0;
    void* primaryView = nullptr; // primary/worker/join rend::View, identity only
    void* cameraView = nullptr; // persistent camera used for the paired guide copy
    ID3D12Device* preparedDevice = nullptr; // identity only, retained by the normal job
    ID3D12CommandQueue* preparedQueue = nullptr;
    unsigned long long frame = 0;
    unsigned int width = 0, height = 0, targetCount = 0;
    std::atomic<unsigned int> ranges{0}, replayedBatches{0}, rejectedRanges{0};
    std::atomic<bool> drawFault{false};
    std::atomic<long long> maximumReplayTicks{0};
};
// One bounded owner is intentionally kept until process exit, including on
// ambiguous native faults. No worker or DLL detach calls engine destructors.
static RRAlbedoCapture* rrAlbedoCapture = nullptr;
static unsigned long long rrAlbedoWaitStart = 0; // guide-lock protected
struct RRAlbedoLastError {
    DWORD value = GetLastError();
    ~RRAlbedoLastError() { SetLastError(value); }
};

static bool RRAlbedoRead(std::uintptr_t address, void* out, size_t bytes) noexcept {
    if (!address || !out || bytes > 16 * 1024 * 1024 || address > UINTPTR_MAX - bytes) return false;
    __try { memcpy(out, reinterpret_cast<const void*>(address), bytes); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static bool RRAlbedoGetShader(std::uintptr_t technique, unsigned int key, std::uintptr_t* out) noexcept {
    if (!technique || !out || !rrAlbedoLookupShader) return false;
    __try {
        *out = reinterpret_cast<std::uintptr_t>(rrAlbedoLookupShader(reinterpret_cast<void*>(technique), key));
        return *out != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static bool RRAlbedoReadCallback(void* callback, std::uintptr_t* renderer, std::uintptr_t* manager) noexcept {
    if (!callback || !verifiedRenderer || !renderer || !manager) return false;
    __try {
        auto base = reinterpret_cast<const unsigned char*>(verifiedRenderer);
        if (base[0x802A33] || base[0x802A6A]) return false; // both native wireframe options
        *renderer = *reinterpret_cast<const std::uintptr_t*>(static_cast<unsigned char*>(callback) + 8);
        if (!*renderer) return false;
        *manager = *reinterpret_cast<const std::uintptr_t*>(*renderer + 0x80);
        return *manager != 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
static bool RRAlbedoDrawSafe(void* manager, DWORD* fault) noexcept {
    *fault = 0;
    __try { rrAlbedoOriginalDraw(manager, 0, -1); return true; }
    __except (EXCEPTION_EXECUTE_HANDLER) { *fault = GetExceptionCode(); return false; }
}
static void RRAlbedoFail(const char* reason, DWORD fault = 0) noexcept {
    rrAlbedoStage.store(RRAlbedoStage::Failed, std::memory_order_release);
    Log("RR_GUIDE_G3_ALBEDO_STOP reason=%s exception=0x%08lX allocations_retained=%u rr_eval=disabled",
        reason, fault, unsigned(rrAlbedoCapture != nullptr));
}
static HRESULT RRAlbedoMakeReadbacks(RRAlbedoCapture* capture, ID3D12Device* device) noexcept {
    D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK;
    heap.CreationNodeMask = 1; heap.VisibleNodeMask = 1;
    for (unsigned int i = 0; i < capture->targetCount; ++i) {
        const auto desc = capture->targets[i].resource->GetDesc();
        UINT rows = 0; UINT64 rowBytes = 0;
        device->GetCopyableFootprints(&desc, 0, 1, 0, &capture->footprints[i], &rows, &rowBytes, &capture->readbackBytes[i]);
        if (rows != capture->height || rowBytes != UINT64(capture->width) * 8 ||
            capture->readbackBytes[i] > kRRGuideMaxPixels * 8 + 2097152) return E_INVALIDARG;
        D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        buffer.Width = capture->readbackBytes[i]; buffer.Height = 1;
        buffer.DepthOrArraySize = 1; buffer.MipLevels = 1; buffer.SampleDesc.Count = 1;
        buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        const HRESULT hr = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&capture->readbacks[i]));
        if (FAILED(hr)) return hr;
    }
    return S_OK;
}

static void RRAlbedoPrepareCapture(void* callback, void* table) noexcept {
    if (rrAlbedoStage.load(std::memory_order_acquire) != RRAlbedoStage::Waiting || rrGuideInactive.load()) return;
    RRGuideEntry entry;
    if (!entry.locked || rrGuideStage != RRGuideStage::Prepared || !rrGuideJob) return;
    const long long start = RRGuideQpc();
    unsigned long long frame = 0; DWORD fault = 0;
    control_rr_albedo::PreparationState preparation{};
    const char* preparationReason = "preparation_state_unavailable";
    if (!ReadEngineFrameSafe(&frame, &fault) || !frame ||
        !control_rr_albedo::ReadPreparationState(reinterpret_cast<std::uintptr_t>(verifiedRenderer),
            callback, table, &preparation, &preparationReason, &fault)) return;
    const auto renderer = preparation.renderer, manager = preparation.manager;
    CameraSnapshot camera{}; const char* cameraReason = "preparation_camera_unavailable";
    if (!ReadCamera(&camera, &fault, &cameraReason) || camera.engineFrame != frame ||
        reinterpret_cast<std::uintptr_t>(camera.renderer) != renderer) return;
    RRAlbedoStage expected = RRAlbedoStage::Waiting;
    if (!rrAlbedoStage.compare_exchange_strong(expected, RRAlbedoStage::Preparing)) return;
    try {
        auto* capture = new (std::nothrow) RRAlbedoCapture;
        if (!capture) { RRAlbedoFail("owner_allocation_failed"); return; }
        rrAlbedoCapture = capture;
        capture->renderer = renderer; capture->manager = manager; capture->frame = frame;
        capture->resourceTable = preparation.resourceTable;
        capture->primaryView = preparation.primaryView;
        capture->cameraView = camera.view;
        capture->width = rrGuideJob->width; capture->height = rrGuideJob->height;
        const control_rr_albedo::Access access{&RRAlbedoRead, &RRAlbedoGetShader};
        if (!control_rr_albedo::prepare(access, manager, capture->replay, true)) {
            RRAlbedoFail("no_supported_primary_opaque_batches"); return;
        }
        std::uintptr_t source = 0;
        memcpy(&source, capture->replay.manager.data() + 0x30, sizeof(source));
        size_t kept = 0;
        for (size_t i = 0; i < capture->replay.batches.size(); ++i) {
            control_rr_albedo::OpaqueBatch original{};
            const auto index = capture->replay.sourceIndices[i];
            const char* reason = "shader_contract_unavailable";
            RRAlbedoShader::Result result{};
            if (!RRAlbedoRead(source + size_t(index) * sizeof(original), &original, sizeof(original)) ||
                !capture->shaders.ValidatePair(original.shader, capture->replay.batches[i].shader, result, &reason)) {
                if (i < 8) Log("RR_GUIDE_G3_SHADER_REJECT index=%u reason=%s", index, reason);
                continue;
            }
            if (capture->targetCount && capture->targetCount != result.targetCount) continue;
            if (result.selectedIndex >= 128) continue;
            capture->targetCount = result.targetCount;
            capture->usedShaders[result.selectedIndex] = true;
            capture->replay.batches[kept] = capture->replay.batches[i];
            capture->replay.sourceIndices[kept++] = index;
        }
        capture->replay.batches.resize(kept); capture->replay.sourceIndices.resize(kept);
        capture->replay.coverage.replayBatches = static_cast<unsigned int>(kept);
        capture->replay.coverage.rejectedBatches = capture->replay.coverage.sourceBatches - static_cast<unsigned int>(kept);
        capture->replay.coverage.complete = capture->replay.coverage.rejectedBatches == 0;
        Log("RR_GUIDE_G3_SHADER_CONTRACT frame=%llu source_batches=%u accepted_batches=%u rejected_batches=%u targets=%u shader_count=%zu cpu_ms=%.3f semantics_verified=0",
            frame, capture->replay.coverage.sourceBatches, unsigned(kept), capture->replay.coverage.rejectedBatches,
            capture->targetCount, capture->shaders.PixelShaders().size(), RRGuideElapsedMs(start));
        if (!kept || capture->targetCount < 1 || capture->targetCount > 2) { RRAlbedoFail("no_validated_albedo_shaders"); return; }
        const long long allocationStart = RRGuideQpc();
        for (unsigned int i = 0; i < capture->targetCount; ++i) {
            const char* reason = "native_target_unavailable";
            if (!RRAlbedoNativeCreate(&rrAlbedoAPI, capture->width, capture->height, &capture->targets[i], &reason, &fault)) {
                RRAlbedoFail(reason, fault); return;
            }
            ID3D12Device* device = nullptr;
            const HRESULT deviceResult = capture->targets[i].resource->GetDevice(IID_PPV_ARGS(&device));
            const bool sameDevice = SUCCEEDED(deviceResult) && device == rrGuideJob->device;
            RRGuideRelease(device);
            if (!sameDevice) { RRAlbedoFail("native_target_preparation_device_changed"); return; }
        }
        const HRESULT hr = RRAlbedoMakeReadbacks(capture, rrGuideJob->device);
        if (FAILED(hr)) { RRAlbedoFail("albedo_readback_allocation_failed", static_cast<DWORD>(hr)); return; }
        RRGuideInputSnapshot clearContext{};
        const char* clearReason = "preclear_context_unavailable";
        if (!RRGuideReadRendererContext(&clearContext, &clearReason) || clearContext.engineFrame != frame) {
            RRAlbedoFail("preclear_frame_or_context_changed"); return;
        }
        clearContext.gbuffer1.resource = capture->targets[0].resource;
        RRGuideJob clearDevice;
        const HRESULT clearIdentity = RRGuideAcquireDeviceQueue(&clearDevice, &clearContext, &clearReason, &fault);
        if (FAILED(clearIdentity) || clearDevice.device != rrGuideJob->device || clearDevice.queue != rrGuideJob->queue) {
            RRAlbedoFail("preclear_device_or_queue_changed", fault); return;
        }
        capture->preparedDevice = clearDevice.device; capture->preparedQueue = clearDevice.queue;
        for (unsigned int i = 0; i < capture->targetCount; ++i) {
            if (!RRAlbedoNativePrepareClear(&rrAlbedoAPI, &capture->targets[i], &fault)) {
                RRAlbedoFail("native_target_clear_failed", fault); return;
            }
        }
        rrAlbedoStage.store(RRAlbedoStage::Recording, std::memory_order_release);
        Log("RR_GUIDE_G3_ALBEDO_PREPARED frame=%llu width=%u height=%u targets=%u allocation_clear_ms=%.3f total_ms=%.3f capture_frames=1 primary_view=%p camera_view=%p",
            frame, capture->width, capture->height, capture->targetCount,
            RRGuideElapsedMs(allocationStart), RRGuideElapsedMs(start), capture->primaryView, capture->cameraView);
    } catch (...) { RRAlbedoFail("albedo_preparation_exception"); }
}

static void RRAlbedoReplayRange(void* manager, int first, int end) noexcept {
    if (rrAlbedoStage.load(std::memory_order_acquire) != RRAlbedoStage::Recording) return;
    RRAlbedoLastError preserveError;
    auto* capture = rrAlbedoCapture;
    if (!capture || capture->manager != reinterpret_cast<std::uintptr_t>(manager) || capture->drawFault.load()) return;
    unsigned long long frame = 0; DWORD fault = 0;
    if (!ReadEngineFrameSafe(&frame, &fault) || frame != capture->frame) return;
    control_rr_albedo::DrawRange range{};
    if (!control_rr_albedo::prepareDrawRange(capture->replay, first, end, range)) return;
    RRAlbedoNativeBindings saved{}; const char* reason = "bindings_unavailable";
    if (!RRAlbedoNativeReadBindings(&rrAlbedoAPI, &saved, &reason, &fault)) {
        const auto n = ++capture->rejectedRanges;
        if (n <= 8) Log("RR_GUIDE_G3_ALBEDO_RANGE_SKIP first=%d end=%d reason=%s exception=0x%08lX", first, end, reason, fault);
        return;
    }
    void* activeView = rrAlbedoSerialJoinView;
    const bool viewReadable = !saved.workerContext || control_rr_albedo::ReadWorkerView(
        reinterpret_cast<std::uintptr_t>(verifiedRenderer), &activeView, &fault);
    if (!viewReadable || !activeView || activeView != capture->primaryView) {
        const auto n = ++capture->rejectedRanges;
        if (n <= 8) Log("RR_GUIDE_G3_ALBEDO_RANGE_SKIP first=%d end=%d reason=primary_view_mismatch worker=%u exception=0x%08lX readable=%u actual_view=%p expected_primary_view=%p camera_view=%p",
            first, end, unsigned(saved.workerContext), fault, unsigned(viewReadable),
            activeView, capture->primaryView, capture->cameraView);
        return;
    }
    RRAlbedoNativeTarget* targets[2]{&capture->targets[0], &capture->targets[1]};
    bool changed = false;
    const long long replayStart = RRGuideQpc();
    const bool bound = RRAlbedoNativeBind(&rrAlbedoAPI, &saved, targets, capture->targetCount, &changed, &fault);
    const bool drawn = bound && RRAlbedoDrawSafe(range.managerView(), &fault);
    DWORD restoreFault = 0;
    const bool restored = !changed || RRAlbedoNativeRestore(&rrAlbedoAPI, &saved, &restoreFault);
    const long long replayTicks = RRGuideQpc() - replayStart;
    auto maximumTicks = capture->maximumReplayTicks.load();
    while (replayTicks > maximumTicks &&
        !capture->maximumReplayTicks.compare_exchange_weak(maximumTicks, replayTicks)) {}
    if (!drawn || !restored) {
        capture->drawFault.store(true);
        Log("RR_GUIDE_G3_ALBEDO_DRAW_FAULT first=%d end=%d bound=%u drawn=%u restored=%u exception=0x%08lX restore_exception=0x%08lX",
            first, end, unsigned(bound), unsigned(drawn), unsigned(restored), fault, restoreFault);
        return;
    }
    ++capture->ranges;
    capture->replayedBatches.fetch_add(range.coverage.replayBatches);
}
static void RRAlbedoHookPrepare(void* callback, void* table) {
    RRAlbedoPrepareCapture(callback, table);
    rrAlbedoOriginalPrepare(callback, table);
}
static void RRAlbedoHookDraw(void* manager, int first, int end) {
    RRAlbedoReplayRange(manager, first, end);
    rrAlbedoOriginalDraw(manager, first, end); // original draw re-establishes native draw bindings
}
static void RRAlbedoHookJoin(void* callback, void* table) {
    if (rrAlbedoStage.load(std::memory_order_acquire) != RRAlbedoStage::Recording) {
        rrAlbedoOriginalJoin(callback, table); return;
    }
    struct SerialViewScope {
        void* previous = rrAlbedoSerialJoinView;
        ~SerialViewScope() { rrAlbedoSerialJoinView = previous; }
    } serialScope;
    void* joinView = nullptr; DWORD viewFault = 0;
    control_rr_albedo::ReadJoinView(callback, &joinView, &viewFault);
    rrAlbedoSerialJoinView = joinView;
    rrAlbedoOriginalJoin(callback, table);
    RRAlbedoLastError preserveError;
    if (rrAlbedoStage.load(std::memory_order_acquire) != RRAlbedoStage::Recording) return;
    auto* capture = rrAlbedoCapture;
    std::uintptr_t renderer = 0, manager = 0, resourceTable = 0;
    if (!capture || !RRAlbedoReadCallback(callback, &renderer, &manager) ||
        renderer != capture->renderer || manager != capture->manager ||
        !joinView || joinView != capture->primaryView) return;
    // Separate render-graph callbacks can have separate resource-table objects.
    // The audited owner/manager/frame/View identities pair this completion.
    RRAlbedoRead(reinterpret_cast<std::uintptr_t>(table), &resourceTable, sizeof(resourceTable));
    unsigned long long frame = 0; DWORD fault = 0;
    if (!ReadEngineFrameSafe(&frame, &fault) || frame != capture->frame || capture->drawFault.load()) {
        RRAlbedoFail("primary_join_frame_or_draw_fault", fault); return;
    }
    if (!capture->ranges.load() || !capture->replayedBatches.load()) {
        RRAlbedoFail("no_native_albedo_draws_completed"); return;
    }
    rrAlbedoStage.store(RRAlbedoStage::Joined, std::memory_order_release);
    const double maximumReplayMs = frequency.QuadPart > 0 ?
        1000.0 * double(capture->maximumReplayTicks.load()) / double(frequency.QuadPart) : -1.0;
    Log("RR_GUIDE_G3_ALBEDO_JOINED frame=%llu ranges=%u replayed_batches=%u skipped_ranges=%u max_replay_cpu_ms=%.3f resource_table_same=%u scene_coverage_validated=0",
        frame, capture->ranges.load(), capture->replayedBatches.load(), capture->rejectedRanges.load(), maximumReplayMs,
        unsigned(resourceTable == capture->resourceTable));
    // OriginalJoin has returned and all capture identities/counts passed. Try
    // the existing guarded copy now; an SRV/context preflight rejection leaves
    // the later reflection or post-AA opportunity available in this frame.
    RRGuideTryCaptureJoined(aaCount.load(), "primary_join");
}
static bool RRAlbedoCapturePending() noexcept {
    return rrAlbedoStage.load(std::memory_order_acquire) == RRAlbedoStage::Joined;
}
static bool RRAlbedoAllowGuideCapture(unsigned long long frame) noexcept {
    const auto stage = rrAlbedoStage.load(std::memory_order_acquire);
    if (stage == RRAlbedoStage::Disabled || stage == RRAlbedoStage::Failed) return true;
    if (stage == RRAlbedoStage::Joined) {
        if (rrAlbedoCapture && rrAlbedoCapture->frame == frame) return true;
        RRAlbedoFail("joined_capture_frame_expired"); return true;
    }
    if (!rrAlbedoWaitStart) rrAlbedoWaitStart = frame;
    if (frame >= rrAlbedoWaitStart && frame - rrAlbedoWaitStart >= 120) {
        RRAlbedoFail("primary_capture_callback_timeout"); return true;
    }
    return false;
}
static bool RRAlbedoCopyForGuide(RRGuideJob* job, const char** reason) noexcept {
    if (rrAlbedoStage.load(std::memory_order_acquire) != RRAlbedoStage::Joined) return true;
    auto* capture = rrAlbedoCapture;
    if (!capture || capture->frame != job->input.engineFrame || capture->width != job->width || capture->height != job->height ||
        capture->preparedDevice != job->device || capture->preparedQueue != job->queue ||
        capture->cameraView != job->input.camera.view) {
        *reason = "albedo_copy_frame_or_extent_changed"; RRAlbedoFail(*reason); return false;
    }
    RRAlbedoNativeTarget* targets[2]{&capture->targets[0], &capture->targets[1]};
    bool commands = false; DWORD fault = 0;
    if (!RRAlbedoNativeCopyAfterJoined(&rrAlbedoAPI, targets, capture->targetCount, capture->readbacks,
        capture->footprints, &job->input, reason, &commands, &fault)) {
        RRAlbedoFail(*reason, fault); return false;
    }
    rrAlbedoStage.store(RRAlbedoStage::Copied, std::memory_order_release);
    Log("RR_GUIDE_G3_ALBEDO_COPIED frame=%llu targets=%u paired_source_states_restored=1", capture->frame, capture->targetCount);
    return true;
}
static void RRAlbedoFillExportView(RRGuideExportView* view) noexcept {
    if (!view) return;
    const auto stage = rrAlbedoStage.load(std::memory_order_acquire);
    if (stage != RRAlbedoStage::Copied && stage != RRAlbedoStage::Failed) return;
    auto* capture = rrAlbedoCapture;
    if (!capture) return;
    try {
        bool imagesReady = stage == RRAlbedoStage::Copied && capture->frame == view->engineFrame;
        for (unsigned int i = 0; imagesReady && i < capture->targetCount; ++i) {
            D3D12_RANGE range{0, static_cast<SIZE_T>(capture->readbackBytes[i])};
            if (FAILED(capture->readbacks[i]->Map(0, &range, &capture->mapped[i])) || !capture->mapped[i]) {
                Log("RR_GUIDE_G3_ALBEDO_EXPORT_SKIP reason=readback_map_failed target=%u", i);
                imagesReady = false; break;
            }
            auto& image = capture->exportImages[i];
            image.semantic = i ? RRGuideImageSemantic::NativeAlbedoTarget1 : RRGuideImageSemantic::NativeAlbedoTarget0;
            image.format = RRGuideImageFormat::Rgba16Float;
            image.width = capture->width; image.height = capture->height;
            image.data = static_cast<const unsigned char*>(capture->mapped[i]) + capture->footprints[i].Offset;
            image.rowPitch = capture->footprints[i].Footprint.RowPitch; image.sourceFrame = capture->frame;
        }
        capture->exportShaders.clear();
        capture->exportShaders.reserve(capture->shaders.PixelShaders().size());
        for (size_t i = 0; i < capture->shaders.PixelShaders().size(); ++i) {
            const auto& shader = capture->shaders.PixelShaders()[i];
            // Retain rejected contracts as offline evidence, but never pass a
            // malformed container to the completed export manifest.
            uint32_t declaredBytes = 0;
            if (shader.bytes.size() < 32 || memcmp(shader.bytes.data(), "DXBC", 4)) continue;
            memcpy(&declaredBytes, shader.bytes.data() + 24, 4);
            if (declaredBytes != shader.bytes.size()) continue;
            RRGuideExportShaderView entry{};
            entry.data = shader.bytes.data(); entry.bytes = shader.bytes.size();
            entry.sourceFrame = capture->frame; entry.renderTargetCount = shader.contract.targetCount;
            capture->exportShaders.push_back(entry);
        }
        if (imagesReady) {
            view->additionalImages = capture->exportImages; view->additionalImageCount = capture->targetCount;
            view->albedoSourceBatches = capture->replay.coverage.sourceBatches;
            view->albedoAcceptedBatches = capture->replay.coverage.replayBatches;
            view->albedoReplayedBatches = capture->replayedBatches.load();
            view->albedoDrawRanges = capture->ranges.load();
            view->albedoSkippedRanges = capture->rejectedRanges.load();
        }
        view->shaders = capture->exportShaders.data(); view->shaderCount = capture->exportShaders.size();
    } catch (...) { Log("RR_GUIDE_G3_ALBEDO_EXPORT_SKIP reason=descriptor_allocation_failed"); }
}
static void RRAlbedoUnmapExports() noexcept {
    auto* capture = rrAlbedoCapture;
    if (!capture) return;
    for (unsigned int i = 0; i < 2; ++i) {
        if (capture->mapped[i]) {
            D3D12_RANGE none{0, 0}; capture->readbacks[i]->Unmap(0, &none); capture->mapped[i] = nullptr;
        }
    }
    if (rrAlbedoStage.load() == RRAlbedoStage::Copied) rrAlbedoStage.store(RRAlbedoStage::Finished);
}
