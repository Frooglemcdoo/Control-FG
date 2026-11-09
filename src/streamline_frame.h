#pragma once

// v0.8.20 keeps the confirmed v0.8.18 frame-data bridge unchanged. Included
// after semantic_capture.h and camera_capture.h so it can reuse Control's build-locked
// camera snapshot, NGX parameter getters, and NativeTexture state tracker. Depth/MV
// remain required; the optional FP16/scRGB HUD-less tag is skipped only while the
// HDR10 bridge is active because final presentation is then RGB10/PQ/BT.2020.

struct SLNgxFrameInputs {
    float jitterX{};
    float jitterY{};
    float ngxMvScaleX{};
    float ngxMvScaleY{};
    unsigned int renderWidth{};
    unsigned int renderHeight{};
    int createFlags{};
    unsigned int jitterXStatus{};
    unsigned int jitterYStatus{};
    unsigned int mvScaleXStatus{};
    unsigned int mvScaleYStatus{};
    unsigned int widthStatus{};
    unsigned int heightStatus{};
    unsigned int flagsStatus{};
    DWORD fault{};
};

static SRWLOCK slCameraHistoryLock = SRWLOCK_INIT;
static CameraSnapshot slPreviousCamera{};
static bool slPreviousCameraValid = false;
static std::atomic<unsigned long long> slConstantsAttempts{0};
static std::atomic<unsigned long long> slConstantsInputFailures{0};
static std::atomic<unsigned long long> slConstantsTokenMisses{0};

static bool ReadSLNgxFrameInputs(SLNgxFrameInputs* out) noexcept {
    if (!out) return false;
    *out = SLNgxFrameInputs{};
    const DWORD saved = GetLastError();
    bool queried = false;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = base ? *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva) : nullptr;
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        if (parameters && ngxGetFloat && ngxGetUInt && ngxGetInt) {
            out->jitterXStatus = ngxGetFloat(parameters, "Jitter.Offset.X", &out->jitterX);
            out->jitterYStatus = ngxGetFloat(parameters, "Jitter.Offset.Y", &out->jitterY);
            out->mvScaleXStatus = ngxGetFloat(parameters, "MV.Scale.X", &out->ngxMvScaleX);
            out->mvScaleYStatus = ngxGetFloat(parameters, "MV.Scale.Y", &out->ngxMvScaleY);
            out->widthStatus = ngxGetUInt(parameters, "Width", &out->renderWidth);
            out->heightStatus = ngxGetUInt(parameters, "Height", &out->renderHeight);
            out->flagsStatus = ngxGetInt(parameters, "DLSS.Feature.Create.Flags", &out->createFlags);
            queried = true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        queried = false;
    }
    SetLastError(saved);
    return queried;
}

static bool SLNgxFrameInputsComplete(const SLNgxFrameInputs& in) noexcept {
    return in.jitterXStatus == 1 && in.jitterYStatus == 1 &&
           in.mvScaleXStatus == 1 && in.mvScaleYStatus == 1 &&
           in.widthStatus == 1 && in.heightStatus == 1 && in.flagsStatus == 1 &&
           in.renderWidth != 0 && in.renderHeight != 0 &&
           std::isfinite(in.jitterX) && std::isfinite(in.jitterY) &&
           std::isfinite(in.ngxMvScaleX) && std::isfinite(in.ngxMvScaleY);
}

static void SLCopyMatrix(sl::float4x4& dst, const double* src) noexcept {
    dst.setRow(0, sl::float4(float(src[0]), float(src[1]), float(src[2]), float(src[3])));
    dst.setRow(1, sl::float4(float(src[4]), float(src[5]), float(src[6]), float(src[7])));
    dst.setRow(2, sl::float4(float(src[8]), float(src[9]), float(src[10]), float(src[11])));
    dst.setRow(3, sl::float4(float(src[12]), float(src[13]), float(src[14]), float(src[15])));
}

static void SLIdentityMatrix(sl::float4x4& dst) noexcept {
    dst.setRow(0, sl::float4(1.f, 0.f, 0.f, 0.f));
    dst.setRow(1, sl::float4(0.f, 1.f, 0.f, 0.f));
    dst.setRow(2, sl::float4(0.f, 0.f, 1.f, 0.f));
    dst.setRow(3, sl::float4(0.f, 0.f, 0.f, 1.f));
}

static void SLMultiplyMatrices(sl::float4x4& dst, const double* a, const double* b) noexcept {
    double m[16]{};
    for (unsigned r = 0; r < 4; ++r) {
        for (unsigned c = 0; c < 4; ++c) {
            double v = 0.0;
            for (unsigned k = 0; k < 4; ++k) v += a[r * 4 + k] * b[k * 4 + c];
            m[r * 4 + c] = v;
        }
    }
    SLCopyMatrix(dst, m);
}

