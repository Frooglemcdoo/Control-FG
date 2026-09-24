from pathlib import Path
import hashlib, json, re, sys

root = Path(sys.argv[1] if len(sys.argv) > 1 else '.').resolve()
checks = []

def ck(name, value):
    checks.append((name, bool(value)))
    if not value:
        raise SystemExit('FAIL: ' + name)

required = [
    'README.md','INSTALL.md','BUILDING.md',
    'RELEASE_NOTES.md','NEXUS_MODS_DESCRIPTION.txt',
    'Build.cmd','Build-Metadata.ps1','Verify-Source.ps1','Verify-Build.ps1','Make-DropIn.ps1',
    'SOURCE-SHA256.json','ARTIFACT-SHA256.json','g3-frozen-files.json','validation/r29-frozen-core-delta.json','validation/r30-frozen-core-delta.json','validation/r31-frozen-core-delta.json',
    
    'src/probe.cpp','src/fg_overlay.h','src/rr_user_control.h','src/rr_native_frame.h',
    'src/rr_evaluation_entry.h','src/rr_reflection_hooks.h','src/rr_live_guides.h','src/rr_live_retire.h',
    'src/release_log_policy.h','src/fg_hdr_ui_policy.h','validation/r22-release/log-filter-test.cpp','validation/r22-release/results.json','validation/hdr-fg-ui/policy-test.cpp','validation/hdr-fg-ui/color-test.cpp','validation/r23-hdr-fg-ui/results.json','validation/r24-hdr-fg-state/results.json','validation/r25-hdr-transition/policy-test.cpp','validation/r25-hdr-transition/results.json','validation/r26-hdr-transition-warmup/policy-test.cpp','validation/r26-hdr-transition-warmup/results.json','validation/r27-hdr-transition-quiesce/policy-test.cpp','validation/r27-hdr-transition-quiesce/results.json','validation/r28-display-hdr-prepresent/policy-test.cpp','validation/r28-display-hdr-prepresent/results.json','validation/r29-fresh-output-hdr/policy-test.cpp','validation/r29-fresh-output-hdr/results.json','validation/r30-hdr-hotkey-guard/policy-test.cpp','validation/r30-hdr-hotkey-guard/results.json','validation/r31-hdr-hard-reset/policy-test.cpp','validation/r31-hdr-hard-reset/results.json','validation/current-release/results.json','src/fg_hdr_hotkey_policy.h','src/fg_hdr_hard_reset_policy.h',
    'validation/evaluation-entry/entry-test.cpp','validation/r21z-preset-default/test.cpp','validation/r21z-release/results.json'
]
ck('required-files', all((root / p).is_file() for p in required))

current_release = json.loads((root/'validation/current-release/results.json').read_text(encoding='utf-8'))
ck('current-release-evidence-status', current_release.get('status') == 'LOCAL_PASS' and str((current_release.get('tests') or {}).get('portable_gate','')).startswith('PASS'))
ck('current-release-evidence-hashes', all(
    (root/name).is_file() and hashlib.sha256((root/name).read_bytes()).hexdigest().lower() == expected.lower()
    for name, expected in (current_release.get('tested_sha256') or {}).items()
))

metadata = (root/'Build-Metadata.ps1').read_text(encoding='utf-8-sig')
probe = (root/'src/probe.cpp').read_text(encoding='utf-8-sig')
overlay = (root/'src/fg_overlay.h').read_text(encoding='utf-8-sig')
verify = (root/'Verify-Build.ps1').read_text(encoding='utf-8-sig')
build = (root/'Build.cmd').read_text(encoding='utf-8-sig')
dropin = (root/'Make-DropIn.ps1').read_text(encoding='utf-8-sig')
manager = (root/'Manage-Probe.ps1').read_text(encoding='utf-8-sig')
source_verify = (root/'Verify-Source.ps1').read_text(encoding='utf-8-sig')

