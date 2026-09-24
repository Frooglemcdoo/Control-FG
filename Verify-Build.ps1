#requires -Version 5.1
param([switch]$AbiOnly)
$ErrorActionPreference = 'Stop'
try {
    . (Join-Path $PSScriptRoot 'Build-Metadata.ps1')
    $expected = '?doAntiAliasing@DLSS@d3d@@SA_NPEAVNativeTexture@2@00000000_NNNMMM@Z'
    $symbols = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\abi-symbols.txt') -Raw
    if (-not $symbols.Contains($expected)) { throw 'The compiler ABI does not match the game import. Do not install this build.' }
    Write-Host 'PASS: MSVC argument types produce the exact game-imported DLSS symbol.'
    if ($AbiOnly) { exit 0 }

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-Streamline.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Streamline SDK staging validation failed.' }

    $coreHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\include\sl_core_api.h') -Raw
    if (-not $coreHeader.Contains('slSetTagForFrame') -or -not $coreHeader.Contains('slFreeResources')) { throw 'Pinned Streamline core header is missing slSetTagForFrame/slFreeResources.' }
    $dlssgHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\include\sl_dlss_g.h') -Raw
    foreach ($marker in @('eFailGetCurrentBackBufferIndexNotCalled','eFailHDRFormatNotSupported','numFramesActuallyPresented','numFramesToGenerateMax','bIsDynamicMFGSupported','dynamicTargetFrameRate','eDynamic','colorBufferFormat','enableUserInterfaceRecomposition')) {
        if (-not $dlssgHeader.Contains($marker)) { throw ('Pinned Streamline DLSS-G header is missing expected marker: ' + $marker) }
    }

    $dlssdHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\include\sl_dlss_d.h') -Raw
    foreach ($marker in @('DLSSDOptions','DLSSDPreset','ePresetF','slDLSSDSetOptions','slDLSSDGetState','slDLSSDGetOptimalSettings')) {
        if (-not $dlssdHeader.Contains($marker)) { throw ('Pinned Streamline DLSS-RR header is missing expected marker: ' + $marker) }
    }

    $exports = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\exports.txt') -Raw
    foreach ($name in @('CreateDXGIFactory','CreateDXGIFactory1','CreateDXGIFactory2','DXGIGetDebugInterface1','DXGIDeclareAdapterRemovalSupport')) {
        if ($exports -notmatch ('(?m)\b' + [regex]::Escape($name) + '\b')) { throw ('Missing proxy export: ' + $name) }
    }
    $imports = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\imports.txt') -Raw
    if ($imports -match '(?im)^\s+dxgi\.dll\s*$') { throw 'The proxy must not import itself.' }
    if ($imports -match '(?im)^\s+sl\.interposer\.dll\s*$') { throw 'v2.0.0 must load Streamline dynamically by absolute path.' }
    if ($imports -match '(?im)^\s+d3dcompiler_47\.dll\s*$') { throw 'v2.0.0 must resolve D3DCompile dynamically; do not add a static d3dcompiler import.' }

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-Source.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'G12 source consistency validation failed.' }

    $sidecar=Join-Path $PSScriptRoot 'build/ControlFG.RTX40MFG.dll'
    $sidecarText=[Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($sidecar))
    foreach ($export in @('ControlFGMFGInitialize','ControlFGMFGPrepare')) {
        if (-not $sidecarText.Contains($export)) { throw ('MFG sidecar export missing: '+$export) }
    }
    $dll = Join-Path $PSScriptRoot 'build\dxgi.dll'
    $bytes = [IO.File]::ReadAllBytes($dll)
    $binaryText = [Text.Encoding]::ASCII.GetString($bytes)
    # UNICODE is defined for the C++ build, so wide string literals are UTF-16LE in the PE.
    $binaryUnicodeText = [Text.Encoding]::Unicode.GetString($bytes)
    $markers = @(
        'FG_OVERLAY_SETTINGS_S5 binding=persisted_single_key options=replacement_page default=F10',
        'PROBE v2.0.0 internal_build=2.0.0-Clean-Native-R12-MFG-Dynamic-Test',
        'source_revision=clean-v2-native-source-r12-mfg-dynamic-test','log_profile=%s','CAPABILITIES fg=fixed_2x_to_6x_plus_dynamic',
        'MONITORING profile=%s rr_perf_sample=240','CONTROLFG_VERBOSE_LOG',
        'RR_PRESET_HOOK_READY','default_preset=F selectable=E,F,K,L,M','RR_PRESET_REQUEST','RR_PRESET_UI','RR_PRESET_LIVE_SWITCH',
        'RR_NATIVE_EVALUATED','RR_FRAME_MODE','RR_RESIZE_EPOCH_BEGIN','RR_RESIZE_EPOCH_RELEASE','RR_FRAME_RECOVERY_SR',
        'RR_TEMPORAL_ACCESS_READY','RR_TEMPORAL_ACCESS_INSTALL','RR_NATIVE_SPECULAR_CLAMP_READY',
        'RR_SPECULAR_SIGNAL frame=%llu mode=raw_copy_fallback',
        'hit_distance=D1_optional','specular_mvec=cleared','reflection_mvec=cleared','matrix_mode=preset_dependent_projection_p1','RR_F_PROJECTION_P1','RR_F_DISTANCE_D1','RR_F_DISTANCE_D1_INSTALL','RR_CLAMP_STRENGTH_CS3','RR_CLAMP_CS3_CAPTURE','RR_SPECULAR_SIGNAL frame=%llu mode=native_control_energy_clamp','RR_INPUT_CAPTURE_C1_REQUEST','RR_INPUT_CAPTURE_C1_RECORDED','RR_INPUT_CAPTURE_C1_EXPORTED','RR_REFLECTION_COPIED','RR_REFLECTION_PREPARED','RR_DISTANCE_RECORDED','RR_DISTANCE_PREPARED',
        'RR_PERF_READY gpu_frequency=%llu sample_every=240','RR_PERF_FRAME','RR_PERF_GPU',
        'SL_BOOTSTRAP_BEGIN','SL_CORE_READY','SL_DEVICE_READY','SL_FEATURE_GATE','SL_RR_FEATURE_GATE','SL_RR_HANDSHAKE','SL_DLSSD_FUNCTION',
        'FG_UI_PRIVATE_DEVICE_FAIL','FG_UI_PRIVATE_WORK_FAIL','FG_RTX40_MFG_INIT','FG_RTX40_MFG_READY','FG_RTX40_MFG_TEST_REQUEST','FG_RTX40_MFG_TEST_RESULT','FG_UI_PRIVATE_SUBMIT','FG_UI_PRIVATE_FAIL','FG_UI_RECOMPOSE_READY','FG_HDR_HUDLESS_PIPELINE_READY','conversion=compute_uav no_graphics_state=1 private_command_list=1','FG_HDR_HUDLESS_CONVERT','FG_HDR_HOTKEY_HOOK','FG_HDR_HARD_RESET_BEGIN','FG_HDR_HARD_RESET_OFF_COMMIT','FG_HDR_HARD_RESET_FREE','FG_HDR_HARD_RESET_GATE','FG_HDR_HARD_RESET_REARM','FG_DISPLAY_FACTORY_REFRESH','FG_DISPLAY_DOMAIN_BASELINE','FG_DISPLAY_DOMAIN_CHANGE','FG_DISPLAY_DOMAIN_GATE','FG_DISPLAY_DOMAIN_HANDOFF','FG_HDR_TRANSITION_PREP','FG_HDR_TRANSITION_QUIESCE','FG_HDR_TRANSITION_GATE','FG_HDR_TRANSITION_SETTLED','FG_UI_RECOMPOSITION_OPTIONS','SL_DLSSG_MODE','SL_DLSSG_STATE',
        'FG_FIRST_GENERATED_FRAME','FG_MULTIPLIER_CHANGE','FG_MODE_CHANGE','FG_DYNAMIC_TARGET_CHANGE','FG_DISPLAY_REFRESH','FG_USER_SELECTION',
        'HDR10_FACTORY_WRAP','HDR10_BRIDGE_CREATE','HDR10_BRIDGE_DORMANT','HDR10_BRIDGE_ACTIVE','HDR10_BRIDGE_DEACTIVATED','HDR10_BRIDGE_FALLBACK',
        'FG_OVERLAY_READY','FG_OVERLAY_SELECTION','FG_SETTINGS_LOAD','FG_SETTINGS_SAVE','RR_TOGGLE',
        'rr_controls=on_off_plus_model_E_F','rr_model_default=F','rr_model_live_switch=E_F','FG_OVERLAY_DYNAMIC_TARGET',
        'dynamic_target_ui=auto_manual_thick_slider','rr_preset_selector=public_E_F','overlay_status=selected,effective,current_fps,hdr,capability','controls=auto,manual,thick_slider_30_1000',
        'persistence=localappdata_ini_schema8_fg_mode_dynamic_target_rr_toggle_preset'
    )
    # Some historical guide strings remain source-contract markers even when their
    # diagnostic runtime path is deliberately compiled out. Build validation must
    # follow the current metadata flags instead of requiring dead-code strings in
    # the optimized PE. r20w disables the skin diagnostic/responsivity experiment.
    $runtimeGuideMarkers = @($ControlFGGuideMarkers)
    if (-not $ControlFGBuild.RRSkinDiagnosticEnabled) {
        $runtimeGuideMarkers = @($runtimeGuideMarkers | Where-Object { $_ -notlike 'RR_SKIN_*' })
    }
    foreach ($marker in ($markers + $runtimeGuideMarkers)) {
        if (-not $binaryText.Contains($marker)) { throw ('Wrong or incomplete ' + $ControlFGBuild.Version + ' DLL: missing ' + $marker) }
    }
    foreach ($wideMarker in @('CONTROL_FG_DISABLE_HDR10_BRIDGE','\ControlFG','\settings-mfg-test.ini','FrameGeneration','RayReconstruction')) {
        if (-not $binaryUnicodeText.Contains($wideMarker)) {
            throw ('Wrong or incomplete ' + $ControlFGBuild.Version + ' DLL: missing UTF-16 marker ' + $wideMarker)
        }
    }
    if ($binaryText.Contains('slEvaluateFeature')) { throw 'RR Native G12 must not contain an slEvaluateFeature call.' }
    if ($binaryText.Contains('RR_SET_NATIVE_TEXTURE') -or $binaryText.Contains('RR_SET_PROVIDER_DATA')) { throw 'RR Native G12 must not contain the retired P4 hot binding-hook labels.' }
    if ($binaryText.Contains('RR_NATIVE_P9_SHADER_CTOR') -or $binaryText.Contains('RR_NATIVE_POST_SETUP_BEGIN')) { throw 'The retired P9 or NGX post-setup capture is still compiled into the G12 runtime.' }
    foreach ($deadMarker in @('RR_G12_LIVE_CAPTURE_READY','RR_G12_LIVE_CAPTURE_RECORDED','ControlFG-RenoDXClampRef-r21w-api18-renodx120663347')) {
        if ($binaryText.Contains($deadMarker)) { throw ('D1 DLL still contains retired diagnostic runtime marker: ' + $deadMarker) }
    }
    if ($binaryUnicodeText.Contains('ControlFG-RenoDXClampRef.addon64')) { throw 'r31 HDR hard-reset candidate DLL must not retain the archived ReShade reference-addon loader path.' }

    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    if ([BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x8664) { throw 'Expected an x64 DLL.' }
    $sdkInfo = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\sdk-info.json') -Raw | ConvertFrom-Json
    $validation = [ordered]@{}
    foreach ($key in $ControlFGBuild.Keys) { $validation[$key] = $ControlFGBuild[$key] }
    $details = [ordered]@{
        RRInstrumentation = 'D1 test: F-only optional hit distance restored; projection retained; E unchanged; GPU validation pending.' 
        RRLiveShaderHeaderSHA256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build/rr_live_compiled.h') -Algorithm SHA256).Hash
        RRReflectanceShaderHeaderSHA256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'build/rr_reflectance_compiled.h') -Algorithm SHA256).Hash
        SHA256 = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
        BuiltUtc = [DateTime]::UtcNow.ToString('o')
        AbiCheck = 'Passed'
        ExportCheck = 'Passed'
        Architecture = 'x64'
        StreamlineSDKVersion = $sdkInfo.Version
        StreamlineSDKArchiveSHA256 = $sdkInfo.SourceArchiveSHA256
        StreamlineLoadMode = 'Dynamic absolute-path loading from ControlFGStreamline. Candidate requires no ReShade, RenoDX add-on, or loose DLSS files. SDR FG/RR behavior remains signed off; r31c changes only HDR-transition DLSS-G lifecycle recovery, retaining duplicate observer suppression and adding display-only no-ResizeBuffers rearm.'
        DLSSGGenerationEnabled = $true
        DLSSGGeneratedFramesRequested = 3
        TargetMultiplier = '4x'
        ExpectedFramesPresented = 4
        MaxSelectableMultiplier = '6x'
        DynamicMFGEnabled = $true
        DynamicMFGMode = 'DLSSGMode::eDynamic'
        DynamicTargetFrameRate = 'Auto=explicit detected game-monitor refresh; Manual=user FPS 30-1000 via restored slider'
        DynamicTargetPolicy = 'Auto uses QueryDisplayConfig/EnumDisplaySettingsEx; Manual exposes 30-1000 FPS slider; Present SyncInterval remains forced to 0 while Dynamic is API-enabled; Reflex frameLimitUs tracks the resolved target'
        DynamicReflexLimiter = 'frameLimitUs=round(1000000/targetFPS) in Dynamic; frameLimitUs=0 in fixed/off modes'
        MFGSidecarSHA256 = (Get-FileHash -LiteralPath $sidecar -Algorithm SHA256).Hash
        OverlayPersistence = '%LOCALAPPDATA%\ControlFG\settings-mfg-test.ini; FG mode + Dynamic target FPS + RR enabled + selected RR model; schema 8'
        OverlayRuntimeStatus = 'FG status row plus RR ON/OFF and live MODEL E/F selector. Internal native signal implementation text is not exposed.'
        Confirmed2xBaseline = 'v0.8.18-r2 SDR + HDR'
        HDR10BridgeEnabled = $true
        HDR10BridgeSourceFormat = 'DXGI_FORMAT_R16G16B16A16_FLOAT'
        HDR10BridgePresentationFormat = 'DXGI_FORMAT_R10G10B10A2_UNORM'
        HDR10BridgePresentationColorSpace = 'DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020'
        HDR10Conversion = 'linear scRGB/Rec.709 -> linear BT.2020 -> ST.2084/PQ; 1.0 scRGB = 80 nits'
        HDRHUDLessPolicy = 'HDR bridge active: compute-converted RGB10/PQ HUDless + R8 UI alpha; r31c does not intercept Win+Alt+B. Fresh-output or ResizeBuffers domain detection commits DLSS-G eOff, calls slFreeResources for the DLSS-G viewport, invalidates FG caches, then waits for either the bridge rebuild plus two fresh frames or a 30-Present unchanged-bridge grace plus two fresh tagged frames before clean re-enable.'
        RuntimeTest = 'Start SDR + FG and press Win+Alt+B normally. A brief first-frame glitch is acceptable for this diagnostic; persistent corruption is not. Require FG_HDR_HARD_RESET_BEGIN, OFF_COMMIT success=1, FREE success=1, matching HDR10_BRIDGE_ACTIVE/DEACTIVATED, FG_HDR_TRANSITION_GATE/SETTLED, then FG_HDR_HARD_RESET_REARM before SL_DLSSG_MODE returns on. Repeat both directions.'
    }
    foreach ($key in $details.Keys) { $validation[$key] = $details[$key] }
    Assert-ControlFGBuildValidation ([pscustomobject]$validation)
    $validation | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'build\build-validation.json') -Encoding UTF8

    Write-Host 'PASS: Control FG v2.0.0 r31c HDR DLSS-G hard-reset candidate validated: Model F default/E alternate, concise support logging, signed-off RR/SDR FG retained; HDR-domain changes commit FG off, free DLSS-G viewport resources, invalidate FG caches, then rearm after bridge/fresh-frame settle.'
    Write-Host 'Build checks do not validate live Control/driver behavior. r31c sign-off requires FG_HDR_HARD_RESET_BEGIN -> OFF_COMMIT success=1 -> FREE success=1, then either bridge transition + two-fresh-frame settle -> FG_HDR_HARD_RESET_REARM or no-resize settle -> FG_HDR_HARD_RESET_REARM_NO_BRIDGE, followed by SL_DLSSG_MODE mode=on.'
} catch { Write-Error $_; exit 1 }
