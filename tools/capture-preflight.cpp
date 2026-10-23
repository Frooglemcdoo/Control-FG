// Standalone driver preflight. Never loads or attaches to Control.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define CONTROL_CAPTURE_PREFLIGHT
#define CONTROL_LIGHTING_NO_OVERLAY
#include <windows.h>
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cwchar>
#include <vector>
#include <stdexcept>
#include <string>
#include <iostream>
static std::atomic<unsigned long long> presentCount{0};
static void Log(const char*, ...) {}
static bool PTReflectionSha256(const unsigned char*,size_t,char*) {return false;}
static HMODULE verifiedRenderer=nullptr,verifiedD3d=nullptr;
struct RRAlbedoCallPatch {};
static bool RRAlbedoPrepareCallPatch(void*,void*,void*,RRAlbedoCallPatch*) {return false;}
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch*,bool) {return false;}
static bool PTReflectionLoadSidecar(const wchar_t*,const char*,std::vector<unsigned char>&,char*,bool&) {return false;}
#include "../src/native_lighting_live_runtime.h"
#include "../build/capture_preflight_assets.h"
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr) {
    if(FAILED(hr)){printf("HRESULT=0x%08lX\n",static_cast<unsigned long>(hr));throw std::runtime_error("D3D call failed");}
}
#include "rt_spot_capture_translation_test.h"
static ComPtr<ID3D12InfoQueue> info;
static void Mark(const char* stage) {
    printf("BEGIN %s\n",stage);fflush(stdout);FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE));
}
static unsigned DrainDebug() {
    if(!info)return 0;
    unsigned errors=0;const auto count=info->GetNumStoredMessagesAllowedByRetrievalFilter();
    for(UINT64 i=0;i<count;++i) {
        SIZE_T size=0;if(FAILED(info->GetMessage(i,nullptr,&size)))continue;
        std::vector<unsigned char> bytes(size);auto* message=reinterpret_cast<D3D12_MESSAGE*>(bytes.data());
        if(FAILED(info->GetMessage(i,message,&size)))continue;
        if(message->Severity<=D3D12_MESSAGE_SEVERITY_ERROR)++errors;
        if(i<256)printf("D3D12 severity=%u id=%u %s\n",unsigned(message->Severity),unsigned(message->ID),message->pDescription);
    }
    if(count>256)printf("D3D12 messages_truncated=%llu\n",count-256);
    info->ClearStoredMessages();return errors;
}
static HRESULT STDMETHODCALLTYPE CreateStateLogged(ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** out) {
    Mark("CreateStateObject");const auto hr=device->CreateStateObject(desc,iid,out);
    printf("END CreateStateObject hr=0x%08lX\n",static_cast<unsigned long>(hr));return hr;
}
struct Roots {
    ComPtr<ID3D12RootSignature> global,local;
    void Create(ID3D12Device* device) {
        Mark("exact_native_global_root");Check(device->CreateRootSignature(0,kNativeRoot,sizeof(kNativeRoot),IID_PPV_ARGS(&global)));
        D3D12_ROOT_PARAMETER parameters[3]{};
        parameters[0].ParameterType=parameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;
        parameters[0].Descriptor={0,3};parameters[1].Descriptor={1,3};
        parameters[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[2].Constants={0,3,10};
        D3D12_ROOT_SIGNATURE_DESC desc{3,parameters,0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
        ComPtr<ID3DBlob> bytes,error;Check(D3D12SerializeRootSignature(&desc,D3D_ROOT_SIGNATURE_VERSION_1,&bytes,&error));
        Mark("native_local_root");Check(device->CreateRootSignature(0,bytes->GetBufferPointer(),bytes->GetBufferSize(),IID_PPV_ARGS(&local)));
    }
};
// Keep the validated R97 library's original hit/miss handlers. The live global
// root is byte-exact; the local root is reconstructed from R129's recorded ABI.
struct NativeState {
    D3D12_DXIL_LIBRARY_DESC library{{kNativeSpot,sizeof(kNativeSpot)},0,nullptr};
    D3D12_HIT_GROUP_DESC groups[2]{};
    D3D12_RAYTRACING_SHADER_CONFIG config{4,8};
    D3D12_RAYTRACING_PIPELINE_CONFIG pipeline{1};
    D3D12_GLOBAL_ROOT_SIGNATURE global{};
    D3D12_LOCAL_ROOT_SIGNATURE local{};
    LPCWSTR exports[2]={L"HitGroup_0_0",L"HitGroup_1_0"};
    D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION association{};
    D3D12_STATE_SUBOBJECT objects[8]{};
    D3D12_STATE_OBJECT_DESC desc{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,8,objects};
    explicit NativeState(const Roots& roots) {
        global.pGlobalRootSignature=roots.global.Get();local.pLocalRootSignature=roots.local.Get();
        for(unsigned i=0;i<2;++i){groups[i].HitGroupExport=exports[i];groups[i].Type=D3D12_HIT_GROUP_TYPE_TRIANGLES;groups[i].ClosestHitShaderImport=L"shadowClosestHit";}
        groups[1].AnyHitShaderImport=L"shadowAlphaTestAnyHit";
        objects[0]={D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library};
        objects[1]={D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&groups[0]};objects[2]={D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP,&groups[1]};
        objects[3]={D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG,&config};
        objects[4]={D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE,&global};
        objects[5]={D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE,&local};
        objects[6]={D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG,&pipeline};
        association={&objects[5],2,exports};objects[7]={D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION,&association};
    }
};
static void TestDescriptorForwarding(ID3D12Device* device) {
    namespace capture=native_lighting::rt_spot_capture_runtime;
    Mark("capture_resource_allocation_and_descriptor_hook_install");capture::Install(device);
    if(!capture::ready.load())throw std::runtime_error("capture hooks unavailable");
    auto& resources=capture::resources;
    D3D12_CONSTANT_BUFFER_VIEW_DESC cb{resources.upload->GetGPUVirtualAddress(),256};
    device->CreateConstantBufferView(&cb,resources.At(0));
    resources.RawSrv(resources.inputs.Get(),0,native_lighting::rt_spot_capture_gpu::kInputBytes,5);
    resources.RawUav(resources.results.Get(),native_lighting::rt_spot_capture_gpu::kResultBytes,41);
    for(UINT slot:{0u,5u,41u}) {
        auto from=resources.At(slot),to=resources.At(49+slot);
        device->CopyDescriptorsSimple(1,to,from,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        to=resources.At(98+slot);device->CopyDescriptors(1,&to,nullptr,1,&from,nullptr,D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    if(capture::phase.load()!=capture::Phase::Idle)throw std::runtime_error("unrequested capture armed");
    puts("PASS descriptor forwarding outside capture scope");
}
static int RunStage(const std::wstring& stage,bool warp) {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);std::cout<<std::unitbuf;
    try {
        Mark("device_setup");ComPtr<ID3D12Debug> debug;
        const auto debugHr=D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
        if(SUCCEEDED(debugHr))debug->EnableDebugLayer();
        printf("debug_layer=%u hr=0x%08lX\n",unsigned(SUCCEEDED(debugHr)),static_cast<unsigned long>(debugHr));
        ComPtr<IDXGIFactory6> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter1> adapter;
        if(warp)Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        else {
            for(UINT i=0;;++i) {
                ComPtr<IDXGIAdapter1> candidate;auto hr=factory->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&candidate));
                if(hr==DXGI_ERROR_NOT_FOUND)break;Check(hr);DXGI_ADAPTER_DESC1 desc{};Check(candidate->GetDesc1(&desc));
                if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
                if(SUCCEEDED(D3D12CreateDevice(candidate.Get(),D3D_FEATURE_LEVEL_12_0,__uuidof(ID3D12Device),nullptr))){adapter=candidate;break;}
            }
            if(!adapter)throw std::runtime_error("No hardware D3D12 adapter available");
        }
        DXGI_ADAPTER_DESC1 adapterDesc{};Check(adapter->GetDesc1(&adapterDesc));
        printf("adapter=%ls vendor=%04X device=%04X vram_bytes=%llu warp=%u\n",adapterDesc.Description,adapterDesc.VendorId,adapterDesc.DeviceId,static_cast<unsigned long long>(adapterDesc.DedicatedVideoMemory),unsigned(warp));
        LARGE_INTEGER version{};if(SUCCEEDED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice),&version)))printf("driver=%u.%u.%u.%u\n",HIWORD(version.HighPart),LOWORD(version.HighPart),HIWORD(version.LowPart),LOWORD(version.LowPart));
        ComPtr<ID3D12Device5> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)));
        device.As(&info);D3D12_FEATURE_DATA_D3D12_OPTIONS5 options{};const auto feature=device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5,&options,sizeof(options));
        printf("dxr_tier=%u feature_hr=0x%08lX\n",unsigned(options.RaytracingTier),static_cast<unsigned long>(feature));
        const bool needsDxr=stage==L"native"||stage==L"hit"||stage==L"input";
        if(needsDxr&&(FAILED(feature)||options.RaytracingTier==D3D12_RAYTRACING_TIER_NOT_SUPPORTED)){puts("SKIP DXR unavailable; this is not a pass");return 77;}
        Roots roots;roots.Create(device.Get());ComPtr<ID3D12Fence> owner;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&owner)));
        native_lighting::rt_shadow_capture::originalState=&CreateStateLogged;
        if(stage==L"native") {NativeState state(roots);ComPtr<ID3D12StateObject> result;Mark("R97_reference_pipeline");Check(CreateStateLogged(device.Get(),&state.desc,IID_PPV_ARGS(&result)));}
        else if(stage==L"hit") {NativeState state(roots);Mark("shared_native_alpha_capture_pipeline");Check(native_lighting::rt_spot_hit_pipeline::Prepare(device.Get(),&state.desc,owner.Get()));}
        else if(stage==L"input") {Mark("shared_R128_input_pipeline");Check(native_lighting::rt_spot_capture_gpu::PrepareInputPipeline(device.Get(),owner.Get(),roots.global.Get()));}
        else if(stage==L"tables") {Mark("shared_translation_pipelines_native_root");Check(native_lighting::rt_spot_capture_gpu::PrepareTablePipelines(device.Get(),owner.Get(),roots.global.Get()));Mark("GPU_translation_readback");TestGpuHitTableTranslation(device.Get());}
        else if(stage==L"hooks") TestDescriptorForwarding(device.Get());
        else throw std::runtime_error("Unknown stage");
        Check(device->GetDeviceRemovedReason());
        if(DrainDebug())throw std::runtime_error("D3D12 validation errors");
        printf("PASS stage=%ls\n",stage.c_str());return 0;
    } catch(const std::exception& e){printf("FAIL stage=%ls reason=%s\n",stage.c_str(),e.what());DrainDebug();return 1;}
}
static void WriteLog(HANDLE file,const std::string& text) {
    DWORD written=0;if(!WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr)||written!=text.size())throw std::runtime_error("Cannot write preflight log");
    FlushFileBuffers(file);
}
static int RunParent(bool warp) {
    wchar_t exe[32768]{},folder[MAX_PATH]{},path[MAX_PATH]{};
    if(!GetModuleFileNameW(nullptr,exe,32768))return 1;
    DWORD length=GetEnvironmentVariableW(L"LOCALAPPDATA",folder,MAX_PATH);
    if(!length||length>=MAX_PATH){if(!GetTempPathW(MAX_PATH,folder))return 1;}
    std::wstring directory=std::wstring(folder)+L"\\Control-FG";CreateDirectoryW(directory.c_str(),nullptr);
    directory+=L"\\Diagnostics";CreateDirectoryW(directory.c_str(),nullptr);
    SYSTEMTIME time{};GetLocalTime(&time);
    swprintf_s(path,L"%s\\Capture-R130-%04u%02u%02u-%02u%02u%02u-%lu.log",directory.c_str(),time.wYear,time.wMonth,time.wDay,time.wHour,time.wMinute,time.wSecond,GetCurrentProcessId());
    SECURITY_ATTRIBUTES security{sizeof(security),nullptr,TRUE};
    HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){printf("Cannot create log. Windows error %lu\n",GetLastError());return 1;}
    printf("Capture preflight R130. Keep Control closed.\nLog: %ls\n",path);
    int result=0;
    try {
        WriteLog(file,"Control-FG capture preflight R130\r\nStandalone; no game process, mod injection, native scene or ray dispatch.\r\n");
        for(const wchar_t* stage:{L"native",L"hit",L"input",L"tables",L"hooks"}) {
            printf("Checking %ls...\n",stage);
            std::wstring command=L"\""+std::wstring(exe)+L"\" --stage "+stage+(warp?L" --warp":L"");
            STARTUPINFOW startup{};startup.cb=sizeof(startup);startup.dwFlags=STARTF_USESTDHANDLES;
            startup.hStdOutput=startup.hStdError=file;startup.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
            PROCESS_INFORMATION process{};
            std::string label(stage,stage+wcslen(stage));WriteLog(file,"\r\nSTAGE "+label+"\r\n");
            if(!CreateProcessW(exe,command.data(),nullptr,nullptr,TRUE,CREATE_NO_WINDOW,nullptr,nullptr,&startup,&process))throw std::runtime_error("Cannot launch stage process");
            CloseHandle(process.hThread);
            const auto wait=WaitForSingleObject(process.hProcess,45000);DWORD code=1;
            if(wait!=WAIT_OBJECT_0){TerminateProcess(process.hProcess,124);WaitForSingleObject(process.hProcess,5000);code=124;}
            else GetExitCodeProcess(process.hProcess,&code);
            CloseHandle(process.hProcess);WriteLog(file,"STAGE_END "+label+" exit="+std::to_string(code)+"\r\n");
            if(code!=0 && !(warp&&code==77))result=1;
            if(code==124 || code>=0x80000000u){WriteLog(file,"Stopped after timeout or process crash. Last BEGIN identifies the active operation.\r\n");break;}
        }
        WriteLog(file,std::string("PREFLIGHT_END result=")+(result?"FAIL":"PASS")+(warp?" scope=WARP_only_DXR_may_be_skipped\r\n":" scope=hardware_startup_only\r\n"));
    } catch(const std::exception& e){printf("Preflight error: %s\n",e.what());result=1;}
    CloseHandle(file);printf("Finished. Send this file:\n%ls\n",path);return result;
}
int wmain(int argc,wchar_t** argv) {
    bool warp=false;std::wstring stage;
    for(int i=1;i<argc;++i){if(!wcscmp(argv[i],L"--warp"))warp=true;else if(!wcscmp(argv[i],L"--stage")&&i+1<argc)stage=argv[++i];else return 2;}
    return stage.empty()?RunParent(warp):RunStage(stage,warp);
}