# Regression gate: historical milestone evidence is provenance only for any path
# owned by the authoritative current-release checkpoint. Every historical
# tested_sha256 loop in Verify-Source.ps1 must skip current-release-owned paths.
historical_loops = re.findall(
    r'foreach \(\$entry in \$(?!currentReleaseValidation)([A-Za-z0-9_]+)\.tested_sha256\.PSObject\.Properties\) \{',
    source_verify)
guarded_historical_loops = re.findall(
    r'foreach \(\$entry in \$(?!currentReleaseValidation)([A-Za-z0-9_]+)\.tested_sha256\.PSObject\.Properties\) \{\s*if \(\$currentReleasePaths -contains \$entry\.Name\) \{ continue \}',
    source_verify)
ck('historical-evidence-current-release-ownership',
   bool(historical_loops) and len(historical_loops) == len(guarded_historical_loops)
   and source_verify.index('$currentReleasePaths = @(') < source_verify.index('foreach ($entry in $sehHotfixValidation.tested_sha256.PSObject.Properties)'))

ck('r31-identity', all(x in metadata for x in [
    "Version = '2.0.0-RR-Native-G12-r31c'",
    "SourceRevision = 'r31c-hdr-dlssg-hard-reset-display-only-rearm'"
]) and all(x in probe for x in [
    'PROBE v2.0.0 internal_build=2.0.0-RR-Native-G12-r31c',
    'source_revision=r31c-hdr-dlssg-hard-reset-display-only-rearm'
]))

# Public UI exposes only production RR controls: ON/OFF plus a live E/F model
# comparison. The internal native clamp implementation remains hidden.
ck('full-fg-overlay-size', all(x in overlay for x in ['kFGOverlayHeight = 730','kFGOverlayCompactHeight = 510','kFGOverlayDynamicSectionHeight = 220']))
ck('full-fg-visible-controls', all(x in overlay for x in [
    'L"MODE"','L"EFFECTIVE"','L"CURRENT FPS"','L"HDR"','L"GPU MAX"',
    'L"FRAME GENERATION"','L"DYNAMIC TARGET FPS"','L"AUTO"','L"MANUAL"',
    'PaintFGTargetSlider','FGOverlayAutoTargetFromPoint','FGOverlayManualTargetFromPoint',
    'FGOverlaySliderFromPoint','FG_OVERLAY_DYNAMIC_TARGET',
    'L"RAY RECONSTRUCTION"','FGOverlayRRToggleFromPoint','RR_TOGGLE','v2.0.0',
    'L"MODEL"','PaintFGButton(dc, rrPresetE, L"E"','PaintFGButton(dc, rrPresetF, L"F"',
    'FGOverlayRRPresetFromPoint','RR_PRESET_UI'
]))
ck('dynamic-section-only-when-dynamic-selected', all(x in overlay for x in [
    'const bool dynamicSectionVisible = IsFGDynamicSelection(selected);',
    'if (dynamicSectionVisible) {',
    'L"DYNAMIC TARGET FPS"', 'L"AUTO"', 'L"MANUAL"', 'PaintFGTargetSlider',
    'return IsFGDynamicSelection(GetFGUserMultiplier()) && y >= 386 && y < 434',
    'return IsFGDynamicSelection(GetFGUserMultiplier()) && y >= 443 && y < 512',
    'if (!IsFGDynamicSelection(GetFGUserMultiplier())) return;',
    'if (fgOverlaySliderDragging && !IsFGDynamicSelection(GetFGUserMultiplier()))'
]))
ck('dynamic-section-layout-collapse', all(x in overlay for x in [
    'const int lowerSectionOffset = dynamicSectionVisible ? 0 : -kFGOverlayDynamicSectionHeight;',
    'RECT footerRc{30, 556 + lowerSectionOffset, 610, 580 + lowerSectionOffset};',
    'RECT rrTitle{30, 614 + lowerSectionOffset, 510, 642 + lowerSectionOffset};',
    'FGOverlayDesiredHeight() noexcept { return IsFGDynamicSelection(GetFGUserMultiplier()) ? kFGOverlayHeight : kFGOverlayCompactHeight; }',
    'FGOverlayResizeWindowForCurrentSelection(hwnd);',
    'kFGOverlayWidth, FGOverlayDesiredHeight()'
]))
ck('diagnostic-rr-controls-hidden', all(x not in overlay for x in [
    'L"PARTIAL"','L"SKIN / SSS TEST"','L"AA SHARPNESS  (SR / DLAA)"',
    'FGOverlayRRModeFromPoint','L"RENODX REFERENCE"',
    'FGOverlayRRReferenceFromPoint','FGOverlayRRRawFromPoint','L"NATIVE CLAMP"'
]))
ck('public-rr-model-ef', all(x in overlay for x in [
    'control_rr::RRPresetE','control_rr::RRPresetF','RRUserSetPresetValue',
    'rr_controls=on_off_plus_model_E_F','rr_model_default=F','rr_model_live_switch=E_F'
]))

