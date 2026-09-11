#pragma once

#include "api/include/ffx_api.h"
#include "api/include/dx12/ffx_api_dx12.h"
#include "framegeneration/include/ffx_framegeneration.h"
#include "framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"

using FFXCreateContextFn = ffxReturnCode_t (*)(ffxContext*, ffxCreateContextDescHeader*, const ffxAllocationCallbacks*);
using FFXDestroyContextFn = ffxReturnCode_t (*)(ffxContext*, const ffxAllocationCallbacks*);
using FFXConfigureFn = ffxReturnCode_t (*)(ffxContext*, const ffxConfigureDescHeader*);
using FFXQueryFn = ffxReturnCode_t (*)(ffxContext*, ffxQueryDescHeader*);
using FFXDispatchFn = ffxReturnCode_t (*)(ffxContext*, const ffxDispatchDescHeader*);

static HMODULE fsrLoaderModule = nullptr;
static FFXCreateContextFn ffxCreateContextApi = nullptr;
static FFXDestroyContextFn ffxDestroyContextApi = nullptr;
static FFXConfigureFn ffxConfigureApi = nullptr;
static FFXQueryFn ffxQueryApi = nullptr;
static FFXDispatchFn ffxDispatchApi = nullptr;
static DLL_DIRECTORY_COOKIE fsrDllDirectoryCookie = nullptr;
static SRWLOCK fsrContextLock = SRWLOCK_INIT;
static ffxContext fsrSwapChainContext = nullptr;
static ffxContext fsrFrameGenerationContext = nullptr;
static std::atomic<IDXGISwapChain4*> fsrSwapChain{nullptr};
static std::atomic<ID3D12Device*> fsrDevice{nullptr};
static std::atomic<ID3D12CommandQueue*> fsrGameQueue{nullptr};
static std::atomic<unsigned int> fsrDisplayWidth{0}, fsrDisplayHeight{0}, fsrBackBufferFormat{0};
static std::atomic<unsigned int> fsrCoreReady{0}, fsrProviderReady{0};
static std::atomic<unsigned long long> fsrPrepareAttempts{0}, fsrPrepareSuccesses{0};
static std::atomic<unsigned long long> fsrConfigureCalls{0}, fsrConfigureSuccesses{0};
static std::atomic<unsigned long long> fsrGenerationCallbacks{0}, fsrDispatchSuccesses{0};
static std::atomic<unsigned long long> fsrPreparedFrame{0}, fsrLastPrepareFrame{0};
static uint64_t fsrSelectedProviderVersion = 0;
static char fsrSelectedProviderName[192]{};
static LONGLONG fsrLastFrameQpc = 0;

