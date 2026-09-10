#pragma once
inline constexpr char kExeHash[] = "9441DB3AE75B267ABD989846AD0895E3FE24ABCC0F06E58F93C34FC8D4736506";
inline constexpr char kD3dHash[] = "CCEB99CBD9C019AF907C24C44701A53C8D230965C1FB31C325233E4C80214AA5";
inline constexpr char kRendererHash[] = "EBFB5B47CEBC2D4482E912B8090BD343F717BAB8A178B5A6385DEC9105E7C433";
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

