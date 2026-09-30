#pragma once
// PT reflection probe R1.
//
// This does two deliberately narrow things:
//  1) overrides only Control's verified g_uRTReflectionRayCount provider call with
//     Native/1/2/4 stochastic reflection rays per pixel; and
//  2) observes ID3D12Device5::CreateStateObject so we can recover the exact
//     reflection DXR library/payload/recursion contract for the recursive-bounce
//     replacement. R1 does not claim that sample count is bounce count.
//
// Every patch is build-locked and falls back to the native path when validation
// fails. No generic D3D12 state object or provider calls are modified.

namespace control_pt_reflection {

inline constexpr std::size_t ReflectionRayCountCallRva = 0x12930D;
inline constexpr std::size_t SetProviderIatRva = 0x5DFF90;
inline constexpr unsigned CreateStateObjectVtableSlot = 62;
inline constexpr char SetProviderName[] =
    "?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z";

static std::atomic<unsigned int> requestedRays{0}; // 0 = native
static std::atomic<unsigned int> effectiveRays{0};
static std::atomic<unsigned int> providerHookReady{0};
static std::atomic<unsigned int> stateObjectHookReady{0};
static std::atomic<unsigned long long> providerCalls{0};
static std::atomic<unsigned long long> stateObjectCalls{0};
static std::atomic<unsigned long long> reflectionStateObjects{0};

inline unsigned NormalizeRays(unsigned value) noexcept {
    return value==1u||value==2u||value==4u ? value : 0u;
}
inline unsigned RequestedRays() noexcept {
    return NormalizeRays(requestedRays.load(std::memory_order_acquire));
}
inline unsigned EffectiveRays() noexcept {
    return effectiveRays.load(std::memory_order_acquire);
}
inline void SelectRays(unsigned value) noexcept {
    requestedRays.store(NormalizeRays(value),std::memory_order_release);
}
inline bool ProviderHookReady() noexcept {
    return providerHookReady.load(std::memory_order_acquire)!=0;
}
inline bool StateObjectHookReady() noexcept {
    return stateObjectHookReady.load(std::memory_order_acquire)!=0;
}

using SetProviderFn = void (*)(int,const void*,const char*);
static SetProviderFn originalSetProvider=nullptr;
static unsigned char* providerCallsite=nullptr;
static unsigned char providerOriginal[6]{};
static unsigned char providerReplacement[6]{};
static void* providerRelay=nullptr;

inline bool ReadU32(const void* data,unsigned& value) noexcept {
    value=0;
    if(!data)return false;
    __try { std::memcpy(&value,data,sizeof(value)); return true; }
    __except(EXCEPTION_EXECUTE_HANDLER) { value=0; return false; }
}

static void HookSetReflectionRayCount(int provider,const void* data,const char* name) noexcept {
    const DWORD incoming=GetLastError();
    unsigned nativeValue=0;
    const bool readable=ReadU32(data,nativeValue);
    const unsigned requested=RequestedRays();
    const unsigned overrideValue=requested?requested:nativeValue;
    const void* forwarded=(requested&&readable)?static_cast<const void*>(&overrideValue):data;

    SetLastError(incoming);
    originalSetProvider(provider,forwarded,name);
    const DWORD nativeError=GetLastError();

    effectiveRays.store((requested&&readable)?requested:nativeValue,std::memory_order_release);
    const auto call=providerCalls.fetch_add(1,std::memory_order_acq_rel)+1;
    static std::atomic<unsigned int> lastRequested{0xFFFFFFFFu};
    const unsigned previous=lastRequested.exchange(requested,std::memory_order_acq_rel);
    if(call<=4||previous!=requested||(call%600)==0) {
        Log("PT_REFLECTION_RAYS call=%llu requested=%u native=%u effective=%u readable=%u provider=%d mode=%s max_native_layers=4",
            call,requested,nativeValue,(requested&&readable)?requested:nativeValue,unsigned(readable),provider,
            requested?"override":"native");
    }
    SetLastError(nativeError);
}

inline bool InstallProviderHook(HMODULE renderer,HMODULE d3d) noexcept {
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d)return false;
    if(providerHookReady.load(std::memory_order_acquire))return true;

