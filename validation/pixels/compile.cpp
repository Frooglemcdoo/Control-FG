#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <atomic>
#include <string>
#include <utility>
#include <climits>
#include <cstdio>
#include <vector>
#include <cstring>
static std::atomic<unsigned long long> presentCount{0},fgAlignEpoch{0};
static void Log(const char*,...) noexcept {}
static void FGAlignArm(unsigned long long,const char*) noexcept {}
#include "../../src/fg_pixel_capture.h"
static bool IsHdr10BridgeActive() noexcept {return true;}
static ID3D12CommandQueue* GetHdr10BridgeDirectQueue() noexcept {return nullptr;}
void CompileCaptureEntryPoints(ID3D12GraphicsCommandList* list,ID3D12Resource* source){FGPixelPoll();FGPixelProducer(list,source,D3D12_RESOURCE_STATE_COMMON,1,source,D3D12_RESOURCE_STATE_COMMON,source,D3D12_RESOURCE_STATE_COMMON,nullptr,D3D12_RESOURCE_STATE_COMMON);FGPixelBridge(list,source,1,0);}

static HMODULE verifiedD3d=nullptr;
static constexpr size_t kNativeDevicePointerRva=0x136D28;
#include "../../src/fg_native_source.h"
void CompileNativeSource(const void* wrapper,ID3D12Resource* const* shadows){(void)FGNativeSourceSelect(wrapper,shadows,2,0);}

static bool FGAlignActive(unsigned long long) noexcept {return false;}
#include "../../src/fg_present_capture.h"
void CompilePresentCapture(IDXGISwapChain3* chain){FGPixelSDRPresent(chain,1);}


#include "../../src/fg_sdr_correction.h"
void CompileSDRCorrection(IDXGISwapChain3* chain){FGSDRCorrection correction;(void)correction.Apply(chain,chain,1);}