static sl::float3 SLNormalized3(double x, double y, double z) noexcept {
    const double len2 = x*x + y*y + z*z;
    if (!(len2 > 1.0e-20) || !std::isfinite(len2)) return sl::float3(0.f, 0.f, 0.f);
    const double inv = 1.0 / std::sqrt(len2);
    return sl::float3(float(x*inv), float(y*inv), float(z*inv));
}

static bool SLProjectionScalars(const CameraSnapshot& camera, bool depthInverted,
                                float* nearPlane, float* farPlane,
                                float* verticalFov, float* aspect) noexcept {
    const double m00 = camera.viewToClip[0];
    const double m11 = camera.viewToClip[5];
    const double m22 = camera.viewToClip[10];
    const double m32 = camera.viewToClip[14];
    if (!std::isfinite(m00) || !std::isfinite(m11) || !std::isfinite(m22) || !std::isfinite(m32) ||
        std::fabs(m00) < 1.0e-12 || std::fabs(m11) < 1.0e-12 || std::fabs(m22) < 1.0e-12 ||
        std::fabs(m22 - 1.0) < 1.0e-12) return false;
    // D3D 0..1 perspective projection. Control's observed DLSS create flags
    // are currently non-inverted, but keep the scalar extraction correct if a
    // reversed-Z configuration is encountered later.
    const double depthAtViewNear = depthInverted ? 1.0 : 0.0;
    const double depthAtViewFar  = depthInverted ? 0.0 : 1.0;
    const double n = -m32 / (m22 - depthAtViewNear);
    const double f = -m32 / (m22 - depthAtViewFar);
    const double vfov = 2.0 * std::atan(1.0 / std::fabs(m11));
    const double ar = std::fabs(m11 / m00);
    if (!(n > 0.0) || !(f > n) || !std::isfinite(f) || !std::isfinite(vfov) || !std::isfinite(ar)) return false;
    *nearPlane = float(n);
    *farPlane = float(f);
    *verticalFov = float(vfov);
    *aspect = float(ar);
    return true;
}

static bool BuildSLCommonConstants(const CameraSnapshot& camera, const SLNgxFrameInputs& in,
                                   bool resetKnown, int resetValue, sl::Constants* out,
                                   bool* usedHistory) noexcept {
    if (!out || !usedHistory) return false;
    sl::Constants constants{};
    SLCopyMatrix(constants.cameraViewToClip, camera.viewToClip);
    SLCopyMatrix(constants.clipToCameraView, camera.clipToView);

    CameraSnapshot previous{};
    bool previousValid = false;
    AcquireSRWLockShared(&slCameraHistoryLock);
    previousValid = slPreviousCameraValid;
    if (previousValid) previous = slPreviousCamera;
    ReleaseSRWLockShared(&slCameraHistoryLock);

    const bool reset = !previousValid || (resetKnown && resetValue != 0);
    if (!reset && previousValid) {
        // Row-vector convention established by v0.4 matrix validation:
        // current clip -> world -> previous clip, and its reverse.
        SLMultiplyMatrices(constants.clipToPrevClip, camera.clipToWorld, previous.worldToClip);
        SLMultiplyMatrices(constants.prevClipToClip, previous.clipToWorld, camera.worldToClip);
        *usedHistory = true;
    } else {
        SLIdentityMatrix(constants.clipToPrevClip);
        SLIdentityMatrix(constants.prevClipToClip);
        *usedHistory = false;
    }

    constants.jitterOffset = sl::float2(in.jitterX, in.jitterY);
    // Control uses a conventional centered pinhole camera. The default sl::float2
    // is INVALID_FLOAT, which v0.8.14 correctly triggered a validation warning.
    constants.cameraPinholeOffset = sl::float2(0.f, 0.f);
    // NGX MV.Scale converts Control's raw motion vectors into pixel displacement.
    // Streamline wants a scale that normalizes the raw values into [-1,1].
    // Therefore SL scale = NGX pixel scale / render dimension. On this build the
    // observed NGX values are 2560/1440 at 2560x1440, yielding {1,1}.
    constants.mvecScale = sl::float2(in.ngxMvScaleX / float(in.renderWidth),
                                    in.ngxMvScaleY / float(in.renderHeight));

    // v0.4 established ViewToWorld as a row-major 4x3 affine matrix.
    constants.cameraRight = SLNormalized3(camera.viewToWorld[0], camera.viewToWorld[1], camera.viewToWorld[2]);
    constants.cameraUp    = SLNormalized3(camera.viewToWorld[3], camera.viewToWorld[4], camera.viewToWorld[5]);
    constants.cameraFwd   = SLNormalized3(camera.viewToWorld[6], camera.viewToWorld[7], camera.viewToWorld[8]);
    constants.cameraPos   = sl::float3(float(camera.viewToWorld[9]), float(camera.viewToWorld[10]), float(camera.viewToWorld[11]));

    const bool depthInverted = (in.createFlags & 8) != 0;
    if (!SLProjectionScalars(camera, depthInverted, &constants.cameraNear, &constants.cameraFar,
                             &constants.cameraFOV, &constants.cameraAspectRatio)) return false;

    constants.depthInverted = depthInverted ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    constants.cameraMotionIncluded = sl::Boolean::eTrue;
    constants.motionVectors3D = sl::Boolean::eFalse;
    constants.reset = reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    // Streamline SDK 2.14.1 ships sl::Constants as kStructVersion2 and the
    // public sl_consts.h does NOT expose a renderingGameFrames member. The
    // DLSS-G guide mentions that field, but adding it here would break the
    // pinned 2.14.1 C++ ABI. For this gate we submit only members that exist
    // in the official 2.14.1 header.
    constants.orthographicProjection = sl::Boolean::eFalse;
    constants.motionVectorsDilated = sl::Boolean::eFalse;
    constants.motionVectorsJittered = (in.createFlags & 4) ? sl::Boolean::eTrue : sl::Boolean::eFalse;

    *out = constants;
    return true;
}