static bool IsFSR3BackendSelected() noexcept {
#ifdef CONTROLFG_FSR3_BRINGUP
    return true;
#else
    return false;
#endif
}
static bool IsFSR3RuntimeEnabled() noexcept {
    wchar_t value[16]{};
    DWORD n = GetEnvironmentVariableW(L"CONTROLFG_FSR3_DISABLE", value, _countof(value));
    return !(n && n < _countof(value) && value[0] != L'0');
}
static void FSR3DebugMessage(uint32_t type, const wchar_t* message) {
    char utf8[1536]{};
    if (message) WideCharToMultiByte(CP_UTF8, 0, message, -1, utf8, int(sizeof(utf8)), nullptr, nullptr);
    Log("FSR3_DEBUG type=%u message=%s", type, utf8[0] ? utf8 : "(empty)");
}
static bool InitializeFSR3Core(const char* trigger) noexcept {
    if (!IsFSR3BackendSelected()) return false;
    if (fsrCoreReady.load(std::memory_order_acquire)) return true;
    AcquireSRWLockExclusive(&fsrContextLock);
    if (fsrCoreReady.load(std::memory_order_relaxed)) { ReleaseSRWLockExclusive(&fsrContextLock); return true; }
    std::wstring runtimeDir = ParentDirectory(ModulePath(selfModule)) + L"\\ControlFGFidelityFX";
    std::wstring loaderPath = runtimeDir + L"\\amd_fidelityfx_loader_dx12.dll";
    std::wstring providerPath = runtimeDir + L"\\amd_fidelityfx_framegeneration_dx12.dll";
    LONG loaderTrust = TRUST_E_NOSIGNATURE, providerTrust = TRUST_E_NOSIGNATURE;
    if (!VerifyAuthenticode(loaderPath, &loaderTrust) || !VerifyAuthenticode(providerPath, &providerTrust)) {
        Log("FSR3_BOOTSTRAP_FAILED trigger=%s reason=signature loader_trust=0x%08lX provider_trust=0x%08lX", trigger, (unsigned long)loaderTrust, (unsigned long)providerTrust);
        ReleaseSRWLockExclusive(&fsrContextLock); return false;
    }
    using AddDllDirectoryFn = DLL_DIRECTORY_COOKIE (WINAPI*)(PCWSTR);
    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    auto addDirectory = kernel ? reinterpret_cast<AddDllDirectoryFn>(GetProcAddress(kernel, "AddDllDirectory")) : nullptr;
    if (addDirectory && !fsrDllDirectoryCookie) fsrDllDirectoryCookie = addDirectory(runtimeDir.c_str());
    fsrLoaderModule = LoadLibraryExW(loaderPath.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
    if (!fsrLoaderModule) {
        Log("FSR3_BOOTSTRAP_FAILED trigger=%s reason=load_loader error=%lu", trigger, GetLastError());
        ReleaseSRWLockExclusive(&fsrContextLock); return false;
    }
    ffxCreateContextApi = reinterpret_cast<FFXCreateContextFn>(GetProcAddress(fsrLoaderModule, "ffxCreateContext"));
    ffxDestroyContextApi = reinterpret_cast<FFXDestroyContextFn>(GetProcAddress(fsrLoaderModule, "ffxDestroyContext"));
    ffxConfigureApi = reinterpret_cast<FFXConfigureFn>(GetProcAddress(fsrLoaderModule, "ffxConfigure"));
    ffxQueryApi = reinterpret_cast<FFXQueryFn>(GetProcAddress(fsrLoaderModule, "ffxQuery"));
    ffxDispatchApi = reinterpret_cast<FFXDispatchFn>(GetProcAddress(fsrLoaderModule, "ffxDispatch"));
    bool ready = ffxCreateContextApi && ffxDestroyContextApi && ffxConfigureApi && ffxQueryApi && ffxDispatchApi;
    if (ready) {
        ffxConfigureDescGlobalDebug debug{};
        debug.header.type = FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG;
        debug.effectId = FFX_API_EFFECT_ID_FRAMEGENERATION;
        debug.fpMessage = &FSR3DebugMessage;
        debug.debugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_WARNINGS;
        auto debugResult = ffxConfigureApi(nullptr, &debug.header);
        fsrCoreReady.store(1, std::memory_order_release);
        Log("FSR3_CORE_READY trigger=%s sdk=2.3.0 fg_provider_target=3.1.6 swapchain=3.1.7 debug_result=%u", trigger, debugResult);
    } else Log("FSR3_BOOTSTRAP_FAILED trigger=%s reason=exports", trigger);
    ReleaseSRWLockExclusive(&fsrContextLock);
    return ready;
}
static void RememberFSR3RuntimeObjects(IDXGISwapChain4* chain, ID3D12CommandQueue* queue, unsigned int width, unsigned int height, DXGI_FORMAT format) noexcept {
    ID3D12Device* device = nullptr;
    if (!chain || !queue || FAILED(queue->GetDevice(IID_PPV_ARGS(&device))) || !device) return;
    queue->AddRef();
    auto oldQueue = fsrGameQueue.exchange(queue); if (oldQueue) oldQueue->Release();
    auto oldDevice = fsrDevice.exchange(device); if (oldDevice) oldDevice->Release();
    fsrSwapChain.store(chain); fsrDisplayWidth.store(width); fsrDisplayHeight.store(height); fsrBackBufferFormat.store(unsigned(format));
    Log("FSR3_SWAPCHAIN_READY chain=%p queue=%p device=%p display=%ux%u format=%u context=%p", chain, queue, device, width, height, unsigned(format), fsrSwapChainContext);
}
static HRESULT CreateFSR3SwapChainForHwnd(IDXGIFactory7* factory, IUnknown* deviceOrQueue, HWND hwnd, const DXGI_SWAP_CHAIN_DESC1* desc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc, IDXGIOutput* restrictOutput, IDXGISwapChain1** output) noexcept {
    if (!output) return E_POINTER; *output = nullptr;
    if (!factory || !deviceOrQueue || !desc || !InitializeFSR3Core("CreateSwapChainForHwnd")) return E_FAIL;
    if (restrictOutput) return E_NOTIMPL;
    ID3D12CommandQueue* queue = nullptr;
    if (FAILED(deviceOrQueue->QueryInterface(IID_PPV_ARGS(&queue))) || !queue) return E_NOINTERFACE;
    DXGI_SWAP_CHAIN_DESC1 copy = *desc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsCopy{}, *fsPtr = nullptr;
    if (fullscreenDesc) { fsCopy = *fullscreenDesc; fsPtr = &fsCopy; }
    IDXGISwapChain4* chain4 = nullptr;
    ffxCreateContextDescFrameGenerationSwapChainForHwndDX12 create{};
    create.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX12;
    create.swapchain=&chain4; create.hwnd=hwnd; create.desc=&copy; create.fullscreenDesc=fsPtr; create.dxgiFactory=factory; create.gameQueue=queue;
    ffxCreateContextDescFrameGenerationSwapChainVersionDX12 version{};
    version.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12;
    version.version=FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION; create.header.pNext=&version.header;
    ffxContext context=nullptr;
    auto result=ffxCreateContextApi(&context,&create.header,nullptr);
    Log("FSR3_SWAPCHAIN_CREATE entrypoint=CreateSwapChainForHwnd result=%u context=%p chain=%p display=%ux%u format=%u buffers=%u", result, context, chain4, copy.Width, copy.Height, unsigned(copy.Format), copy.BufferCount);
    if(result!=FFX_API_RETURN_OK||!context||!chain4){ if(context)ffxDestroyContextApi(&context,nullptr); if(chain4)chain4->Release(); queue->Release(); return E_FAIL; }
    fsrSwapChainContext=context; RememberFSR3RuntimeObjects(chain4,queue,copy.Width,copy.Height,copy.Format); queue->Release();
    *output=static_cast<IDXGISwapChain1*>(chain4); return S_OK;
}
static HRESULT CreateFSR3SwapChainLegacy(IDXGIFactory7* factory, IUnknown* deviceOrQueue, DXGI_SWAP_CHAIN_DESC* desc, IDXGISwapChain** output) noexcept {
    if(!output)return E_POINTER; *output=nullptr;
    if(!factory||!deviceOrQueue||!desc||!InitializeFSR3Core("CreateSwapChain"))return E_FAIL;
    ID3D12CommandQueue* queue=nullptr; if(FAILED(deviceOrQueue->QueryInterface(IID_PPV_ARGS(&queue)))||!queue)return E_NOINTERFACE;
    DXGI_SWAP_CHAIN_DESC copy=*desc; IDXGISwapChain4* chain4=nullptr;
    ffxCreateContextDescFrameGenerationSwapChainNewDX12 create{};
    create.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX12;
    create.swapchain=&chain4; create.desc=&copy; create.dxgiFactory=factory; create.gameQueue=queue;
    ffxCreateContextDescFrameGenerationSwapChainVersionDX12 version{};
    version.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_VERSION_DX12; version.version=FFX_FRAMEGENERATION_SWAPCHAIN_DX12_VERSION; create.header.pNext=&version.header;
    ffxContext context=nullptr; auto result=ffxCreateContextApi(&context,&create.header,nullptr);
    Log("FSR3_SWAPCHAIN_CREATE entrypoint=CreateSwapChain result=%u context=%p chain=%p display=%ux%u format=%u buffers=%u",result,context,chain4,copy.BufferDesc.Width,copy.BufferDesc.Height,unsigned(copy.BufferDesc.Format),copy.BufferCount);
    if(result!=FFX_API_RETURN_OK||!context||!chain4){if(context)ffxDestroyContextApi(&context,nullptr);if(chain4)chain4->Release();queue->Release();return E_FAIL;}
    fsrSwapChainContext=context;RememberFSR3RuntimeObjects(chain4,queue,copy.BufferDesc.Width,copy.BufferDesc.Height,copy.BufferDesc.Format);queue->Release();
    *output=static_cast<IDXGISwapChain*>(chain4);return S_OK;
}
static bool SelectFSR3Provider(ID3D12Device* device) noexcept {
    ffxQueryDescGetVersions query{}; query.header.type=FFX_API_QUERY_DESC_TYPE_GET_VERSIONS; query.createDescType=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION; query.device=device;
    uint64_t count=0; query.outputCount=&count; auto result=ffxQueryApi(nullptr,&query.header);
    if(result!=FFX_API_RETURN_OK||!count||count>64){Log("FSR3_PROVIDER_QUERY_FAILED result=%u count=%llu",result,count);return false;}
    std::vector<uint64_t> ids{size_t(count)}; std::vector<const char*> names{size_t(count)};
    query.versionIds=ids.data();query.versionNames=names.data();result=ffxQueryApi(nullptr,&query.header);
    if(result!=FFX_API_RETURN_OK)return false;
    for(uint64_t i=0;i<count;++i){const char* name=names[size_t(i)]?names[size_t(i)]:"(null)";Log("FSR3_PROVIDER index=%llu id=%llu name=%s",i,ids[size_t(i)],name);
        if(!fsrSelectedProviderVersion&&(strstr(name,"3.1")||strstr(name,"FSR3")||strstr(name,"FSR 3"))){fsrSelectedProviderVersion=ids[size_t(i)];strncpy_s(fsrSelectedProviderName,name,_TRUNCATE);}}
    if(!fsrSelectedProviderVersion){Log("FSR3_PROVIDER_QUERY_FAILED reason=no_fsr3_provider default_fsr4_not_allowed=1");return false;}
    Log("FSR3_PROVIDER_SELECTED id=%llu name=%s",fsrSelectedProviderVersion,fsrSelectedProviderName);return true;
}
static bool EnsureFSR3FrameGenerationContext(const SLNgxFrameInputs& inputs) noexcept {
    if(fsrFrameGenerationContext)return true; ID3D12Device* device=fsrDevice.load();
    if(!fsrCoreReady.load()||!fsrSwapChain.load()||!device||!SelectFSR3Provider(device))return false;
    ffxCreateContextDescFrameGeneration create{}; create.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION;
    create.flags=FFX_FRAMEGENERATION_ENABLE_DEBUG_CHECKING;
    if(inputs.createFlags&4)create.flags|=FFX_FRAMEGENERATION_ENABLE_MOTION_VECTORS_JITTER_CANCELLATION;
    if(inputs.createFlags&8)create.flags|=FFX_FRAMEGENERATION_ENABLE_DEPTH_INVERTED;
    create.displaySize={fsrDisplayWidth.load(),fsrDisplayHeight.load()};create.maxRenderSize=create.displaySize;
    create.backBufferFormat=ffxApiGetSurfaceFormatDX12(DXGI_FORMAT(fsrBackBufferFormat.load()));
    ffxCreateBackendDX12Desc backend{};backend.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;backend.device=device;
    ffxCreateContextDescFrameGenerationVersion version{};version.header.type=FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION_VERSION;version.version=FFX_FRAMEGENERATION_VERSION;
    ffxOverrideVersion provider{};provider.header.type=FFX_API_DESC_TYPE_OVERRIDE_VERSION;provider.versionId=fsrSelectedProviderVersion;
    create.header.pNext=&backend.header;backend.header.pNext=&version.header;version.header.pNext=&provider.header;
    ffxContext context=nullptr;auto result=ffxCreateContextApi(&context,&create.header,nullptr);
    Log("FSR3_CONTEXT_CREATE result=%u context=%p provider_id=%llu provider=%s display=%ux%u format=%u flags=0x%X",result,context,fsrSelectedProviderVersion,fsrSelectedProviderName,create.displaySize.width,create.displaySize.height,create.backBufferFormat,create.flags);
    if(result!=FFX_API_RETURN_OK||!context)return false;fsrFrameGenerationContext=context;fsrProviderReady.store(1);return true;
}
static ffxReturnCode_t FSR3GenerationCallback(ffxDispatchDescFrameGeneration* params,void* userContext){
    auto ordinal=++fsrGenerationCallbacks;if(!params||!userContext||!ffxDispatchApi)return FFX_API_RETURN_ERROR_PARAMETER;
    auto result=ffxDispatchApi(reinterpret_cast<ffxContext*>(userContext),&params->header);if(result==FFX_API_RETURN_OK)++fsrDispatchSuccesses;
    if(ordinal<=24||result!=FFX_API_RETURN_OK||(ordinal%240)==0)Log("FSR3_GENERATION_CALLBACK ordinal=%llu frame_id=%llu generated=%u reset=%u result=%u successes=%llu",ordinal,params->frameID,params->numGeneratedFrames,unsigned(params->reset),result,fsrDispatchSuccesses.load());
    return result;
}
static float FSR3FrameDeltaMilliseconds() noexcept {
    LARGE_INTEGER now{};QueryPerformanceCounter(&now);LONGLONG previous=fsrLastFrameQpc;fsrLastFrameQpc=now.QuadPart;
    if(!previous||!frequency.QuadPart)return 16.667f;double value=1000.0*double(now.QuadPart-previous)/double(frequency.QuadPart);
    if(!std::isfinite(value)||value<1.0)value=1.0;if(value>100.0)value=100.0;return float(value);
}
static void FSR3NormalizeVector(float out[3],double x,double y,double z) noexcept {double length=std::sqrt(x*x+y*y+z*z);if(length>1e-12&&std::isfinite(length)){out[0]=float(x/length);out[1]=float(y/length);out[2]=float(z/length);}else out[0]=out[1]=out[2]=0.f;}
static void SubmitFSR3PrepareForAA(unsigned long long call,unsigned long long currentPresent,void* const* textures,unsigned int textureCount,bool cameraKnown,const CameraSnapshot& camera,bool resetKnown,int resetValue) noexcept {
    if(!IsFSR3BackendSelected()||!IsFSR3RuntimeEnabled()||!fsrSwapChain.load())return;auto attempt=++fsrPrepareAttempts;
    SLNgxFrameInputs inputs{};SLTaggedResourceInput depth{},motion{};
    bool inputsReady=ReadSLNgxFrameInputs(&inputs)&&SLNgxFrameInputsComplete(inputs);
    bool depthReady=ReadSLNgxResourceWithState("Depth",textures,textureCount,&depth);bool motionReady=ReadSLNgxResourceWithState("MotionVectors",textures,textureCount,&motion);
    if(!cameraKnown||!inputsReady||!depthReady||!motionReady){if(attempt<=24||(attempt%240)==0)Log("FSR3_PREPARE_SKIP call=%llu present=%llu reason=inputs camera=%u ngx=%u depth=%u motion=%u",call,currentPresent,unsigned(cameraKnown),unsigned(inputsReady),unsigned(depthReady),unsigned(motionReady));return;}
    if(!EnsureFSR3FrameGenerationContext(inputs))return;
    EngineCommandContextSnapshot command{};bool commandKnown=ReadEngineCommandContext(depth.resource,&command);
    if(!commandKnown||!command.commandList||command.commandType!=D3D12_COMMAND_LIST_TYPE_DIRECT||!command.deviceMatch){if(attempt<=24||(attempt%240)==0)Log("FSR3_PREPARE_SKIP call=%llu present=%llu reason=command known=%u list=%p type=%u match=%u",call,currentPresent,unsigned(commandKnown),command.commandList,command.commandType,command.deviceMatch);return;}
    float nearPlane=0,farPlane=0,fov=0,aspect=0;bool inverted=(inputs.createFlags&8)!=0;
    if(!SLProjectionScalars(camera,inverted,&nearPlane,&farPlane,&fov,&aspect))return;
    uint64_t frameId=currentPresent+1,previous=fsrLastPrepareFrame.exchange(frameId);
    ffxDispatchDescFrameGenerationPrepareV2 prepare{};prepare.header.type=FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATION_PREPARE_V2;prepare.frameID=frameId;prepare.commandList=command.commandList;
    prepare.renderSize={inputs.renderWidth,inputs.renderHeight};prepare.jitterOffset={inputs.jitterX,inputs.jitterY};prepare.motionVectorScale={inputs.ngxMvScaleX,inputs.ngxMvScaleY};
    prepare.frameTimeDelta=FSR3FrameDeltaMilliseconds();prepare.reset=!previous||previous+1!=frameId||(resetKnown&&resetValue!=0);
    prepare.cameraNear=nearPlane;prepare.cameraFar=farPlane;prepare.cameraFovAngleVertical=fov;prepare.viewSpaceToMetersFactor=1.f;
    prepare.depth=ffxApiGetResourceDX12(depth.resource,FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);prepare.motionVectors=ffxApiGetResourceDX12(motion.resource,FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ);
    prepare.cameraPosition[0]=float(camera.viewToWorld[9]);prepare.cameraPosition[1]=float(camera.viewToWorld[10]);prepare.cameraPosition[2]=float(camera.viewToWorld[11]);
    FSR3NormalizeVector(prepare.cameraRight,camera.viewToWorld[0],camera.viewToWorld[1],camera.viewToWorld[2]);FSR3NormalizeVector(prepare.cameraUp,camera.viewToWorld[3],camera.viewToWorld[4],camera.viewToWorld[5]);FSR3NormalizeVector(prepare.cameraForward,camera.viewToWorld[6],camera.viewToWorld[7],camera.viewToWorld[8]);
    auto result=ffxDispatchApi(&fsrFrameGenerationContext,&prepare.header);if(result==FFX_API_RETURN_OK){++fsrPrepareSuccesses;fsrPreparedFrame.store(frameId,std::memory_order_release);}
    if(attempt<=24||result!=FFX_API_RETURN_OK||(attempt%240)==0)Log("FSR3_PREPARE call=%llu present=%llu frame_id=%llu result=%u render=%ux%u depth=%p mv=%p jitter=%.6g,%.6g mv_scale=%.6g,%.6g delta_ms=%.4f reset=%u command_list=%p successes=%llu",call,currentPresent,frameId,result,inputs.renderWidth,inputs.renderHeight,depth.resource,motion.resource,double(inputs.jitterX),double(inputs.jitterY),double(inputs.ngxMvScaleX),double(inputs.ngxMvScaleY),double(prepare.frameTimeDelta),unsigned(prepare.reset),command.commandList,fsrPrepareSuccesses.load());
}
static void ConfigureFSR3ForPresent(unsigned long long present) noexcept {
    if(!IsFSR3BackendSelected()||!ffxConfigureApi||!fsrFrameGenerationContext||!fsrSwapChain.load())return;auto call=++fsrConfigureCalls;
    bool hdrEnabled=false;if(originalIsHDREnabled){__try{hdrEnabled=originalIsHDREnabled();}__except(EXCEPTION_EXECUTE_HANDLER){hdrEnabled=true;}}
    bool prepared=fsrPreparedFrame.load(std::memory_order_acquire)==present;bool enabled=IsFSR3RuntimeEnabled()&&prepared&&!hdrEnabled;
    ffxConfigureDescFrameGeneration config{};config.header.type=FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATION;config.swapChain=fsrSwapChain.load();config.frameGenerationCallback=&FSR3GenerationCallback;config.frameGenerationCallbackUserContext=&fsrFrameGenerationContext;
    config.frameGenerationEnabled=enabled;config.allowAsyncWorkloads=false;config.HUDLessColor={};config.flags=0;config.onlyPresentGenerated=false;config.generationRect={0,0,fsrDisplayWidth.load(),fsrDisplayHeight.load()};config.frameID=present;
    auto result=ffxConfigureApi(&fsrFrameGenerationContext,&config.header);if(result==FFX_API_RETURN_OK)++fsrConfigureSuccesses;
    if(call<=24||result!=FFX_API_RETURN_OK||(call%240)==0)Log("FSR3_CONFIGURE present=%llu result=%u enabled=%u prepared=%u hdr=%u sync_compute=1 calls=%llu successes=%llu callbacks=%llu dispatch_successes=%llu",present,result,unsigned(enabled),unsigned(prepared),unsigned(hdrEnabled),call,fsrConfigureSuccesses.load(),fsrGenerationCallbacks.load(),fsrDispatchSuccesses.load());
}