user_control = (root/'src/rr_user_control.h').read_text(encoding='utf-8-sig')
native_frame = (root/'src/rr_native_frame.h').read_text(encoding='utf-8-sig')
native_preset = (root/'src/rr_native_preset.h').read_text(encoding='utf-8-sig')
ck('preset-f-release-default', 'rrUserPreset{RRPresetF}' in user_control and 'unsigned preset=control_rr::RRPresetF;' in native_frame and 'default_preset=F selectable=E,F,K,L,M' in native_preset)
ck('preset-ef-live-switch', 'RRUserPresetLiveSwitchable(unsigned int value) noexcept {return value==RRPresetE||value==RRPresetF;}' in user_control)
ck('settings-schema8', 'L"Schema", L"8"' in overlay and 'RayReconstruction", L"Enabled"' in overlay and 'RayReconstruction", L"Preset"' in overlay and 'FrameGeneration", L"DynamicTargetFPS"' in overlay)
ck('verifier-current-ui-contract', all(x in verify for x in [
    'dynamic_target_ui=auto_manual_thick_slider','rr_preset_selector=public_E_F',
    'rr_model_default=F','rr_model_live_switch=E_F','RR_PRESET_UI',
    'overlay_status=selected,effective,current_fps,hdr,capability',
    'controls=auto,manual,thick_slider_30_1000','rr_controls=on_off_plus_model_E_F'
]))
ck('preset-test-built', 'r21z-preset-default' in build and 'test.cpp' in build)
release_log_policy = (root/'src/release_log_policy.h').read_text(encoding='utf-8-sig')
ck('release-log-filter', all(x in probe for x in ['ReleaseLogSuppressed','CONTROLFG_VERBOSE_LOG']) and all(x in release_log_policy for x in ['ShouldSuppress','CAMERA_VALUES','HUD_RTT_TARGET','SWAPCHAIN_FRAME','NGX_RESOURCE','RR_NOISY_REFLECTION']))
ck('release-log-test-built', 'r22-release' in build and 'log-filter-test.cpp' in build)
ck('hdr-fg-tests-built', 'hdr-fg-ui\\policy-test.cpp' in build and 'hdr-fg-ui\\color-test.cpp' in build)
perf = (root/'src/rr_performance.h').read_text(encoding='utf-8-sig')
collector = (root/'Collect-ControlFG-Compact-Logs.ps1').read_text(encoding='utf-8-sig')
ck('release-perf-cadence', all(x in perf for x in ['sample_every=240','(in.engineFrame%240)!=0','if(frame%240==0||reset||!success)']))
ck('compact-support-v2', 'ControlFG.CompactLogs.v2' in collector and 'Build.log' not in collector and 'third_party/streamline/sdk-info.json' not in collector and 'capture-metadata/' not in collector)

