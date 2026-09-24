#pragma once
// Verified storefront identities. Keep the original k*Hash names as Steam aliases
// for historical validation/tests; runtime admission requires one complete triple.
inline constexpr char kExeHash[] = "9441DB3AE75B267ABD989846AD0895E3FE24ABCC0F06E58F93C34FC8D4736506";
inline constexpr char kD3dHash[] = "CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5";
inline constexpr char kRendererHash[] = "EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433";
inline constexpr char kEpicExeHash[] = "E1D11616941FAD767B20CD0FB1AB442771A912FF34EF404B5FB95136F53B6175";
inline constexpr char kEpicD3dHash[] = "EB598AAD837AF7B42305EB24D5F026AF416D46F055BDC5A2DA063BB99E6ABFC1";
inline constexpr char kEpicRendererHash[] = "85BAAF7026AFF1403ABAD6F497441963CE2F6012E9B84D462FED972E42723C6E";
// GOG uses the exact verified Steam renderer/D3D binaries; only Control_DX12.exe
// has a storefront-specific identity. Keep a complete triple so mixed installs fail closed.
inline constexpr char kGogExeHash[] = "57A8912F1FD839E99162AED2536914DEE01FF298938132354DA579ADC690E91E";
inline constexpr char kGogD3dHash[] = "CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5";
inline constexpr char kGogRendererHash[] = "EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433";
inline constexpr char kSteamTargetLabel[] = "steam_21225456";
inline constexpr char kEpicTargetLabel[] = "epic_0.0.518.2177";
inline constexpr char kGogTargetLabel[] = "gog_57a8912f";
inline constexpr char kAASymbol[] = "?doAntiAliasing@DLSS@d3d@@SA_NPEAVNativeTexture@2@00000000_NNNMMM@Z";
inline constexpr char kPresentSymbol[] = "?present@DeviceUtil@d3d@@SAXXZ";
inline constexpr char kBeginSymbol[] = "?beginFrame@FrameBeginHandler@d3d@@SAXXZ";
inline constexpr char kHUDSymbol[] = "?renderHUD@RendererInterfaceWrapper@rend@@QEAAXXZ";
inline constexpr char kRenderToTextureCtorSymbol[] = "??0RenderToTexture@d3d@@QEAA@PEBVNativeTexture@1@0@Z";
inline constexpr char kIsHdrEnabledSymbol[] = "?isHDREnabled@DeviceUtil@d3d@@SA_NXZ";
// Verified against the SHA-256 locked d3d DLL; relative to its loaded base.
inline constexpr size_t kDlssContextPointerRva = 0x111BE0;
inline constexpr size_t kNgxParametersOffset = 0x30;
inline constexpr size_t kNativeDevicePointerRva = 0x136D28;
inline constexpr size_t kSwapChainOffset = 0x178;

inline constexpr size_t kEngineFrameCounterRva = 0x136C68;

// v0.7 build-locked HUD-path evidence carried forward from v0.6 runtime/static validation. The renderer imports the two-NativeTexture
// RenderToTexture constructor through IAT RVA 0x5DFD80. Native HUD slot 17
// invokes that constructor at RVAs 0x134702 and 0x134870.
inline constexpr size_t kRenderToTextureCtorImportRva = 0x5DFD80;
inline constexpr size_t kHudPrimaryRttCallRva = 0x134702;
inline constexpr size_t kHudSecondaryRttCallRva = 0x134870;
inline constexpr size_t kNativeTextureStateOwnerOffset = 0x50;
inline constexpr size_t kNativeTextureInlineStateOffset = 0x40;
inline constexpr size_t kNativeTextureTrackedStateOffset = 0x20;

// v0.7 build-locked engine command-context evidence. DeviceState::retrieveForThread
// reads the TLS index at d3d+0x1115FC and returns TLSBlock+0x44250. The engine's
// own NativeTextureUtil::copy then reads *(TLSBlock+8), falls back to the global
// pointer at d3d+0x111C18, and invokes ID3D12GraphicsCommandList::CopyResource
// through the object stored at context+0x18. v0.7 only reads/queries these fields.
inline constexpr size_t kD3dTlsIndexRva = 0x1115FC;
inline constexpr size_t kD3dTlsBlockContextPointerOffset = 0x8;
inline constexpr size_t kD3dFallbackContextPointerRva = 0x111C18;
inline constexpr size_t kD3dContextGraphicsCommandListOffset = 0x18;

// Historical v0.8/v0.8.1 Streamline device-hook evidence retained for later stages. The hash-locked d3d module imports
// D3D12CreateDevice from d3d12.dll by ordinal 101 at IAT RVA 0x5D510.
// Runtime validation also compares that resolved IAT target with the named
// D3D12CreateDevice export before installing any hook.
inline constexpr unsigned short kD3D12CreateDeviceOrdinal = 101;
inline constexpr size_t kD3D12CreateDeviceImportRva = 0x5D510;

// The exact device-create call requests base ID3D12Device using the canonical
// IID stored at d3d+0x662A0. Control's CommandQueue constructor later invokes
// ID3D12Device::CreateCommandQueue through vtable slot 8 (offset 0x40) at
// d3d+0x4332D. v0.8.15 keeps Control's returned host device native, while
// routing the exact internal queue-constructor call through a private Streamline
// device proxy so Streamline can wrap the created command queues.
inline constexpr size_t kD3D12DeviceIidRva = 0x662A0;
inline constexpr size_t kCommandQueueCreateCallRva = 0x4332D;
inline constexpr size_t kD3D12DeviceCreateCommandQueueVtableSlot = 8;
inline constexpr size_t kID3D12DeviceVtableEntryCount = 44;