    auto* base=reinterpret_cast<unsigned char*>(renderer);
    auto* site=base+ReflectionRayCountCallRva;
    auto** expectedSlot=reinterpret_cast<void**>(base+SetProviderIatRva);
    void* expectedFunction=GetProcAddress(d3d,SetProviderName);
    if(!expectedFunction||!expectedSlot||*expectedSlot!=expectedFunction)return false;

    // Exact six-byte call recovered from the verified renderer:
    // FF 15 7D 6C 4B 00 -> [renderer+0x5DFF90].
    const unsigned char expectedBytes[6]{0xFF,0x15,0x7D,0x6C,0x4B,0x00};
    if(std::memcmp(site,expectedBytes,sizeof(expectedBytes))!=0)return false;
    std::int32_t displacement=0;std::memcpy(&displacement,site+2,sizeof(displacement));
    auto** decoded=reinterpret_cast<void**>(site+6+displacement);
    if(decoded!=expectedSlot)return false;

    providerRelay=AllocateExecutableRelayNear(site);
    if(!providerRelay)return false;
    void* hook=reinterpret_cast<void*>(&HookSetReflectionRayCount);
    unsigned char stub[14]{0xFF,0x25,0,0,0,0};
    std::memcpy(stub+6,&hook,sizeof(hook));
    std::memcpy(providerRelay,stub,sizeof(stub));
    DWORD relayProtection=0;
    if(!VirtualProtect(providerRelay,4096,PAGE_EXECUTE_READ,&relayProtection)||
       !FlushInstructionCache(GetCurrentProcess(),providerRelay,sizeof(stub))||
       !Rel32Fits(site+5,providerRelay)) {
        Log("PT_REFLECTION_PROVIDER_HOOK ready=0 reason=relay_prepare_failed error=%lu",GetLastError());
        return false;
    }

    std::memcpy(providerOriginal,site,sizeof(providerOriginal));
    providerReplacement[0]=0xE8;
    const auto delta64=reinterpret_cast<std::intptr_t>(providerRelay)-reinterpret_cast<std::intptr_t>(site+5);
    if(delta64<INT32_MIN||delta64>INT32_MAX)return false;
    const std::int32_t delta=static_cast<std::int32_t>(delta64);
    std::memcpy(providerReplacement+1,&delta,sizeof(delta));
    providerReplacement[5]=0x90;

    originalSetProvider=reinterpret_cast<SetProviderFn>(expectedFunction);
    DWORD previous=0;
    if(!VirtualProtect(site,sizeof(providerReplacement),PAGE_EXECUTE_READWRITE,&previous))return false;
    const bool unchanged=std::memcmp(site,providerOriginal,sizeof(providerOriginal))==0;
    if(unchanged)std::memcpy(site,providerReplacement,sizeof(providerReplacement));
    const bool flushed=unchanged&&FlushInstructionCache(GetCurrentProcess(),site,sizeof(providerReplacement))!=FALSE;
    DWORD ignored=0;const bool restored=VirtualProtect(site,sizeof(providerReplacement),previous,&ignored)!=FALSE;
    const bool installed=flushed&&restored&&std::memcmp(site,providerReplacement,sizeof(providerReplacement))==0;
    if(!installed&&unchanged) {
        DWORD rollback=0;
        if(VirtualProtect(site,sizeof(providerOriginal),PAGE_EXECUTE_READWRITE,&rollback)) {
            std::memcpy(site,providerOriginal,sizeof(providerOriginal));
            FlushInstructionCache(GetCurrentProcess(),site,sizeof(providerOriginal));
            DWORD rollbackIgnored=0;VirtualProtect(site,sizeof(providerOriginal),rollback,&rollbackIgnored);
        }
    }
    providerCallsite=installed?site:nullptr;
    providerHookReady.store(installed?1u:0u,std::memory_order_release);
    Log("PT_REFLECTION_PROVIDER_HOOK ready=%u callsite_rva=0x%zX iat_rva=0x%zX options=native,1,2,4 max_native_layers=4",
        unsigned(installed),ReflectionRayCountCallRva,SetProviderIatRva);
    return installed;
}

