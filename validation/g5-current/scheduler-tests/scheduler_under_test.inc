static void RRGuideStop(const char* reason, HRESULT hr) noexcept {
    Log("RR_GUIDE_G3_STOP reason=%s hr=0x%08lX allocations_retained=%u",
        reason, static_cast<unsigned long>(hr), unsigned(rrGuideJob != nullptr));
    rrGuideStage = RRGuideStage::Stopped;
    rrGuideInactive.store(true, std::memory_order_release);
}

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
            d.Width * d.Height <= kRRGuideMaxPixels && RRGuideCameraValid(input.camera);
        reason = "dimensions_or_camera_invalid";
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