// v0.8.15 queue-routing hook. Control has exactly one direct call to its
// internal CommandQueue constructor at d3d+0x2B47E. That constructor
// targets d3d+0x43280 and reads the native ID3D12Device* from d3d+0x111C08
// immediately before ID3D12Device::CreateCommandQueue. The hook temporarily
// substitutes a private Streamline device proxy only while this constructor
// executes, then restores the native pointer before returning.
inline constexpr size_t kControlCommandQueueCtorCallRva = 0x2B47E;
inline constexpr size_t kControlCommandQueueCtorRva = 0x43280;
inline constexpr size_t kControlGlobalD3D12DevicePointerRva = 0x111C08;

// Control's existing NGX project identity recovered from the exact d3d binary.
inline constexpr char kControlNgxProjectId[] = "305914b8-cf5b-4535-8e53-5589bf8cefa5";

// v0.8.15 build-locked Control NGX coexistence hook. The only call to the
// internal NGX Init_with_ProjectID wrapper is the rel32 CALL at d3d+0x1E88A.
// The wrapper target is d3d+0x52CC0. v0.8.15 changes only the FeatureCommonInfo
// argument for that call by adding ControlFGStreamline as an additional NGX
// feature-DLL search path; Project ID, engine identity, device and all other
// arguments remain Control-owned.
inline constexpr size_t kControlNgxInitCallRva = 0x1E88A;
inline constexpr size_t kControlNgxInitWrapperRva = 0x52CC0;


// Ray Reconstruction Phase 2 observation-only hooks. These are exported by the
// hash-locked d3d module and imported by the renderer. Phase 2 patches only the
// renderer IAT slots for these exports so it can timestamp RT pipeline setup and
// dispatch ordering; renderer/d3d code bytes and GPU work remain untouched.
inline constexpr char kRRBeginPipelineSetupSymbol[] =
    "?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z";
inline constexpr char kRRSetRayGenerationSymbol[] =
    "?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z";
inline constexpr char kRRRaytraceSymbol[] =
    "?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z";


// Ray Reconstruction Phase 5 low-overhead direct snapshot path. P4 proved
// that intercepting ShaderTexture::setNativeTexture is far too hot for runtime
// use (~28k calls/sec in the validation capture). P5 does NOT hook that path.
// Instead it queries a small set of build-locked ShaderTexture globals through
// the read-only getNativeTexture() export only at the existing sampled DLSS
// boundary.
inline constexpr char kRRGetNativeTextureSymbol[] =
    "?getNativeTexture@ShaderTexture@d3d@@QEAAPEAVNativeTexture@2@XZ";

// Stable renderer-relative ShaderTexture objects recovered by the P4 capture.
// These are object addresses, not resource pointers; getNativeTexture() resolves
// the current per-frame NativeTexture without intercepting the hot binding path.
inline constexpr size_t kRRShaderReflectionTargetRva      = 0x01296788;
inline constexpr size_t kRRShaderDiffuseGIColorRva        = 0x012973E8;
inline constexpr size_t kRRShaderDiffuseGIWeightUavRva    = 0x01297478;
inline constexpr size_t kRRShaderDiffuseGIWeightSrvRva    = 0x012974A0;
inline constexpr size_t kRRShaderGBufferCandidate0Rva     = 0x01296440;
inline constexpr size_t kRRShaderGBufferCandidate1Rva     = 0x01296468;
inline constexpr size_t kRRShaderGBufferCandidate2Rva     = 0x01296490;
inline constexpr size_t kRRShaderGBufferCandidate3Rva     = 0x012964B8;
inline constexpr size_t kRRShaderGBufferCandidate4Rva     = 0x012964E0;
inline constexpr size_t kRRShaderLightBufferDiffuseRva    = 0x01297EF8;
inline constexpr size_t kRRShaderLightBufferSpecularRva   = 0x01297F20;

// P3 runtime/static correlation proved these build-locked renderer function
// ranges contain the relevant RR producer/guide setup work.
inline constexpr size_t kRRReflectionDiffuseFunctionBeginRva = 0x00128D20;
inline constexpr size_t kRRReflectionDiffuseFunctionEndRva   = 0x0012DA38;
inline constexpr size_t kRRGBufferNormalsFunctionBeginRva   = 0x001268A0;
inline constexpr size_t kRRGBufferNormalsFunctionEndRva     = 0x001278F4;
inline constexpr size_t kRRLightInputsFunctionBeginRva      = 0x000C86B0;
inline constexpr size_t kRRLightInputsFunctionEndRva        = 0x000C92E6;
inline constexpr size_t kRRLightBuffersFunctionBeginRva     = 0x001371A0;
inline constexpr size_t kRRLightBuffersFunctionEndRva       = 0x001373AB;
inline constexpr size_t kRRRadianceFunctionBeginRva         = 0x000A4AD0;
inline constexpr size_t kRRRadianceFunctionEndRva           = 0x000A5383;
inline constexpr size_t kRRShadowFunctionBeginRva           = 0x0023ED10;
inline constexpr size_t kRRShadowFunctionEndRva             = 0x0023FCD1;