// ---------------- Ray Reconstruction Phase 7: options/optimal-settings handshake ----------------
// P6 proved the official DLSS-RR plugin loads and is supported. P7 exercises
// only the CPU-side configuration/query surface. It deliberately does NOT tag
// RR material guides, call slEvaluateFeature, bypass Control's native denoisers,
// or submit any additional GPU work.
struct RRPhase7NgxOptionsInputs {
    unsigned int renderWidth{};
    unsigned int renderHeight{};
    unsigned int outputWidth{};
    unsigned int outputHeight{};
    float preExposure{1.0f};
    float exposureScale{1.0f};
    unsigned int renderWidthStatus{};
    unsigned int renderHeightStatus{};
    unsigned int outputWidthStatus{};
    unsigned int outputHeightStatus{};
    unsigned int preExposureStatus{};
    unsigned int exposureScaleStatus{};
    DWORD fault{};
};

static std::atomic<unsigned int> rrP7OptionsApplied{0};
static std::atomic<unsigned int> rrP7OptionsQueryFinished{0};
static std::atomic<unsigned int> rrP7OptionsAttempts{0};
static SRWLOCK rrP7OptionsLock = SRWLOCK_INIT;

static bool ReadRRPhase7NgxOptionsInputs(RRPhase7NgxOptionsInputs* out) noexcept {
    if (!out) return false;
    *out = RRPhase7NgxOptionsInputs{};
    const DWORD saved = GetLastError();
    bool queried = false;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = base ? *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva) : nullptr;
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        if (parameters && ngxGetUInt && ngxGetFloat) {
            out->renderWidthStatus = ngxGetUInt(parameters, "Width", &out->renderWidth);
            out->renderHeightStatus = ngxGetUInt(parameters, "Height", &out->renderHeight);
            out->outputWidthStatus = ngxGetUInt(parameters, "OutWidth", &out->outputWidth);
            out->outputHeightStatus = ngxGetUInt(parameters, "OutHeight", &out->outputHeight);
            out->preExposureStatus = ngxGetFloat(parameters, "DLSS.Pre.Exposure", &out->preExposure);
            out->exposureScaleStatus = ngxGetFloat(parameters, "DLSS.Exposure.Scale", &out->exposureScale);
            queried = true;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        queried = false;
    }
    SetLastError(saved);
    return queried;
}

static bool RRPhase7NgxOptionsComplete(const RRPhase7NgxOptionsInputs& in) noexcept {
    return in.renderWidthStatus == 1 && in.renderHeightStatus == 1 &&
           in.outputWidthStatus == 1 && in.outputHeightStatus == 1 &&
           in.preExposureStatus == 1 && in.exposureScaleStatus == 1 &&
           in.renderWidth != 0 && in.renderHeight != 0 &&
           in.outputWidth != 0 && in.outputHeight != 0 &&
           std::isfinite(in.preExposure) && std::isfinite(in.exposureScale);
}

static void RRCopyAffine4x3(sl::float4x4& dst, const double* src) noexcept {
    // Control's validated camera affine arrays are row-major 4x3 matrices.
    // Promote to the row-major 4x4 form required by DLSSDOptions.
    dst.setRow(0, sl::float4(float(src[0]), float(src[1]), float(src[2]), 0.f));
    dst.setRow(1, sl::float4(float(src[3]), float(src[4]), float(src[5]), 0.f));
    dst.setRow(2, sl::float4(float(src[6]), float(src[7]), float(src[8]), 0.f));
    dst.setRow(3, sl::float4(float(src[9]), float(src[10]), float(src[11]), 1.f));
}

static const char* RRDLSSModeName(sl::DLSSMode mode) noexcept {
    switch (mode) {
        case sl::DLSSMode::eOff: return "off";
        case sl::DLSSMode::eMaxPerformance: return "performance";
        case sl::DLSSMode::eBalanced: return "balanced";
        case sl::DLSSMode::eMaxQuality: return "quality";
        case sl::DLSSMode::eUltraPerformance: return "ultra_performance";
        case sl::DLSSMode::eUltraQuality: return "ultra_quality";
        case sl::DLSSMode::eDLAA: return "dlaa";
        default: return "unknown";
    }
}