# HDR FG candidate extends the signed-off SDR UI-recomposition path by converting
# the pre-UI FP16/scRGB snapshot into the same RGB10/PQ/BT.2020 domain as the
# intercepted HDR backbuffer before tagging it for DLSS-G.
recompose = (root/'src/fg_ui_recomposition.h').read_text(encoding='utf-8')
sl_bridge = (root/'src/streamline_bridge.h').read_text(encoding='utf-8')
hdr_policy = (root/'src/fg_hdr_ui_policy.h').read_text(encoding='utf-8')
ck('fg-ui-r10-support', 'DXGI_FORMAT_R10G10B10A2_UNORM' in recompose and 'FG_UI_RECOMPOSE_SKIP' in recompose)
ck('fg-hdr-hudless-conversion', all(x in recompose for x in ['FG_HDR_HUDLESS_PIPELINE_READY','FG_HDR_HUDLESS_CONVERT','kFGHdrHudlessComputeShader','hdrSlots[3]','target_space=bt2020_pq','D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS','D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE','conversion=compute_uav no_graphics_state=1 restore_r22_compute_state=1']))
ck('fg-ui-recompose-runtime', all(x in probe+recompose for x in ['FG_UI_RECOMPOSE_READY','FG_UI_HUDLESS_SNAPSHOT','FG_UI_RECOMPOSE_TAG','resource_tags=depth_mv_plus_private_hudless_plus_ui_alpha_sdr_and_hdr10','hdr10_resource_tags=depth_mv_plus_rgb10_pq_hudless_plus_ui_alpha']))
ck('fg-hdr-recompose-options', all(x in sl_bridge for x in ['control_fg_hdr_ui::ShouldEnableRecomposition','control_fg_hdr_ui::HudlessOptionFormat','hdr10_domain_ready=%u']))
ck('fg-hdr-policy', all(x in hdr_policy for x in ['HudlessDomainReady','ShouldEnableRecomposition','HudlessOptionFormat']))
ck('fg-hdr-no-blanket-disable', 'enable && !IsHdr10BridgeActive()' not in sl_bridge and 'if(IsHdr10BridgeActive()||!finalColor' not in recompose)
ck('fg-hdr-no-graphics-state-mutation', all(x not in recompose for x in ['OMSetRenderTargets(','SetGraphicsRootSignature(','SetGraphicsRootDescriptorTable(','RSSetViewports(','RSSetScissorRects(','IASetPrimitiveTopology(','DrawInstanced(']))
ck('fg-hdr-restores-r22-compute-state', all(x in recompose for x in ['SetComputeRootSignature(o->hdrRoot)','SetComputeRootDescriptorTable(0,gpu)','SetComputeRootSignature(o->root)','SetPipelineState(o->pipeline)','restore_r22_compute_state=1']))
ck('fg-hdr-build-metadata', all(x in metadata for x in ['FGHDRHUDLessEnabled = $true',"FGHDRHUDLessTaggedFormat = 'DXGI_FORMAT_R10G10B10A2_UNORM_HDR10_BT2100_PQ'",'FGHDRUIRecompositionEnabled = $true',"FGHDRHUDLessCommandPath = 'compute_only_no_graphics_state_restore_r22_compute_state'",'FGHDRHUDLessUAVSupportRequired = $true']))

hdr_transition = (root/'src/fg_hdr_transition_policy.h').read_text(encoding='utf-8-sig')
hdr_bridge = (root/'src/hdr10_bridge.h').read_text(encoding='utf-8-sig')
sl_frame = (root/'src/streamline_frame.h').read_text(encoding='utf-8-sig')
ck('fg-hdr-transition-policy', all(x in hdr_transition for x in ['DisplayDomainFlip','HoldForPreResizeDisplayTransition','BridgeTransitionObserved','TransitionPending','NextFreshFrameCount','WarmupComplete','kFreshFramesRequired = 2','CanEnable','NeedsCommittedOffPresent','QuiesceCommitted']))
ck('fg-hdr-hard-reset-runtime', all(x in sl_bridge+hdr_bridge for x in [
    'ObserveSLDisplayHdrDomainBeforePresent','GetSLFreshDisplayDescForSwapChain','IDXGIFactory1','IsCurrent()',
    'BeginSLDLSSGHardReset','CompleteSLDLSSGHardResetAfterPresent','InvalidateSLDLSSGStateAfterHardReset',
    'slFreeResourcesApi','GetProcAddress(slInterposerModule, "slFreeResources")','FG_HDR_HARD_RESET_BEGIN',
    'FG_HDR_HARD_RESET_OFF_COMMIT','FG_HDR_HARD_RESET_FREE','FG_HDR_HARD_RESET_GATE','FG_HDR_HARD_RESET_REARM',
    'QuiesceDLSSGForHdrSwapchainTransition','sdr_to_hdr_resize','hdr_to_sdr_resize','FG_HDR_TRANSITION_QUIESCE']))
