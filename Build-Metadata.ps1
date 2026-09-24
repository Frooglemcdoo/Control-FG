#requires -Version 5.1
# Shared identity and installer contract. Dot-source this file; it performs no I/O.
$ControlFGBuild = [ordered]@{
    Version = '2.1.1'
    BaseProductionVersion = '2.1.0'
    SourceRevision = 'v2.1.1-unified-storefront-r3'
    RRPhase = 'NativeG12RRExperiment'
    RRNativePreset = 'Preset F public default with live E/F model selection'
    RRRuntimeVersion = '310.9.1'
    RRPreLightingPauseRecovery = 'complete_native_sr_then_reset_rr'
    RRUserToggleDefault = 'persisted_on_off_setting'
    RRUserToggleLocation = 'F10_overlay'
    RRExperimentScope = 'release_candidate_hdr_fg_hard_lifecycle_reset_with_signed_off_rr_path_unchanged'
    RRRepresentativeReflectionRay = 0
    RRReflectionCaptureEnabled = $true
    RRReflectionCopyOnly = $false
    NativeDenoiserBypassScope = 'opaque_reflection_diffuse_gi_contact_shadow_and_remaining_broad_diffuse_boundaries_full_rr_only'
    RRAdditionalRTEffects = 'transparent_reflections_and_debris_no_direct_DLF_denoiser_found_contact_shadow_neutralized_full_rr'
    RRRTSettingsReset = $true
    RRReflectionCaptureBudgetPolicy = 'dxgi_local_headroom_minus_ten_percent'
    RRRenderDimensionLimit = 8192
    RRDiagnosticPixelLimit = 8388608
    RRReflectionAllocationDiagnostics = $false
    RRReflectionRetirementPolicy = 'D1_primary_queue_present_fence_three_slots'
    RRRecurringNativeDiffuseReplayEnabled = $false
    RRLiveDiffuseCandidateEnabled = $true
    RRLiveDiffuseAlphaPolicy = 'integer_binary16_one_preserve_rgb_bits'
    RRRecurringDiffuseResizeSupported = $true
    RRResolutionRecovery = 'hard_epoch_pause_drain_settle8_rebuild_warmup_rr_reset'
    RRPerformanceCapture = 'gpu_timestamps_every_60_frames_nonblocking_fence_readback'
    RRPerformanceWarmupSeconds = 3
    RRPerformanceOffBaseline = 'native_sr_no_rr_auxiliary_work_no_contact_shadow_override'
    RRLiveNormalSpecularCandidatesEnabled = $true
    RRLiveCandidateSlots = 3
    RRLiveCandidateReadbackEnabled = $false
    RRMaterialBinding = 'bounded_structured_buffer_srv_t2_stride8_num_elements_buffer_bytes_div8'
    RRRegisteredOptionWritePolicy = 'force_false_only_never_true'
    RRNativeOptionIsolation = 'reflection_gi_radius_gi_bypass_mod_state_jitter_private_mirror'
    RRStateLeakRepairEnabled = $true
    RRSkinDiagnosticEnabled = $false
    RRSkinResponsivityExperimentEnabled = $false
    RRSkinCharacterCoverageReplayEnabled = $false
    RRSkinDiagnosticLoggingPolicy = 'disabled_release_candidate'
    RRPresetSelection = 'Public E/F model selector; F default; K/L/M not exposed'
    RRPresetTransitionRecovery = 'hard_epoch_pause_drain_preset_only_streamline_options_window_parameter_resync_fresh_history' 
    RRPartialModeEnabled = $true
    RRPartialReleasePolicy = 'diagnostic_test_only_hidden_from_public_overlay'
    RRPartialModeContract = 'mod_owned_hidden_rr_render_switches_jitter_mirror_native_sr_evaluation_no_feature13_no_guide_injection'
    RRPresetRestartGateEnabled = $true
    RRPresetLiveSwitchEnabled = $true
    RRPresetLiveSwitchScope = 'E_to_F_and_F_to_E_only'
    RRPresetNativeCreateConfirmationEnabled = $true
    RRSubsurfaceGuideEnabled = $false
    RRExplicitStreamlineOffOnUserDisable = $true
    RRSharpnessControl = 'hidden_from_public_overlay_native_game_control_only'
    RRSkinCharacterFingerprintEnabled = $false
    RRResponsivityObservationEnabled = $false
    RRResponsivityInjectionEnabled = $false
    RRSkinBypassCompositeEnabled = $false
    RROptionalSubsurfaceObservation = 'disabled_release_candidate'
    RRNativeEvaluationEntryEnabled = $true
    RRNativeEvaluationForwardingOnly = $false
    RRFeatureRequested = $true
    RRRuntimeHandshakeEnabled = $true
    RROptionsHandshakeEnabled = $true
    RRSemanticGuideMappingEnabled = $true
    RRNativeNgxPostSetupProbeEnabled = $false
    RRNativeRtWindowProbeEnabled = $false
    RRNativeRaytracingShaderCaptureEnabled = $false
    RRRendererObserverHooksEnabled = $false
    RRPrimaryGuideCaptureEnabled = $false
    RRPrimaryGuideComputeEnabled = $false
    RRPrimaryGuideExportEnabled = $false
    RRPrimaryGuideCaptureLimit = 1
    RRGuideOutputs = 'RuntimeNormalRoughness_RGBA16F_RuntimeSpecular_RGBA16F_RuntimeDiffuse_RGBA16F'
    RRGuideInputs = 'GBuffer1_GBuffer2_MaterialDataPart1_NativeEnvBRDF'
    RRAsyncGuidePreparationEnabled = $true
    RRNativeOpaqueMaterialReplayEnabled = $false
    RRNativePrimaryViewRoutingCorrected = $true
    RRNativeJoinedFrameCaptureEnabled = $false
    RRNativeAlbedoTargetCaptureEnabled = $false
    RRNativeAlbedoTargetLimit = 2
    RRNativeSelectedPixelShaderCaptureEnabled = $false
    RRNativeSelectedPixelShaderCaptureLimit = 128
    RRMaterialRejectionAuditEnabled = $true
    RRMaterialRejectionSelectionExampleLimit = 64
    RRMaterialRejectionShaderExampleLimit = 32
    RRMaterialFamilyContractAuditEnabled = $true
    RRMaterialFamilyContractAdmissionChanged = $true
    RRCanonicalTechniqueKeyAuditEnabled = $true
    RRReplayRangeTimingAuditEnabled = $true
    RRAuditedCharacterAdmissionEnabled = $true
    RRAuditedClothAdmissionEnabled = $true
    RRFoliageAdmissionEnabled = $true
    RRHairAdmissionEnabled = $true
    RRHairWarmReplayAuditEnabled = $true
    RRHairWarmReplayRepeatPerRange = 1
    RRFoliageUnsafeVertexMismatchPolicy = 'strict_reject'
    RREyeMissingLookupPolicy = 'strict_reject'
    RREyeAdmissionEnabled = $true
    RRReflectancePrimitivesImplemented = $true
    RRDiffuseReflectanceByteTransferImplemented = $true
    RRDiffuseReflectanceAlphaPolicy = 'binary16_one_preserve_rgb'
    RRMaterialF0DecodeImplemented = $true
    RRSpecularReflectanceReference = 'NVIDIA_EnvBRDFApprox2'
    RRZeroReflectancePolicy = 'preserve_zero'
    RRReflectanceLiveResourceBindingEnabled = $true
    RRPart1MetadataDiagnosticEnabled = $false
    RRPart1MetadataDiagnosticLimit = 1
    RRPart1ResourceAcquisitionEnabled = $false
    RRPart1ReadbackEnabled = $false
    RRGpuReflectanceCandidateCaptureEnabled = $false
    RRGpuReflectanceComparedToCpu = $false
    RRPart1ReadbackMaximumBytes = 524288
    RRPart1BorrowedResourceObservationEnabled = $false
    RRDiffuseAlbedoOutputEnabled = $true
    RRSpecularAlbedoOutputEnabled = $true
    RRHitDistanceOutputEnabled = $true
    RRHitDistanceRRBindingEnabled = $true
    RRSpecularMotionRRBindingEnabled = $false
    RRGuideMatrixMode = 'identity_world_to_view_identity_view_to_clip'
    RRSpecularSignalAB = 'CS4_RR_settings_reset_60;default_60;distance_cap_65504'
    RRReferenceClampTargetShader = '0x600347E7'
    RRReferenceClampRenoDXRevision = '120663347a89ebe36c37eeb14653ad6b50958bc6'
    RRReferenceClampReShadeRevision = '4a50d1eddace85734871d91792ff214f13f66c01_api18'
    RRReferenceClampFailClosed = $true
    RRReferenceClampUnreplacedTargetReject = $true
    RRReferenceClampRequiresReShadeFullAddonSupport = $true
    RRReferenceClampRuntime = 'archived_source_only_not_built_not_packaged_not_public'
    RRNativeSpecularClampRequiresReShade = $false
    RRNativeSpecularClampTargetShader = '0x600347E7'
    RRNativeSpecularClampCameraCutProvider = 'g_uCameraCut'
    RRNativeSpecularClampProviderSpan = 'renderer_0x16744_temporal_bind_through_0x16786_dispatch'
    RRNativeSpecularClampHistoryPolicy = 'force_camera_cut_1_restore_after_dispatch'
    RRNativeSpecularClampRawCopyPolicy = 'off_or_emergency_fallback'
    RRReferenceClampRequiresLooseDLSSFiles = $false
    RRReferenceClampTouchesNGX = $false
    RRProductionReflectionGeometryCapture = 'D1_F_only_owned_hit_arrays'
    RRProductionHitDistanceRuntime = 'D1_F_only_optional'
    RRProductionGuideStatsReadback = 'disabled_diagnostic_only'
    ReleaseLogProfile = 'concise_support_default_verbose_via_CONTROLFG_VERBOSE_LOG'
    ReleasePerformanceSampleCadence = 240
    CompactSupportBundleSchema = 'ControlFG.CompactLogs.v2'
    RRProductionMeasuredDeadGpuWorkRemoved4KMs = 'approximately_0.33_from_r21x_clean_run'
    RRProductionMeasuredReflectionCopy4KMs = 'approximately_0.206'
    RRProductionMeasuredHitDistance4KMs = 'D1_NOT_MEASURED'
    RRProductionReflectionGeometryAllocation4KBytes = 501350400
    RRProductionGuideStatsReadback4KBytes = 199065600
    RREvaluationEnabled = $true
    NativeDenoiserBypassEnabled = $true
    RRContactShadowDenoiserNeutralizationEnabled = $true
    RRContactShadowTemporalPolicy = 'off_during_full_rr'
    RRContactShadowSpatialPolicy = 'size_zero_step_one_native_output_pass_preserved'
    RRTransparentDirectDLFDenoiserFound = $false
    RRDebrisDirectDLFDenoiserFound = $false
    RRScreenSpaceCombinedFilterPolicy = 'parent_preserved_internal_diffuse_denoiser_calls_bypassed_full_rr_when_rt_ao_or_sun_shadow_active'
    StreamlineSDKVersion = '2.14.1'
    FGUIRecompositionEnabled = $true
    FGUIRecompositionPolicy = 'private_pre_ui_snapshot_plus_r8_ui_alpha_sdr_native_hdr_compute_uav_fp16_snapshot_to_rgb10_pq_bt2020_tagged_valid_until_present_no_graphics_state_mutation'
    FGUIRecompositionColorFormats = 'R16G16B16A16_FLOAT,R10G10B10A2_UNORM,R8G8B8A8_UNORM/BGRA8'
    FGHDRHUDLessEnabled = $true
    FGHDRHUDLessSourceFormat = 'DXGI_FORMAT_R16G16B16A16_FLOAT_scRGB'
    FGHDRHUDLessTaggedFormat = 'DXGI_FORMAT_R10G10B10A2_UNORM_HDR10_BT2100_PQ'
    FGHDRHUDLessConversion = 'compute_uav_same_scrgb709_to_bt2020_pq_80nit_math_no_graphics_state'
    FGHDRUIRecompositionEnabled = $true
    FGHDRHUDLessCommandPath = 'private_direct_list_fence_retired_per_slot_descriptors'
    FGHDRHUDLessUAVSupportRequired = $true
    FGHDRUIAlphaFormat = 'DXGI_FORMAT_R8_UNORM'
    FGHDRFailurePolicy = 'fail_closed_disable_recomposition_for_frame_keep_depth_mv_fg_path'
    FGHDRModeResourcePolicy = 'separate_three_slot_sdr_and_hdr_resource_sets_reused_across_live_hdr_transitions'
    FGHDRTransitionPolicy = 'detect_display_or_resize_hdr_domain_change_commit_dlssg_eoff_on_real_present_slFreeResources_viewport_invalidate_fg_caches_then_bridge_rebuild_plus_two_fresh_frames'
    FGHDRTransitionReset = 'r31_hard_dlssg_resource_reset_slFreeResources_not_r25_constants_reset'
    FGHDRTransitionOffCommit = 'fresh_output_or_resize_detection_queues_eoff_real_present_commits_off_then_slFreeResources_releases_dlssg_viewport_before_rearm'
    FGHDRTransitionDetection = 'native_IDXGIFactory1_fresh_output_observer_plus_Control_HDR_bridge_ResizeBuffers_domain_transition_no_keyboard_interception'
    FGHDRTransitionFreshFrames = 2
    FGHDRTransitionReenable = 'after_resource_free_bridge_generation_change_and_two_fresh_tagged_frames_or_after_30_present_no_bridge_grace_plus_two_fresh_tagged_frames_then_clean_options_recreate_on_next_present'
    DLSSGGenerationEnabled = $true
    DLSSGGeneratedFramesRequested = 3
    TargetMultiplier = '4x'
}
$ControlFGGuideMarkers = @(
    'RR_FRAME_HOOKS_READY','RR_FRAME_MODE','RR_NATIVE_EVALUATED','RR_PARTIAL_FILTER','RR_PARTIAL_GI','RR_PARTIAL_EVALUATION_FORWARD','RR_PARTIAL_EVALUATED','RR_TEMPORAL_ACCESS_READY',
    'RR_NATIVE_OPTION_ISOLATION','RR_NATIVE_OPTION_REPAIRED','RR_NATIVE_OPTION_GUARD_FAILED','SL_RR_OPTIONS_DISABLED','RR_SKIN_INPUTS','RR_SKIN_RESOURCE','RR_SKIN_PATH','RR_SKIN_CHARACTER_BATCHES',
    'RR_TEMPORAL_ACCESS_INSTALL',
    'RR_G12_LIVE_PREPARED','RR_G12_LIVE_DISPATCH','RR_G12_LIVE_STATUS',
    'RR_G12_LIVE_STOP',
    'RR_G12_EVALUATION_ENTRY','RR_G12_EVALUATION_HOOKS_READY',
    'RR_GUIDE_G12_GPU_COMPARE','RR_GUIDE_G12_GPU_RESULT',
    'RR_GUIDE_G12_PART1_COPY','RR_GUIDE_G12_PART1_READBACK',
    'RR_GUIDE_G12_PART1_RESOURCE',
    'RR_GUIDE_G12_PART1_SAMPLE','RR_GUIDE_G12_PART1_PAIR',
    'RR_GUIDE_G3_READY','RR_GUIDE_G3_CAPTURE_SKIP','RR_GUIDE_G3_INPUTS_COPIED',
    'RR_GUIDE_G3_DISPATCH_SUBMITTED','RR_GUIDE_G3_EXPORT_COMPLETE',
    'RR_GUIDE_G3_EXPORT_FAILED','RR_GUIDE_G3_STOP','RR_GUIDE_G3_NATIVE_PATH',
    'RR_GUIDE_G3_PREPARE_TIMING','RR_GUIDE_G3_PREPARE_COMPLETE','RR_GUIDE_G3_PREPARE_QUEUED',
    'RR_GUIDE_G3_PREPARE_DISCARD','RR_GUIDE_G3_CAPTURE_TIMING',
    'RR_GUIDE_G3_ALBEDO_HOOKS_READY','RR_GUIDE_G3_SHADER_CONTRACT','RR_GUIDE_G3_ALBEDO_PREPARED',
    'RR_GUIDE_G3_ALBEDO_JOINED','RR_GUIDE_G3_ALBEDO_COPIED','RR_GUIDE_G3_ALBEDO_STOP',
    'primary_view=%p camera_view=%p','reason=primary_view_mismatch',
    'expected_primary_view=%p camera_view=%p',
    'RR_GUIDE_G5_PAIRED_OPPORTUNITY',
    'RR_GUIDE_G11_FAMILY_ADMISSION','RR_GUIDE_G11_REPLAY_RANGE',
    'RR_GUIDE_G11_HAIR_WARM_REPEAT','RR_GUIDE_G11_HAIR_WARM_SUMMARY'
)
# G12 retains the G3 exporter schema, directory prefix and existing guide marker names.
# The additional G5 marker identifies explicit same-frame paired capture opportunities.
# Package identity and the first PROBE log line distinguish G12 from earlier diagnostic builds.
# Capture identity and exact file layout are shared with the collector and its fixtures.
# These entries describe completed exports, not arbitrary files under the log directory.
$ControlFGCapture = [ordered]@{
    DirectoryPrefix = 'rr-guide-g3-'
    Schema = 'ControlFG.RRGuideG3.Capture.v1'
    CollectionSchema = 'ControlFG.RRGuideG3.Collection.v1'
    CompletedStatus = 'GUIDE_CAPTURE_EXPORTED'
    MaximumCaptures = 2
    MaximumCaptureBytes = 1024MB
    MaximumDimension = 8192
    MaximumPixels = 8388608
    MetadataName = 'metadata.json'
    MaterialRejectionAuditName = 'material-rejection-audit.json'
    MaximumMaterialRejectionAuditBytes = 8MB
    Images = @(
        [ordered]@{ Name='gbuffer1.rgba8'; Format='raw'; BytesPerPixel=4 },
        [ordered]@{ Name='gbuffer2.rgba8'; Format='raw'; BytesPerPixel=4 },
        [ordered]@{ Name='normal-roughness.rgba32f'; Format='raw'; BytesPerPixel=16 },
        [ordered]@{ Name='world-normal.bmp'; Format='bmp24'; BytesPerPixel=3 },
        [ordered]@{ Name='roughness.bmp'; Format='bmp24'; BytesPerPixel=3 },
        [ordered]@{ Name='material-id.bmp'; Format='bmp24'; BytesPerPixel=3 }
    )
    NativeTargetLimit = 2
    NativeTargetPrefix = 'native-albedo-target'
    NativeTargetRawSuffix = '.rgba16f'
    NativeTargetPreviewSuffix = '.bmp'
    NativeTargetAlphaSuffix = '-alpha.bmp'
    ShaderPrefix = 'native-albedo-ps-'
    ShaderSuffix = '.dxbc'
    ShaderLimit = 128
    MaximumShaderBytes = 256KB
    MaximumShaderAggregateBytes = 16MB
}
function Assert-ControlFGBuildValidation {
    param([Parameter(Mandatory=$true)]$Validation)
    foreach ($key in $ControlFGBuild.Keys) {
        $expected = $ControlFGBuild[$key]
        $property = $Validation.PSObject.Properties[$key]
        if ($null -eq $property) {
            throw ('Wrong/incomplete RR Native G12 build validation: ' + $key + '. Run Build.cmd in this extracted package.')
        }
        $actual = $property.Value
        $typeMatches = (($expected -is [bool]) -and ($actual -is [bool])) -or
            (($expected -is [string]) -and ($actual -is [string])) -or
            (($expected -is [int] -or $expected -is [long]) -and ($actual -is [int] -or $actual -is [long]))
        if (-not $typeMatches -or $actual -cne $expected) {
            throw ('Wrong/incomplete RR Native G12 build validation: ' + $key + '. Run Build.cmd in this extracted package.')
        }
    }
    if ($Validation.AbiCheck -cne 'Passed' -or $Validation.ExportCheck -cne 'Passed' -or $Validation.Architecture -cne 'x64') {
        throw 'The G12 ABI, export or x64 build check did not pass. Run Build.cmd again.'
    }
    if ($Validation.SHA256 -isnot [string] -or $Validation.SHA256 -notmatch '^[0-9A-Fa-f]{64}$') { throw 'The validated proxy hash is missing.' }
}