static void FillRRPhase7BaseOptions(const RRPhase7NgxOptionsInputs& in, const CameraSnapshot& camera,
                                    sl::DLSSMode mode, sl::DLSSDOptions* out) noexcept {
    sl::DLSSDOptions options{};
    options.mode = mode;
    options.outputWidth = in.outputWidth;
    options.outputHeight = in.outputHeight;
    options.sharpness = 0.0f;
    options.preExposure = in.preExposure;
    options.exposureScale = in.exposureScale;
    // DLSS-RR requires HDR/linear color input even when final presentation is SDR.
    options.colorBuffersHDR = sl::Boolean::eTrue;
    options.indicatorInvertAxisX = sl::Boolean::eFalse;
    options.indicatorInvertAxisY = sl::Boolean::eFalse;
    // P7 does not submit normal/roughness guides yet. Keep the option explicit
    // and conservative until the semantic guide map is validated.
    options.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
    RRCopyAffine4x3(options.worldToCameraView, camera.worldToView);
    RRCopyAffine4x3(options.cameraViewToWorld, camera.viewToWorld);
    options.alphaUpscalingEnabled = sl::Boolean::eFalse;
    options.dlaaPreset = sl::DLSSDPreset::ePresetF;
    options.qualityPreset = sl::DLSSDPreset::ePresetF;
    options.balancedPreset = sl::DLSSDPreset::ePresetF;
    options.performancePreset = sl::DLSSDPreset::ePresetF;
    options.ultraPerformancePreset = sl::DLSSDPreset::ePresetF;
    options.ultraQualityPreset = sl::DLSSDPreset::ePresetF;
    *out = options;
}

