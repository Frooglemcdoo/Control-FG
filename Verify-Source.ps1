#requires -Version 5.1
param()
$ErrorActionPreference = 'Stop'
try {
    . (Join-Path $PSScriptRoot 'Build-Metadata.ps1')
    # The current-release checkpoint is authoritative for every file intentionally
    # advanced beyond historical milestone evidence. Load ownership before any
    # historical hash validation so an older milestone can never reject a newer
    # source revision merely because it still records that path's old hash.
    $currentReleaseValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/current-release/results.json') -Raw | ConvertFrom-Json
    if ($currentReleaseValidation.status -cne 'LOCAL_PASS' -or -not ([string]$currentReleaseValidation.tests.portable_gate).StartsWith('PASS')) { throw 'Current-release evidence checkpoint is incomplete.' }
    $currentReleasePaths = @($currentReleaseValidation.tested_sha256.PSObject.Properties | ForEach-Object { $_.Name })
    # Keep normal support simple; specialized collectors belong in tools/diagnostics.
    $supportCollectors = @('Collect-ControlFG-Compact-Logs.cmd', 'Collect-ControlFG-Compact-Logs.ps1')
    foreach ($file in @(Get-ChildItem -LiteralPath $PSScriptRoot -File -Filter 'Collect-*')) {
        if ($supportCollectors -cnotcontains $file.Name) { throw ('Developer collector in source root: ' + $file.Name) }
    }
    # Parse root and relocated diagnostic PowerShell scripts before the C++ build.
    $scriptFiles = @(Get-ChildItem -LiteralPath $PSScriptRoot -File -Filter '*.ps1')
    $scriptFiles += @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'tools/diagnostics') -File -Filter '*.ps1')
    foreach ($file in $scriptFiles) {
        $tokens = $null; $errors = $null
        [void][System.Management.Automation.Language.Parser]::ParseFile($file.FullName, [ref]$tokens, [ref]$errors)
        if ($errors.Count) { throw ('PowerShell parse error in ' + $file.Name + ': ' + $errors[0].Message) }
    }
    $probe = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/probe.cpp') -Raw
    foreach ($marker in @('PROBE v2.1.1 internal_build=2.1.1','source_revision=v2.1.1-pt2-r2-proxy-device-composite','log_profile=%s','CAPABILITIES fg=fixed_2x_to_6x_plus_dynamic','MONITORING profile=%s rr_perf_sample=240','CONTROLFG_VERBOSE_LOG')) {
        if (-not $probe.Contains($marker)) { throw ('Release source identity/logging mismatch: ' + $marker) }
    }
    $guide = ''
    foreach ($file in @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'src') -File -Filter 'rr_*.h')) {
        $guide += Get-Content -LiteralPath $file.FullName -Raw
    }
    # r21t retains the r21s hotfix regression gate: the production guide header must make the
    # owned-guide capture recorder visible before its call site. The standalone
    # capture unit test includes this helper directly and therefore cannot catch
    # a missing production include on its own.
    $liveGuides = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_live_guides.h') -Raw
    $slotDeclaration = $liveGuides.IndexOf('struct RRLiveSlot')
    $captureInclude = $liveGuides.IndexOf('#include "rr_live_capture_record.h"')
    $captureCall = $liveGuides.IndexOf('RRLiveCaptureRecord(')
    if ($slotDeclaration -lt 0 -or $captureInclude -le $slotDeclaration -or $captureCall -le $captureInclude) {
        throw 'r21t production capture recorder declaration is not visible before the live-guide call site.'
    }
    $liveRecord = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_live_record.h') -Raw
    $liveRecordTest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/live-guides/record-test.cpp') -Raw
    foreach ($contract in @('#define RR_SEH_TRY __try','#define RR_SEH_EXCEPT(filter) __except(filter)','RR_SEH_TRY {','RR_SEH_EXCEPT(EXCEPTION_EXECUTE_HANDLER)')) {
        if (-not $liveRecord.Contains($contract)) { throw ('r21t SEH wrapper contract missing: ' + $contract) }
    }
    foreach ($contract in @('#define RR_SEH_TRY try','#define RR_SEH_EXCEPT(x) catch(...)')) {
        if (-not $liveRecordTest.Contains($contract)) { throw ('r21t portable live-record exception wrapper missing: ' + $contract) }
    }
    $sehHotfixValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/live-guides/r21t-seh-hotfix-results.json') -Raw | ConvertFrom-Json
    if ($sehHotfixValidation.status -cne 'PASS' -or $sehHotfixValidation.normal -cne 'PASS' -or $sehHotfixValidation.asan_ubsan -cne 'PASS') { throw 'r21t SEH wrapper hotfix portable validation is incomplete.' }
    foreach ($entry in $sehHotfixValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('r21t SEH wrapper hotfix evidence does not match source: ' + $entry.Name)
        }
    }
    foreach ($contract in @('NumDescriptors=4','OffsetInDescriptorsFromTableStart=4','hd.NumDescriptors=7','materialSRV.Buffer.NumElements=static_cast<UINT>(sample.metadata.bytes/8)','materialSRV.Buffer.StructureByteStride=8','CreateShaderResourceView(material,&materialSRV,handle)')) {
        if (-not $liveGuides.Contains($contract)) { throw ('r21t bounded material SRV contract missing: ' + $contract) }
    }
    foreach ($contract in @('SetComputeRoot32BitConstants(0,4,constants,0)','SetComputeRootDescriptorTable(1,s->heap->GetGPUDescriptorHandleForHeapStart())')) {
        if (-not $liveRecord.Contains($contract)) { throw ('r21t RenoDX root binding contract missing: ' + $contract) }
    }
    if ($liveRecord.Contains('SetComputeRootShaderResourceView')) { throw 'r21t must not bind MaterialDataPart1 as an unbounded root SRV.' }
    # Archived r21w reference evidence remains in full source for provenance only. It is not built, packaged, or used by the public r22 release runtime.
    $referenceAddon = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'diagnostic_addon/ControlFG-RenoDXClampRef.cpp') -Raw
    $referenceAbi = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'diagnostic_addon/reshade_minimal_api18.h') -Raw
    $referenceShader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'diagnostic_addon/rr_specular_temporal_clamp.hlsl') -Raw
    $referenceContract = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'diagnostic_addon/reference_clamp_contract.h') -Raw
    $nativeFrame = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_native_frame.h') -Raw
    foreach ($contract in @('ControlFGClampRef_Status','ControlFGClampRef_ReplacementConfirms','ControlFGClampRef_BuildId','TargetInitializedUnreplaced','ReShadeRegisterAddon','ReShadeRegisterEventForAddon')) {
        if (-not ($referenceAddon + $referenceAbi + $referenceContract).Contains($contract)) { throw ('r21w reference add-on contract missing: ' + $contract) }
    }
    foreach ($contract in @('TargetShaderCRC = 0x600347E7u','BuildId[] = "ControlFG-RenoDXClampRef-r21w-api18-renodx120663347"','ReadyMask','RejectMask','TargetInitializedUnreplaced','replacementCrc != 0u')) {
        if (-not $referenceContract.Contains($contract)) { throw ('r21w shared reference contract missing: ' + $contract) }
    }
    foreach ($forbidden in @('NVSDK_NGX','sl.dlss','Streamline','DetourAttach','D3D12CreateDevice','CreateComputePipelineState')) {
        if ($referenceAddon.Contains($forbidden)) { throw ('r21w reference add-on must not own NGX/Streamline/device hooks: ' + $forbidden) }
    }
    foreach ($contract in @('kApiVersion = 18','kEventInitPipeline = 26','kEventCreatePipeline = 27')) {
        if (-not $referenceAbi.Contains($contract)) { throw ('r21w pinned ReShade ABI contract missing: ' + $contract) }
    }
    foreach ($contract in @('FIREFLY_CLAMP 1.0','TEMPORAL_WEIGHT 0.0','register(s11, space1)','register(t3)','register(u0)','2.5')) {
        if (-not $referenceShader.Contains($contract)) { throw ('r21w reference clamp shader contract missing: ' + $contract) }
    }
    if ($nativeFrame.Contains('rr_specular_clamp_device.h') -or $probe.Contains('RR_SPECULAR_CLAMP_DEVICE_HOOK')) { throw 'r21u device-vtable/PSO-clone mechanism must not be present in r31.' }
    # r30 retains the signed-off r21x-r24 native RR path: retain Control's exact 0x600347E7 current-frame clamp and
    # zero only the history contribution by forcing the current-thread
    # g_uCameraCut provider during the exact temporal bind->dispatch span.
    $nativeClamp = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_specular_native_clamp.h') -Raw
    $nativePolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_specular_clamp_policy.h') -Raw
    $userControl = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_user_control.h') -Raw
    foreach ($contract in @('ProviderResolverRva = 0x210270','ProviderMetadataRva = 0x112840','ProviderTlsIndexRva = 0x1115fc','ProviderArenaOffset = 0x4250','SetProviderIatRva = 0x5dff90','CompareProviderRva = 0x16a40','CameraCutName[] = "g_uCameraCut"','SetVerified(id, 1u)','camera_cut_restore_failed')) {
        if (-not $nativeClamp.Contains($contract)) { throw ('r21y native clamp provider contract missing: ' + $contract) }
    }
    foreach ($contract in @('rrNativeClampApi.Initialize(renderer,d3d)','base+0x16744','RRNativeHookTemporalBind','RR_NATIVE_SPECULAR_CLAMP_READY','RR_NATIVE_SPECULAR_CLAMP_CONTAMINATION','mode=native_control_energy_clamp','rrNativeClampApi.Restore(rrNativeClampLease)','raw_copy_fallback')) {
        if (-not $nativeFrame.Contains($contract)) { throw ('r21y native clamp runtime contract missing: ' + $contract) }
    }
    foreach ($contract in @('enum class RRSpecularSignalMode : unsigned int { NativeClamp=0, ReferenceClamp=1 }','RRSpecularSignalMode::NativeClamp','native_control_clamp')) {
        if (-not $userControl.Contains($contract)) { throw ('r21y native clamp user-control contract missing: ' + $contract) }
    }
    foreach ($contract in @('Path::Reference','Path::NativeClamp','Path::RawCopy')) {
        if (-not $nativePolicy.Contains($contract)) { throw ('r21y native clamp policy contract missing: ' + $contract) }
    }
    $dropIn = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Make-DropIn.ps1') -Raw
    $manager = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Manage-Probe.ps1') -Raw
    $buildCmd = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Build.cmd') -Raw
    if (-not $buildCmd.Contains('validation\r21z-preset-default\test.cpp')) { throw 'r30 Build.cmd must retain the dedicated Preset F/E live-switch policy test.' }
    foreach ($contract in @('validation\hdr-fg-ui\policy-test.cpp','validation\hdr-fg-ui\color-test.cpp')) {
        if (-not $buildCmd.Contains($contract)) { throw ('r30 Build.cmd must compile HDR FG validation: ' + $contract) }
    }
    $compactCollector = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Collect-ControlFG-Compact-Logs.ps1') -Raw
    $releaseLogPolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/release_log_policy.h') -Raw
    $perfHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_performance.h') -Raw
    foreach ($contract in @('ReleaseLogSuppressed','verboseAuditLogging','CONTROLFG_VERBOSE_LOG')) {
        if (-not $probe.Contains($contract)) { throw ('r22 concise release logging integration missing: ' + $contract) }
    }
    foreach ($contract in @('ShouldSuppress','CAMERA_VALUES','HUD_RTT_TARGET','SWAPCHAIN_FRAME','NGX_RESOURCE','RR_NOISY_REFLECTION','RR_NOISY_GI','FG_RUNTIME_STATS')) {
        if (-not $releaseLogPolicy.Contains($contract)) { throw ('r22 concise release logging policy missing: ' + $contract) }
    }
    if (-not $buildCmd.Contains('validation\r22-release\log-filter-test.cpp')) { throw 'r22 Build.cmd must compile the release-log policy test.' }
    foreach ($contract in @('sample_every=240','(in.engineFrame%240)!=0','if(frame%240==0||reset||!success)')) {
        if (-not $perfHeader.Contains($contract)) { throw ('r22 performance telemetry cadence contract missing: ' + $contract) }
    }
    foreach ($forbidden in @("'Build.log'","'third_party/streamline/sdk-info.json'","'capture-metadata/'")) {
        if ($compactCollector.Contains($forbidden)) { throw ('r22 compact release collector still packages retired/noisy support artifact: ' + $forbidden) }
    }
    foreach ($contract in @('ControlFG.CompactLogs.v2','build/build-validation.json','installation.json','package/settings.ini')) {
        if (-not $compactCollector.Contains($contract)) { throw ('r22 compact support collector contract missing: ' + $contract) }
    }
    $evaluationEntry = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_evaluation_entry.h') -Raw
    $overlay = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_overlay.h') -Raw
    $reflectionHooks = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_reflection_hooks.h') -Raw
    foreach ($contract in @('RRReflectionInitializeAccessOnly(d3d)','RR_TEMPORAL_ACCESS_INSTALL')) {
        if (-not ($probe + $reflectionHooks).Contains($contract)) { throw ('GI27 temporal-access contract missing: ' + $contract) }
    }
    foreach ($contract in @('specular_mvec=cleared reflection_mvec=cleared hit_distance=cleared matrices=identity','RR_GI30_GEOMETRY_BIND','matrix_mode=identity')) {
        if (-not $evaluationEntry.Contains($contract)) { throw ('PT0 clean RR evaluation contract missing: ' + $contract) }
    }
    foreach ($required in @('RRReflectionInstall(renderer,d3d)','RRReflectionAfterPresent(count)','RRDistanceAfterPresent()','#include "rr_distance_runtime.h"','#include "rr_specular_mv_runtime.h"')) {
        if (-not $probe.Contains($required)) { throw ('GI27 reflection/spec-MV lifetime integration missing: ' + $required) }
    }
    foreach ($required in @('RR_GI30_RENODX_BASELINE','RR_GI30_GEOMETRY_BIND','RRNativeSetGuides(parameters,&bindings,nullptr','hit_distance=cleared','matrices=identity')) {
        if (-not $evaluationEntry.Contains($required)) { throw ('GI30 clean RR baseline evaluation contract missing: ' + $required) }
    }
    foreach ($retiredActive in @('RRSpecMvBeforeEvaluation(','RRDistanceBeforeEvaluation(','fallback_hit_distance','RR_GI27_SPECMV_BIND')) {
        if ($evaluationEntry.Contains($retiredActive)) { throw ('GI30 retired reflection-geometry path is still active: ' + $retiredActive) }
    }
    $specMvRuntime = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_specular_mv_runtime.h') -Raw
    $specMvShader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/shaders/rr_specular_mv.hlsl') -Raw
    $guideParameters = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_guide_parameters.h') -Raw
    foreach ($required in @('RR_GI27_SPECMV_READY','RR_GI27_SPECMV frame=%llu','TakeOnPrimaryQueue','reflection->backend.Destinations()','DXGI_FORMAT_R16G16_FLOAT','mvOutScaleX=0.5f*float(constants.width)/mvScaleX','mvOutScaleY=-0.5f*float(constants.height)/mvScaleY')) {
        if (-not $specMvRuntime.Contains($required)) { throw ('GI27 specular-MV runtime contract missing: ' + $required) }
    }
    foreach ($required in @('virtual_pos = surface_pos + view_dir * effective_t','NdcDelta(virtual_pos) - NdcDelta(surface_pos)','SpecMV[tid.xy] = game_mv + scaled','effective_t = hit_t * gloss * gloss')) {
        if (-not $specMvShader.Contains($required)) { throw ('GI27 reference-derived shader contract missing: ' + $required) }
    }
    if ($specMvShader.Contains('reflect(view_dir')) { throw 'GI27 must use reflected-image virtual position, not physical reflected-ray projection.' }
    foreach ($required in @('api.Resource("GBuffer.SpecularMvec",none)','api.Resource("MotionVectorsReflection",none)','api.Resource("DLSSD.SpecularHitDistance",none)','(void)distance')) {
        if (-not $guideParameters.Contains($required)) { throw ('GI30 cleared optional RR geometry binding contract missing: ' + $required) }
    }
    if ($guideParameters.Contains('Resource* const specMv=') -or $guideParameters.Contains('Resource* const specDistance=')) {
        throw 'GI30 guide binding must not select MV/hit-distance resources.'
    }
    $nativeGuides = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_native_guides.h') -Raw
    foreach ($required in @('bindings.projectionValid=false','identity WorldToView AND identity ViewToClip','Do not inject the')) {
        if (-not $nativeGuides.Contains($required)) { throw ('GI30 identity-matrix guide contract missing: ' + $required) }
    }
    $rrUserControl = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_user_control.h') -Raw
    foreach ($required in @('rrUserSpecularMotion{false}','rrUserDirectDlfParity{true}')) {
        if (-not $rrUserControl.Contains($required)) { throw ('PT0 baseline control contract missing: ' + $required) }
    }
    $pt0 = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/pt0_dxr_probe.h') -Raw
    foreach ($required in @('PT0_DEVICE_HOOKS','PT0_COMMAND_HOOKS','PT0_BUILD_AS','PT0_DXR_PASS','PT0_DISPATCH_CAPTURE','PT0_TLAS_OK','PT0_DESCRIPTOR','PT0_RESOURCE_CENSUS','PT0_PASS_SUMMARY','PT0_SUMMARY','D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE','t[76]','t[75]','t[72]')) {
        if (-not $pt0.Contains($required)) { throw ('PT0 fat DXR probe contract missing: ' + $required) }
    }
    foreach ($required in @('#include "pt0_dxr_probe.h"','InstallRRObservationHooks(renderer,d3d)','PT0EnsureCommandHooks','PT0SetSemantic','PT0Summary(count)','PT0_READY mode=fat_dxr_probe')) {
        if (-not $probe.Contains($required)) { throw ('PT0 integration contract missing: ' + $required) }
    }
    $pt2 = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/pt2_pathtrace.h') -Raw
    $pt2Shader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/shaders/pt2_inline_ray.hlsl') -Raw
    $pt2Compiler = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'tools/Compile-PT2-Ray.ps1') -Raw
    foreach ($required in @('PT2_READY','PT2_TOGGLE','PT2_MODE','PT2_DISPATCH_OK','PT2_COUNTERS','PT2_COMPOSITE_READY','PT2_COMPOSITE_OK','PT2_COMPOSITE_SKIP','SubmitPT2CompositeBeforePresent','PT0LatestTlas()','PT0LatestReflectionSig()','D3D12_RAYTRACING_TIER_1_1')) {
        if (-not $pt2.Contains($required)) { throw ('PT2 runtime contract missing: ' + $required) }
    }
    foreach ($required in @('RayQuery<RAY_FLAG_FORCE_OPAQUE>','TraceRayInline','CommittedRayT','CommittedInstanceID','RWTexture2D<float4> Output','RWStructuredBuffer<uint> Counters','InterlockedAdd','InterlockedMin','InterlockedMax','InterlockedXor')) {
        if (-not $pt2Shader.Contains($required)) { throw ('PT2 inline-ray/counter shader contract missing: ' + $required) }
    }
    foreach ($required in @('-T cs_6_5','-E main','pt2_inline_ray_compiled.h')) {
        if (-not $pt2Compiler.Contains($required)) { throw ('PT2 DXC compiler contract missing: ' + $required) }
    }
    foreach ($required in @('#include "pt2_pathtrace.h"','PT2Poll(presentCount.load())','PT2DeferredReflectionBoundary()','PT2_RUNTIME_READY','PT2IsEnabled()','PT2SetEnabled','PT2SetViewMode','PT2GetOverlayStats')) {
        if (-not ($probe + $overlay).Contains($required)) { throw ('PT2 integration/overlay contract missing: ' + $required) }
    }
    $pt2HdrBridge = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/hdr10_bridge.h') -Raw
    foreach ($required in @('SubmitPT2CompositeBeforePresent(this,true,present)','SubmitPT2CompositeBeforePresent(this,false,present)')) {
        if (-not $pt2HdrBridge.Contains($required)) { throw ('PT2 late-composite HDR/SDR boundary missing: ' + $required) }
    }
    foreach ($required in @('#include "fg_ui_device_identity.h"','FGUISameDevice(a,b,resolveNative)','slGetNativeInterfaceApi','native_identity_compare=1')) {
        if (-not $pt2.Contains($required)) { throw ('PT2 R2 proxy-aware device identity contract missing: ' + $required) }
    }
    foreach ($required in @('Compile-PT2-Ray.ps1','d3d12.lib','d3dcompiler.lib')) {
        if (-not $buildCmd.Contains($required)) { throw ('PT2 build contract missing: ' + $required) }
    }
    foreach ($required in @('L"PathTracing", L"Enabled"','L"PathTracing", L"View"','L"PATH TRACING"','L"PT ON"','L"PT OFF"','FGOverlayPTToggleFromPoint','FGOverlayPTViewFromPoint')) {
        if (-not $overlay.Contains($required)) { throw ('PT2 persisted overlay contract missing: ' + $required) }
    }
    if (-not $reflectionHooks.Contains('RRUserPresetValue()==control_rr::RRPresetF')) { throw 'D1 producer must be F only.' }
    if (-not $liveGuides.Contains('static constexpr bool rrLiveGuideStatsEnabled=false;')) { throw 'r22 guide-stat readback must be compiled out in production.' }
    if (-not $liveGuides.Contains('if(SUCCEEDED(hr)&&rrLiveGuideStatsEnabled)RRLiveCapturePrepare(c);')) { throw 'r22 guide capture allocation must be gated by the production diagnostic constant.' }
    foreach ($forbidden in @('RENODX REFERENCE','FGOverlayRRReferenceFromPoint','FGOverlayRRRawFromPoint')) {
        if ($overlay.Contains($forbidden)) { throw ('r22 public overlay still exposes diagnostic signal controls: ' + $forbidden) }
    }
    foreach ($forbidden in @('NATIVE CLAMP','native_clamp=fixed','preset=E_hidden')) {
        if ($overlay.Contains($forbidden)) { throw ('r22 public overlay still exposes stale diagnostic/fixed-E text: ' + $forbidden) }
    }
    foreach ($contract in @('MODEL','PaintFGButton(dc, rrPresetE, L"E"','PaintFGButton(dc, rrPresetF, L"F"','FGOverlayRRPresetFromPoint','rr_controls=on_off_plus_model_E_F','rr_model_default=F','rr_model_live_switch=E_F','RR_PRESET_UI')) {
        if (-not $overlay.Contains($contract)) { throw ('r22 E/F overlay contract missing: ' + $contract) }
    }
    foreach ($contract in @('inline std::atomic<unsigned int> rrUserPreset{RRPresetF};','RRUserPresetLiveSwitchable(unsigned int value) noexcept {return value==RRPresetE||value==RRPresetF;}')) {
        if (-not $userControl.Contains($contract)) { throw ('r22 Preset F default/live-switch contract missing: ' + $contract) }
    }
    if (-not $nativeFrame.Contains('unsigned preset=control_rr::RRPresetF;')) { throw 'r22 preset epoch must initialize to Preset F.' }
    if (-not $overlay.Contains('GetPrivateProfileIntW(L"RayReconstruction", L"Preset", control_rr::RRPresetF')) { throw 'r22 persisted RR preset must default to F.' }
    if (-not $overlay.Contains('WritePrivateProfileStringW(L"RayReconstruction", L"Preset", rrPresetText')) { throw 'r22 E/F selection must persist.' }
    foreach ($forbidden in @('#include "rr_specular_reference.h"','control_rr_reference::','RR_SPECULAR_REFERENCE_FALLBACK','mode=renodx_reference_dispatch')) {
        if ($nativeFrame.Contains($forbidden)) { throw ('r22 production temporal runtime still contains archived reference logic: ' + $forbidden) }
    }
    if (-not $nativeFrame.Contains('full_renodx_addon_loaded')) { throw 'r22 production temporal path must retain the full-RenoDX contamination fail-closed guard.' }
    $epochStart = $nativeFrame.IndexOf('static bool RRNativeEpochOldWorkRetired() noexcept')
    $epochEnd = $nativeFrame.IndexOf('// r20x:', $epochStart)
    if ($epochStart -lt 0 -or $epochEnd -le $epochStart) { throw 'r22 epoch-retirement function not found.' }
    $epochBody = $nativeFrame.Substring($epochStart, $epochEnd - $epochStart)
    if ($epochBody.Contains('rrReflection') -or $epochBody.Contains('rrDistance')) { throw 'r22 epoch retirement still waits on disabled reflection/distance owners.' }
    foreach ($forbidden in @('ControlFG-RenoDXClampRef.addon64','compile-reference-clamp.cpp')) {
        if ($buildCmd.Contains($forbidden)) { throw ('r22 production build still builds a retired diagnostic/dead stage: ' + $forbidden) }
    }
    if (-not $buildCmd.Contains('validation\evaluation-entry\entry-test.cpp')) { throw 'r22 production build must validate the distance-free RR evaluation gateway.' }
    foreach ($forbidden in @('diagnostics\reshade-reference','ControlFG-RenoDXClampRef.addon64')) {
        if ($dropIn.Contains($forbidden)) { throw ('r22 binary package must not ship the archived reference add-on: ' + $forbidden) }
    }
    if ($manager.Contains("RelativePath = 'ControlFG-RenoDXClampRef.addon64'")) { throw 'r22 installer must not install any reference add-on.' }
    $nativeShaderJsonPath = Join-Path $PSScriptRoot 'research/native-integration-audit/evidence/deferredlight_filtering_specular-000.json'
    $nativeShaderPath = Join-Path $PSScriptRoot 'research/native-integration-audit/shaders/deferredlight_filtering_specular-000.dxbc'
    $nativeShader = Get-Content -LiteralPath $nativeShaderJsonPath -Raw | ConvertFrom-Json
    if ((Get-FileHash -LiteralPath $nativeShaderPath -Algorithm SHA256).Hash -ine $nativeShader.sha256) { throw 'r21y exact Control temporal shader evidence hash mismatch.' }
    $dlFilter = @($nativeShader.constant_buffers | Where-Object { $_.name -ceq 'deferredlight_filtering' })
    if ($dlFilter.Count -ne 1 -or $dlFilter[0].size -ne 48) { throw 'r21y Control deferredlight_filtering cbuffer evidence mismatch.' }
    $cameraCut = @($dlFilter[0].variables | Where-Object { $_.name -ceq 'g_uCameraCut' })
    if ($cameraCut.Count -ne 1 -or $cameraCut[0].offset -ne 32 -or $cameraCut[0].size -ne 4) { throw 'r21y g_uCameraCut offset/size evidence mismatch.' }
    $cameraCutUses = @($nativeShader.instructions | Where-Object { (($_.operands -join ' ') -match 'cb\[1\]\[1\]\[2\]\.x') })
    if ($cameraCutUses.Count -ne 1 -or $cameraCutUses[0].name -cne 'iadd') { throw 'r21y native shader must use g_uCameraCut exactly once, only in the temporal confidence gate.' }
    $sample25 = @($nativeShader.instructions | Where-Object { $_.name -ceq 'sample_l' -and (($_.operands -join ' ') -match 't\[3\]\[3\].*s\[1\]\[11\].*l\(2\.5\)') })
    $cameraGate = @($nativeShader.instructions | Where-Object { $_.name -ceq 'iadd' -and (($_.operands -join ' ') -match '\-cb\[1\]\[1\]\[2\]\.x.*l\(1\)') })
    $historyWeight = @($nativeShader.instructions | Where-Object { $_.name -ceq 'mul' -and (($_.operands -join ' ') -match '0\.600000024') })
    if ($sample25.Count -lt 1 -or $cameraGate.Count -lt 1 -or $historyWeight.Count -lt 1) { throw 'r21y native shader does not prove mip-2.5 clamp plus (1-CameraCut)*0.6 history gate.' }
    $specularAsm = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'research/native-integration-audit/evidence/specular-filter.asm') -Raw
    foreach ($contract in @('180016744:', 'call   0x1801db750', '180016786:', '0x1805dfcc0')) {
        if (-not $specularAsm.Contains($contract)) { throw ('r21y temporal bind/dispatch evidence missing: ' + $contract) }
    }
    $providerClipAsm = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'research/provider-auth/evidence/clip_provider_name.asm') -Raw
    $providerCompareAsm = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'research/provider-auth/evidence/compare_tls.asm') -Raw
    $providerSetAsm = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'research/provider-auth/evidence/whole_provider_set.asm') -Raw
    foreach ($contract in @('18017e2e9:', 'call   0x180210270')) { if (-not $providerClipAsm.Contains($contract)) { throw ('r21y provider resolver proof missing: ' + $contract) } }
    foreach ($contract in @('180016a40:', '0x180112840', '0x1801115fc', '0x4250')) { if (-not $providerCompareAsm.Contains($contract)) { throw ('r21y provider comparator proof missing: ' + $contract) } }
    foreach ($contract in @('180016a10:', 'call   0x180016980')) { if (-not $providerSetAsm.Contains($contract)) { throw ('r21y provider setter proof missing: ' + $contract) } }
    $nativeClampValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/native-specular-clamp/results.json') -Raw | ConvertFrom-Json
    if ($nativeClampValidation.status -cne 'PASS' -or $nativeClampValidation.normal -cne 'PASS' -or $nativeClampValidation.asan_ubsan -cne 'PASS') { throw 'r21y native-clamp portable validation is incomplete.' }
    foreach ($entry in $nativeClampValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('r21y native-clamp validation evidence does not match source: ' + $entry.Name) }
    }
    $referenceValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/reference-clamp/results.json') -Raw | ConvertFrom-Json
    if ($referenceValidation.status -cne 'PASS' -or $referenceValidation.normal -cne 'PASS' -or $referenceValidation.asan_ubsan -cne 'PASS') { throw 'Archived r21w reference-clamp portable validation is incomplete.' }
    foreach ($entry in $referenceValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Archived r21w reference-clamp validation evidence does not match source: ' + $entry.Name)
        }
    }
    foreach ($entry in $currentReleaseValidation.tested_sha256.PSObject.Properties) {
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Current-release evidence does not match source: ' + $entry.Name)
        }
    }
    $specularValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/specular-noisy/results.json') -Raw | ConvertFrom-Json
    if ($specularValidation.status -cne 'LOCAL_PASS' -or -not $specularValidation.tests.normal.StartsWith('PASS reference-dispatch admission') -or -not $specularValidation.tests.asan_ubsan.StartsWith('PASS reference-dispatch admission')) { throw 'Archived r21w specular temporal admission validation is incomplete.' }
    foreach ($entry in $specularValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Archived r21w specular temporal validation evidence does not match retained source: ' + $entry.Name)
        }
    }
    $productionValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r21y-production/results.json') -Raw | ConvertFrom-Json
    if ($productionValidation.status -cne 'LOCAL_PASS' -or -not $productionValidation.tests.evaluation_entry_normal.StartsWith('PASS: r21y gateway') -or -not $productionValidation.tests.evaluation_entry_asan_ubsan.StartsWith('PASS: r21y gateway')) { throw 'r21y production-cleanup portable validation is incomplete.' }
    foreach ($entry in $productionValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('r21y inherited production-cleanup validation evidence does not match retained source: ' + $entry.Name)
        }
    }
    $r21zValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r21z-release/results.json') -Raw | ConvertFrom-Json
    if ($r21zValidation.status -cne 'LOCAL_PASS' -or
        -not $r21zValidation.tests.preset_policy_normal.StartsWith('PASS r21z preset policy') -or
        -not $r21zValidation.tests.preset_policy_asan_ubsan.StartsWith('PASS r21z preset policy')) {
        throw 'Inherited r21z Preset F validation is incomplete.'
    }
    foreach ($entry in $r21zValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Inherited r21z validation evidence does not match retained source: ' + $entry.Name)
        }
    }
    $r22Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r22-release/results.json') -Raw | ConvertFrom-Json
    if ($r22Validation.status -cne 'LOCAL_PASS' -or -not $r22Validation.tests.log_filter.StartsWith('PASS') -or -not $r22Validation.tests.preset_policy.StartsWith('PASS')) { throw 'r22 release cleanup validation is incomplete.' }
    foreach ($entry in $r22Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r22 release validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r23Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r23-hdr-fg-ui/results.json') -Raw | ConvertFrom-Json
    if ($r23Validation.status -cne 'LOCAL_PASS' -or -not $r23Validation.tests.policy_normal.StartsWith('PASS HDR FG UI policy') -or -not $r23Validation.tests.color_normal.StartsWith('PASS HDR HUDless color conversion')) { throw 'r23 HDR FG validation is incomplete.' }
    foreach ($entry in $r23Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r23 HDR FG validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r24Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r24-hdr-fg-state/results.json') -Raw | ConvertFrom-Json
    if ($r24Validation.status -cne 'LOCAL_PASS' -or -not $r24Validation.tests.policy_normal.StartsWith('PASS HDR FG UI policy') -or -not $r24Validation.tests.color_normal.StartsWith('PASS HDR HUDless color conversion')) { throw 'r24 HDR FG state-safety validation is incomplete.' }
    foreach ($entry in $r24Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r24 HDR FG state-safety validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r25Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r25-hdr-transition/results.json') -Raw | ConvertFrom-Json
    if ($r25Validation.status -cne 'LOCAL_PASS' -or -not $r25Validation.tests.policy_normal.StartsWith('PASS HDR transition policy') -or -not $r25Validation.tests.policy_asan_ubsan.StartsWith('PASS HDR transition policy')) { throw 'r25 HDR transition reset validation is incomplete.' }
    foreach ($entry in $r25Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('r25 HDR transition validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r26Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r26-hdr-transition-warmup/results.json') -Raw | ConvertFrom-Json
    if ($r26Validation.status -cne 'LOCAL_PASS' -or -not $r26Validation.tests.policy_normal.StartsWith('PASS HDR transition warmup policy') -or -not $r26Validation.tests.policy_asan_ubsan.StartsWith('PASS HDR transition warmup policy')) { throw 'r26 HDR transition warmup validation is incomplete.' }
    foreach ($entry in $r26Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r26 HDR transition warmup validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r27Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r27-hdr-transition-quiesce/results.json') -Raw | ConvertFrom-Json
    if ($r27Validation.status -cne 'LOCAL_PASS' -or -not $r27Validation.tests.policy_normal.StartsWith('PASS HDR transition quiesce policy') -or -not $r27Validation.tests.policy_asan_ubsan.StartsWith('PASS HDR transition quiesce policy')) { throw 'r27 HDR transition quiesce validation is incomplete.' }
    foreach ($entry in $r27Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r27 HDR transition quiesce validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r28Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r28-display-hdr-prepresent/results.json') -Raw | ConvertFrom-Json
    if ($r28Validation.status -cne 'LOCAL_PASS' -or -not $r28Validation.tests.policy_normal.StartsWith('PASS HDR display-domain pre-Present guard') -or -not $r28Validation.tests.policy_asan_ubsan.StartsWith('PASS HDR display-domain pre-Present guard')) { throw 'r28 display HDR pre-Present validation is incomplete.' }
    foreach ($entry in $r28Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('r28 display HDR pre-Present validation evidence does not match source: ' + $entry.Name) }
    }
    $r29Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r29-fresh-output-hdr/results.json') -Raw | ConvertFrom-Json
    if ($r29Validation.status -cne 'LOCAL_PASS' -or -not $r29Validation.tests.policy_normal.StartsWith('PASS r29 fresh-output HDR guard policy') -or -not $r29Validation.tests.policy_asan_ubsan.StartsWith('PASS r29 fresh-output HDR guard policy')) { throw 'r29 fresh-output HDR guard validation is incomplete.' }
    foreach ($entry in $r29Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('Inherited r29 fresh-output HDR guard validation evidence does not match retained source: ' + $entry.Name) }
    }
    $r30Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r30-hdr-hotkey-guard/results.json') -Raw | ConvertFrom-Json
    if ($r30Validation.status -cne 'LOCAL_PASS' -or -not $r30Validation.tests.policy_normal.StartsWith('PASS r30 HDR hotkey guard policy') -or -not $r30Validation.tests.policy_asan_ubsan.StartsWith('PASS r30 HDR hotkey guard policy')) { throw 'r30 HDR hotkey guard validation is incomplete.' }
    foreach ($entry in $r30Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('r30 HDR hotkey guard validation evidence does not match source: ' + $entry.Name) }
    }
    $r31Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r31-hdr-hard-reset/results.json') -Raw | ConvertFrom-Json
    if ($r31Validation.status -cne 'LOCAL_PASS' -or -not $r31Validation.tests.policy_normal.StartsWith('PASS r31c HDR hard-reset policy') -or -not $r31Validation.tests.policy_asan_ubsan.StartsWith('PASS r31c HDR hard-reset policy')) { throw 'r31 HDR hard-reset validation is incomplete.' }
    foreach ($entry in $r31Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) { throw ('r31 HDR hard-reset validation evidence does not match source: ' + $entry.Name) }
    }
    $fgUi = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_ui_recomposition.h') -Raw
    $slBridge = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/streamline_bridge.h') -Raw
    $hdrUiPolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_hdr_ui_policy.h') -Raw
    foreach ($contract in @('FG_HDR_HUDLESS_PIPELINE_READY','FG_HDR_HUDLESS_CONVERT','kFGHdrHudlessComputeShader','hdrSlots[3]','DXGI_FORMAT_R10G10B10A2_UNORM','D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS','D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE','conversion=compute_uav no_graphics_state=1 private_command_list=1','taggedHudless=s.hdr10?s.hdrHudless:s.hudless')) {
        if (-not $fgUi.Contains($contract)) { throw ('r24 HDR HUDless compute conversion contract missing: ' + $contract) }
    }
    foreach ($forbidden in @('OMSetRenderTargets(','SetGraphicsRootSignature(','SetGraphicsRootDescriptorTable(','RSSetViewports(','RSSetScissorRects(','IASetPrimitiveTopology(','DrawInstanced(')) {
        if ($fgUi.Contains($forbidden)) { throw ('r24 HDR HUDless path must not mutate native graphics command-list state: ' + $forbidden) }
    }
    foreach ($contract in @('SetComputeRootSignature(o->hdrRoot)','SetComputeRootDescriptorTable(0,gpu)','list->Dispatch((s.width+7)/8,(s.height+7)/8,1)','SetComputeRootSignature(o->root)','SetPipelineState(o->pipeline)','private_command_list=1')) {
        if (-not $fgUi.Contains($contract)) { throw ('r24 HDR compute state/restore contract missing: ' + $contract) }
    }
    foreach ($contract in @('list=s.work.list','descriptorSlot=slotIndex+(s.hdr10?3u:0u)','hd.NumDescriptors=18','s.work.Begin(o->device,target)','SubmitFGUIRecompositionBeforePresent','s.work.Submit(directQueue,present)')) {
        if (-not $fgUi.Contains($contract)) { throw ('R11 private UI work contract missing: ' + $contract) }
    }
    foreach ($contract in @('control_fg_hdr_ui::ShouldEnableRecomposition','control_fg_hdr_ui::HudlessOptionFormat','hdr10_domain_ready=%u','options.hudLessBufferFormat = control_fg_hdr_ui::HudlessOptionFormat')) {
        if (-not $slBridge.Contains($contract)) { throw ('r24 HDR FG Streamline contract missing: ' + $contract) }
    }
    foreach ($contract in @('HudlessDomainReady','ShouldEnableRecomposition','HudlessOptionFormat')) {
        if (-not $hdrUiPolicy.Contains($contract)) { throw ('r24 HDR FG policy source missing: ' + $contract) }
    }
    $hdrTransitionPolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_hdr_transition_policy.h') -Raw
    $hdrHotkeyPolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_hdr_hotkey_policy.h') -Raw
    $hdrHardResetPolicy = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_hdr_hard_reset_policy.h') -Raw
    $hdrBridge = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/hdr10_bridge.h') -Raw
    $slFrame = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/streamline_frame.h') -Raw
    $overlaySource = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/fg_overlay.h') -Raw
    foreach ($contract in @('DisplayDomainFlip','HoldForPreResizeDisplayTransition','BridgeTransitionObserved','TransitionPending','NextFreshFrameCount','WarmupComplete','kFreshFramesRequired = 2','CanEnable','NeedsCommittedOffPresent','QuiesceCommitted')) {
        if (-not $hdrTransitionPolicy.Contains($contract)) { throw ('r31 HDR transition policy source missing: ' + $contract) }
    }
    foreach ($contract in @('ObserveSLDisplayHdrDomainBeforePresent','GetSLFreshDisplayDescForSwapChain','IDXGIFactory1','IsCurrent()','EnumAdapters1','EnumOutputs','MonitorFromWindow','IDXGIOutput6','GetDesc1','FG_DISPLAY_FACTORY_REFRESH','reason=%s','source=fresh_native_factory','FG_DISPLAY_DOMAIN_CHANGE','FG_DISPLAY_DOMAIN_GATE','FG_DISPLAY_DOMAIN_HANDOFF','slFgDisplayTransitionPending','slFgDisplayTransitionBridgeGeneration','QuiesceDLSSGForHdrSwapchainTransition','FG_HDR_TRANSITION_QUIESCE','swapChain->Present(0, 0)','FG_HDR_TRANSITION_GATE','FG_HDR_TRANSITION_SETTLED','slFgHdrTransitionSettledGeneration','slFgHdrTransitionWarmupFrames')) {
        if (-not $slBridge.Contains($contract)) { throw ('r31 HDR transition Streamline contract missing: ' + $contract) }
    }
    foreach ($contract in @('enum class Stage','OffCommittedWaitKeyRelease','ReplayedWaitHdrTransition','ChooseReplayMode','BridgeCanTakeOwnership','ShouldHoldFG')) {
        if (-not $hdrHotkeyPolicy.Contains($contract)) { throw ('r30 retained HDR hotkey policy provenance missing: ' + $contract) }
    }
    foreach ($contract in @('enum class Stage','OffQueuedWaitPresent','ResourcesFreedWaitBridge','CanFreeAfterPresent','CanRearm')) {
        if (-not $hdrHardResetPolicy.Contains($contract)) { throw ('r31 HDR hard-reset policy source missing: ' + $contract) }
    }
    foreach ($contract in @('slFreeResourcesApi','GetProcAddress(slInterposerModule, "slFreeResources")','BeginSLDLSSGHardReset','CompleteSLDLSSGHardResetAfterPresent','InvalidateSLDLSSGStateAfterHardReset','FG_HDR_HARD_RESET_BEGIN','FG_HDR_HARD_RESET_OFF_COMMIT','FG_HDR_HARD_RESET_FREE','FG_HDR_HARD_RESET_GATE','FG_HDR_HARD_RESET_REARM','control_fg_hdr_hard_reset::CanFreeAfterPresent','control_fg_hdr_hard_reset::CanRearm')) {
        if (-not $slBridge.Contains($contract)) { throw ('r31 HDR hard-reset Streamline contract missing: ' + $contract) }
    }
    # Compile-contract regression from the first r31 Windows/MSVC attempt: the
    # hard-reset invalidation path must reset the already-declared shared
    # backbuffer-index tracker. A misspelled slFgLastBackBufferIndex token is
    # undeclared and caused C2065 before the target build could start.
    if (-not $slBridge.Contains('static std::atomic<unsigned int> slLastBackBufferIndex{0xFFFFFFFFu};')) { throw 'r31 shared backbuffer-index declaration is missing.' }
    if (-not $slBridge.Contains('slLastBackBufferIndex.store(0xFFFFFFFFu, std::memory_order_release);')) { throw 'r31 hard-reset invalidation must reset slLastBackBufferIndex.' }
    if ($slBridge.Contains('slFgLastBackBufferIndex')) { throw 'r31 compile regression: undeclared slFgLastBackBufferIndex token must not exist.' }
    foreach ($contract in @('policy=disabled_r31_display_or_resize_detected_hard_dlssg_reset','fgOverlayKeyboardHook = nullptr')) {
        if (-not $overlaySource.Contains($contract)) { throw ('r31 HDR no-hotkey-interception contract missing: ' + $contract) }
    }
    if ($hdrBridge.Contains('ServiceSLHdrHotkeyGuardBeforePresent') -or $hdrBridge.Contains('CompleteSLHdrHotkeyGuardAfterPresent')) { throw 'r31 HDR bridge must not service the retired r30 hotkey serialization path.' }
    if ($overlaySource.Contains('SetWindowsHookExW(')) { throw 'r31 overlay must not install a low-level keyboard hook for HDR.' }
    $observerSignature = 'static void ObserveSLDisplayHdrDomainBeforePresent(IDXGISwapChain* swapChain, unsigned long long present) noexcept {'
    $observerStart = $slBridge.IndexOf($observerSignature)
    $observerEnd = $slBridge.IndexOf('static void SetDLSSGModeForPresent', $observerStart)
    if ($observerStart -lt 0 -or $observerEnd -le $observerStart) { throw 'r31 HDR observer source range is missing.' }
    $observerText = $slBridge.Substring($observerStart, $observerEnd - $observerStart)
    if ($observerText.Contains('swapChain->GetContainingOutput')) { throw 'r31 retained fresh-output HDR observer must not use stale swapChain->GetContainingOutput().'  }
    foreach ($contract in @('hdr10BridgeTransitionGeneration','GetHdr10BridgeTransitionGeneration','ObserveSLDisplayHdrDomainBeforePresent(inner_','QuiesceDLSSGForHdrSwapchainTransition(inner_','sdr_to_hdr_resize','hdr_to_sdr_resize','sdr_to_hdr_resize1','hdr_to_sdr_resize1')) {
        if (-not $hdrBridge.Contains($contract)) { throw ('r31 HDR bridge transition contract missing: ' + $contract) }
    }
    foreach ($forbidden in @('FG_HDR_TRANSITION_RESET','effectiveResetValue = hdrTransitionReset ? 1 : resetValue','slFgHdrTransitionResetGeneration')) {
        if ($slFrame.Contains($forbidden) -or $slBridge.Contains($forbidden)) { throw ('r26 must not retain the regressive r25 forced-reset path: ' + $forbidden) }
    }
    if (-not $buildCmd.Contains('validation\r26-hdr-transition-warmup\policy-test.cpp')) { throw 'r29 Build.cmd must retain the r26 HDR transition warmup policy test.' }
    if (-not $buildCmd.Contains('validation\r27-hdr-transition-quiesce\policy-test.cpp')) { throw 'r29 Build.cmd must retain the r27 resize-side quiesce fallback test.' }
    if (-not $buildCmd.Contains('validation\r28-display-hdr-prepresent\policy-test.cpp')) { throw 'r29 Build.cmd must retain the r28 stale-output regression policy test.' }
    if (-not $buildCmd.Contains('validation\r29-fresh-output-hdr\policy-test.cpp')) { throw 'r29 Build.cmd must compile the fresh-output HDR guard policy test.' }
    if (-not $buildCmd.Contains('validation\r30-hdr-hotkey-guard\policy-test.cpp')) { throw 'r31 Build.cmd must retain the r30 historical hotkey policy test.' }
    if (-not $buildCmd.Contains('validation\r31-hdr-hard-reset\policy-test.cpp')) { throw 'r31 Build.cmd must compile the HDR hard-reset policy test.' }
    if ($fgUi.Contains('if(IsHdr10BridgeActive()||!finalColor')) { throw 'r24 must not skip FGUI recomposition merely because HDR10 bridge is active.' }
    if ($slBridge.Contains('enable && !IsHdr10BridgeActive()')) { throw 'r24 must not blanket-disable DLSS-G UI recomposition in HDR.' }
    $buildMetadataText = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'Build-Metadata.ps1') -Raw
    foreach ($contract in @('FGHDRHUDLessEnabled = $true',"FGHDRHUDLessTaggedFormat = 'DXGI_FORMAT_R10G10B10A2_UNORM_HDR10_BT2100_PQ'",'FGHDRUIRecompositionEnabled = $true',"FGHDRHUDLessCommandPath = 'private_direct_list_fence_retired_per_slot_descriptors'",'FGHDRHUDLessUAVSupportRequired = $true',"FGHDRTransitionPolicy = 'detect_display_or_resize_hdr_domain_change_commit_dlssg_eoff_on_real_present_slFreeResources_viewport_invalidate_fg_caches_then_bridge_rebuild_plus_two_fresh_frames'","FGHDRTransitionReset = 'r31_hard_dlssg_resource_reset_slFreeResources_not_r25_constants_reset'","FGHDRTransitionOffCommit = 'fresh_output_or_resize_detection_queues_eoff_real_present_commits_off_then_slFreeResources_releases_dlssg_viewport_before_rearm'","FGHDRTransitionDetection = 'native_IDXGIFactory1_fresh_output_observer_plus_Control_HDR_bridge_ResizeBuffers_domain_transition_no_keyboard_interception'",'FGHDRTransitionFreshFrames = 2',"FGHDRTransitionReenable = 'after_resource_free_bridge_generation_change_and_two_fresh_tagged_frames_or_after_30_present_no_bridge_grace_plus_two_fresh_tagged_frames_then_clean_options_recreate_on_next_present'")) {
        if (-not $buildMetadataText.Contains($contract)) { throw ('r31 HDR build metadata missing: ' + $contract) }
    }
    $reflectance = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'src/rr_reflectance.h') -Raw
    foreach ($contract in @('CopyRGBA16FToDiffuseReflectance','0x3cu','DecodeControlMaterialF0','EnvBRDFApprox2','zeroWeight','SpecularReflectanceFromLinearRoughness')) {
        if (-not $reflectance.Contains($contract)) { throw ('G12 reflectance source contract missing: ' + $contract) }
    }
    $reflectanceValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/g12-current/reflectance-validation.json') -Raw | ConvertFrom-Json
    if ($reflectanceValidation.status -cne 'PASS' -or $reflectanceValidation.normal -cne 'PASS' -or
        $reflectanceValidation.asan_ubsan -cne 'PASS' -or $reflectanceValidation.assertions.Count -ne 8 -or
        $reflectanceValidation.matrix_reference_cases -ne 101505 -or $reflectanceValidation.rr_evaluation_enabled -ne $false) {
        throw 'G12 reflectance numerical/sanitizer validation is incomplete.'
    }
    foreach ($entry in $reflectanceValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Reflectance test evidence does not match source: ' + $entry.Name)
        }
    }
    foreach ($marker in $ControlFGGuideMarkers) {
        # Includes r3 read-only Part1 diagnostic markers; binary verifier shares this list.
        if (-not ($probe + $guide).Contains($marker)) { throw ('G12 source marker missing: ' + $marker) }
    }
    foreach ($marker in @('RRGuideInitializeInputs','RRGuideTryCapture','RRGuideAfterPresent','RRAlbedoInstallHooks')) {
        if (-not $probe.Contains($marker)) { throw ('G12 hook wiring missing: ' + $marker) }
    }
    foreach ($value in @($ControlFGCapture.DirectoryPrefix, $ControlFGCapture.Schema, $ControlFGCapture.CompletedStatus, $ControlFGCapture.MetadataName, $ControlFGCapture.NativeTargetPrefix, $ControlFGCapture.ShaderPrefix) + @($ControlFGCapture.Images | ForEach-Object { $_.Name })) {
        if (-not $guide.Contains($value)) { throw ('Exporter/collector capture contract mismatch: ' + $value) }
    }
    $optionValidation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/native-option-isolation/results.json') -Raw | ConvertFrom-Json
    if ($optionValidation.status -cne 'PASS') { throw 'r20n native-option isolation portable tests incomplete.' }
    foreach ($entry in $optionValidation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Native-option isolation test evidence does not match source: ' + $entry.Name)
        }
    }
    $part1Validation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/part1/validation.json') -Raw | ConvertFrom-Json
    if ($part1Validation.status -cne 'PASS') { throw 'Part1 portable tests incomplete.' }
    foreach ($entry in $part1Validation.tested_sha256.PSObject.Properties) {
        if ($currentReleasePaths -contains $entry.Name) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Part1 test evidence does not match source: ' + $entry.Name)
        }
    }
    foreach ($retired in @('#include "rr_discovery.h"','PrepareNativeNgxPostSetupHook(', 'CaptureNativeRRNgxPostSetup(', 'HookRRNativeP9RaytracingShaderCtor', 'CaptureRRPhase9RTWindowSnapshot(')) {
        if ($probe.Contains($retired)) { throw ('Retired discovery code is still active: ' + $retired) }
    }
    $frozenDelta = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'validation/r31-frozen-core-delta.json') -Raw | ConvertFrom-Json
    if ($frozenDelta.status -cne 'PASS' -or $frozenDelta.total_count -ne 11 -or $frozenDelta.unchanged_count -ne 9) { throw 'r31 frozen-core delta evidence is incomplete.' }
    $deltaPaths = @($frozenDelta.changed_files | ForEach-Object { $_.Path } | Sort-Object)
    if ($deltaPaths.Count -ne 2 -or $deltaPaths[0] -cne 'src/hdr10_bridge.h' -or $deltaPaths[1] -cne 'src/streamline_bridge.h') { throw 'r31 frozen-core delta must contain exactly hdr10_bridge.h and streamline_bridge.h relative to r30.' }
    foreach ($entry in $frozenDelta.changed_files) {
        # Diagnostic revisions are verified against the current checkpoint above.
        if ($currentReleasePaths -contains $entry.Path) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Path) -Algorithm SHA256).Hash -ine $entry.R31SHA256) {
            throw ('r31 frozen-core delta does not match current source: ' + $entry.Path)
        }
    }
    $frozen = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'g3-frozen-files.json') -Raw | ConvertFrom-Json
    foreach ($file in $frozen.Files) {
        if ($currentReleasePaths -contains $file.Path) { continue }
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $file.Path) -Algorithm SHA256).Hash -ne $file.SHA256) {
            throw ('G12 frozen baseline changed: ' + $file.Path)
        }
    }
    # Manifest is regenerated after all package files and local validation reports are final.
    $sourceManifest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'SOURCE-SHA256.json') -Raw | ConvertFrom-Json
    foreach ($entry in $sourceManifest.PSObject.Properties) {
        if ((Get-FileHash -LiteralPath (Join-Path $PSScriptRoot $entry.Name) -Algorithm SHA256).Hash -ine $entry.Value) {
            throw ('Source package hash mismatch: ' + $entry.Name)
        }
    }
    foreach ($name in @('Verify-Build.ps1','Manage-Probe.ps1','Make-DropIn.ps1')) {
        $text = Get-Content -LiteralPath (Join-Path $PSScriptRoot $name) -Raw
        if (-not $text.Contains('Build-Metadata.ps1') -or -not $text.Contains('Assert-ControlFGBuildValidation')) {
            throw ('Shared validation contract missing from ' + $name)
        }
    }
    Write-Host 'PASS: Clean Native R12 MFG Dynamic Test source identity and hashes; diagnostic instrumentation included. Runtime validation pending.'
} catch { Write-Error $_; exit 1 }