# Windows/MSVC compile regression from the first r31 candidate: the hard-reset
# invalidation function accidentally referenced a nonexistent
# slFgLastBackBufferIndex instead of the shared slLastBackBufferIndex tracker.
# Keep this as a package gate so the same class of typo cannot ship again.
ck('r31-hard-reset-backbuffer-symbol-contract',
   'static std::atomic<unsigned int> slLastBackBufferIndex{0xFFFFFFFFu};' in sl_bridge
   and 'slLastBackBufferIndex.store(0xFFFFFFFFu, std::memory_order_release);' in sl_bridge
   and 'slFgLastBackBufferIndex' not in sl_bridge)
observer_signature = 'static void ObserveSLDisplayHdrDomainBeforePresent(IDXGISwapChain* swapChain, unsigned long long present) noexcept {'
observer_start = sl_bridge.index(observer_signature)
observer = sl_bridge[observer_start:sl_bridge.index('static void SetDLSSGModeForPresent', observer_start)]
ck('fg-hdr-observer-no-stale-swapchain-output', 'swapChain->GetContainingOutput' not in observer)
ck('r31c-display-duplicate-reset-suppression', all(x in sl_bridge for x in [
    'slFgDisplayObservedBridgeGeneration','BridgeAlreadyOwnsObservedDisplayFlip',
    'FG_DISPLAY_DOMAIN_DUPLICATE_HANDOFF','action=skip_duplicate_hard_reset'
]) and all(x in hdr_transition for x in [
    'BridgeAlreadyOwnsObservedDisplayFlip','currentBridgeGeneration != observedBridgeGeneration',
    'bridgeHdrActive == observedDisplayHdrActive'
]) and all(x in (root/'validation/r31-hdr-hard-reset/policy-test.cpp').read_text(encoding='utf-8') for x in [
    'BridgeAlreadyOwnsObservedDisplayFlip(1, 2, false, false)',
    'BridgeAlreadyOwnsObservedDisplayFlip(2, 3, true, true)',
    '!control_fg_hdr_transition::BridgeAlreadyOwnsObservedDisplayFlip(1, 1, true, false)'
]))
hdr_hard_reset_policy = (root/'src/fg_hdr_hard_reset_policy.h').read_text(encoding='utf-8-sig')
ck('fg-hdr-hard-reset-policy', all(x in hdr_hard_reset_policy for x in [
    'OffQueuedWaitPresent','ResourcesFreedWaitBridge','CanFreeAfterPresent','CanRearm',
    'kNoBridgeGracePresents = 30ull','kNoBridgeFreshFramesRequired = 2u',
    'NextNoBridgeFreshFrameCount','CanRearmWithoutBridge'
]))
ck('r31c-display-only-no-resize-rearm', all(x in sl_bridge for x in [
    'slFgHdrHardResetFreePresent','slFgDisplayTransitionLastChangePresent',
    'FG_HDR_HARD_RESET_NO_BRIDGE_SETTLE','FG_HDR_HARD_RESET_REARM_NO_BRIDGE',
    'hold_fg_off_waiting_for_resize_or_display_only_settle'
]) and all(x in (root/'validation/r31-hdr-hard-reset/policy-test.cpp').read_text(encoding='utf-8') for x in [
    'NoBridgeGraceElapsed(130, 100, 100)',
    'NextNoBridgeFreshFrameCount(Stage::ResourcesFreedWaitBridge, 7, 7, true, true, 131, 100, 100, 130, 1) == 2',
    'CanRearmWithoutBridge(Stage::ResourcesFreedWaitBridge, 7, 7, true, 131, 100, 100, 2)'
]))
ck('fg-hdr-hotkey-disabled', 'policy=disabled_r31_display_or_resize_detected_hard_dlssg_reset' in overlay and 'SetWindowsHookExW(' not in overlay and 'ServiceSLHdrHotkeyGuardBeforePresent' not in hdr_bridge and 'CompleteSLHdrHotkeyGuardAfterPresent' not in hdr_bridge)
ck('fg-hdr-transition-generation', all(x in hdr_bridge for x in ['hdr10BridgeTransitionGeneration','GetHdr10BridgeTransitionGeneration','transition_generation=%llu']))
ck('fg-hdr-transition-fresh-pair-gate', all(x in sl_bridge for x in ['FG_HDR_TRANSITION_GATE','FG_HDR_TRANSITION_SETTLED','slFgHdrTransitionSettledGeneration','slFgHdrTransitionWarmupFrames']) and all(x not in sl_bridge+sl_frame for x in ['FG_HDR_TRANSITION_RESET','slFgHdrTransitionResetGeneration','effectiveResetValue = hdrTransitionReset ? 1 : resetValue']))
ck('fg-hdr-transition-test-built', r'r26-hdr-transition-warmup\policy-test.cpp' in build and r'r27-hdr-transition-quiesce\policy-test.cpp' in build and r'r28-display-hdr-prepresent\policy-test.cpp' in build and r'r29-fresh-output-hdr\policy-test.cpp' in build and r'r30-hdr-hotkey-guard\policy-test.cpp' in build and r'r31-hdr-hard-reset\policy-test.cpp' in build)
ck('fg-hdr-transition-metadata', all(x in metadata for x in [
    "FGHDRTransitionPolicy = 'detect_display_or_resize_hdr_domain_change_commit_dlssg_eoff_on_real_present_slFreeResources_viewport_invalidate_fg_caches_then_bridge_rebuild_plus_two_fresh_frames'",
    "FGHDRTransitionReset = 'r31_hard_dlssg_resource_reset_slFreeResources_not_r25_constants_reset'",
    "FGHDRTransitionOffCommit = 'fresh_output_or_resize_detection_queues_eoff_real_present_commits_off_then_slFreeResources_releases_dlssg_viewport_before_rearm'",
    "FGHDRTransitionDetection = 'native_IDXGIFactory1_fresh_output_observer_plus_Control_HDR_bridge_ResizeBuffers_domain_transition_no_keyboard_interception'",
    'FGHDRTransitionFreshFrames = 2',
    "FGHDRTransitionReenable = 'after_resource_free_bridge_generation_change_and_two_fresh_tagged_frames_or_after_30_present_no_bridge_grace_plus_two_fresh_tagged_frames_then_clean_options_recreate_on_next_present'"]))