static void ConfigureRRPhase7OptionsForAA(unsigned long long call, unsigned long long currentPresent,
                                          bool cameraKnown, const CameraSnapshot& camera) noexcept {
    if (rrP7OptionsQueryFinished.load(std::memory_order_acquire)) return;
    if (!cameraKnown || !slDeviceConfigured.load() || !slRrFeatureLoaded.load() ||
        !slRrFeatureSupported.load() || !slRrFunctionTableReady.load() ||
        !slDLSSDGetOptimalSettingsApi || !slDLSSDSetOptionsApi || !slDLSSDGetStateApi) return;

    const unsigned int attempt = ++rrP7OptionsAttempts;
    if (attempt > 16) return;

    RRPhase7NgxOptionsInputs in{};
    if (!ReadRRPhase7NgxOptionsInputs(&in) || !RRPhase7NgxOptionsComplete(in)) {
        Log("SL_RR_OPTIONS_SKIP call=%llu present=%llu attempt=%u reason=ngx_inputs_incomplete fault=0x%08lX statuses=%u,%u,%u,%u,%u,%u render=%ux%u output=%ux%u",
            call, currentPresent, attempt, in.fault, in.renderWidthStatus, in.renderHeightStatus,
            in.outputWidthStatus, in.outputHeightStatus, in.preExposureStatus, in.exposureScaleStatus,
            in.renderWidth, in.renderHeight, in.outputWidth, in.outputHeight);
        return;
    }

    AcquireSRWLockExclusive(&rrP7OptionsLock);
    if (rrP7OptionsQueryFinished.load(std::memory_order_relaxed)) {
        ReleaseSRWLockExclusive(&rrP7OptionsLock);
        return;
    }

    const sl::DLSSMode modes[] = {
        sl::DLSSMode::eDLAA,
        sl::DLSSMode::eMaxQuality,
        sl::DLSSMode::eBalanced,
        sl::DLSSMode::eMaxPerformance,
        sl::DLSSMode::eUltraPerformance,
        sl::DLSSMode::eUltraQuality,
    };

    sl::DLSSMode bestMode = sl::DLSSMode::eOff;
    unsigned long long bestError = ~0ull;
    sl::DLSSDOptimalSettings bestSettings{};
    sl::Result bestResult = sl::Result::eErrorMissingInputParameter;

    for (sl::DLSSMode mode : modes) {
        sl::DLSSDOptions options{};
        FillRRPhase7BaseOptions(in, camera, mode, &options);
        sl::DLSSDOptimalSettings settings{};
        const sl::Result result = slDLSSDGetOptimalSettingsApi(options, settings);
        unsigned long long error = ~0ull;
        if (result == sl::Result::eOk && settings.optimalRenderWidth && settings.optimalRenderHeight) {
            const long long dx = static_cast<long long>(settings.optimalRenderWidth) - static_cast<long long>(in.renderWidth);
            const long long dy = static_cast<long long>(settings.optimalRenderHeight) - static_cast<long long>(in.renderHeight);
            error = static_cast<unsigned long long>(dx < 0 ? -dx : dx) +
                    static_cast<unsigned long long>(dy < 0 ? -dy : dy);
            if (error < bestError) {
                bestError = error;
                bestMode = mode;
                bestSettings = settings;
                bestResult = result;
            }
        }
        Log("SL_RR_OPTIMAL_CANDIDATE call=%llu present=%llu mode=%s mode_value=%u result=%lld output=%ux%u current_render=%ux%u optimal=%ux%u min=%ux%u max=%ux%u sharpness=%.9g error_pixels=%llu",
            call, currentPresent, RRDLSSModeName(mode), unsigned(mode), SLResultCode(result),
            in.outputWidth, in.outputHeight, in.renderWidth, in.renderHeight,
            settings.optimalRenderWidth, settings.optimalRenderHeight,
            settings.renderWidthMin, settings.renderHeightMin, settings.renderWidthMax, settings.renderHeightMax,
            double(settings.optimalSharpness), error == ~0ull ? 0xffffffffffffffffull : error);
    }

    rrP7OptionsQueryFinished.store(1, std::memory_order_release);
    const unsigned long long tolerance = 8;
    if (bestResult != sl::Result::eOk || bestMode == sl::DLSSMode::eOff || bestError > tolerance) {
        Log("SL_RR_OPTIONS_SKIP call=%llu present=%llu attempt=%u reason=no_matching_dlss_mode current_render=%ux%u output=%ux%u best_mode=%s best_error_pixels=%llu tolerance=%llu",
            call, currentPresent, attempt, in.renderWidth, in.renderHeight, in.outputWidth, in.outputHeight,
            RRDLSSModeName(bestMode), bestError, tolerance);
        ReleaseSRWLockExclusive(&rrP7OptionsLock);
        return;
    }

    sl::DLSSDOptions selected{};
    FillRRPhase7BaseOptions(in, camera, bestMode, &selected);
    const sl::Result setResult = slDLSSDSetOptionsApi(slFgViewport, selected);
    sl::DLSSDState state{};
    const sl::Result stateResult = setResult == sl::Result::eOk ? slDLSSDGetStateApi(slFgViewport, state) : sl::Result::eErrorFeatureFailedToLoad;
    const bool success = setResult == sl::Result::eOk;
    if (success) rrP7OptionsApplied.store(1, std::memory_order_release);

    Log("SL_RR_OPTIONS_HANDSHAKE call=%llu present=%llu attempt=%u success=%u mode=%s mode_value=%u render=%ux%u output=%ux%u optimal=%ux%u pre_exposure=%.9g exposure_scale=%.9g hdr_input=1 normal_roughness=unpacked preset=dlaa:F,quality:F,balanced:F,performance:F,ultra_performance:F,ultra_quality:F set_result=%lld state_result=%lld estimated_vram=%llu rr_evaluate=0 native_denoiser_bypass=0",
        call, currentPresent, attempt, unsigned(success), RRDLSSModeName(bestMode), unsigned(bestMode),
        in.renderWidth, in.renderHeight, in.outputWidth, in.outputHeight,
        bestSettings.optimalRenderWidth, bestSettings.optimalRenderHeight,
        double(in.preExposure), double(in.exposureScale), SLResultCode(setResult), SLResultCode(stateResult),
        static_cast<unsigned long long>(state.estimatedVRAMUsageInBytes));

    ReleaseSRWLockExclusive(&rrP7OptionsLock);
}

static void CommitSLPreviousCamera(const CameraSnapshot& camera) noexcept {
    AcquireSRWLockExclusive(&slCameraHistoryLock);
    slPreviousCamera = camera;
    slPreviousCameraValid = true;
    ReleaseSRWLockExclusive(&slCameraHistoryLock);
}