using CreateStateObjectFn = HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device5*,const D3D12_STATE_OBJECT_DESC*,REFIID,void**);
static SRWLOCK stateObjectLock=SRWLOCK_INIT;
static ID3D12Device5* stateObjectHost=nullptr;
static void** stateObjectTable=nullptr;
static CreateStateObjectFn originalCreateStateObject=nullptr;

inline bool ExchangePointer(void** slot,void* expected,void* replacement) noexcept {
    if(!slot||!expected||!replacement||*slot!=expected)return false;
    DWORD protection=0;
    if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&protection))return false;
    const bool changed=InterlockedCompareExchangePointer(slot,replacement,expected)==expected;
    DWORD ignored=0;const bool restored=VirtualProtect(slot,sizeof(void*),protection,&ignored)!=FALSE;
    if(!restored)Log("PT_REFLECTION_STATE_VTABLE_PROTECTION_RESTORE_FAILED slot=%p error=%lu",slot,GetLastError());
    return changed&&restored;
}

inline std::uint32_t CRC32(const void* data,std::size_t size) noexcept {
    std::uint32_t crc=0xffffffffu;
    const auto* bytes=static_cast<const unsigned char*>(data);
    for(std::size_t i=0;i<size;++i){
        crc^=bytes[i];
        for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^(0xedb88320u&(0u-(crc&1u)));
    }
    return crc^0xffffffffu;
}