# The guide shader/bounded material path signed off in r21t remains intact.
live_guides = (root/'src/rr_live_guides.h').read_text(encoding='utf-8-sig')
live_record = (root/'src/rr_live_record.h').read_text(encoding='utf-8-sig')
ck('gbuffer-guide-mode', 'RRGBufferGuideMode = true' in user_control and 'architecture=renodx_descriptor_parity' in live_guides)
ck('guide-shader-contract', all(x in (root/'src/shaders/rr_live_guides.hlsl').read_text() for x in ['MaterialDataPart1','EnvBRDF','[numthreads(16,16,1)]','NormalRoughness','DiffuseGuide','SpecularGuide']))
ck('bounded-material-srv', all(x in live_guides for x in ['NumDescriptors=4','hd.NumDescriptors=7','materialSRV.Buffer.NumElements=static_cast<UINT>(sample.metadata.bytes/8)','materialSRV.Buffer.StructureByteStride=8','CreateShaderResourceView(material,&materialSRV,handle)']) and 'SetComputeRootShaderResourceView' not in live_record)
ck('renodx-root-order', 'SetComputeRoot32BitConstants(0,4,constants,0)' in live_record and 'SetComputeRootDescriptorTable(1,s->heap->GetGPUDescriptorHandleForHeapStart())' in live_record)