static void SubmitSLCommonConstantsForAA(unsigned long long call, unsigned long long currentPresent,
                                         bool cameraKnown, const CameraSnapshot& camera,
                                         bool resetKnown, int resetValue) noexcept {
    if (!slDeviceConfigured.load() || !slSetConstantsApi) return;
    ++slConstantsAttempts;

    // v0.4 runtime proved sampled AA frames observe begin == present + 1.
    // Bind this AA work to that upcoming Present index explicitly so PCL markers
    // and common constants use the same Streamline frame token.
    const unsigned long long targetPresent = currentPresent + 1;
    sl::FrameToken* token = GetSLFrameToken(targetPresent, false);
    if (!token) {
        // Covers the first frame if the device became ready after HookBegin.
        PrepareSLFrameToken(targetPresent);
        token = GetSLFrameToken(targetPresent, false);
    }
    if (token && GetSLFrameToken(targetPresent, true)) {
        ++slConstantsDuplicateSkips;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_CONSTANTS_SKIP call=%llu present=%llu target_present=%llu reason=already_submitted duplicate_skips=%llu",
                call, currentPresent, targetPresent, slConstantsDuplicateSkips.load());
        return;
    }
    if (!token) {
        ++slConstantsTokenMisses;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_CONSTANTS_SKIP call=%llu present=%llu target_present=%llu reason=frame_token_missing begin=%llu token_misses=%llu",
                call, currentPresent, targetPresent, beginCount.load(), slConstantsTokenMisses.load());
        return;
    }
    if (!cameraKnown) {
        ++slConstantsInputFailures;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_CONSTANTS_SKIP call=%llu present=%llu target_present=%llu reason=camera_unavailable input_failures=%llu",
                call, currentPresent, targetPresent, slConstantsInputFailures.load());
        return;
    }

    SLNgxFrameInputs inputs{};
    const bool queried = ReadSLNgxFrameInputs(&inputs);
    if (!queried || !SLNgxFrameInputsComplete(inputs)) {
        ++slConstantsInputFailures;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_CONSTANTS_SKIP call=%llu present=%llu target_present=%llu reason=ngx_inputs_incomplete fault=0x%08lX statuses=%u,%u,%u,%u,%u,%u,%u dims=%ux%u input_failures=%llu",
                call, currentPresent, targetPresent, inputs.fault, inputs.jitterXStatus, inputs.jitterYStatus,
                inputs.mvScaleXStatus, inputs.mvScaleYStatus, inputs.widthStatus, inputs.heightStatus,
                inputs.flagsStatus, inputs.renderWidth, inputs.renderHeight, slConstantsInputFailures.load());
        return;
    }

    // HDR/SDR color-domain transitions are handled by the FG eOff + fresh-frame
    // warmup gate in streamline_bridge.h. Do not manufacture a common-constants
    // reset here: r25 proved that forcing reset=true at this boundary regresses
    // interpolation quality after HDR -> SDR.
    sl::Constants constants{};
    bool usedHistory = false;
    if (!BuildSLCommonConstants(camera, inputs, resetKnown, resetValue, &constants, &usedHistory)) {
        ++slConstantsInputFailures;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_CONSTANTS_SKIP call=%llu present=%llu target_present=%llu reason=matrix_or_projection_invalid input_failures=%llu",
                call, currentPresent, targetPresent, slConstantsInputFailures.load());
        return;
    }

    const sl::Result result = slSetConstantsApi(constants, *token, slFgViewport);
    if(FGAlignActive(targetPresent)) Log("ALIGN_CONSTANTS target=%llu call=%llu token=%p result=%lld",targetPresent,call,token,SLResultCode(result));
    const bool success = result == sl::Result::eOk;
    MarkSLFrameConstantsReady(targetPresent, success);
    if (success) {
        ++slConstantsSuccesses;
        CommitSLPreviousCamera(camera);
    }
    if (call <= 12 || (call % 240) == 0 || !success) {
        Log("SL_CONSTANTS call=%llu present=%llu begin=%llu target_present=%llu token=%p result=%lld success=%u successes=%llu engine_frame=%llu render=%ux%u jitter=%.9g,%.9g ngx_mv_scale=%.9g,%.9g sl_mv_scale=%.9g,%.9g near=%.9g far=%.9g fov=%.9g aspect=%.9g reset=%u history=%u depth_inverted=%u mv_jittered=%u rendering_game_frames=1 camera_motion=1",
            call, currentPresent, beginCount.load(), targetPresent, token, SLResultCode(result), unsigned(success),
            slConstantsSuccesses.load(), camera.engineFrame, inputs.renderWidth, inputs.renderHeight,
            double(inputs.jitterX), double(inputs.jitterY), double(inputs.ngxMvScaleX), double(inputs.ngxMvScaleY),
            double(constants.mvecScale.x), double(constants.mvecScale.y), double(constants.cameraNear),
            double(constants.cameraFar), double(constants.cameraFOV), double(constants.cameraAspectRatio),
            unsigned(constants.reset == sl::Boolean::eTrue), unsigned(usedHistory),
            unsigned(constants.depthInverted == sl::Boolean::eTrue), unsigned(constants.motionVectorsJittered == sl::Boolean::eTrue));
    }
}

struct SLTaggedResourceInput {
    ID3D12Resource* resource{};
    D3D12_RESOURCE_DESC desc{};
    unsigned int trackedState{UINT_MAX};
    unsigned int stateKnown{};
    unsigned int ngxStatus{};
    unsigned int slot{};
    DWORD fault{};
};