inline bool ContainsAscii(const void* data,std::size_t size,const char* needle) noexcept {
    if(!data||!needle)return false;
    const std::size_t n=std::strlen(needle);
    if(!n||size<n||size>(128ull<<20))return false;
    const auto* bytes=static_cast<const unsigned char*>(data);
    __try {
        for(std::size_t i=0;i+n<=size;++i)
            if(bytes[i]==static_cast<unsigned char>(needle[0])&&std::memcmp(bytes+i,needle,n)==0)return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    return false;
}

inline bool ExportNamesContainReflection(const D3D12_DXIL_LIBRARY_DESC* library) noexcept {
    if(!library||!library->pExports||!library->NumExports||library->NumExports>4096)return false;
    __try {
        for(UINT i=0;i<library->NumExports;++i) {
            const wchar_t* name=library->pExports[i].Name;
            const wchar_t* rename=library->pExports[i].ExportToRename;
            if((name&&wcsstr(name,L"reflectionRayGeneration"))||
               (rename&&wcsstr(rename,L"reflectionRayGeneration")))return true;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
    return false;
}

inline void InspectStateObject(const D3D12_STATE_OBJECT_DESC* desc) noexcept {
    if(!desc||!desc->pSubobjects||!desc->NumSubobjects||desc->NumSubobjects>512)return;
    unsigned libraries=0,hitGroups=0,payloadBytes=0,attributeBytes=0,recursionDepth=0;
    bool reflection=false;std::size_t reflectionBytes=0;std::uint32_t reflectionCrc=0;
    __try {
        for(UINT i=0;i<desc->NumSubobjects;++i) {
            const auto& sub=desc->pSubobjects[i];
            if(!sub.pDesc)continue;
            if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY) {
                ++libraries;
                const auto* lib=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(sub.pDesc);
                const auto& code=lib->DXILLibrary;
                const bool match=ExportNamesContainReflection(lib)||
                    ContainsAscii(code.pShaderBytecode,code.BytecodeLength,"reflectionRayGeneration");
                if(match&&!reflection) {
                    reflection=true;reflectionBytes=code.BytecodeLength;
                    if(code.pShaderBytecode&&code.BytecodeLength&&code.BytecodeLength<=(128ull<<20))
                        reflectionCrc=CRC32(code.pShaderBytecode,code.BytecodeLength);
                }
            } else if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP) {
                ++hitGroups;
            } else if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG) {
                const auto* cfg=static_cast<const D3D12_RAYTRACING_SHADER_CONFIG*>(sub.pDesc);
                payloadBytes=cfg->MaxPayloadSizeInBytes;attributeBytes=cfg->MaxAttributeSizeInBytes;
            } else if(sub.Type==D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG) {
                const auto* cfg=static_cast<const D3D12_RAYTRACING_PIPELINE_CONFIG*>(sub.pDesc);
                recursionDepth=cfg->MaxTraceRecursionDepth;
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("PT_REFLECTION_STATE_OBJECT_INSPECT fault=0x%08lX",GetExceptionCode());
        return;
    }
    const auto call=stateObjectCalls.load(std::memory_order_acquire);
    if(reflection)reflectionStateObjects.fetch_add(1,std::memory_order_acq_rel);
    if(reflection||call<=12) {
        Log("PT_REFLECTION_STATE_OBJECT call=%llu type=%u subobjects=%u libraries=%u hit_groups=%u reflection_library=%u reflection_bytes=%zu reflection_crc32=0x%08X payload_bytes=%u attribute_bytes=%u recursion_depth=%u",
            call,unsigned(desc->Type),desc->NumSubobjects,libraries,hitGroups,unsigned(reflection),reflectionBytes,
            reflectionCrc,payloadBytes,attributeBytes,recursionDepth);
    }
}

static HRESULT STDMETHODCALLTYPE HookCreateStateObject(
    ID3D12Device5* device,const D3D12_STATE_OBJECT_DESC* desc,REFIID iid,void** result) noexcept {
    const DWORD incoming=GetLastError();
    SetLastError(incoming);
    const HRESULT hr=originalCreateStateObject(device,desc,iid,result);
    const DWORD nativeError=GetLastError();
    const auto call=stateObjectCalls.fetch_add(1,std::memory_order_acq_rel)+1;
    (void)call;
    if(SUCCEEDED(hr)&&device==stateObjectHost)InspectStateObject(desc);
    SetLastError(nativeError);
    return hr;
}

inline bool InstallDevice(IUnknown* unknown) noexcept {
    ID3D12Device5* device=nullptr;
    if(!unknown||FAILED(unknown->QueryInterface(IID_PPV_ARGS(&device)))||!device) {
        Log("PT_REFLECTION_STATE_HOOK ready=0 reason=device5_unavailable");
        return false;
    }
    AcquireSRWLockExclusive(&stateObjectLock);
    bool okay=false;
    auto** table=*reinterpret_cast<void***>(device);
    if(stateObjectTable) {
        okay=stateObjectTable==table&&stateObjectHost==device&&
            table[CreateStateObjectVtableSlot]==reinterpret_cast<void*>(&HookCreateStateObject);
    } else {
        stateObjectHost=device;
        originalCreateStateObject=reinterpret_cast<CreateStateObjectFn>(table[CreateStateObjectVtableSlot]);
        if(originalCreateStateObject&&ExchangePointer(
            &table[CreateStateObjectVtableSlot],reinterpret_cast<void*>(originalCreateStateObject),
            reinterpret_cast<void*>(&HookCreateStateObject))) {
            stateObjectTable=table;device->AddRef();okay=true;
        } else {
            stateObjectHost=nullptr;originalCreateStateObject=nullptr;
        }
    }
    ReleaseSRWLockExclusive(&stateObjectLock);
    device->Release();
    stateObjectHookReady.store(okay?1u:0u,std::memory_order_release);
    Log("PT_REFLECTION_STATE_HOOK ready=%u interface=ID3D12Device5 vtable_slot=%u mode=observe_only",
        unsigned(okay),CreateStateObjectVtableSlot);
    return okay;
}

} // namespace control_pt_reflection