# Native clamp is the signed-off signal path.
native_clamp = (root/'src/rr_specular_native_clamp.h').read_text(encoding='utf-8-sig')
native_policy = (root/'src/rr_specular_clamp_policy.h').read_text(encoding='utf-8-sig')
ck('native-clamp-provider-contract', all(x in native_clamp for x in [
    'ProviderResolverRva = 0x210270','ProviderMetadataRva = 0x112840',
    'ProviderTlsIndexRva = 0x1115fc','ProviderArenaOffset = 0x4250',
    'CameraCutName[] = "g_uCameraCut"','SetVerified(id, 1u)','camera_cut_restore_failed'
]))
ck('native-clamp-runtime', all(x in native_frame for x in [
    'RRNativeHookTemporalBind','base+0x16744','RR_NATIVE_SPECULAR_CLAMP_READY',
    'mode=native_control_energy_clamp','rrNativeClampApi.Restore(rrNativeClampLease)','raw_copy_fallback'
]))
ck('no-unsafe-device-hook', 'rr_specular_clamp_device.h' not in native_frame and 'RR_SPECULAR_CLAMP_DEVICE_HOOK' not in probe)
ck('production-reference-runtime-removed', all(x not in native_frame for x in [
    '#include "rr_specular_reference.h"','control_rr_reference::','RR_SPECULAR_REFERENCE_FALLBACK','mode=renodx_reference_dispatch'
]) and 'full_renodx_addon_loaded' in native_frame)

# r21y production cleanup: preserve only read-only temporal access; remove all
# runtime invocation of the old hit-array/distance/readback diagnostic paths.
reflection_hooks = (root/'src/rr_reflection_hooks.h').read_text(encoding='utf-8-sig')
eval_entry = (root/'src/rr_evaluation_entry.h').read_text(encoding='utf-8-sig')
live_retire = (root/'src/rr_live_retire.h').read_text(encoding='utf-8-sig')
ck('temporal-access-only-install', all(x in probe+reflection_hooks for x in [
    'RRReflectionInitializeAccessOnly(d3d)','RR_TEMPORAL_ACCESS_READY','RR_TEMPORAL_ACCESS_INSTALL',
    'reflection_geometry_capture=0','hit_distance_runtime=0','specular_mvec_runtime=0'
]))
ck('D1-reflection-lifetime', all(x in probe for x in ['RRReflectionInstall(renderer,d3d)','RRReflectionAfterPresent(count)','#include "rr_distance_runtime.h"']))
ck('D1-F-only', 'RRUserPresetValue()==control_rr::RRPresetF' in reflection_hooks and 'DistanceEligible(beginEvaluation,bindings.projectionValid,activePreset' in eval_entry)
ck('D1-optional-binding', 'RRNativeSetGuides(parameters,&bindings,hitDistance' in eval_entry and 'RR_F_DISTANCE_D1' in eval_entry)
ck('guide-stats-readback-disabled', 'static constexpr bool rrLiveGuideStatsEnabled=false;' in live_guides and 'if(SUCCEEDED(hr)&&rrLiveGuideStatsEnabled)RRLiveCapturePrepare(c);' in live_guides and 'if(rrLiveGuideStatsEnabled&&rrLiveCaptureRRFrame' in live_guides and 'rrLiveGuideStatsEnabled && !c->stopped' in live_retire)
ck('metadata-dead-work-disabled', all(x in metadata for x in [
    'RRReflectionCaptureEnabled = $true','RRHitDistanceOutputEnabled = $true','RRLiveCandidateReadbackEnabled = $false',
    "RRProductionReflectionGeometryCapture = 'D1_F_only_owned_hit_arrays'",
    "RRProductionHitDistanceRuntime = 'D1_F_only_optional'",
    "RRProductionGuideStatsReadback = 'disabled_diagnostic_only'"
]))
ck('build-dead-stages-disabled', all(x not in build for x in [
    'ControlFG-RenoDXClampRef.addon64','compile-reference-clamp.cpp'
]) and 'validation\\evaluation-entry\\entry-test.cpp' in build)
ck('binary-package-no-reference-addon', all(x not in dropin for x in [
    'diagnostics\\reshade-reference','ControlFG-RenoDXClampRef.addon64'
]))
ck('public-dropin-no-internal-milestone-text', all(x not in dropin for x in [
    'Internal r27','r31c HDR','Summarize-ControlFG-Performance.ps1','README.txt'
]))
ck('installer-no-reference-addon', "RelativePath = 'ControlFG-RenoDXClampRef.addon64'" not in manager)

