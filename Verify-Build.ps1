#requires -Version 5.1
param([switch]$AbiOnly)
$ErrorActionPreference = 'Stop'
try {
    $expected = '?doAntiAliasing@DLSS@d3d@@SA_NPEAVNativeTexture@2@00000000_NNNMMM@Z'
    $symbols = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\abi-symbols.txt') -Raw
    if (-not $symbols.Contains($expected)) { throw 'The compiler ABI does not match the game import. Do not install this build.' }
    Write-Host 'PASS: MSVC argument types produce the exact game-imported DLSS symbol.'
    if ($AbiOnly) { exit 0 }

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'Verify-Streamline.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'Streamline SDK staging validation failed.' }

    $coreHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\include\sl_core_api.h') -Raw
    if (-not $coreHeader.Contains('slSetTagForFrame')) { throw 'Pinned Streamline core header is missing slSetTagForFrame.' }
    $dlssgHeader = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\include\sl_dlss_g.h') -Raw
    foreach ($marker in @('eFailGetCurrentBackBufferIndexNotCalled','eFailHDRFormatNotSupported','numFramesActuallyPresented','numFramesToGenerateMax','bIsDynamicMFGSupported','dynamicTargetFrameRate','eDynamic','colorBufferFormat')) {
        if (-not $dlssgHeader.Contains($marker)) { throw ('Pinned Streamline DLSS-G header is missing expected marker: ' + $marker) }
    }

    $exports = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\exports.txt') -Raw
    foreach ($name in @('CreateDXGIFactory','CreateDXGIFactory1','CreateDXGIFactory2','DXGIGetDebugInterface1','DXGIDeclareAdapterRemovalSupport')) {
        if ($exports -notmatch ('(?m)\b' + [regex]::Escape($name) + '\b')) { throw ('Missing proxy export: ' + $name) }
    }
    $imports = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'build\imports.txt') -Raw
    if ($imports -match '(?im)^\s+dxgi\.dll\s*$') { throw 'The proxy must not import itself.' }
    if ($imports -match '(?im)^\s+sl\.interposer\.dll\s*$') { throw 'v1.0.0 must load Streamline dynamically by absolute path.' }
    if ($imports -match '(?im)^\s+d3dcompiler_47\.dll\s*$') { throw 'v1.0.0 must resolve D3DCompile dynamically; do not add a static d3dcompiler import.' }

    $dll = Join-Path $PSScriptRoot 'build\dxgi.dll'
    $bytes = [IO.File]::ReadAllBytes($dll)
    $binaryText = [Text.Encoding]::ASCII.GetString($bytes)
    # UNICODE is defined for the C++ build, so wide string literals are UTF-16LE in the PE.
    $binaryUnicodeText = [Text.Encoding]::Unicode.GetString($bytes)
    $markers = @(
        'PROBE v1.0.0 ','source_revision=r1','mfg_mode=fixed_plus_native_dynamic','default_multiplier=4x','generated_frames_requested=3','target_frames_presented=4','max_selector=6x','dynamic_mode=native_eDynamic','dynamic_auto_target=explicit_game_monitor_refresh','dynamic_manual_target=30-1000_fps','dynamic_vsync_policy=syncinterval0_while_active','dynamic_reflex_limiter=target_fps','dynamic_pcl_simulation_start=control_beginframe_after_sleep','dynamic_target_ui=auto_manual_thick_slider','overlay=win32_layered_control_native_menu','persistence=localappdata_ini','overlay_status=selected,effective,current_fps,hdr,capability','overlay_title=embedded_control_fg_logo_control_native','overlay_font=bahnschrift_semicondensed','overlay_selected=white_fill_black_text','overlay_sections=control_red','selector=off,dynamic,2x,3x,4x,5x,6x','hdr10_bridge=transition_aware','hdr_transition_watch=dormant_rgb10_to_fp16',
        'CAMERA_SNAPSHOT','NGX_POST_EVAL','SWAPCHAIN_FRAME','HUD_RENDER_ENTER','PRESENT_PATH',
        'AA_TRANSITION','HUD_RTT_TARGET','HUD_RTT_FRAME_SUMMARY','HUD_COMMAND_CONTEXT','HUD_PRE_UI_CANDIDATE','HUD_COMMAND_CONTEXT_TLS_REJECT',
        'SL_BOOTSTRAP_BEGIN','SL_CORE_READY','SL_FACTORY_UPGRADE','SL_FACTORY_RETURN','SL_DEVICE_READY','SL_FEATURE_GATE','CONTROL_DLSS_READY',
        'SL_FRAME_FUNCTION','SL_REFLEX_OPTIONS','mode=low_latency','frame_limit_us=%u','FG_DYNAMIC_REFLEX_LIMITER','FG_DYNAMIC_REFLEX_LIMITER_RELEASE','dynamic_reflex_limiter=target_fps','reflex_mode=low_latency','SL_FRAME_TOKEN','SL_CONSTANTS','SL_CONSTANTS_SKIP',
        'SL_RESOURCE_TAG','kind=depth_mv','kind=hudless','SL_RESOURCE_TAG_SKIP','hdr10_bridge_optional_hudless_color_domain_mismatch','slSetTagForFrame',
        'SL_DEVICE_PROXY_READY','SL_QUEUE_CTOR_TARGET','SL_QUEUE_ROUTE_BEGIN','SL_QUEUE_ROUTE_END','HDR10_BRIDGE_QUEUE_CAPTURE',
        'SL_PRESENT_FRAME_GATE','SL_PRESENT_MARKER_START','SL_PRESENT_MARKER_END','SL_BACKBUFFER_INDEX','timing=before_present',
        'SL_DLSSG_MFG_CAPABILITY','SL_DLSSG_DYNAMIC_CAPABILITY','SL_DLSSG_MODE','SL_DLSSG_STATE','SL_DLSSG_STATE_SAMPLE','FG_FIRST_GENERATED_FRAME','FG_FIRST_TARGET_MFG_FRAME','FG_FIRST_4X_MFG_FRAME','FG_FIRST_5X_MFG_FRAME','FG_FIRST_6X_MFG_FRAME','FG_FIRST_DYNAMIC_MFG_FRAME','FG_DYNAMIC_SEGMENT_CONFIRMED','FG_MULTIPLIER_CHANGE','FG_MODE_CHANGE','FG_DYNAMIC_TARGET_CHANGE','FG_DISPLAY_REFRESH','FG_DYNAMIC_TARGET_SELECTION','FG_USER_SELECTION','FG_MODE_SEGMENT','FG_GENERATION_SEGMENT_CONFIRMED','dynamic_generated_state_samples','FG_RUNTIME_STATS',
        'slSetConstants','slGetNewFrameToken','slPCLSetMarker','slReflexSetOptions','slReflexSleep','SL_SIMULATION_MARKER_START','options_num_frames_to_generate=',
        'HDR10_FACTORY_WRAP','HDR10_BRIDGE_CREATE','initial_mode=%s','HDR10_BRIDGE_DORMANT','transition_watch=ResizeBuffers','HDR10_BRIDGE_DORMANT_RESIZE','HDR10_BRIDGE_ACTIVE','resize_activate','HDR10_BRIDGE_RESIZE_OK','HDR10_BRIDGE_DEACTIVATED','HDR10_BRIDGE_RESOURCES','HDR10_BRIDGE_PIPELINE_READY',
        'HDR10_BRIDGE_CONVERT','HDR10_BRIDGE_PRESENT_FAIL','HDR10_BRIDGE_COLORSPACE_','HDR10_BRIDGE_METADATA_SET','HDR10_BRIDGE_FALLBACK',
        'source_space=scrgb_rec709_linear','target_space=hdr10_bt2100','source_white_nits=80','target_transfer=st2084','target_primaries=bt2020',
        '305914b8-cf5b-4535-8e53-5589bf8cefa5','control_ngx_feature_path=ControlFGStreamline',
        'factory_upgrade=presentation_only','swapchain_upgrade=via_factory','command_queue_proxy=exact_ctor_private_device','queue_route=exact_constructor_only',
        'fg_activation=resource_gated','presentation_proxy=active','overlay=win32_layered_control_native_menu','persistence=localappdata_ini','overlay_status=selected,effective,current_fps,hdr,capability','overlay_title=embedded_control_fg_logo_control_native','overlay_font=bahnschrift_semicondensed','overlay_selected=white_fill_black_text','overlay_sections=control_red','FG_OVERLAY_READY','FG_OVERLAY_SELECTION','FG_OVERLAY_SELECTION_BLOCKED','FG_OVERLAY_DYNAMIC_TARGET','FG_SETTINGS_LOAD','FG_SETTINGS_SAVE','runtime_status=selected,effective,current_fps,hdr,capability','controls=auto,manual,thick_slider_30_1000','title=embedded_control_fg_logo_control_native','font=bahnschrift_semicondensed','selected_style=white_fill_black_text','section_headers=control_red','no_side_scrollbar=1','FG_OVERLAY_LOGO_LOAD','FG_DYNAMIC_VSYNC_BYPASS','requested_sync=%u','applied_sync=0','resource_tags=depth_mv_plus_hudless_sdr','hdr10_resource_tags=depth_mv_only_optional_hudless_off',
        'frame_gate=constants_plus_depth_mv','backbuffer_index=every_present','fg_baseline=working'
    )
    foreach ($marker in $markers) {
        if (-not $binaryText.Contains($marker)) { throw ('Wrong or incomplete v1.0.0 DLL: missing ' + $marker) }
    }
    foreach ($wideMarker in @('CONTROL_FG_DISABLE_HDR10_BRIDGE','\ControlFG','\settings.ini','FrameGeneration','DynamicTargetFPS','CURRENT FPS')) {
        if (-not $binaryUnicodeText.Contains($wideMarker)) {
            throw ('Wrong or incomplete v1.0.0 DLL: missing UTF-16 marker ' + $wideMarker)
        }
    }
    if ($binaryText.Contains('slEvaluateFeature')) { throw 'v1.0.0 DLSS-G Present path must not add an slEvaluateFeature call.' }

    $pe = [BitConverter]::ToInt32($bytes, 0x3c)
    if ([BitConverter]::ToUInt16($bytes, $pe + 4) -ne 0x8664) { throw 'Expected an x64 DLL.' }
    $sdkInfo = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'third_party\streamline\sdk-info.json') -Raw | ConvertFrom-Json
    [ordered]@{
        Version = '1.0.0'
        SourceRevision = 'r1'
        SHA256 = (Get-FileHash -LiteralPath $dll -Algorithm SHA256).Hash
        BuiltUtc = [DateTime]::UtcNow.ToString('o')
        AbiCheck = 'Passed'
        ExportCheck = 'Passed'
        Architecture = 'x64'
        StreamlineSDKVersion = $sdkInfo.Version
        StreamlineSDKArchiveSHA256 = $sdkInfo.SourceArchiveSHA256
        StreamlineLoadMode = 'Dynamic absolute-path manual hooking. v0.8.26 fixed 2x-6x, native Dynamic, HDR, Reflex and PCL SimulationStart runtime baseline is frozen. v1.0.0 changes only the external overlay presentation/style resource and retains persistence and read-only FG/HDR status telemetry; latency is intentionally absent.'
        DLSSGGenerationEnabled = $true
        DLSSGGeneratedFramesRequested = 3
        TargetMultiplier = '4x'
        ExpectedFramesPresented = 4
        MaxSelectableMultiplier = '6x'
        DynamicMFGEnabled = $true
        DynamicMFGMode = 'DLSSGMode::eDynamic'
        DynamicTargetFrameRate = 'Auto=explicit detected game-monitor refresh; Manual=user FPS'
        DynamicTargetPolicy = 'Auto uses QueryDisplayConfig/EnumDisplaySettingsEx; manual range 30-1000 FPS via slider; Present SyncInterval remains forced to 0 while Dynamic is API-enabled; Reflex frameLimitUs tracks the resolved Dynamic target'
        DynamicReflexLimiter = 'frameLimitUs=round(1000000/targetFPS) in Dynamic; frameLimitUs=0 in fixed/off modes'
        OverlayPersistence = '%LOCALAPPDATA%\ControlFG\settings.ini; mode + Dynamic target; schema 1'
        OverlayRuntimeStatus = 'Selected mode + last matching effective multiplier + current output FPS + resolved Dynamic target + HDR bridge state + capability; no latency metric'
        Confirmed2xBaseline = 'v0.8.18-r2 SDR + HDR'
        HDR10BridgeEnabled = $true
        HDR10BridgeSourceFormat = 'DXGI_FORMAT_R16G16B16A16_FLOAT'
        HDR10BridgePresentationFormat = 'DXGI_FORMAT_R10G10B10A2_UNORM'
        HDR10BridgePresentationColorSpace = 'DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020'
        HDR10Conversion = 'linear scRGB/Rec.709 -> linear BT.2020 -> ST.2084/PQ; 1.0 scRGB = 80 nits'
        HDRHUDLessPolicy = 'Optional HUD-less color tag disabled only while HDR10 bridge is active; Depth and MotionVectors remain required.'
        RuntimeTest = 'Confirm overlay has no latency field or external helper dependency; change FG mode and Dynamic target, restart Control, confirm persisted restore, and verify one HDR off/on transition.'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $PSScriptRoot 'build\build-validation.json') -Encoding UTF8
    Write-Host 'PASS: x64 DLL, DXGI exports, frozen v0.8.26 FG/Dynamic/HDR core, sleek self-contained FPS slider overlay, embedded CONTROL FG title resource, no latency field, and runtime status markers validated.'
    Write-Host 'Compilation/static checks only; the in-game test must prove settings restore across restart, overlay effective/HDR status is accurate, and the v0.8.26 fixed/Dynamic/HDR baseline remains healthy.'
} catch { Write-Error $_; exit 1 }
