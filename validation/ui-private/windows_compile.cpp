// Compile the production UI path against real Windows/D3D12 interfaces.
// Host logging, capture and Streamline entrypoints are stubs in this TU only.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <atomic>
#include <vector>
#include <utility>
#include <new>
#include <climits>
#include <cstdio>
using D3DCompileDynamicFn=decltype(&D3DCompile);
using D3D12SerializeRootSignatureDynamicFn=decltype(&D3D12SerializeRootSignature);
template<class T> static void RRGuideRelease(T*& p){if(p)p->Release();p=nullptr;}
static void Log(const char*,...){}
static bool IsHdr10BridgeActive(){return false;}
static bool FGAlignActive(unsigned long long){return false;}
static ID3D12CommandQueue* GetHdr10BridgeDirectQueue(){return nullptr;}
static void FGPixelProducer(ID3D12GraphicsCommandList*,ID3D12Resource*,D3D12_RESOURCE_STATES,unsigned long long,ID3D12Resource*,D3D12_RESOURCE_STATES,ID3D12Resource*,D3D12_RESOURCE_STATES,ID3D12Resource*,D3D12_RESOURCE_STATES){}
namespace sl {
struct FrameToken{};struct CommandBuffer{};
struct Extent{unsigned width=0,height=0;};
enum class ResourceType{eTex2d};enum class ResourceLifecycle{eValidUntilPresent};enum class Result{eOk};
constexpr unsigned kBufferTypeHUDLessColor=0,kBufferTypeUIAlpha=1;
struct Resource{Resource(ResourceType,ID3D12Resource*,D3D12_RESOURCE_STATES){}};
struct ResourceTag{ResourceTag(Resource*,unsigned,ResourceLifecycle,Extent*){}};
}
static sl::Result (*slGetNativeInterfaceApi)(void*,void**)=nullptr;
static sl::FrameToken* GetSLFrameToken(unsigned long long,bool){return nullptr;}
static unsigned GetSLFrameTagMask(unsigned long long){return 0;}
static void MarkSLFrameTagBits(unsigned long long,unsigned){}
static long long SLResultCode(sl::Result){return 0;}
static sl::Result slSetTagForFrameApi(sl::FrameToken&,unsigned,sl::ResourceTag*,size_t,sl::CommandBuffer*){return sl::Result::eOk;}
static constexpr unsigned slFgViewport=0,kSLTagHUDLess=1,kSLTagUIAlpha=2;
static std::atomic<unsigned> slResourceTagCalls{0},slResourceTagSuccesses{0},slResourceTagFailures{0},slFgColorWidth{0},slFgColorHeight{0},slFgColorFormat{0},slFgHudLessFormat{0};
#include "../../src/fg_ui_recomposition.h"
void CompileUIEntryPoints(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,const D3D12_RESOURCE_DESC& desc){
 PrepareFGUIRecompositionBeforeHUD(0,resource,desc,0,true,list);
 FinalizeFGUIRecompositionAfterHUD(0,resource,desc,0,true,list);
 SubmitFGUIRecompositionBeforePresent(1);
}