# Current source package integrity.
for manifest_name in ['SOURCE-SHA256.json','ARTIFACT-SHA256.json']:
    manifest = json.loads((root/manifest_name).read_text(encoding='utf-8'))
    ck(manifest_name + '-count', len(manifest) >= 700)
    ck(manifest_name + '-hashes', all(
        (root/name).is_file() and hashlib.sha256((root/name).read_bytes()).hexdigest().upper() == expected.upper()
        for name, expected in manifest.items()
    ))

frozen_delta = json.loads((root/'validation/r31-frozen-core-delta.json').read_text(encoding='utf-8'))
ck('r31-frozen-delta-status', frozen_delta.get('status') == 'PASS' and frozen_delta.get('total_count') == 11 and frozen_delta.get('unchanged_count') == 9)
ck('r31-frozen-delta-paths', sorted(x.get('Path') for x in frozen_delta.get('changed_files', [])) == ['src/hdr10_bridge.h','src/streamline_bridge.h'])
ck('r31-frozen-delta-current-hashes', all(
    (root/item['Path']).is_file() and hashlib.sha256((root/item['Path']).read_bytes()).hexdigest().upper() == item['R31SHA256'].upper()
    for item in frozen_delta.get('changed_files', [])
))

frozen = json.loads((root/'g3-frozen-files.json').read_text(encoding='utf-8'))
ck('frozen-core-count', len(frozen.get('Files', [])) == 11)
ck('frozen-core-hashes', all(
    hashlib.sha256((root/item['Path']).read_bytes()).hexdigest().upper() == item['SHA256'].upper()
    for item in frozen['Files']
))

# Keep numerical/static evidence intact.
reflectance = json.loads((root/'validation/g12-current/reflectance-validation.json').read_text(encoding='utf-8'))
ck('reflectance-evidence', reflectance.get('status') == 'PASS' and reflectance.get('normal') == 'PASS' and reflectance.get('asan_ubsan') == 'PASS')
option = json.loads((root/'validation/native-option-isolation/results.json').read_text(encoding='utf-8'))
ck('native-option-evidence', option.get('status') == 'PASS')
part1 = json.loads((root/'validation/part1/validation.json').read_text(encoding='utf-8'))
ck('part1-evidence', part1.get('status') == 'PASS')
native_clamp_validation = json.loads((root/'validation/native-specular-clamp/results.json').read_text(encoding='utf-8'))
ck('native-clamp-evidence', native_clamp_validation.get('status') == 'PASS' and native_clamp_validation.get('normal') == 'PASS' and native_clamp_validation.get('asan_ubsan') == 'PASS')

ck('no-built-binaries', not any(
    p.is_file() and p.suffix.lower() in {'.dll','.exe','.obj','.lib','.pdb'}
    for p in root.rglob('*')
))

print('PASS: Control FG v2.0.0 r31c HDR DLSS-G hard-reset candidate package validated')
print('checks=' + str(len(checks)))
