static bool HookAA(void* t1, void* t2, void* t3, void* t4, void* t5, void* t6,
                   void* t7, void* t8, void* t9, bool flag, double d1, double d2,
                   float f1, float f2, float f3) {
    const auto call = ++aaCount;
    const auto currentPresent = presentCount.load();
    const auto previousPresent = lastAaObservedPresent.exchange(currentPresent);
    const auto gapPresents = (previousPresent && currentPresent > previousPresent) ? currentPresent - previousPresent : 0;
    const bool resumeGap = previousPresent != 0 && gapPresents > 2;
    const bool startupCandidate = call <= 12;
    const bool periodicCandidate = (call % 240) == 0;
    const bool baselineSample = (startupCandidate || periodicCandidate) && baselineSampleCount.load() < 32;
    const bool resumeSample = resumeGap && transitionSampleCount.load() < 32;
    bool sample = baselineSample || resumeSample;

    if (baselineSample) ++baselineSampleCount;
    if (resumeSample) ++transitionSampleCount;
    if (sample) ++sampleCount;

    unsigned long long engineFrame = 0;
    DWORD frameFault = 0;
    const bool engineFrameKnown = ReadEngineFrameSafe(&engineFrame, &frameFault);
    RememberAaFrame(call, currentPresent, engineFrame);
    if (call == 1 || resumeGap) ExtendPathTrace(currentPresent, 12);

    CameraSnapshot slCamera{};
    DWORD slCameraFault = 0;
    const char* slCameraReason = nullptr;
    const bool slCameraKnown = ReadCamera(&slCamera, &slCameraFault, &slCameraReason);

    void* textures[] = {t1,t2,t3,t4,t5,t6,t7,t8,t9};
    void* sampledResources[9]{};
    const char* preReason = resumeGap ? "resume_gap" : startupCandidate ? "startup" : "periodic";
    DWORD saved = GetLastError();
    if (sample) {
        CaptureCamera(call, "pre_aa");
        Log("AA_ENTER call=%llu begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu flag=%u d1=%.9g d2=%.9g f1=%.9g f2=%.9g f3=%.9g gap_presents=%llu sample_reason_pre=%s engine_frame=%llu engine_frame_known=%u engine_frame_exception=0x%08lX",
            call, beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), flag ? 1u : 0u,
            d1, d2, double(f1), double(f2), double(f3), gapPresents, preReason,
            engineFrame, unsigned(engineFrameKnown), frameFault);
        CaptureTextureArguments(call, textures, sampledResources, "pre_aa");
    }
    SetLastError(saved);

    const bool result = originalAA(t1,t2,t3,t4,t5,t6,t7,t8,t9,flag,d1,d2,f1,f2,f3);
    MarkControlDLSSReady(call, result);

    // Reset is an NGX input populated inside the original DLSS wrapper, so read
    // it only after that wrapper returns. A pre-call read could be stale.
    int resetValue = 0;
    unsigned int resetStatus = 0;
    DWORD resetFault = 0;
    const bool resetQueried = ReadNgxReset(&resetValue, &resetStatus, &resetFault);
    const bool resetKnown = resetQueried && resetStatus == 1;
    const bool resetFrame = resetKnown && resetValue != 0;
    bool resetTransitionAdded = false;
    if (resetFrame && !resumeSample && transitionSampleCount.load() < 32) {
        ++transitionSampleCount;
        resetTransitionAdded = true;
    }
    if (resetFrame) ExtendPathTrace(currentPresent, 12);

    if (resetFrame && !sample && resetTransitionAdded) {
        sample = true;
        ++sampleCount;
        CaptureCamera(call, "post_aa_reset");
        Log("AA_POST_SAMPLE call=%llu reason=reset begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu engine_frame=%llu",
            call, beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), engineFrame);
        CaptureTextureArguments(call, textures, sampledResources, "post_aa_reset");
    }

    const char* sampleReason = resetFrame && resumeGap ? "reset_resume" :
                               resetFrame ? "reset" :
                               resumeGap ? "resume_gap" : preReason;
    if (resetFrame || resumeGap) {
        Log("AA_TRANSITION call=%llu reason=%s gap_presents=%llu reset_known=%u reset=%d reset_status=0x%08X reset_exception=0x%08lX begin=%llu present=%llu hud_dispatch=%llu hud_render=%llu engine_frame=%llu",
            call, sampleReason, gapPresents, unsigned(resetKnown), resetValue, resetStatus, resetFault,
            beginCount.load(), currentPresent, hudCount.load(), hudRenderCount.load(), engineFrame);
    } else if ((!resetQueried || resetStatus != 1) && call <= 8) {
        Log("NGX_RESET_POST_UNAVAILABLE call=%llu queried=%u status=0x%08X exception=0x%08lX",
            call, unsigned(resetQueried), resetStatus, resetFault);
    }

    if (result) {
        ConfigureRRPhase7OptionsForAA(call, currentPresent, slCameraKnown, slCamera);
        SubmitSLCommonConstantsForAA(call, currentPresent, slCameraKnown, slCamera, resetKnown, resetValue);
        SubmitSLDepthMotionTagsForAA(call, currentPresent, textures, _countof(textures));
        if (!slCameraKnown && (call <= 12 || (call % 240) == 0)) {
            Log("SL_CAMERA_FRAME_UNAVAILABLE call=%llu present=%llu reason=%s exception=0x%08lX",
                call, currentPresent, slCameraReason ? slCameraReason : "unknown", slCameraFault);
        }
    }

    if (sample && result) {
        CaptureSemantic(call, sampledResources, sampleReason);
    }
    if (!result) {
        ++aaFailures;
        ExtendPathTrace(currentPresent, 12);
    }
    if (sample || (!result && aaFailures.load() <= 20)) {
        Log("AA_RETURN call=%llu success=%u failures=%llu reset_known=%u reset=%d gap_presents=%llu sample_reason=%s",
            call, result ? 1u : 0u, aaFailures.load(), unsigned(resetKnown), resetValue, gapPresents, sampleReason);
    }
    // RR diagnostic only: preserve production AA/FG ordering, then give a
    // joined material capture one later same-frame copy opportunity.
    if (result) RRGuideTryCaptureJoined(call, "post_aa");
    return result;
}