static bool ReadSLNgxResourceWithState(const char* key, void* const* textures, unsigned int textureCount,
                                       SLTaggedResourceInput* out) noexcept {
    if (!key || !out) return false;
    *out = SLTaggedResourceInput{};
    const DWORD saved = GetLastError();
    bool ok = false;
    __try {
        auto base = reinterpret_cast<unsigned char*>(verifiedD3d);
        auto context = base ? *reinterpret_cast<unsigned char**>(base + kDlssContextPointerRva) : nullptr;
        void* parameters = context ? *reinterpret_cast<void**>(context + kNgxParametersOffset) : nullptr;
        ID3D12Resource* resource = nullptr;
        if (parameters && ngxGetResource) {
            out->ngxStatus = ngxGetResource(parameters, key, &resource);
            out->resource = resource;
            if (out->ngxStatus == 1 && resource) {
                out->desc = resource->GetDesc();
                for (unsigned int i = 0; textures && i < textureCount; ++i) {
                    NativeTextureStateSnapshot snapshot{};
                    if (ReadNativeTextureState(textures[i], &snapshot) && snapshot.resource == resource) {
                        out->trackedState = snapshot.trackedState;
                        out->stateKnown = snapshot.stateKnown;
                        out->slot = i + 1;
                        break;
                    }
                }
                ok = out->stateKnown != 0 && out->trackedState != UINT_MAX;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out->fault = GetExceptionCode();
        ok = false;
    }
    SetLastError(saved);
    return ok;
}

static void SubmitSLDepthMotionTagsForAA(unsigned long long call, unsigned long long currentPresent,
                                         void* const* textures, unsigned int textureCount) noexcept {
    if (!slDeviceConfigured.load() || !slSetTagForFrameApi) return;
    const unsigned long long targetPresent = currentPresent + 1;
    sl::FrameToken* token = GetSLFrameToken(targetPresent, false);
    if (!token) {
        PrepareSLFrameToken(targetPresent);
        token = GetSLFrameToken(targetPresent, false);
    }
    if (!token) {
        ++slResourceTagFailures;
        if (call <= 12 || (call % 240) == 0)
            Log("SL_RESOURCE_TAG_SKIP kind=depth_mv call=%llu present=%llu target_present=%llu reason=frame_token_missing failures=%llu",
                call, currentPresent, targetPresent, slResourceTagFailures.load());
        return;
    }

    SLTaggedResourceInput depth{}, motion{};
    const bool depthReady = ReadSLNgxResourceWithState("Depth", textures, textureCount, &depth);
    const bool motionReady = ReadSLNgxResourceWithState("MotionVectors", textures, textureCount, &motion);
    if (!depthReady || !motionReady || depth.desc.Width == 0 || depth.desc.Height == 0 ||
        motion.desc.Width == 0 || motion.desc.Height == 0) {
        ++slResourceTagFailures;
        if (call <= 12 || (call % 240) == 0) {
            Log("SL_RESOURCE_TAG_SKIP kind=depth_mv call=%llu present=%llu target_present=%llu reason=resource_or_state_unavailable depth_ready=%u depth_status=0x%X depth=%p depth_slot=%u depth_state=0x%X depth_fault=0x%08lX mv_ready=%u mv_status=0x%X mv=%p mv_slot=%u mv_state=0x%X mv_fault=0x%08lX failures=%llu",
                call, currentPresent, targetPresent, unsigned(depthReady), depth.ngxStatus, depth.resource, depth.slot,
                depth.trackedState, depth.fault, unsigned(motionReady), motion.ngxStatus, motion.resource, motion.slot,
                motion.trackedState, motion.fault, slResourceTagFailures.load());
        }
        return;
    }

    sl::Extent depthExtent{};
    depthExtent.width = static_cast<uint32_t>(depth.desc.Width);
    depthExtent.height = depth.desc.Height;
    sl::Extent motionExtent{};
    motionExtent.width = static_cast<uint32_t>(motion.desc.Width);
    motionExtent.height = motion.desc.Height;

    sl::Resource depthResource(sl::ResourceType::eTex2d, depth.resource, depth.trackedState);
    sl::Resource motionResource(sl::ResourceType::eTex2d, motion.resource, motion.trackedState);
    sl::ResourceTag tags[] = {
        sl::ResourceTag(&depthResource, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &depthExtent),
        sl::ResourceTag(&motionResource, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &motionExtent)
    };

    ++slResourceTagCalls;
    const sl::Result result = slSetTagForFrameApi(*token, slFgViewport, tags, _countof(tags), nullptr);
    const bool success = result == sl::Result::eOk;
    if(FGAlignActive(targetPresent)) Log("ALIGN_DEPTH_MV target=%llu call=%llu token=%p depth=%p motion=%p success=%u depth_state=0x%X motion_state=0x%X",targetPresent,call,token,depth.resource,motion.resource,unsigned(success),depth.trackedState,motion.trackedState);
    if (success) {
        ++slResourceTagSuccesses;
        slFgMvecDepthWidth.store(static_cast<unsigned int>(depth.desc.Width));
        slFgMvecDepthHeight.store(depth.desc.Height);
        slFgDepthFormat.store(static_cast<unsigned int>(depth.desc.Format));
        slFgMvecFormat.store(static_cast<unsigned int>(motion.desc.Format));
        MarkSLFrameTagBits(targetPresent, kSLTagDepth | kSLTagMotionVectors);
    } else {
        ++slResourceTagFailures;
    }
    if (call <= 12 || (call % 240) == 0 || !success) {
        Log("SL_RESOURCE_TAG kind=depth_mv call=%llu present=%llu target_present=%llu token=%p result=%lld success=%u tag_mask=0x%X depth=%p depth=%llux%u depth_format=%u depth_slot=%u depth_state=0x%X mv=%p mv=%llux%u mv_format=%u mv_slot=%u mv_state=0x%X lifecycle=valid_until_present cmd=null calls=%llu successes=%llu failures=%llu",
            call, currentPresent, targetPresent, token, SLResultCode(result), unsigned(success), GetSLFrameTagMask(targetPresent),
            depth.resource, depth.desc.Width, depth.desc.Height, unsigned(depth.desc.Format), depth.slot, depth.trackedState,
            motion.resource, motion.desc.Width, motion.desc.Height, unsigned(motion.desc.Format), motion.slot, motion.trackedState,
            slResourceTagCalls.load(), slResourceTagSuccesses.load(), slResourceTagFailures.load());
    }
}

static void SubmitSLHUDLessTagForFrame(unsigned long long currentPresent, unsigned long long engineFrame,
                                       ID3D12Resource* resource, const D3D12_RESOURCE_DESC& desc,
                                       unsigned int trackedState, bool stateKnown,
                                       ID3D12GraphicsCommandList* commandList, bool commandDirect, bool deviceMatch) noexcept {
    if (!slDeviceConfigured.load() || !slSetTagForFrameApi) return;
    if (IsHdr10BridgeActive()) {
        if (currentPresent <= 16 || (currentPresent % 240) == 0)
            Log("SL_RESOURCE_TAG_SKIP kind=hudless present=%llu target_present=%llu reason=hdr10_bridge_optional_hudless_color_domain_mismatch source_format=%u final_format=%u failure_count_unchanged=%llu",
                currentPresent, currentPresent + 1, unsigned(desc.Format), unsigned(DXGI_FORMAT_R10G10B10A2_UNORM), slResourceTagFailures.load());
        return;
    }
    const unsigned long long targetPresent = currentPresent + 1;
    sl::FrameToken* token = GetSLFrameToken(targetPresent, false);
    if (!token || !resource || !stateKnown || trackedState == UINT_MAX || !commandList || !commandDirect || !deviceMatch ||
        desc.Width == 0 || desc.Height == 0) {
        ++slResourceTagFailures;
        if (currentPresent <= 16 || (currentPresent % 240) == 0)
            Log("SL_RESOURCE_TAG_SKIP kind=hudless present=%llu target_present=%llu engine_frame=%llu token=%p resource=%p state_known=%u state=0x%X command_list=%p direct=%u device_match=%u dims=%llux%u failures=%llu",
                currentPresent, targetPresent, engineFrame, token, resource, unsigned(stateKnown), trackedState, commandList,
                unsigned(commandDirect), unsigned(deviceMatch), desc.Width, desc.Height, slResourceTagFailures.load());
        return;
    }

    sl::Extent extent{};
    extent.width = static_cast<uint32_t>(desc.Width);
    extent.height = desc.Height;
    sl::Resource hudlessResource(sl::ResourceType::eTex2d, resource, trackedState);
    sl::ResourceTag tag(&hudlessResource, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eOnlyValidNow, &extent);

    ++slResourceTagCalls;
    const sl::Result result = slSetTagForFrameApi(*token, slFgViewport, &tag, 1,
        reinterpret_cast<sl::CommandBuffer*>(commandList));
    const bool success = result == sl::Result::eOk;
    if (success) {
        ++slResourceTagSuccesses;
        slFgColorWidth.store(static_cast<unsigned int>(desc.Width));
        slFgColorHeight.store(desc.Height);
        slFgColorFormat.store(static_cast<unsigned int>(desc.Format));
        slFgHudLessFormat.store(static_cast<unsigned int>(desc.Format));
        MarkSLFrameTagBits(targetPresent, kSLTagHUDLess);
    } else {
        ++slResourceTagFailures;
    }
    if (currentPresent <= 16 || (currentPresent % 240) == 0 || !success || slResourceTagSuccesses.load() <= 16) {
        Log("SL_RESOURCE_TAG kind=hudless present=%llu target_present=%llu engine_frame=%llu token=%p result=%lld success=%u tag_mask=0x%X resource=%p width=%llu height=%u format=%u state=0x%X lifecycle=only_valid_now command_list=%p direct=%u device_match=%u calls=%llu successes=%llu failures=%llu",
            currentPresent, targetPresent, engineFrame, token, SLResultCode(result), unsigned(success), GetSLFrameTagMask(targetPresent),
            resource, desc.Width, desc.Height, unsigned(desc.Format), trackedState, commandList, unsigned(commandDirect), unsigned(deviceMatch),
            slResourceTagCalls.load(), slResourceTagSuccesses.load(), slResourceTagFailures.load());
    }
}
