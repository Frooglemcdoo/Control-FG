#pragma once
#include "rr_indirect_call_windows.h"
#include "pt_reflection_user_control.h"
// Experimental PT reflection P1/P2 runtime bridge.
//
// Scope is deliberately limited to the three build-locked direct calls in the
// native RT-reflection pass:
//   renderer+0x12953C -> renderer+0x1D9960 ("rt_reflection.rfx")
//   renderer+0x12954B -> renderer+0x1D2CE0 ("rt_reflectionDeferredShading")
//   renderer+0x12956C -> renderer+0x1DD450 (RT variant resolver)
//
// Every hook calls the native helper exactly once and returns its exact result.
// No renderer object, shader bytecode, state object, descriptor, or GPU command
// is modified in P1-discovery mode. The final hook identifies the selected
// resident ray library byte-for-byte and admits only one of the 32 known
// rt_reflection DXIL containers recovered from the exact supported renderer.

using PTReflectionRfxLookupFn = void* (*)(void*, const char*);
using PTReflectionTechniqueLookupFn = void* (*)(void*, const char*);
using PTReflectionVariantLookupFn = void* (*)(void*, unsigned);
using PTShaderCodeStoragePersistFn = int (__cdecl*)(const char*, unsigned long long, unsigned);
using PTShaderCodeStorageIsPersistedFn = bool (__cdecl*)(const char*);
using PTShaderCodeStorageGetFn = const char* (__cdecl*)(int);

static PTReflectionRfxLookupFn ptReflectionOriginalRfxLookup=nullptr;
static PTReflectionTechniqueLookupFn ptReflectionOriginalTechniqueLookup=nullptr;
static PTReflectionVariantLookupFn ptReflectionOriginalVariantLookup=nullptr;
static PTShaderCodeStoragePersistFn ptReflectionStoragePersist=nullptr;
static PTShaderCodeStorageIsPersistedFn ptReflectionStorageIsPersisted=nullptr;
static RRAlbedoCallPatch ptReflectionLookupPatches[6]{};
static RRAlbedoCallPatch ptReflectionStorageGetPatches[6]{};
static PTShaderCodeStorageGetFn ptReflectionOriginalStorageGet=nullptr;
static constexpr int kPTReflectionP1SyntheticStorageId=0x6F475031;
static constexpr int kPTReflectionP2RaySyntheticStorageId=0x6F475032;
static constexpr int kPTReflectionP2CompositeSyntheticStorageId=0x6F475033;
static std::atomic<unsigned long long> ptReflectionP1GetIntercepts{0};
static std::atomic<unsigned long long> ptReflectionP2RayGetIntercepts{0};
static std::atomic<unsigned long long> ptReflectionP2CompositeGetIntercepts{0};
static unsigned char ptReflectionP1ShaderRecord[0xA8]{};
static unsigned char ptReflectionP1ShaderDescriptor[0x100]{};
static unsigned char ptReflectionP2RayShaderRecord[0xA8]{};
static unsigned char ptReflectionP2RayShaderDescriptor[0x100]{};
static unsigned char ptReflectionP2CompositeShaderRecord[0xA8]{};
static unsigned char ptReflectionP2CompositeShaderDescriptor[0x100]{};
static void* ptReflectionNativeDeferredShaderRecord=nullptr;
static bool ptReflectionP1ShaderRecordReady=false;
static bool ptReflectionP2RayShaderRecordReady=false;
static bool ptReflectionP2CompositeShaderRecordReady=false;
static bool ptReflectionP1InjectionEnabled=false;
static bool ptReflectionP2InjectionEnabled=false;
static std::atomic<unsigned int> ptReflectionRecordMapLogged{0};
static std::atomic<unsigned int> ptReflectionDeferredRecordMapLogged{0};
static std::atomic<unsigned int> ptReflectionDeferredDxbcDumped{0};
static std::atomic<unsigned int> ptReflectionStorageRoundtripDone{0};
static int ptReflectionStorageRoundtripPersistResult=-0x7fffffff;
static int ptReflectionStorageRoundtripNativeId=-1;
static unsigned ptReflectionStorageRoundtripStage=0;
static unsigned ptReflectionStorageRoundtripVariant=0xffffffffu;
static bool ptReflectionStorageRoundtripBefore=false;
static bool ptReflectionStorageRoundtripSame=false;
static bool ptReflectionStorageRoundtripValid=false;
static void* ptReflectionStorageRoundtripTechnique=nullptr;
static void* ptReflectionStorageRoundtripShader=nullptr;

static thread_local unsigned ptReflectionLookupKind=0;
static thread_local void* ptReflectionLastRfxObject=nullptr;
static thread_local void* ptReflectionTechniqueObject=nullptr;
static thread_local void* ptReflectionDeferredTechniqueObject=nullptr;

static std::atomic<unsigned long long> ptReflectionRfxLookups{0};
static std::atomic<unsigned long long> ptReflectionTechniqueLookups{0};
static std::atomic<unsigned long long> ptReflectionVariantLookups{0};
static std::atomic<unsigned long long> ptReflectionVariantRejects{0};
static std::atomic<unsigned long long> ptReflectionDeferredRfxLookups{0};
static std::atomic<unsigned long long> ptReflectionDeferredTechniqueLookups{0};
static std::atomic<unsigned long long> ptReflectionDeferredVariantLookups{0};
static bool PTReflectionSha256(const unsigned char* data,size_t bytes,char (&hex)[65]) noexcept;
static bool PTReflectionPrepareP2CompositeShaderRecord(void* nativeShader) noexcept;
static void PTReflectionHookDeferredDispatch(void* state,const void* dimensions);
static bool PTReflectionInstallComputePsoHook(HMODULE d3d) noexcept;
static bool PTReflectionInstallComputePsoHookOnDevice(ID3D12Device* device) noexcept;
static bool PTReflectionInstallCommandListCaptureHooks(void* commandList) noexcept;
static bool PTReflectionEnsureCapturedCompositePso() noexcept;

static int ptReflectionForcedNativeVariant=-1;
static void* ptReflectionSwapTechnique=nullptr;
static void* ptReflectionSwapShader=nullptr;
static int ptReflectionSwapVariant=-1;
static std::vector<unsigned char> ptReflectionP1Bytes;
static char ptReflectionP1Sha256[65]{};
static bool ptReflectionP1Loaded=false;
static int ptReflectionP1StorageId=-1;
static bool ptReflectionP1StorageVerified=false;
static bool ptReflectionP1StorageAttempted=false;
static std::vector<unsigned char> ptReflectionP2Bytes;
static char ptReflectionP2Sha256[65]{};
static bool ptReflectionP2Loaded=false;
static std::vector<unsigned char> ptReflectionP2CompositeBytes;
static char ptReflectionP2CompositeSha256[65]{};
static bool ptReflectionP2CompositeLoaded=false;
static std::vector<unsigned char> ptReflectionP2VerifyBytes;
static char ptReflectionP2VerifySha256[65]{};
static bool ptReflectionP2VerifyLoaded=false;
static std::vector<unsigned char> ptReflectionP2LocalLightProbeBytes;
static char ptReflectionP2LocalLightProbeSha256[65]{};
static bool ptReflectionP2LocalLightProbeLoaded=false;
static std::atomic<unsigned int> ptReflectionVerifyKeyDown{0u};
static std::vector<unsigned char> ptReflectionP2DiffuseBridgeBytes;
static char ptReflectionP2DiffuseBridgeSha256[65]{};
static bool ptReflectionP2DiffuseBridgeLoaded=false;

using PTReflectionBindComputeFn=void (*)(void*);
using PTReflectionDispatchFn=void (*)(void*,const void*);
static PTReflectionBindComputeFn ptReflectionBindCompute=nullptr;
static PTReflectionDispatchFn ptReflectionOriginalDispatch=nullptr;
static RRIndirectCallPatch ptReflectionDeferredDispatchPatch{};
static std::atomic<unsigned long long> ptReflectionP2PostDispatches{0};
static unsigned char ptReflectionNativeCompositeCacheTail[0x28]{};
static unsigned char ptReflectionP2CompositeCacheTail[0x28]{};
static bool ptReflectionNativeCompositeCacheTailReady=false;
static bool ptReflectionP2CompositeCacheTailReady=false;

using PTCreateComputePipelineStateFn=HRESULT (STDMETHODCALLTYPE*)(
    ID3D12Device*,const D3D12_COMPUTE_PIPELINE_STATE_DESC*,REFIID,void**);
static PTCreateComputePipelineStateFn ptReflectionOriginalCreateComputePipelineState=nullptr;
static void** ptReflectionCreateComputePipelineStateSlot=nullptr;
static ID3D12PipelineState* ptReflectionNativeCompositePso=nullptr;
static ID3D12PipelineState* ptReflectionP2CompositePso=nullptr;
static ID3D12PipelineState* ptReflectionP2VerifyPso=nullptr;
static ID3D12PipelineState* ptReflectionP2LocalLightProbePso=nullptr;
static ID3D12RootSignature* ptReflectionCompositeRoot=nullptr;
static UINT ptReflectionCompositeNodeMask=0;
static D3D12_PIPELINE_STATE_FLAGS ptReflectionCompositeFlags=D3D12_PIPELINE_STATE_FLAG_NONE;
static std::atomic<unsigned int> ptReflectionCompositePsoCaptured{0};
static SRWLOCK ptReflectionCompositePsoLock=SRWLOCK_INIT;

using PTSetPipelineStateFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12PipelineState*);
using PTSetComputeRootSignatureFn=void (STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
static PTSetPipelineStateFn ptReflectionOriginalSetPipelineState=nullptr;
static PTSetComputeRootSignatureFn ptReflectionOriginalSetComputeRootSignature=nullptr;
static void** ptReflectionCommandListVtable=nullptr;
static thread_local bool ptReflectionAwaitingCompositeBind=false;
static std::atomic<unsigned long long> ptReflectionCompositeBindCaptures{0};


static bool PTReflectionLoadSidecar(const wchar_t* fileName,const char* label,
    std::vector<unsigned char>& bytes,char (&sha)[65],bool& loaded) noexcept {
    bytes.clear();sha[0]=0;loaded=false;
    std::wstring self=ModulePath(selfModule);
    if(self.empty())return false;
    const auto slash=self.find_last_of(L"\\/");
    if(slash==std::wstring::npos)return false;
    const std::wstring path=self.substr(0,slash+1)+fileName;
    HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,
        nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE){
        Log("PT_REFLECTION_%s_SIDECAR loaded=0 reason=missing error=%lu",label,GetLastError());
        return false;
    }
    LARGE_INTEGER size{};
    bool okay=GetFileSizeEx(file,&size)!=FALSE&&size.QuadPart>=32&&size.QuadPart<=1024*1024;
    if(okay){
        try { bytes.resize(static_cast<size_t>(size.QuadPart)); }
        catch(...) { okay=false; }
    }
    DWORD read=0;
    if(okay)okay=ReadFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&read,nullptr)!=FALSE&&read==bytes.size();
    CloseHandle(file);
    if(okay){
        uint32_t declared=0;
        memcpy(&declared,bytes.data()+24,sizeof(declared));
        okay=memcmp(bytes.data(),"DXBC",4)==0&&declared==bytes.size()&&
            PTReflectionSha256(bytes.data(),bytes.size(),sha);
    }
    if(!okay){
        Log("PT_REFLECTION_%s_SIDECAR loaded=0 reason=invalid bytes=%llu",
            label,static_cast<unsigned long long>(bytes.size()));
        bytes.clear();sha[0]=0;return false;
    }
    loaded=true;
    Log("PT_REFLECTION_%s_SIDECAR loaded=1 bytes=%llu sha256=%s address=%p activation=disabled transport=pending",
        label,static_cast<unsigned long long>(bytes.size()),sha,bytes.data());
    return true;
}

static bool PTReflectionLoadP1Sidecar() noexcept {
    return PTReflectionLoadSidecar(L"pt_reflection_p1.dxil","P1",
        ptReflectionP1Bytes,ptReflectionP1Sha256,ptReflectionP1Loaded);
}
static bool PTReflectionLoadP2Sidecar() noexcept {
    return PTReflectionLoadSidecar(L"pt_reflection_p2.dxil","P2",
        ptReflectionP2Bytes,ptReflectionP2Sha256,ptReflectionP2Loaded);
}
static bool PTReflectionLoadP2VerifySidecar() noexcept {
    return PTReflectionLoadSidecar(L"pt_reflection_p2_verify.dxbc","P2_VERIFY",
        ptReflectionP2VerifyBytes,ptReflectionP2VerifySha256,ptReflectionP2VerifyLoaded);
}
static bool PTReflectionLoadP2LocalLightProbeSidecar() noexcept {
    return PTReflectionLoadSidecar(L"pt_reflection_p2_local_light_probe.dxbc","P2_NATIVE_LIGHT_PROBE",
        ptReflectionP2LocalLightProbeBytes,ptReflectionP2LocalLightProbeSha256,
        ptReflectionP2LocalLightProbeLoaded);
}
static bool PTReflectionLoadP2CompositeSidecar() noexcept {
    const bool loaded=PTReflectionLoadSidecar(L"pt_reflection_p2_composite.dxbc","P2_COMPOSITE",
        ptReflectionP2CompositeBytes,ptReflectionP2CompositeSha256,ptReflectionP2CompositeLoaded);
    if(loaded)PTReflectionEnsureCapturedCompositePso();
    return loaded;
}
static bool PTReflectionLoadP2DiffuseBridgeSidecar() noexcept {
    return PTReflectionLoadSidecar(L"pt_reflection_p2_diffuse_bridge.dxbc","P2_DIFFUSE_BRIDGE",
        ptReflectionP2DiffuseBridgeBytes,ptReflectionP2DiffuseBridgeSha256,ptReflectionP2DiffuseBridgeLoaded);
}

static constexpr const char* kPTReflectionNativeHashes[32]={
    "c2e4c0c4b48b191ba281595dcf11c7962c5c05c615cbe036ca6e8616c09fd30e",
    "ab8df1c9b3263f534b6d43f1beb90acb5e7d0b8956844047e6abbd83aa964ed6",
    "a2ed39a3d4c1eaa9a2e401bc1f04df468007a77a97b6105215991622005a5ecb",
    "9d8bf04e11cafae08916137cf8e4dd66730d8beac6daa2fd6add19e8dc9b5da5",
    "7e701da941303a50fa89f2e6fce276d11788e718c3e93a221e7a17a883ea968d",
    "707337cbef486f61e0577b70e021b98397f6085e4069982d78591883b6b04585",
    "0411098fa35a9f6e388bc2f30e2dd79240cb0fb39654f53ab8b54d1d4785f00a",
    "bd8cd8bd7d87cca2d1e5f6ddb2bb3a239d4e69422d2e9777979b1d29791a2727",
    "3b58fb0edaac252905cfce5533c82c275cba7752e074449e1ac6fe8bc640f95a",
    "dc43fec4e661fa830db96d9f1187ae10d45f135c6e7f97a197cce88a44154ba2",
    "3f999b7dea64a02cd195f9af515a5e034508a548571d86beed62d1421d0fd8a9",
    "814a8027eaccd5af570435800a72ea93e39631ee8a7ad81a6e5ac1d4c5cb9564",
    "2a86923b674f36c6df3928dbb3396b0616fa9a9feb224ded9f66580585d49bae",
    "5b9d5e7f3fa0065d1f338eed7f523740c6c3cb239352789c7d64d843870add7a",
    "b71d5079430b9721802cbb17aa3d4544ac0eb98388782f179669e5f30523e1a6",
    "9c61d79b9a042510a13b14acdc00e28fb30b870955c0167977242d24059082d0",
    "4ef8cbe9e4fc81b5ced1026e6681732d4518db1aa96d26d4ae41fd13965bce91",
    "b188002bdb6ab4feb7af2d4cc95e5c40b7671d746028059ddfd97ab326295dbf",
    "4b26c839a2fe5397922ebebe722b0fbf54f94bfaa2bdeb2f7984250f27454423",
    "799ac13dbfbb26e703179c95d2a9bbfaf91c6d3cc17cf4c394fcee0575ac2297",
    "b2549f5c5005000cfd548ad1977fad632cf5f6dae5f4784055e7c5072102d4a1",
    "6c98b4e52a114ffa64a4416b16f4fa3f7e35cb15b1aed9b225a55de2c5500373",
    "f92bb7c2703f220f474b5f9e26337b08a81e8271a525df0d0b45160912b5930c",
    "ec647c1608fc6d3268720378d287f936de6f4dd8bdfcb21ee64f7b0516e4c571",
    "265bc062fcf2129a404ce56dc15c0883da0d52c3c22a78bd1b02d5c54f7c5645",
    "f20ed1cb40ef15b74e94101dcf16ea137315ad96d475b09a595b0e03cbb6238e",
    "4104edb9538ee61c94b0f814e61d5b3d6caa96d106c8b1f9b26a92808ac8570e",
    "429c3b507ec77d5b616c299fbbfd1d531c069b31cbf77da459295d0f147914d6",
    "0550895d74dccae3ba8766cfe7f6437bf2fe24e9a404cc4325713314d3b6d8a6",
    "075d43923bd074702bdfd28bbfeb5dff2ff35e6f91510566328de5b5f0f279ac",
    "f6a84e6fad328e1e994b442c0556f8e2815eeab14acc813fb6eff3fc7324bfbe",
    "039a1dfd754cf675aafab55d979f612d82000f872c04844232dc24eb80e2eebe"
};

static bool PTReflectionSha256(const unsigned char* data,size_t bytes,char (&hex)[65]) noexcept {
    hex[0]=0;
    if(!data||bytes<32||bytes>RRAlbedoShader::kMaximumBlobBytes)return false;
    BCRYPT_ALG_HANDLE alg=nullptr;
    BCRYPT_HASH_HANDLE hash=nullptr;
    PUCHAR object=nullptr;
    DWORD objectSize=0,received=0;
    UCHAR digest[32]{};
    bool good=BCryptOpenAlgorithmProvider(&alg,BCRYPT_SHA256_ALGORITHM,nullptr,0)>=0;
    if(good)good=BCryptGetProperty(alg,BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectSize),sizeof(objectSize),&received,0)>=0;
    if(good&&(objectSize==0||objectSize>65536))good=false;
    if(good){
        object=static_cast<PUCHAR>(HeapAlloc(GetProcessHeap(),0,objectSize));
        good=object!=nullptr;
    }
    if(good)good=BCryptCreateHash(alg,&hash,object,objectSize,nullptr,0,0)>=0;
    if(good)good=BCryptHashData(hash,const_cast<PUCHAR>(data),static_cast<ULONG>(bytes),0)>=0;
    if(good)good=BCryptFinishHash(hash,digest,sizeof(digest),0)>=0;
    if(hash)BCryptDestroyHash(hash);
    if(object)HeapFree(GetProcessHeap(),0,object);
    if(alg)BCryptCloseAlgorithmProvider(alg,0);
    if(!good)return false;
    for(size_t i=0;i<sizeof(digest);++i)sprintf_s(hex+i*2,3,"%02x",digest[i]);
    return true;
}

static int PTReflectionKnownVariant(const char* sha) noexcept {
    if(!sha||strlen(sha)!=64)return -1;
    for(unsigned i=0;i<_countof(kPTReflectionNativeHashes);++i)
        if(strcmp(sha,kPTReflectionNativeHashes[i])==0)return static_cast<int>(i);
    return -1;
}

static bool PTReflectionTechniqueOwnsShader(void* technique,void* shader,
    unsigned* indexOut,unsigned* countOut,unsigned* actualKeyOut) noexcept {
    if(indexOut)*indexOut=0xffffffffu;
    if(countOut)*countOut=0;
    if(actualKeyOut)*actualKeyOut=0;
    if(!technique||!shader)return false;
    __try {
        auto* base=*reinterpret_cast<unsigned char**>(
            reinterpret_cast<unsigned char*>(technique)+0x238);
        const unsigned count=*reinterpret_cast<unsigned*>(
            reinterpret_cast<unsigned char*>(technique)+0x240);
        if(!base||count==0||count>4096)return false;
        auto* p=reinterpret_cast<unsigned char*>(shader);
        if(p<base)return false;
        const auto delta=static_cast<std::uintptr_t>(p-base);
        if(delta%0xA8u)return false;
        const auto index=static_cast<unsigned>(delta/0xA8u);
        if(index>=count)return false;
        const unsigned actualKey=*reinterpret_cast<unsigned*>(p+4);
        if(indexOut)*indexOut=index;
        if(countOut)*countOut=count;
        if(actualKeyOut)*actualKeyOut=actualKey;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}


struct PTReflectionRayIdentity {
    RRAlbedoShader::NativeDescriptor descriptor{};
    unsigned tableIndex=0xffffffffu;
    unsigned tableCount=0;
    unsigned actualKey=0;
    char sha256[65]{};
    int variant=-1;
};

static bool PTReflectionReadRayIdentity(void* technique,void* shader,
    PTReflectionRayIdentity* out) noexcept {
    if(out)*out={};
    if(!technique||!shader||!out)return false;
    if(!PTReflectionTechniqueOwnsShader(technique,shader,&out->tableIndex,
        &out->tableCount,&out->actualKey))return false;
    RRAlbedoShader::NativeDescriptor before{},after{};
    if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(shader),0x70,&before)||
       before.identifier<0||before.size<32||before.size>RRAlbedoShader::kMaximumBlobBytes)return false;
    const size_t bytes=static_cast<size_t>(before.size);
    unsigned char* copy=static_cast<unsigned char*>(HeapAlloc(GetProcessHeap(),0,bytes));
    if(!copy)return false;
    const bool copied=RRAlbedoShader::CopyResident(&before,copy,bytes);
    const bool stable=RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(shader),0x70,&after) &&
        before.shaderBase==after.shaderBase&&before.stage==after.stage&&
        before.identifier==after.identifier&&before.size==after.size;
    uint32_t declared=0;
    if(copied&&bytes>=28)memcpy(&declared,copy+24,sizeof(declared));
    const bool container=copied&&stable&&bytes>=32&&memcmp(copy,"DXBC",4)==0&&declared==bytes;
    const bool hashed=container&&PTReflectionSha256(copy,bytes,out->sha256);
    HeapFree(GetProcessHeap(),0,copy);
    if(!hashed)return false;
    out->variant=PTReflectionKnownVariant(out->sha256);
    out->descriptor=before;
    return out->variant>=0;
}

static bool PTReflectionFindNativeVariant(void* technique,int wanted,void** shaderOut,
    PTReflectionRayIdentity* identityOut) noexcept {
    if(shaderOut)*shaderOut=nullptr;
    if(identityOut)*identityOut={};
    if(!technique||wanted<0||wanted>=32)return false;
    unsigned char* base=nullptr;unsigned count=0;
    __try {
        base=*reinterpret_cast<unsigned char**>(
            reinterpret_cast<unsigned char*>(technique)+0x238);
        count=*reinterpret_cast<unsigned*>(
            reinterpret_cast<unsigned char*>(technique)+0x240);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if(!base||count==0||count>128)return false;
    for(unsigned i=0;i<count;++i){
        auto* candidate=base+static_cast<size_t>(i)*0xA8u;
        PTReflectionRayIdentity identity{};
        if(!PTReflectionReadRayIdentity(technique,candidate,&identity))continue;
        if(identity.variant!=wanted)continue;
        if(shaderOut)*shaderOut=candidate;
        if(identityOut)*identityOut=identity;
        return true;
    }
    return false;
}

static int PTReflectionReadForcedNativeVariant() noexcept {
    wchar_t value[32]{};
    const DWORD n=GetEnvironmentVariableW(L"CONTROLFG_PT_REFLECTION_NATIVE_VARIANT",
        value,_countof(value));
    if(!n||n>=_countof(value))return -1;
    wchar_t* end=nullptr;
    const long parsed=wcstol(value,&end,10);
    if(end==value||*end!=0||parsed<0||parsed>31)return -1;
    return static_cast<int>(parsed);
}

static bool PTReflectionInspectRayLibrary(void* technique,void* shader,unsigned key) noexcept {
    PTReflectionRayIdentity identity{};
    if(!PTReflectionReadRayIdentity(technique,shader,&identity)){
        const auto reject=++ptReflectionVariantRejects;
        Log("PT_REFLECTION_P1_VARIANT_REJECT count=%llu key=0x%X technique=%p shader=%p reason=identity_or_unknown_native_library",
            reject,key,technique,shader);
        return false;
    }
    const auto& d=identity.descriptor;
    std::uintptr_t storageWord0=0;
    const void* resident=nullptr;
    DWORD storageFault=0;
    __try {
        storageWord0=*reinterpret_cast<const std::uintptr_t*>(d.shaderBase);
        resident=RRAlbedoShader::residentGetter?RRAlbedoShader::residentGetter(d.identifier):nullptr;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        storageFault=GetExceptionCode();
        storageWord0=0;
        resident=nullptr;
    }
    Log("PT_REFLECTION_P1_VARIANT_ADMIT key=0x%X actual_key=0x%X table_index=%u table_count=%u shader=%p ray_descriptor=%p storage_q0=0x%llX resident=%p storage_q0_matches_resident=%u storage_fault=0x%08lX stage=%u identifier=%d bytes=%llu sha256=%s variant=%03d exact_native_hash=1 replacement=%s",
        key,identity.actualKey,identity.tableIndex,identity.tableCount,shader,
        reinterpret_cast<void*>(d.shaderBase),static_cast<unsigned long long>(storageWord0),resident,
        unsigned(resident&&storageWord0==reinterpret_cast<std::uintptr_t>(resident)),storageFault,
        d.stage,d.identifier,static_cast<unsigned long long>(d.size),identity.sha256,identity.variant,
        ptReflectionForcedNativeVariant>=0?"native_probe_enabled":"disabled");
    return true;
}

static void* PTReflectionHookRfxLookup(void* manager,const char* name) {
    char safeName[128]{};
    RRSafeCopyCString(name,safeName,sizeof(safeName));
    const bool rayTarget=strcmp(safeName,"rt_reflection.rfx")==0;
    const bool deferredTarget=strcmp(safeName,"rt_deferred_reflection.rfx")==0;
    void* result=ptReflectionOriginalRfxLookup(manager,name);
    if(rayTarget||deferredTarget){
        ptReflectionLookupKind=rayTarget?1u:2u;
        ptReflectionLastRfxObject=result;
        ptReflectionTechniqueObject=nullptr;
        ptReflectionDeferredTechniqueObject=nullptr;
        if(rayTarget){
            const auto call=++ptReflectionRfxLookups;
            if(call<=16 || (call%240)==0)
                Log("PT_REFLECTION_P1_RFX_LOOKUP call=%llu manager=%p name=%s result=%p",
                    call,manager,safeName,result);
        }else{
            const auto call=++ptReflectionDeferredRfxLookups;
            if(call<=16 || (call%240)==0)
                Log("PT_REFLECTION_P2_DEFERRED_RFX call=%llu manager=%p name=%s result=%p composite_probe=1",
                    call,manager,safeName,result);
        }
    } else {
        ptReflectionLookupKind=0;
        ptReflectionLastRfxObject=nullptr;
        ptReflectionTechniqueObject=nullptr;
        ptReflectionDeferredTechniqueObject=nullptr;
    }
    return result;
}

static void* PTReflectionHookTechniqueLookup(void* rfx,const char* technique) {
    char safeTechnique[128]{};
    RRSafeCopyCString(technique,safeTechnique,sizeof(safeTechnique));
    const bool rayTarget=ptReflectionLookupKind==1u &&
        rfx==ptReflectionLastRfxObject &&
        strcmp(safeTechnique,"rt_reflectionDeferredShading")==0;
    const bool deferredTarget=ptReflectionLookupKind==2u &&
        rfx==ptReflectionLastRfxObject &&
        strcmp(safeTechnique,"rt_deferredReflection")==0;
    void* result=ptReflectionOriginalTechniqueLookup(rfx,technique);
    if(rayTarget){
        ptReflectionTechniqueObject=result;
        ptReflectionDeferredTechniqueObject=nullptr;
        const auto call=++ptReflectionTechniqueLookups;
        Log("PT_REFLECTION_P1_TECHNIQUE call=%llu rfx=%p technique=%s technique_object=%p",
            call,rfx,safeTechnique,result);
    }else if(deferredTarget){
        ptReflectionDeferredTechniqueObject=result;
        ptReflectionTechniqueObject=nullptr;
        const auto call=++ptReflectionDeferredTechniqueLookups;
        Log("PT_REFLECTION_P2_DEFERRED_TECHNIQUE call=%llu rfx=%p technique=%s technique_object=%p composite_probe=1",
            call,rfx,safeTechnique,result);
    }else{
        ptReflectionTechniqueObject=nullptr;
        ptReflectionDeferredTechniqueObject=nullptr;
    }
    ptReflectionLookupKind=0;
    ptReflectionLastRfxObject=nullptr;
    return result;
}

#include "pt_glass_capture.h"

static const char* PTReflectionHookShaderStorageGet(int id) {
    if(id==kPTReflectionP2RaySyntheticStorageId&&ptReflectionP2InjectionEnabled&&
       ptReflectionP2Loaded&&!ptReflectionP2Bytes.empty()){
        const auto n=++ptReflectionP2RayGetIntercepts;
        if(n<=16 || (n%240)==0)
            Log("PT_REFLECTION_P2_RAY_GET_INTERCEPT count=%llu id=%d bytes=%llu address=%p sha256=%s",
                n,id,static_cast<unsigned long long>(ptReflectionP2Bytes.size()),
                ptReflectionP2Bytes.data(),ptReflectionP2Sha256);
        return reinterpret_cast<const char*>(ptReflectionP2Bytes.data());
    }
    if(id==kPTReflectionP2CompositeSyntheticStorageId&&ptReflectionP2InjectionEnabled&&
       ptReflectionP2CompositeLoaded&&!ptReflectionP2CompositeBytes.empty()){
        const auto n=++ptReflectionP2CompositeGetIntercepts;
        if(n<=16 || (n%240)==0)
            Log("PT_REFLECTION_P2_COMPOSITE_GET_INTERCEPT count=%llu id=%d bytes=%llu address=%p sha256=%s",
                n,id,static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size()),
                ptReflectionP2CompositeBytes.data(),ptReflectionP2CompositeSha256);
        return reinterpret_cast<const char*>(ptReflectionP2CompositeBytes.data());
    }
    if(id==kPTReflectionP1SyntheticStorageId&&ptReflectionP1InjectionEnabled&&
       ptReflectionP1Loaded&&!ptReflectionP1Bytes.empty()){
        const auto n=++ptReflectionP1GetIntercepts;
        if(n<=16 || (n%240)==0)
            Log("PT_REFLECTION_P1_GET_INTERCEPT count=%llu id=%d bytes=%llu address=%p sha256=%s",
                n,id,static_cast<unsigned long long>(ptReflectionP1Bytes.size()),
                ptReflectionP1Bytes.data(),ptReflectionP1Sha256);
        return reinterpret_cast<const char*>(ptReflectionP1Bytes.data());
    }
    const char* resident=ptReflectionOriginalStorageGet?ptReflectionOriginalStorageGet(id):nullptr;
    PTGlassCaptureResident(id,resident);
    return resident;
}

static void PTReflectionLogNativeRecordMap(void* nativeShader) noexcept {
    if(!nativeShader||ptReflectionRecordMapLogged.exchange(1u,std::memory_order_acq_rel))return;
    const size_t slots[5]{0x10,0x30,0x50,0x70,0x90};
    for(unsigned i=0;i<5;++i){
        RRAlbedoShader::NativeDescriptor d{};
        if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(nativeShader),slots[i],&d)){
            Log("PT_REFLECTION_P2_RECORD_SLOT index=%u slot=0x%zX readable=0",i,slots[i]);
            continue;
        }
        char sha[65]{};
        char head[65]{};
        DWORD fault=0;
        bool residentOkay=false;
        const char* resident=nullptr;
        __try {
            resident=RRAlbedoShader::residentGetter&&d.identifier>=0?
                RRAlbedoShader::residentGetter(d.identifier):nullptr;
            if(resident&&d.size>=32&&d.size<=RRAlbedoShader::kMaximumBlobBytes){
                const size_t n=(std::min)(static_cast<size_t>(d.size),size_t(32));
                for(size_t b=0;b<n;++b)
                    sprintf_s(head+b*2,3,"%02X",static_cast<unsigned char>(resident[b]));
                residentOkay=PTReflectionSha256(
                    reinterpret_cast<const unsigned char*>(resident),
                    static_cast<size_t>(d.size),sha);
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault=GetExceptionCode();residentOkay=false;
        }
        Log("PT_REFLECTION_P2_RECORD_SLOT index=%u slot=0x%zX readable=1 descriptor=%p stage=%u id=%d bytes=%llu resident=%p resident_ok=%u sha256=%s head=%s exception=0x%08lX",
            i,slots[i],reinterpret_cast<void*>(d.shaderBase),d.stage,d.identifier,
            static_cast<unsigned long long>(d.size),resident,unsigned(residentOkay),sha,head,fault);
    }
}

static bool PTReflectionValidateDeferredDescriptor(const unsigned char* descriptor,
    size_t recordOffset,const char* kind) noexcept {
    if(!descriptor||!RRAlbedoShader::residentGetter)return false;
    unsigned stage=0;int id=-1;unsigned long long bytes=0;const char* resident=nullptr;
    char sha[65]{},head[65]{};
    uint32_t declared=0;DWORD fault=0;
    bool valid=false;
    __try {
        memcpy(&stage,descriptor+8,sizeof(stage));
        memcpy(&id,descriptor+12,sizeof(id));
        memcpy(&bytes,descriptor+16,sizeof(bytes));
        if(stage>6u||id<0||bytes<32||bytes>RRAlbedoShader::kMaximumBlobBytes)return false;
        resident=RRAlbedoShader::residentGetter(id);
        if(!resident)return false;
        memcpy(&declared,resident+24,sizeof(declared));
        if(memcmp(resident,"DXBC",4)!=0||declared!=bytes)return false;
        const size_t n=(std::min)(static_cast<size_t>(bytes),size_t(32));
        for(size_t i=0;i<n;++i)
            sprintf_s(head+i*2,3,"%02X",static_cast<unsigned char>(resident[i]));
        valid=PTReflectionSha256(reinterpret_cast<const unsigned char*>(resident),
            static_cast<size_t>(bytes),sha);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();valid=false;
    }
    if(valid){
        Log("PT_REFLECTION_P2_DEFERRED_DESCRIPTOR kind=%s record_offset=0x%zX descriptor=%p stage=%u id=%d bytes=%llu resident=%p sha256=%s head=%s exception=0x%08lX",
            kind,recordOffset,descriptor,stage,id,bytes,resident,sha,head,fault);
        if(stage==5u&&ptReflectionDeferredDxbcDumped.exchange(1u,std::memory_order_acq_rel)==0u){
            constexpr size_t chunkBytes=384u;
            const size_t totalChunks=(static_cast<size_t>(bytes)+chunkBytes-1u)/chunkBytes;
            Log("PT_REFLECTION_P2_DEFERRED_DXBC_BEGIN id=%d bytes=%llu sha256=%s chunks=%llu chunk_bytes=%llu",
                id,bytes,sha,static_cast<unsigned long long>(totalChunks),
                static_cast<unsigned long long>(chunkBytes));
            for(size_t chunk=0;chunk<totalChunks;++chunk){
                const size_t offset=chunk*chunkBytes;
                const size_t count=(std::min)(chunkBytes,static_cast<size_t>(bytes)-offset);
                char hex[chunkBytes*2u+1u]{};
                DWORD dumpFault=0;
                __try{
                    for(size_t i=0;i<count;++i)
                        sprintf_s(hex+i*2u,3,"%02X",static_cast<unsigned char>(resident[offset+i]));
                }__except(EXCEPTION_EXECUTE_HANDLER){
                    dumpFault=GetExceptionCode();hex[0]=0;
                }
                Log("PT_REFLECTION_P2_DEFERRED_DXBC_CHUNK index=%llu offset=%llu bytes=%llu hex=%s exception=0x%08lX",
                    static_cast<unsigned long long>(chunk),static_cast<unsigned long long>(offset),
                    static_cast<unsigned long long>(count),hex,dumpFault);
                if(dumpFault)break;
            }
            Log("PT_REFLECTION_P2_DEFERRED_DXBC_END id=%d bytes=%llu sha256=%s",
                id,bytes,sha);
        }
    }
    return valid;
}

static void PTReflectionLogDeferredRecordMap(void* nativeShader) noexcept {
    if(!nativeShader||ptReflectionDeferredRecordMapLogged.exchange(1u,std::memory_order_acq_rel))return;

    char rawHex[0xA8*2+1]{};
    DWORD rawFault=0;
    __try {
        auto* record=reinterpret_cast<const unsigned char*>(nativeShader);
        for(size_t i=0;i<0xA8;++i)sprintf_s(rawHex+i*2,3,"%02X",record[i]);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        rawFault=GetExceptionCode();rawHex[0]=0;
    }
    Log("PT_REFLECTION_P2_DEFERRED_RECORD_RAW shader=%p bytes=168 hex=%s exception=0x%08lX",
        nativeShader,rawHex,rawFault);

    unsigned validPointers=0,validInline=0;
    for(size_t off=0;off+sizeof(std::uintptr_t)<=0xA8;off+=sizeof(std::uintptr_t)){
        std::uintptr_t candidate=0;DWORD fault=0;
        __try {
            memcpy(&candidate,reinterpret_cast<const unsigned char*>(nativeShader)+off,sizeof(candidate));
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault=GetExceptionCode();candidate=0;
        }
        if(candidate>=0x10000u){
            bool okay=false;
            __try {
                okay=PTReflectionValidateDeferredDescriptor(
                    reinterpret_cast<const unsigned char*>(candidate),off,"pointer");
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                okay=false;
            }
            if(okay)++validPointers;
        }

        bool inlineOkay=false;
        __try {
            inlineOkay=PTReflectionValidateDeferredDescriptor(
                reinterpret_cast<const unsigned char*>(nativeShader)+off,off,"inline");
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            inlineOkay=false;
        }
        if(inlineOkay)++validInline;
    }

    Log("PT_REFLECTION_P2_DEFERRED_DESCRIPTOR_SUMMARY shader=%p valid_pointer=%u valid_inline=%u scan_bytes=168 stride=8",
        nativeShader,validPointers,validInline);
}

static bool PTReflectionPrepareP1ShaderRecord(void* nativeShader,
    const PTReflectionRayIdentity& identity) noexcept {
    ptReflectionP1ShaderRecordReady=false;
    PTReflectionLogNativeRecordMap(nativeShader);
    if(!nativeShader||identity.variant!=0||!ptReflectionP1Loaded||
       ptReflectionP1Bytes.empty()||!ptReflectionOriginalStorageGet)return false;
    auto* nativeRecord=reinterpret_cast<unsigned char*>(nativeShader);
    auto* nativeDescriptor=reinterpret_cast<unsigned char*>(identity.descriptor.shaderBase);
    if(!nativeDescriptor)return false;
    DWORD fault=0;
    __try {
        memcpy(ptReflectionP1ShaderRecord,nativeRecord,sizeof(ptReflectionP1ShaderRecord));
        memcpy(ptReflectionP1ShaderDescriptor,nativeDescriptor,sizeof(ptReflectionP1ShaderDescriptor));

        const auto nativeBegin=reinterpret_cast<std::uintptr_t>(nativeDescriptor);
        const auto nativeEnd=nativeBegin+sizeof(ptReflectionP1ShaderDescriptor);
        const auto cloneBegin=reinterpret_cast<std::uintptr_t>(ptReflectionP1ShaderDescriptor);
        for(size_t off=0;off+sizeof(std::uintptr_t)<=sizeof(ptReflectionP1ShaderDescriptor);off+=sizeof(std::uintptr_t)){
            std::uintptr_t value=0;
            memcpy(&value,ptReflectionP1ShaderDescriptor+off,sizeof(value));
            if(value>=nativeBegin&&value<nativeEnd){
                const std::uintptr_t relocated=cloneBegin+(value-nativeBegin);
                memcpy(ptReflectionP1ShaderDescriptor+off,&relocated,sizeof(relocated));
            }
        }

        const unsigned stage=identity.descriptor.stage;
        const int id=kPTReflectionP1SyntheticStorageId;
        const unsigned long long bytes=static_cast<unsigned long long>(ptReflectionP1Bytes.size());
        memcpy(ptReflectionP1ShaderDescriptor+8,&stage,sizeof(stage));
        memcpy(ptReflectionP1ShaderDescriptor+12,&id,sizeof(id));
        memcpy(ptReflectionP1ShaderDescriptor+16,&bytes,sizeof(bytes));
        const auto descriptorPtr=reinterpret_cast<std::uintptr_t>(ptReflectionP1ShaderDescriptor);
        memcpy(ptReflectionP1ShaderRecord+0x70,&descriptorPtr,sizeof(descriptorPtr));
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    if(fault){
        Log("PT_REFLECTION_P1_RECORD_PREPARE ready=0 exception=0x%08lX action=native_fallback",fault);
        return false;
    }
    ptReflectionP1ShaderRecordReady=true;
    Log("PT_REFLECTION_P1_RECORD_PREPARE ready=1 native_shader=%p clone_shader=%p native_descriptor=%p clone_descriptor=%p stage=%u native_id=%d synthetic_id=%d native_bytes=%llu p1_bytes=%llu native_sha256=%s p1_sha256=%s variant=%03d record_bytes=168 descriptor_bytes=128 self_pointers_relocated=1",
        nativeShader,ptReflectionP1ShaderRecord,reinterpret_cast<void*>(identity.descriptor.shaderBase),
        ptReflectionP1ShaderDescriptor,identity.descriptor.stage,identity.descriptor.identifier,
        kPTReflectionP1SyntheticStorageId,static_cast<unsigned long long>(identity.descriptor.size),
        static_cast<unsigned long long>(ptReflectionP1Bytes.size()),identity.sha256,ptReflectionP1Sha256,identity.variant);
    return true;
}



static bool PTReflectionCreateCapturedP2PsoLocked(ID3D12Device* device) noexcept {
    if(ptReflectionP2CompositePso)return true;
    if(!device||!ptReflectionCompositeRoot||!ptReflectionNativeCompositePso||
       !ptReflectionP2CompositeLoaded||ptReflectionP2CompositeBytes.empty()||
       !ptReflectionOriginalCreateComputePipelineState)return false;

    D3D12_COMPUTE_PIPELINE_STATE_DESC custom{};
    custom.pRootSignature=ptReflectionCompositeRoot;
    custom.CS.pShaderBytecode=ptReflectionP2CompositeBytes.data();
    custom.CS.BytecodeLength=ptReflectionP2CompositeBytes.size();
    custom.NodeMask=ptReflectionCompositeNodeMask;
    custom.CachedPSO={nullptr,0};
    custom.Flags=ptReflectionCompositeFlags;

    ID3D12PipelineState* customPso=nullptr;
    const HRESULT hr=ptReflectionOriginalCreateComputePipelineState(
        device,&custom,__uuidof(ID3D12PipelineState),
        reinterpret_cast<void**>(&customPso));
    if(FAILED(hr)||!customPso){
        Log("PT_REFLECTION_P2_PSO_CREATE ready=0 hr=0x%08lX device=%p root=%p native_pso=%p p2_bytes=%llu node_mask=%u flags=%u",
            static_cast<unsigned long>(hr),device,ptReflectionCompositeRoot,
            ptReflectionNativeCompositePso,
            static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size()),
            ptReflectionCompositeNodeMask,unsigned(ptReflectionCompositeFlags));
        return false;
    }

    ptReflectionP2CompositePso=customPso;
    if(ptReflectionP2VerifyLoaded&&!ptReflectionP2VerifyBytes.empty()){
        D3D12_COMPUTE_PIPELINE_STATE_DESC verify=custom;
        verify.CS.pShaderBytecode=ptReflectionP2VerifyBytes.data();
        verify.CS.BytecodeLength=ptReflectionP2VerifyBytes.size();
        ID3D12PipelineState* verifyPso=nullptr;
        const HRESULT verifyHr=ptReflectionOriginalCreateComputePipelineState(
            device,&verify,__uuidof(ID3D12PipelineState),
            reinterpret_cast<void**>(&verifyPso));
        if(SUCCEEDED(verifyHr)&&verifyPso)ptReflectionP2VerifyPso=verifyPso;
        Log("PT_REFLECTION_VERIFY_PSO_CREATE ready=%u hr=0x%08lX pso=%p shader_sha256=%s bytes=%llu native_root=%p",
            unsigned(ptReflectionP2VerifyPso!=nullptr),static_cast<unsigned long>(verifyHr),
            ptReflectionP2VerifyPso,ptReflectionP2VerifySha256,
            static_cast<unsigned long long>(ptReflectionP2VerifyBytes.size()),
            ptReflectionCompositeRoot);
    }
    if(ptReflectionP2LocalLightProbeLoaded&&!ptReflectionP2LocalLightProbeBytes.empty()){
        D3D12_COMPUTE_PIPELINE_STATE_DESC local=custom;
        local.CS.pShaderBytecode=ptReflectionP2LocalLightProbeBytes.data();
        local.CS.BytecodeLength=ptReflectionP2LocalLightProbeBytes.size();
        ID3D12PipelineState* localPso=nullptr;
        const HRESULT localHr=ptReflectionOriginalCreateComputePipelineState(
            device,&local,__uuidof(ID3D12PipelineState),
            reinterpret_cast<void**>(&localPso));
        if(SUCCEEDED(localHr)&&localPso)ptReflectionP2LocalLightProbePso=localPso;
        Log("PT_REFLECTION_P2_NATIVE_LIGHT_PROBE_PSO ready=%u hr=0x%08lX pso=%p shader_sha256=%s bytes=%llu native_root=%p",
            unsigned(ptReflectionP2LocalLightProbePso!=nullptr),
            static_cast<unsigned long>(localHr),ptReflectionP2LocalLightProbePso,
            ptReflectionP2LocalLightProbeSha256,
            static_cast<unsigned long long>(ptReflectionP2LocalLightProbeBytes.size()),
            ptReflectionCompositeRoot);
    }
    Log("PT_REFLECTION_P2_PSO_CREATE ready=1 device=%p root=%p native_pso=%p p2_pso=%p p2_sha256=%s p2_bytes=%llu node_mask=%u flags=%u",
        device,ptReflectionCompositeRoot,ptReflectionNativeCompositePso,customPso,
        ptReflectionP2CompositeSha256,
        static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size()),
        ptReflectionCompositeNodeMask,unsigned(ptReflectionCompositeFlags));
    return true;
}

static bool PTReflectionEnsureCapturedCompositePso() noexcept {
    if(ptReflectionP2CompositePso)return true;
    if(!ptReflectionNativeCompositePso||!ptReflectionCompositeRoot||
       !ptReflectionP2CompositeLoaded)return false;

    ID3D12Device* device=nullptr;
    const HRESULT hr=ptReflectionNativeCompositePso->GetDevice(
        __uuidof(ID3D12Device),reinterpret_cast<void**>(&device));
    if(FAILED(hr)||!device)return false;

    AcquireSRWLockExclusive(&ptReflectionCompositePsoLock);
    const bool ready=PTReflectionCreateCapturedP2PsoLocked(device);
    ReleaseSRWLockExclusive(&ptReflectionCompositePsoLock);
    device->Release();
    return ready;
}

static HRESULT STDMETHODCALLTYPE PTReflectionHookCreateComputePipelineState(
    ID3D12Device* device,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,
    REFIID riid,void** out) {
    if(!ptReflectionOriginalCreateComputePipelineState)
        return E_UNEXPECTED;

    char sha[65]{};
    bool target=false;
    if(desc&&desc->CS.pShaderBytecode&&desc->CS.BytecodeLength==42408u){
        target=PTReflectionSha256(
            static_cast<const unsigned char*>(desc->CS.pShaderBytecode),
            desc->CS.BytecodeLength,sha)&&
            strcmp(sha,"4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9")==0;
    }

    if(desc) PTGlassCaptureCompute(desc->CS.pShaderBytecode,desc->CS.BytecodeLength);

    const HRESULT nativeHr=ptReflectionOriginalCreateComputePipelineState(device,desc,riid,out);
    if(!target||FAILED(nativeHr)||!out||!*out||!desc)return nativeHr;

    AcquireSRWLockExclusive(&ptReflectionCompositePsoLock);
    if(!ptReflectionCompositePsoCaptured.load(std::memory_order_acquire)){
        ID3D12PipelineState* nativePso=nullptr;
        const HRESULT nativeQi=reinterpret_cast<IUnknown*>(*out)->QueryInterface(
            __uuidof(ID3D12PipelineState),reinterpret_cast<void**>(&nativePso));
        if(SUCCEEDED(nativeQi)&&nativePso&&desc->pRootSignature){
            desc->pRootSignature->AddRef();
            ptReflectionCompositeRoot=desc->pRootSignature;
            ptReflectionNativeCompositePso=nativePso;
            ptReflectionCompositeNodeMask=desc->NodeMask;
            ptReflectionCompositeFlags=desc->Flags;
            ptReflectionCompositePsoCaptured.store(1u,std::memory_order_release);
            const bool customReady=PTReflectionCreateCapturedP2PsoLocked(device);
            Log("PT_REFLECTION_P2_PSO_CAPTURE ready=1 native_pso=%p p2_pso=%p custom_ready=%u root=%p native_sha256=%s native_bytes=%llu node_mask=%u flags=%u composite_sidecar_loaded=%u",
                nativePso,ptReflectionP2CompositePso,unsigned(customReady),
                desc->pRootSignature,sha,
                static_cast<unsigned long long>(desc->CS.BytecodeLength),
                desc->NodeMask,unsigned(desc->Flags),
                unsigned(ptReflectionP2CompositeLoaded));
        }else{
            if(nativePso)nativePso->Release();
            Log("PT_REFLECTION_P2_PSO_CAPTURE ready=0 native_hr=0x%08lX native_qi=0x%08lX root=%p",
                static_cast<unsigned long>(nativeHr),static_cast<unsigned long>(nativeQi),
                desc->pRootSignature);
        }
    }
    ReleaseSRWLockExclusive(&ptReflectionCompositePsoLock);
    return nativeHr;
}

static bool PTReflectionInstallComputePsoHookOnDevice(ID3D12Device* device) noexcept {
    if(!device)return false;

    void** vtable=nullptr;
    __try {vtable=*reinterpret_cast<void***>(device);}
    __except(EXCEPTION_EXECUTE_HANDLER){vtable=nullptr;}
    if(!vtable)return false;

    constexpr size_t slotIndex=11u; // ID3D12Device::CreateComputePipelineState
    auto** slot=&vtable[slotIndex];
    void* current=nullptr;
    __try {current=*slot;} __except(EXCEPTION_EXECUTE_HANDLER){current=nullptr;}
    if(!current)return false;

    if(current==reinterpret_cast<void*>(&PTReflectionHookCreateComputePipelineState)){
        if(!ptReflectionOriginalCreateComputePipelineState)return false;
        ptReflectionCreateComputePipelineStateSlot=slot;
        return true;
    }

    auto original=reinterpret_cast<PTCreateComputePipelineStateFn>(current);
    DWORD old=0;
    if(!VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&old))return false;
    void* previous=InterlockedCompareExchangePointer(
        slot,reinterpret_cast<void*>(&PTReflectionHookCreateComputePipelineState),
        reinterpret_cast<void*>(original));
    const DWORD exchangeError=GetLastError();
    DWORD ignored=0;
    const BOOL restored=VirtualProtect(slot,sizeof(void*),old,&ignored);
    if(previous!=reinterpret_cast<void*>(original)||!restored){
        if(previous==reinterpret_cast<void*>(original)){
            DWORD retryOld=0;
            if(VirtualProtect(slot,sizeof(void*),PAGE_EXECUTE_READWRITE,&retryOld)){
                InterlockedCompareExchangePointer(slot,reinterpret_cast<void*>(original),
                    reinterpret_cast<void*>(&PTReflectionHookCreateComputePipelineState));
                DWORD retryIgnored=0;
                VirtualProtect(slot,sizeof(void*),retryOld,&retryIgnored);
            }
        }
        return false;
    }

    ptReflectionOriginalCreateComputePipelineState=original;
    ptReflectionCreateComputePipelineStateSlot=slot;
    Log("PT_REFLECTION_P2_PSO_HOOK ready=1 source=device_create device=%p vtable=%p slot_index=%llu slot=%p original=%p hook=%p exchange_error=%lu",
        device,vtable,static_cast<unsigned long long>(slotIndex),slot,
        reinterpret_cast<void*>(original),
        reinterpret_cast<void*>(&PTReflectionHookCreateComputePipelineState),
        exchangeError);
    return true;
}

static bool PTReflectionInstallComputePsoHook(HMODULE d3d) noexcept {
    if(!d3d||d3d!=verifiedD3d)return false;
    ID3D12Device* device=nullptr;
    __try {
        device=*reinterpret_cast<ID3D12Device**>(
            reinterpret_cast<unsigned char*>(d3d)+kControlGlobalD3D12DevicePointerRva);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        device=nullptr;
    }
    return device&&PTReflectionInstallComputePsoHookOnDevice(device);
}

static void STDMETHODCALLTYPE PTReflectionHookSetComputeRootSignature(
    ID3D12GraphicsCommandList* list,ID3D12RootSignature* root) {
    if(ptReflectionAwaitingCompositeBind&&root){
        AcquireSRWLockExclusive(&ptReflectionCompositePsoLock);
        if(!ptReflectionCompositeRoot){
            root->AddRef();
            ptReflectionCompositeRoot=root;
            const auto n=ptReflectionCompositeBindCaptures.fetch_add(1,std::memory_order_relaxed)+1;
            Log("PT_REFLECTION_P2_LIVE_ROOT_CAPTURE count=%llu list=%p root=%p",n,list,root);
        }
        ReleaseSRWLockExclusive(&ptReflectionCompositePsoLock);
    }
    if(ptReflectionOriginalSetComputeRootSignature)
        ptReflectionOriginalSetComputeRootSignature(list,root);
}

static void STDMETHODCALLTYPE PTReflectionHookSetPipelineState(
    ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) {
    if(ptReflectionAwaitingCompositeBind&&pso){
        AcquireSRWLockExclusive(&ptReflectionCompositePsoLock);
        if(!ptReflectionNativeCompositePso){
            pso->AddRef();
            ptReflectionNativeCompositePso=pso;
            const auto n=ptReflectionCompositeBindCaptures.fetch_add(1,std::memory_order_relaxed)+1;
            Log("PT_REFLECTION_P2_LIVE_PSO_CAPTURE count=%llu list=%p native_pso=%p root=%p",
                n,list,pso,ptReflectionCompositeRoot);
        }
        if(ptReflectionCompositeRoot&&ptReflectionNativeCompositePso)
            ptReflectionCompositePsoCaptured.store(1u,std::memory_order_release);
        ReleaseSRWLockExclusive(&ptReflectionCompositePsoLock);
    }
    if(ptReflectionOriginalSetPipelineState)
        ptReflectionOriginalSetPipelineState(list,pso);
}

static bool PTReflectionInstallCommandListCaptureHooks(void* commandList) noexcept {
    auto* list=static_cast<ID3D12GraphicsCommandList*>(commandList);
    if(!list)return false;
    void** vtable=nullptr;
    __try {vtable=*reinterpret_cast<void***>(list);}
    __except(EXCEPTION_EXECUTE_HANDLER){vtable=nullptr;}
    if(!vtable)return false;

    if(ptReflectionCommandListVtable==vtable&&
       ptReflectionOriginalSetPipelineState&&ptReflectionOriginalSetComputeRootSignature)
        return true;
    if(ptReflectionCommandListVtable&&ptReflectionCommandListVtable!=vtable)
        return false;

    constexpr size_t kSetPipelineStateSlot=25u;
    constexpr size_t kSetComputeRootSignatureSlot=29u;
    auto* originalPso=reinterpret_cast<PTSetPipelineStateFn>(vtable[kSetPipelineStateSlot]);
    auto* originalRoot=reinterpret_cast<PTSetComputeRootSignatureFn>(vtable[kSetComputeRootSignatureSlot]);
    if(!originalPso||!originalRoot)return false;

    DWORD oldPso=0,oldRoot=0;
    if(!VirtualProtect(&vtable[kSetPipelineStateSlot],sizeof(void*),PAGE_EXECUTE_READWRITE,&oldPso))
        return false;
    void* previousPso=InterlockedCompareExchangePointer(
        &vtable[kSetPipelineStateSlot],
        reinterpret_cast<void*>(&PTReflectionHookSetPipelineState),
        reinterpret_cast<void*>(originalPso));
    DWORD ignoredPso=0;
    const BOOL psoProtect=VirtualProtect(&vtable[kSetPipelineStateSlot],sizeof(void*),oldPso,&ignoredPso);
    if(previousPso!=reinterpret_cast<void*>(originalPso)||!psoProtect)return false;

    if(!VirtualProtect(&vtable[kSetComputeRootSignatureSlot],sizeof(void*),PAGE_EXECUTE_READWRITE,&oldRoot)){
        DWORD rollbackOld=0;
        if(VirtualProtect(&vtable[kSetPipelineStateSlot],sizeof(void*),PAGE_EXECUTE_READWRITE,&rollbackOld)){
            InterlockedCompareExchangePointer(&vtable[kSetPipelineStateSlot],
                reinterpret_cast<void*>(originalPso),
                reinterpret_cast<void*>(&PTReflectionHookSetPipelineState));
            DWORD rollbackIgnored=0;
            VirtualProtect(&vtable[kSetPipelineStateSlot],sizeof(void*),rollbackOld,&rollbackIgnored);
        }
        return false;
    }
    void* previousRoot=InterlockedCompareExchangePointer(
        &vtable[kSetComputeRootSignatureSlot],
        reinterpret_cast<void*>(&PTReflectionHookSetComputeRootSignature),
        reinterpret_cast<void*>(originalRoot));
    DWORD ignoredRoot=0;
    const BOOL rootProtect=VirtualProtect(&vtable[kSetComputeRootSignatureSlot],sizeof(void*),oldRoot,&ignoredRoot);
    if(previousRoot!=reinterpret_cast<void*>(originalRoot)||!rootProtect)return false;

    ptReflectionOriginalSetPipelineState=originalPso;
    ptReflectionOriginalSetComputeRootSignature=originalRoot;
    ptReflectionCommandListVtable=vtable;
    Log("PT_REFLECTION_P2_COMMAND_LIST_HOOKS ready=1 list=%p vtable=%p set_pso_slot=%llu set_root_slot=%llu original_pso=%p original_root=%p",
        list,vtable,static_cast<unsigned long long>(kSetPipelineStateSlot),
        static_cast<unsigned long long>(kSetComputeRootSignatureSlot),
        reinterpret_cast<void*>(originalPso),reinterpret_cast<void*>(originalRoot));
    return true;
}

static bool PTReflectionEnsureLiveCompositePso(ID3D12Device* device) noexcept {
    if(!device||!ptReflectionP2CompositeLoaded||ptReflectionP2CompositeBytes.empty())
        return false;
    if(ptReflectionP2CompositePso)return true;

    AcquireSRWLockExclusive(&ptReflectionCompositePsoLock);
    if(!ptReflectionP2CompositePso&&ptReflectionCompositeRoot&&ptReflectionNativeCompositePso){
        D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};
        pd.pRootSignature=ptReflectionCompositeRoot;
        pd.CS.pShaderBytecode=ptReflectionP2CompositeBytes.data();
        pd.CS.BytecodeLength=ptReflectionP2CompositeBytes.size();
        pd.NodeMask=0;
        pd.CachedPSO={nullptr,0};
        pd.Flags=D3D12_PIPELINE_STATE_FLAG_NONE;
        ID3D12PipelineState* custom=nullptr;
        const HRESULT hr=device->CreateComputePipelineState(
            &pd,__uuidof(ID3D12PipelineState),reinterpret_cast<void**>(&custom));
        if(SUCCEEDED(hr)&&custom){
            ptReflectionP2CompositePso=custom;
            Log("PT_REFLECTION_P2_LIVE_PSO_CREATE ready=1 device=%p root=%p native_pso=%p p2_pso=%p p2_sha256=%s p2_bytes=%llu hr=0x%08lX",
                device,ptReflectionCompositeRoot,ptReflectionNativeCompositePso,custom,
                ptReflectionP2CompositeSha256,
                static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size()),
                static_cast<unsigned long>(hr));
        }else{
            Log("PT_REFLECTION_P2_LIVE_PSO_CREATE ready=0 device=%p root=%p native_pso=%p hr=0x%08lX",
                device,ptReflectionCompositeRoot,ptReflectionNativeCompositePso,
                static_cast<unsigned long>(hr));
        }
    }
    const bool ready=ptReflectionP2CompositePso!=nullptr;
    ReleaseSRWLockExclusive(&ptReflectionCompositePsoLock);
    return ready;
}

static bool PTReflectionCloneShaderRecord(void* nativeShader,size_t descriptorSlot,
    unsigned stage,int syntheticId,size_t shaderBytes,
    unsigned char (&record)[0xA8],unsigned char (&descriptor)[0x100]) noexcept {
    if(!nativeShader||descriptorSlot+sizeof(std::uintptr_t)>sizeof(record)||
       shaderBytes<32||shaderBytes>RRAlbedoShader::kMaximumBlobBytes)return false;
    std::uintptr_t nativeDescriptorAddress=0;
    DWORD fault=0;
    __try {
        auto* nativeRecord=reinterpret_cast<unsigned char*>(nativeShader);
        memcpy(record,nativeRecord,sizeof(record));
        memcpy(&nativeDescriptorAddress,nativeRecord+descriptorSlot,
            sizeof(nativeDescriptorAddress));
        if(nativeDescriptorAddress<=0xFFFFu)return false;
        auto* nativeDescriptor=reinterpret_cast<unsigned char*>(nativeDescriptorAddress);
        memcpy(descriptor,nativeDescriptor,sizeof(descriptor));

        const auto nativeRecordBegin=reinterpret_cast<std::uintptr_t>(nativeRecord);
        const auto nativeRecordEnd=nativeRecordBegin+sizeof(record);
        const auto cloneRecordBegin=reinterpret_cast<std::uintptr_t>(record);
        const auto nativeDescriptorBegin=nativeDescriptorAddress;
        const auto nativeDescriptorEnd=nativeDescriptorBegin+sizeof(descriptor);
        const auto cloneDescriptorBegin=reinterpret_cast<std::uintptr_t>(descriptor);

        // Relocate every pointer in the 0xA8 shader record that refers either
        // to the record itself or into the descriptor object. Compute records
        // use both descriptor+0x00 and descriptor+0xC0, plus an embedded
        // self-pointer at record+0x88.
        for(size_t off=0;off+sizeof(std::uintptr_t)<=sizeof(record);off+=sizeof(std::uintptr_t)){
            std::uintptr_t value=0;
            memcpy(&value,record+off,sizeof(value));
            if(value>=nativeRecordBegin&&value<nativeRecordEnd){
                value=cloneRecordBegin+(value-nativeRecordBegin);
                memcpy(record+off,&value,sizeof(value));
            }else if(value>=nativeDescriptorBegin&&value<nativeDescriptorEnd){
                value=cloneDescriptorBegin+(value-nativeDescriptorBegin);
                memcpy(record+off,&value,sizeof(value));
            }
        }

        // Relocate descriptor-internal pointers as well.
        for(size_t off=0;off+sizeof(std::uintptr_t)<=sizeof(descriptor);off+=sizeof(std::uintptr_t)){
            std::uintptr_t value=0;
            memcpy(&value,descriptor+off,sizeof(value));
            if(value>=nativeDescriptorBegin&&value<nativeDescriptorEnd){
                value=cloneDescriptorBegin+(value-nativeDescriptorBegin);
                memcpy(descriptor+off,&value,sizeof(value));
            }else if(value>=nativeRecordBegin&&value<nativeRecordEnd){
                value=cloneRecordBegin+(value-nativeRecordBegin);
                memcpy(descriptor+off,&value,sizeof(value));
            }
        }

        unsigned nativeStage=0;int nativeId=-1;unsigned long long nativeBytes=0;
        memcpy(&nativeStage,nativeDescriptor+8,sizeof(nativeStage));
        memcpy(&nativeId,nativeDescriptor+12,sizeof(nativeId));
        memcpy(&nativeBytes,nativeDescriptor+16,sizeof(nativeBytes));

        const unsigned long long bytes=static_cast<unsigned long long>(shaderBytes);
        unsigned patchedDescriptors=0;
        for(size_t off=0;off+24u<=sizeof(descriptor);off+=8u){
            unsigned candidateStage=0;int candidateId=-1;unsigned long long candidateBytes=0;
            memcpy(&candidateStage,descriptor+off+8,sizeof(candidateStage));
            memcpy(&candidateId,descriptor+off+12,sizeof(candidateId));
            memcpy(&candidateBytes,descriptor+off+16,sizeof(candidateBytes));
            if(candidateStage==nativeStage&&candidateId==nativeId&&candidateBytes==nativeBytes){
                memcpy(descriptor+off+8,&stage,sizeof(stage));
                memcpy(descriptor+off+12,&syntheticId,sizeof(syntheticId));
                memcpy(descriptor+off+16,&bytes,sizeof(bytes));
                ++patchedDescriptors;
            }
        }
        if(!patchedDescriptors){
            memcpy(descriptor+8,&stage,sizeof(stage));
            memcpy(descriptor+12,&syntheticId,sizeof(syntheticId));
            memcpy(descriptor+16,&bytes,sizeof(bytes));
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    if(fault){
        Log("PT_REFLECTION_P2_RECORD_CLONE ready=0 slot=0x%zX stage=%u synthetic_id=%d exception=0x%08lX",
            descriptorSlot,stage,syntheticId,fault);
        return false;
    }
    return true;
}

static bool PTReflectionPrepareP2RayShaderRecord(void* nativeShader,
    const PTReflectionRayIdentity& identity) noexcept {
    ptReflectionP2RayShaderRecordReady=false;
    if(!nativeShader||identity.variant!=0||!ptReflectionP2Loaded||
       ptReflectionP2Bytes.empty())return false;
    if(!PTReflectionCloneShaderRecord(nativeShader,0x70u,6u,
        kPTReflectionP2RaySyntheticStorageId,ptReflectionP2Bytes.size(),
        ptReflectionP2RayShaderRecord,ptReflectionP2RayShaderDescriptor))return false;
    ptReflectionP2RayShaderRecordReady=true;
    Log("PT_REFLECTION_P2_RAY_RECORD_PREPARE ready=1 native_shader=%p clone_shader=%p native_descriptor=%p clone_descriptor=%p native_id=%d synthetic_id=%d native_bytes=%llu p2_bytes=%llu native_sha256=%s p2_sha256=%s variant=%03d",
        nativeShader,ptReflectionP2RayShaderRecord,
        reinterpret_cast<void*>(identity.descriptor.shaderBase),ptReflectionP2RayShaderDescriptor,
        identity.descriptor.identifier,kPTReflectionP2RaySyntheticStorageId,
        static_cast<unsigned long long>(identity.descriptor.size),
        static_cast<unsigned long long>(ptReflectionP2Bytes.size()),
        identity.sha256,ptReflectionP2Sha256,identity.variant);
    return true;
}

static bool PTReflectionPrepareP2CompositeShaderRecord(void* nativeShader) noexcept {
    ptReflectionP2CompositeShaderRecordReady=false;
    if(!nativeShader||!ptReflectionP2CompositeLoaded||
       ptReflectionP2CompositeBytes.empty()||!RRAlbedoShader::residentGetter)return false;

    RRAlbedoShader::NativeDescriptor d{};
    if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(nativeShader),0x60u,&d)||
       d.stage!=5u||d.identifier<0||d.size!=42408ull)return false;

    char sha[65]{};
    bool exact=false;
    __try {
        const char* resident=RRAlbedoShader::residentGetter(d.identifier);
        exact=resident&&PTReflectionSha256(reinterpret_cast<const unsigned char*>(resident),
            static_cast<size_t>(d.size),sha)&&
            strcmp(sha,"4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9")==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        exact=false;
    }
    if(!exact)return false;

    if(!PTReflectionCloneShaderRecord(nativeShader,0x60u,5u,
        kPTReflectionP2CompositeSyntheticStorageId,ptReflectionP2CompositeBytes.size(),
        ptReflectionP2CompositeShaderRecord,ptReflectionP2CompositeShaderDescriptor))return false;

    ptReflectionNativeDeferredShaderRecord=nativeShader;
    ptReflectionP2CompositeShaderRecordReady=true;
    Log("PT_REFLECTION_P2_COMPOSITE_RECORD_PREPARE ready=1 native_shader=%p clone_shader=%p native_descriptor=%p clone_descriptor=%p native_id=%d synthetic_id=%d native_bytes=%llu p2_bytes=%llu native_sha256=%s p2_sha256=%s descriptor_slot=0x60 stage=5",
        nativeShader,ptReflectionP2CompositeShaderRecord,
        reinterpret_cast<void*>(d.shaderBase),ptReflectionP2CompositeShaderDescriptor,
        d.identifier,kPTReflectionP2CompositeSyntheticStorageId,
        static_cast<unsigned long long>(d.size),
        static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size()),
        sha,ptReflectionP2CompositeSha256);
    return true;
}


struct PTReflectionCompositeDescriptorSnapshot {
    unsigned char* base=nullptr;
    unsigned stage0=0;
    int id0=-1;
    unsigned long long bytes0=0;
    std::uintptr_t secondaryAddress=0;
};

static bool PTReflectionPatchNativeCompositeDescriptor(
    PTReflectionCompositeDescriptorSnapshot* snapshot,bool custom) noexcept {
    if(!snapshot||!ptReflectionNativeDeferredShaderRecord)return false;
    DWORD fault=0;
    bool good=false;
    __try {
        auto* record=reinterpret_cast<unsigned char*>(ptReflectionNativeDeferredShaderRecord);
        std::uintptr_t baseAddress=0,secondaryAddress=0;
        memcpy(&baseAddress,record+0x60,sizeof(baseAddress));
        memcpy(&secondaryAddress,record+0x68,sizeof(secondaryAddress));
        if(baseAddress<=0xFFFFu)return false;

        snapshot->base=reinterpret_cast<unsigned char*>(baseAddress);
        snapshot->secondaryAddress=secondaryAddress;
        memcpy(&snapshot->stage0,snapshot->base+8,sizeof(snapshot->stage0));
        memcpy(&snapshot->id0,snapshot->base+12,sizeof(snapshot->id0));
        memcpy(&snapshot->bytes0,snapshot->base+16,sizeof(snapshot->bytes0));

        if(snapshot->stage0!=5u||snapshot->id0!=9248336||
           snapshot->bytes0!=42408ull||
           secondaryAddress!=baseAddress+0xC0u||
           !ptReflectionP2CompositeLoaded||ptReflectionP2CompositeBytes.empty()){
            Log("PT_REFLECTION_P2_DESCRIPTOR_PATCH custom=%u ready=0 reason=contract stage=%u id=%d bytes=%llu base=%p secondary=%p secondary_delta=0x%llX expected_secondary_delta=0xC0 composite_loaded=%u",
                unsigned(custom),snapshot->stage0,snapshot->id0,snapshot->bytes0,
                snapshot->base,reinterpret_cast<void*>(secondaryAddress),
                secondaryAddress>=baseAddress?
                    static_cast<unsigned long long>(secondaryAddress-baseAddress):~0ull,
                unsigned(ptReflectionP2CompositeLoaded));
            return false;
        }

        if(!custom){
            good=true;
        }else{
            const unsigned stage=5u;
            const int id=kPTReflectionP2CompositeSyntheticStorageId;
            const unsigned long long bytes=
                static_cast<unsigned long long>(ptReflectionP2CompositeBytes.size());
            memcpy(snapshot->base+8,&stage,sizeof(stage));
            memcpy(snapshot->base+12,&id,sizeof(id));
            memcpy(snapshot->base+16,&bytes,sizeof(bytes));
            MemoryBarrier();

            unsigned verifyStage=0;int verifyId=-1;unsigned long long verifyBytes=0;
            memcpy(&verifyStage,snapshot->base+8,sizeof(verifyStage));
            memcpy(&verifyId,snapshot->base+12,sizeof(verifyId));
            memcpy(&verifyBytes,snapshot->base+16,sizeof(verifyBytes));
            good=verifyStage==stage&&verifyId==id&&verifyBytes==bytes;
            Log("PT_REFLECTION_P2_DESCRIPTOR_PATCH custom=1 ready=%u base=%p secondary=%p secondary_delta=0x%llX native_stage=%u native_id=%d native_bytes=%llu custom_stage=%u custom_id=%d custom_bytes=%llu secondary_untouched=1",
                unsigned(good),snapshot->base,reinterpret_cast<void*>(secondaryAddress),
                static_cast<unsigned long long>(secondaryAddress-baseAddress),
                snapshot->stage0,snapshot->id0,snapshot->bytes0,
                verifyStage,verifyId,verifyBytes);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        good=false;
    }
    if(fault)
        Log("PT_REFLECTION_P2_DESCRIPTOR_PATCH custom=%u ready=0 exception=0x%08lX",
            unsigned(custom),fault);
    return good;
}

static bool PTReflectionRestoreNativeCompositeDescriptor(
    const PTReflectionCompositeDescriptorSnapshot& snapshot) noexcept {
    if(!snapshot.base)return false;
    DWORD fault=0;
    bool restored=false;
    __try {
        memcpy(snapshot.base+8,&snapshot.stage0,sizeof(snapshot.stage0));
        memcpy(snapshot.base+12,&snapshot.id0,sizeof(snapshot.id0));
        memcpy(snapshot.base+16,&snapshot.bytes0,sizeof(snapshot.bytes0));
        MemoryBarrier();

        unsigned stage=0;int id=-1;unsigned long long bytes=0;
        memcpy(&stage,snapshot.base+8,sizeof(stage));
        memcpy(&id,snapshot.base+12,sizeof(id));
        memcpy(&bytes,snapshot.base+16,sizeof(bytes));
        restored=stage==snapshot.stage0&&id==snapshot.id0&&bytes==snapshot.bytes0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        restored=false;
    }
    Log("PT_REFLECTION_P2_DESCRIPTOR_RESTORE ready=%u base=%p stage=%u id=%d bytes=%llu secondary_untouched=1 exception=0x%08lX",
        unsigned(restored),snapshot.base,snapshot.stage0,snapshot.id0,snapshot.bytes0,fault);
    return restored;
}


static bool PTReflectionSnapshotCompositeCacheTail(bool custom) noexcept {
    if(!ptReflectionNativeDeferredShaderRecord)return false;
    DWORD fault=0;
    __try {
        auto* record=reinterpret_cast<unsigned char*>(ptReflectionNativeDeferredShaderRecord);
        auto* target=custom?ptReflectionP2CompositeCacheTail:ptReflectionNativeCompositeCacheTail;
        memcpy(target,record+0x80,0x28);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    if(fault)return false;
    if(custom)ptReflectionP2CompositeCacheTailReady=true;
    else ptReflectionNativeCompositeCacheTailReady=true;
    return true;
}

static bool PTReflectionRestoreCompositeCacheTail(bool custom) noexcept {
    if(!ptReflectionNativeDeferredShaderRecord)return false;
    const bool ready=custom?ptReflectionP2CompositeCacheTailReady:ptReflectionNativeCompositeCacheTailReady;
    if(!ready)return false;
    DWORD fault=0;
    __try {
        auto* record=reinterpret_cast<unsigned char*>(ptReflectionNativeDeferredShaderRecord);
        const auto* source=custom?ptReflectionP2CompositeCacheTail:ptReflectionNativeCompositeCacheTail;
        memcpy(record+0x80,source,0x28);
        MemoryBarrier();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    return fault==0;
}

static bool PTReflectionInvalidateCompositeCacheForCustomBind() noexcept {
    if(!ptReflectionNativeDeferredShaderRecord)return false;
    DWORD fault=0;
    __try {
        auto* record=reinterpret_cast<unsigned char*>(ptReflectionNativeDeferredShaderRecord);
        if(!ptReflectionNativeCompositeCacheTailReady){
            memcpy(ptReflectionNativeCompositeCacheTail,record+0x80,0x28);
            ptReflectionNativeCompositeCacheTailReady=true;
        }
        // Preserve every engine-private flag/self pointer. Clear only the
        // external cached shader-state pointer at record+0x80 so the native
        // binder must materialize state from the now-patched descriptor.
        std::uintptr_t nullCache=0;
        memcpy(record+0x80,&nullCache,sizeof(nullCache));
        MemoryBarrier();
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    if(fault)
        Log("PT_REFLECTION_P2_CACHE_INVALIDATE ready=0 exception=0x%08lX",fault);
    return fault==0;
}

static bool PTReflectionInsertReflectionUavBarrier() noexcept {
    using GetNativeTextureFn=void* (*)(void*);
    auto getter=reinterpret_cast<GetNativeTextureFn>(
        verifiedD3d?GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol):nullptr);
    if(!getter||!verifiedRenderer)return false;

    void* wrapper=nullptr;
    void* raw=nullptr;
    D3D12_RESOURCE_DESC desc{};
    DWORD describeFault=0;
    __try {
        auto* object=reinterpret_cast<unsigned char*>(verifiedRenderer)+kRRShaderReflectionTargetRva;
        wrapper=getter(object);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    if(!wrapper||!Describe(wrapper,&raw,&desc,&describeFault)||!raw)return false;

    EngineCommandContextSnapshot command{};
    auto* resource=static_cast<ID3D12Resource*>(raw);
    if(!ReadEngineCommandContext(resource,&command)||!command.commandList)return false;

    bool good=false;
    __try {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags=D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource=resource;
        static_cast<ID3D12GraphicsCommandList*>(command.commandList)->ResourceBarrier(1,&barrier);
        good=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        good=false;
    }
    return good;
}

// Renderer ShaderTexture identities from the hash-locked native bind asm.
// The source buffer pair was independently captured as EXACTLY two layers
// by the RR reflection capture. Check material, normal and position live
// GPU resources rather than assuming that the additional layers exist.
static std::atomic<unsigned int> ptReflectionTwoLayerContract{0u};
static std::atomic<unsigned int> ptReflectionTwoLayerAttempts{0u};
static bool PTReflectionAuditTwoLayerHitTextures() noexcept {
    const unsigned state=ptReflectionTwoLayerContract.load(std::memory_order_acquire);
    if(state==1u)return true;
    if(state==2u)return false;
    constexpr std::uintptr_t kMaterial=0x1291888u;
    constexpr std::uintptr_t kNormal=0x12918B0u;
    constexpr std::uintptr_t kPosition=0x12918D8u;
    using Getter=void* (*)(void*);
    Getter get=reinterpret_cast<Getter>(
        verifiedD3d?GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol):nullptr);
    const unsigned attempt=ptReflectionTwoLayerAttempts.fetch_add(1u,std::memory_order_relaxed)+1u;
    if(!verifiedRenderer||!get)return false;
    D3D12_RESOURCE_DESC descriptions[3]{};
    bool known[3]{};
    DWORD faults[3]{};
    const std::uintptr_t offsets[3]{kMaterial,kNormal,kPosition};
    for(unsigned i=0u;i<3u;++i){
        void* wrapper=nullptr;
        __try {
            wrapper=get(reinterpret_cast<unsigned char*>(verifiedRenderer)+offsets[i]);
        } __except(EXCEPTION_EXECUTE_HANDLER) {faults[i]=GetExceptionCode();}
        void* raw=nullptr;
        if(wrapper)
            known[i]=Describe(wrapper,&raw,&descriptions[i],&faults[i])&&raw!=nullptr;
    }
    if(!known[0]||!known[1]||!known[2]){
        if(attempt<=4u || (attempt%240u)==0u)
            Log("PT_REFLECTION_P2_TWO_LAYER_CONTRACT pending=1 attempt=%u material_found=%u normal_found=%u position_found=%u faults=0x%08lX_0x%08lX_0x%08lX",
                attempt,unsigned(known[0]),unsigned(known[1]),unsigned(known[2]),
                faults[0],faults[1],faults[2]);
        return false;
    }
    const auto& m=descriptions[0];
    const auto& n=descriptions[1];
    const auto& p=descriptions[2];
    const bool valid=
        m.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        n.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        p.Dimension==D3D12_RESOURCE_DIMENSION_TEXTURE2D&&
        m.Format==DXGI_FORMAT_R16_UINT&&
        n.Format==DXGI_FORMAT_R16G16B16A16_FLOAT&&
        p.Format==DXGI_FORMAT_R16G16B16A16_FLOAT&&
        m.DepthOrArraySize==2u&&n.DepthOrArraySize==2u&&p.DepthOrArraySize==2u&&
        m.Width==n.Width&&n.Width==p.Width&&
        m.Height==n.Height&&n.Height==p.Height&&
        m.SampleDesc.Count==1u&&n.SampleDesc.Count==1u&&p.SampleDesc.Count==1u;
    ptReflectionTwoLayerContract.store(valid?1u:2u,std::memory_order_release);
    Log("PT_REFLECTION_P2_TWO_LAYER_CONTRACT ready=%u result=%s layout=native_first_layer_0_secondary_layer_1 material_layers=%u normal_layers=%u position_layers=%u material_format=%u normal_format=%u position_format=%u width=%llu height=%u second_id_encoding=fp16_numeric_0_to_2047 uv_encoding=numeric_5x5 native_first_samples=1",
        unsigned(valid),valid?"pass":"incompatible_resource_shape",
        unsigned(m.DepthOrArraySize),unsigned(n.DepthOrArraySize),
        unsigned(p.DepthOrArraySize),unsigned(m.Format),unsigned(n.Format),
        unsigned(p.Format),static_cast<unsigned long long>(m.Width),m.Height);
    if(!valid){
        ptReflectionP2InjectionEnabled=false;
        Log("PT_REFLECTION_P2_DISABLE reason=two_layer_contract_mismatch fallback=native");
    }
    return valid;
}

static void PTReflectionHookDeferredDispatch(void* state,const void* dimensions) {
    if(!ptReflectionOriginalDispatch)return;
    ptReflectionAwaitingCompositeBind=false;

    // First execute Control's native rt_deferredReflection composite exactly as-is.
    ptReflectionOriginalDispatch(state,dimensions);

    if(!ptReflectionP2InjectionEnabled)return;

    // F11 advances on key-down edge only. No overlay, screen-size changes or
    // tool dependency. Only this experimental test DLL reads the key.
    const unsigned pressed=(GetAsyncKeyState(VK_F11)&0x8000)?1u:0u;
    if(pressed){
        if(ptReflectionVerifyKeyDown.exchange(1u,std::memory_order_acq_rel)==0u){
            const unsigned prior=ptReflectionUserState.load(std::memory_order_acquire)>>1u;
            const unsigned next=PTReflectionUserCycleDiagnostic();
            Log("PT_REFLECTION_VERIFY_SWITCH from=%u to=%u hotkey=F11 label=%s",
                prior,next,next==0u?"normal":next==1u?
                    "P2_quadrant_native_admission_specular_retained":next==2u?
                    "GI_native_texture_copy_proof":"native_point_light_probe");
        }
    }else{
        ptReflectionVerifyKeyDown.store(0u,std::memory_order_release);
    }

    const unsigned userState=ptReflectionUserState.load(std::memory_order_acquire);
    const bool userEnabled=(userState&1u)!=0u;
    // A/B only the added lighting: native composite above always runs, and the
    // stabilized ray library/first-hit sampling remain identical in both views.
    static std::atomic<unsigned int> lastUserEnabled{2u};
    if(lastUserEnabled.exchange(unsigned(userEnabled),std::memory_order_acq_rel)!=unsigned(userEnabled))
        Log("PT_REFLECTION_AB_APPLIED enabled=%u native_composite=1 added_composite=%u ray_tracing_unchanged=1 verify_mode=%u",
            unsigned(userEnabled),unsigned(userEnabled),userState>>1u);
    if(!userEnabled)return;
    if(!PTReflectionAuditTwoLayerHitTextures())return;

    const auto call=ptReflectionP2PostDispatches.fetch_add(1,std::memory_order_relaxed)+1;
    const bool psoCaptured=ptReflectionCompositePsoCaptured.load(std::memory_order_acquire)!=0u;
    const bool customReady=psoCaptured&&PTReflectionEnsureCapturedCompositePso();
    if(!customReady){
        if(call<=16 || (call%240)==0)
            Log("PT_REFLECTION_P2_POST_DISPATCH_WAIT call=%llu reason=pso_not_ready pso_captured=%u native_pso=%p p2_pso=%p root=%p",
                call,unsigned(psoCaptured),ptReflectionNativeCompositePso,
                ptReflectionP2CompositePso,ptReflectionCompositeRoot);
        return;
    }

    unsigned x=0,y=0,z=0;
    DWORD dimFault=0;
    __try {
        const auto* d=static_cast<const int*>(dimensions);
        if(d){
            x=d[0]>0?static_cast<unsigned>(d[0]):0u;
            y=d[1]>0?static_cast<unsigned>(d[1]):0u;
            z=d[2]>0?static_cast<unsigned>(d[2]):0u;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        dimFault=GetExceptionCode();
    }
    if(dimFault||!x||!y||!z||x>16384u||y>16384u||z>64u){
        ptReflectionP2InjectionEnabled=false;
        Log("PT_REFLECTION_P2_POST_DISPATCH_DISABLE call=%llu reason=invalid_dimensions x=%u y=%u z=%u exception=0x%08lX fallback=p1",
            call,x,y,z,dimFault);
        return;
    }

    using GetNativeTextureFn=void* (*)(void*);
    auto getter=reinterpret_cast<GetNativeTextureFn>(
        verifiedD3d?GetProcAddress(verifiedD3d,kRRGetNativeTextureSymbol):nullptr);
    void* wrapper=nullptr;void* raw=nullptr;D3D12_RESOURCE_DESC resourceDesc{};
    DWORD describeFault=0;
    if(getter&&verifiedRenderer){
        __try {
            auto* object=reinterpret_cast<unsigned char*>(verifiedRenderer)+kRRShaderReflectionTargetRva;
            wrapper=getter(object);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            wrapper=nullptr;
        }
    }
    if(!wrapper||!Describe(wrapper,&raw,&resourceDesc,&describeFault)||!raw){
        ptReflectionP2InjectionEnabled=false;
        Log("PT_REFLECTION_P2_POST_DISPATCH_DISABLE call=%llu reason=reflection_target_unavailable describe_fault=0x%08lX fallback=p1",
            call,describeFault);
        return;
    }

    EngineCommandContextSnapshot command{};
    auto* resource=static_cast<ID3D12Resource*>(raw);
    if(!ReadEngineCommandContext(resource,&command)||!command.commandList){
        ptReflectionP2InjectionEnabled=false;
        Log("PT_REFLECTION_P2_POST_DISPATCH_DISABLE call=%llu reason=command_context_unavailable fault=0x%08lX fallback=p1",
            call,command.fault);
        return;
    }

    auto* list=static_cast<ID3D12GraphicsCommandList*>(command.commandList);
    const unsigned verifyMode=userState>>1u;
    ID3D12PipelineState* activeP2Pso=verifyMode==1u?
        ptReflectionP2VerifyPso:verifyMode==3u?
        ptReflectionP2LocalLightProbePso:ptReflectionP2CompositePso;
    if(!activeP2Pso){
        if(call<=16 || (call%240)==0)
            Log("PT_REFLECTION_VERIFY_MISSING_PSO call=%llu mode=%u verify_sidecar=%u native_light_probe_sidecar=%u",
                call,verifyMode,unsigned(ptReflectionP2VerifyLoaded),
                unsigned(ptReflectionP2LocalLightProbeLoaded));
        return;
    }
    DWORD fault=0,restoreFault=0;
    bool dispatched=false,restored=false;
    __try {
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barrier.Flags=D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.UAV.pResource=resource;

        // Control's native composite just established the correct root signature,
        // descriptor heaps/tables, CBVs/SRVs/UAV and dimensions. Swap only the PSO.
        list->ResourceBarrier(1,&barrier);
        list->SetPipelineState(activeP2Pso);
        list->Dispatch(x,y,z);
        list->ResourceBarrier(1,&barrier);
        list->SetPipelineState(ptReflectionNativeCompositePso);
        dispatched=true;
        restored=true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        __try {
            list->SetPipelineState(ptReflectionNativeCompositePso);
            restored=true;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            restoreFault=GetExceptionCode();
            restored=false;
        }
    }

    if(fault||restoreFault||!dispatched||!restored){
        ptReflectionP2InjectionEnabled=false;
        Log("PT_REFLECTION_P2_POST_DISPATCH_DISABLE call=%llu reason=direct_pso_dispatch_failed dispatched=%u restored=%u exception=0x%08lX restore_exception=0x%08lX x=%u y=%u z=%u fallback=p1",
            call,unsigned(dispatched),unsigned(restored),fault,restoreFault,x,y,z);
        return;
    }

    if(call<=16 || (call%240)==0)
        Log("PT_REFLECTION_P2_POST_DISPATCH call=%llu path=early_captured_direct_pso dispatched=1 restored=1 x=%u y=%u z=%u native_pso=%p p2_pso=%p root=%p command_list=%p verify_mode=%u verify_pso=%u",
            call,x,y,z,ptReflectionNativeCompositePso,activeP2Pso,
            ptReflectionCompositeRoot,list,verifyMode,unsigned(verifyMode==1u));
}

static bool PTReflectionInstallStorageGetHooks(HMODULE d3d) noexcept {
    if(!d3d||d3d!=verifiedD3d)return false;
    ptReflectionOriginalStorageGet=reinterpret_cast<PTShaderCodeStorageGetFn>(
        GetProcAddress(d3d,"?get@ShaderCodeStorage@d3d@@YAPEBDH@Z"));
    if(!ptReflectionOriginalStorageGet||
       reinterpret_cast<std::uintptr_t>(ptReflectionOriginalStorageGet)!=
       reinterpret_cast<std::uintptr_t>(d3d)+0x40820u)return false;
    auto* base=reinterpret_cast<unsigned char*>(d3d);
    const size_t sites[6]{0x2D44F,0x2D46C,0x2D492,0x2D4B8,0x2D4DE,0x348DC};
    for(unsigned i=0;i<6;++i){
        if(!RRAlbedoPrepareCallPatch(base+sites[i],reinterpret_cast<void*>(ptReflectionOriginalStorageGet),
            reinterpret_cast<void*>(&PTReflectionHookShaderStorageGet),&ptReflectionStorageGetPatches[i])){
            Log("PT_REFLECTION_P1_GET_HOOK_PREPARE ready=0 index=%u rva=0x%zX",i,sites[i]);
            return false;
        }
    }
    unsigned applied=0;
    for(;applied<6;++applied){
        if(!RRAlbedoExchangeCall(&ptReflectionStorageGetPatches[applied],true))break;
        if(!ptReflectionStorageGetPatches[applied].writeHealthy){++applied;break;}
    }
    if(applied!=6){
        while(applied){
            --applied;
            RRAlbedoExchangeCall(&ptReflectionStorageGetPatches[applied],false);
        }
        Log("PT_REFLECTION_P1_GET_HOOKS ready=0 applied=%u expected=6",applied);
        return false;
    }
    Log("PT_REFLECTION_P1_GET_HOOKS ready=1 count=6 sentinel_id=%d rvas=2D44F,2D46C,2D492,2D4B8,2D4DE,348DC native_passthrough=1",
        kPTReflectionP1SyntheticStorageId);
    return true;
}

static void* PTReflectionHookVariantLookup(void* technique,unsigned key) {
    void* nativeResult=ptReflectionOriginalVariantLookup(technique,key);
    void* result=nativeResult;

    if(technique&&technique==ptReflectionDeferredTechniqueObject){
        const auto call=++ptReflectionDeferredVariantLookups;
        PTReflectionLogDeferredRecordMap(nativeResult);
        ptReflectionNativeDeferredShaderRecord=nativeResult;
        ptReflectionAwaitingCompositeBind=false;
        Log("PT_REFLECTION_P2_DEFERRED_VARIANT call=%llu technique=%p key=0x%X shader=%p native_record_captured=1 replacement=native_then_descriptor_swap_add native_result_preserved=1",
            call,technique,key,nativeResult);
        ptReflectionDeferredTechniqueObject=nullptr;
        return nativeResult;
    }

    if(technique&&technique==ptReflectionTechniqueObject){
        const auto call=++ptReflectionVariantLookups;
        PTReflectionRayIdentity identity{};
        const bool identityKnown=PTReflectionReadRayIdentity(technique,nativeResult,&identity);
        const bool admitted=identityKnown&&identity.variant>=0;
        if(admitted) PTReflectionInspectRayLibrary(technique,nativeResult,key);

        bool p2Injected=false;
        bool p1Injected=false;
        bool nativeSwapped=false;

        if(admitted&&identity.variant==0&&ptReflectionP2InjectionEnabled){
            if(!ptReflectionP2RayShaderRecordReady)
                PTReflectionPrepareP2RayShaderRecord(nativeResult,identity);
            if(ptReflectionP2RayShaderRecordReady){
                result=ptReflectionP2RayShaderRecord;
                p2Injected=true;
            }
        }

        if(!p2Injected&&admitted&&ptReflectionP1InjectionEnabled&&identity.variant==0){
            if(!ptReflectionP1ShaderRecordReady)
                PTReflectionPrepareP1ShaderRecord(nativeResult,identity);
            if(ptReflectionP1ShaderRecordReady){
                result=ptReflectionP1ShaderRecord;
                p1Injected=true;
            }
        } else if(!p2Injected&&!p1Injected&&admitted&&ptReflectionForcedNativeVariant>=0){
            if(ptReflectionSwapTechnique!=technique||
               ptReflectionSwapVariant!=ptReflectionForcedNativeVariant||
               !ptReflectionSwapShader){
                void* candidate=nullptr;PTReflectionRayIdentity swapIdentity{};
                if(PTReflectionFindNativeVariant(technique,ptReflectionForcedNativeVariant,
                    &candidate,&swapIdentity)){
                    ptReflectionSwapTechnique=technique;
                    ptReflectionSwapVariant=ptReflectionForcedNativeVariant;
                    ptReflectionSwapShader=candidate;
                }else{
                    ptReflectionSwapTechnique=technique;
                    ptReflectionSwapVariant=ptReflectionForcedNativeVariant;
                    ptReflectionSwapShader=nullptr;
                }
            }
            if(ptReflectionSwapShader){
                result=ptReflectionSwapShader;
                nativeSwapped=result!=nativeResult;
            }
        }
        Log("PT_REFLECTION_P1_VARIANT call=%llu technique=%p key=0x%X native_shader=%p returned_shader=%p admitted=%u variant=%d p2_requested=%u p2_record_ready=%u p2_injected=%u p1_requested=%u p1_record_ready=%u p1_injected=%u native_swapped=%u forced_native_variant=%d mode=%s native_result_preserved=%u",
            call,technique,key,nativeResult,result,unsigned(admitted),identityKnown?identity.variant:-1,
            unsigned(ptReflectionP2InjectionEnabled),unsigned(ptReflectionP2RayShaderRecordReady),
            unsigned(p2Injected),unsigned(ptReflectionP1InjectionEnabled),
            unsigned(ptReflectionP1ShaderRecordReady),unsigned(p1Injected),
            unsigned(nativeSwapped),ptReflectionForcedNativeVariant,
            p2Injected?"p2_paired_ray_bridge":(p1Injected?"p1_raw_get_bridge":(nativeSwapped?"native_variant_swap_probe":"observe_only")),
            unsigned(result==nativeResult));
        ptReflectionTechniqueObject=nullptr;
    }
    return result;
}

static void PTReflectionLogShaderCodeStorageGetterBytes(HMODULE d3d) noexcept {
    if(!d3d||d3d!=verifiedD3d)return;
    char hex[65]{};
    DWORD fault=0;
    __try {
        auto* p=reinterpret_cast<const unsigned char*>(d3d)+0x40820u;
        for(unsigned i=0;i<32;++i)sprintf_s(hex+i*2,3,"%02X",p[i]);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
        hex[0]=0;
    }
    Log("PT_REFLECTION_P1_SHADER_STORAGE_GET_BYTES rva=0x40820 bytes=%s fault=0x%08lX",hex,fault);
}

static unsigned PTReflectionLogShaderCodeStorageGetterCallsites(HMODULE d3d) noexcept {
    if(!d3d||d3d!=verifiedD3d)return 0;
    unsigned matches=0;
    __try {
        auto* base=reinterpret_cast<unsigned char*>(d3d);
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return 0;
        auto* section=IMAGE_FIRST_SECTION(nt);
        const auto target=reinterpret_cast<std::uintptr_t>(base)+0x40820u;
        const std::uintptr_t rtEntries[5]{
            reinterpret_cast<std::uintptr_t>(GetProcAddress(d3d,kRRBeginPipelineSetupSymbol)),
            reinterpret_cast<std::uintptr_t>(GetProcAddress(d3d,kRRSetRayGenerationSymbol)),
            reinterpret_cast<std::uintptr_t>(GetProcAddress(d3d,kRRSetMissSymbol)),
            reinterpret_cast<std::uintptr_t>(GetProcAddress(d3d,kRRSetHitGroupSymbol)),
            reinterpret_cast<std::uintptr_t>(GetProcAddress(d3d,kRRRaytraceSymbol))
        };
        const char* rtNames[5]{"begin_pipeline","set_raygen","set_miss","set_hitgroup","raytrace"};
        for(unsigned si=0;si<nt->FileHeader.NumberOfSections;++si){
            const auto& sh=section[si];
            if(!(sh.Characteristics&IMAGE_SCN_CNT_CODE))continue;
            auto* start=base+sh.VirtualAddress;
            const size_t bytes=sh.Misc.VirtualSize;
            if(bytes<5)continue;
            for(size_t i=0;i+5<=bytes;++i){
                if(start[i]!=0xE8)continue;
                int32_t rel=0;memcpy(&rel,start+i+1,sizeof(rel));
                const auto destination=reinterpret_cast<std::uintptr_t>(start+i+5)+static_cast<std::intptr_t>(rel);
                if(destination!=target)continue;
                ++matches;
                if(matches<=64){
                    const auto callAddress=reinterpret_cast<std::uintptr_t>(start+i);
                    size_t nearest=0;std::uintptr_t best=~std::uintptr_t(0);
                    for(size_t e=0;e<_countof(rtEntries);++e){
                        if(!rtEntries[e])continue;
                        const auto distance=callAddress>rtEntries[e]?callAddress-rtEntries[e]:rtEntries[e]-callAddress;
                        if(distance<best){best=distance;nearest=e;}
                    }
                    Log("PT_REFLECTION_P1_SHADER_STORAGE_GET_CALLSITE index=%u rva=0x%zX address=%p nearest_rt=%s distance=0x%llX",
                        matches,static_cast<size_t>(sh.VirtualAddress+i),start+i,rtNames[nearest],
                        static_cast<unsigned long long>(best));
                }
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("PT_REFLECTION_P1_SHADER_STORAGE_GET_CALLSITE_SCAN exception=0x%08lX matches=%u",
            GetExceptionCode(),matches);
    }
    Log("PT_REFLECTION_P1_SHADER_STORAGE_GET_CALLSITE_SUMMARY matches=%u target_rva=0x40820",matches);
    return matches;
}

static unsigned PTReflectionLogShaderCodeStorageExports(HMODULE d3d) noexcept {
    if(!d3d||d3d!=verifiedD3d)return 0;
    unsigned matches=0;
    __try {
        auto* base=reinterpret_cast<unsigned char*>(d3d);
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return 0;
        auto* nt=reinterpret_cast<IMAGE_NT_HEADERS64*>(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||
           nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC)return 0;
        const auto& dir=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        if(!dir.VirtualAddress||dir.Size<sizeof(IMAGE_EXPORT_DIRECTORY))return 0;
        auto* exports=reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(base+dir.VirtualAddress);
        if(!exports->AddressOfNames||!exports->AddressOfNameOrdinals||
           !exports->AddressOfFunctions||exports->NumberOfNames>65536)return 0;
        auto* names=reinterpret_cast<DWORD*>(base+exports->AddressOfNames);
        auto* ordinals=reinterpret_cast<WORD*>(base+exports->AddressOfNameOrdinals);
        auto* functions=reinterpret_cast<DWORD*>(base+exports->AddressOfFunctions);
        for(DWORD i=0;i<exports->NumberOfNames && matches<32;++i){
            const char* name=reinterpret_cast<const char*>(base+names[i]);
            if(!name||!strstr(name,"ShaderCodeStorage"))continue;
            const WORD ordinalIndex=ordinals[i];
            if(ordinalIndex>=exports->NumberOfFunctions)continue;
            const DWORD rva=functions[ordinalIndex];
            ++matches;
            Log("PT_REFLECTION_P1_SHADER_STORAGE_EXPORT index=%u name=%s rva=0x%X address=%p",
                matches,name,rva,base+rva);
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        Log("PT_REFLECTION_P1_SHADER_STORAGE_EXPORT_SCAN exception=0x%08lX matches=%u",
            GetExceptionCode(),matches);
    }
    Log("PT_REFLECTION_P1_SHADER_STORAGE_EXPORT_SUMMARY matches=%u known_get_rva=0x40820",matches);
    return matches;
}



static void PTReflectionDescribeStoredBlob(const unsigned char* source,size_t bytes,
    const char* sourceSha,const char* resolved,int id,const char* label) noexcept {
    char resolvedSha[65]{};
    char sourceHead[65]{};
    char resolvedHead[65]{};
    unsigned long long firstDiff=~0ull;
    uint32_t resolvedDeclared=0;
    bool resolvedDxbc=false;
    bool hashOkay=false;
    DWORD fault=0;
    __try {
        const size_t head=(std::min)(bytes,size_t(32));
        for(size_t i=0;i<head;++i){
            sprintf_s(sourceHead+i*2,3,"%02X",source[i]);
            sprintf_s(resolvedHead+i*2,3,"%02X",static_cast<unsigned char>(resolved[i]));
        }
        if(bytes>=28){
            memcpy(&resolvedDeclared,resolved+24,sizeof(resolvedDeclared));
            resolvedDxbc=memcmp(resolved,"DXBC",4)==0;
        }
        for(size_t i=0;i<bytes;++i){
            if(static_cast<unsigned char>(resolved[i])!=source[i]){firstDiff=i;break;}
        }
        hashOkay=PTReflectionSha256(reinterpret_cast<const unsigned char*>(resolved),bytes,resolvedSha);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();
    }
    Log("PT_REFLECTION_%s_STORAGE_RESOLVE id=%d resident=%p bytes=%llu source_sha256=%s resolved_sha256=%s hash_ok=%u exact=%u first_diff=%llu source_head=%s resolved_head=%s resolved_dxbc=%u resolved_declared=%u exception=0x%08lX",
        label,id,resolved,static_cast<unsigned long long>(bytes),sourceSha?sourceSha:"",resolvedSha,
        unsigned(hashOkay),unsigned(hashOkay&&sourceSha&&strcmp(sourceSha,resolvedSha)==0),
        firstDiff,sourceHead,resolvedHead,unsigned(resolvedDxbc),resolvedDeclared,fault);
}

static void PTReflectionRegisterP1InNativeStorage() noexcept {
    if(ptReflectionP1StorageVerified||ptReflectionP1StorageAttempted||
       !ptReflectionP1Loaded||ptReflectionP1Bytes.empty()||
       !ptReflectionStoragePersist||!RRAlbedoShader::residentGetter)return;

    ptReflectionP1StorageAttempted=true;
    int id=-1;const char* resident=nullptr;DWORD fault=0;
    __try {
        id=ptReflectionStoragePersist(
            reinterpret_cast<const char*>(ptReflectionP1Bytes.data()),
            static_cast<unsigned long long>(ptReflectionP1Bytes.size()),6u);
        if(id>=0)resident=RRAlbedoShader::residentGetter(id);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();id=-1;resident=nullptr;
    }
    ptReflectionP1StorageId=id;
    bool exact=false;
    if(id>=0&&resident&&fault==0){
        char resolvedSha[65]{};
        __try {
            exact=PTReflectionSha256(reinterpret_cast<const unsigned char*>(resident),
                ptReflectionP1Bytes.size(),resolvedSha)&&
                strcmp(resolvedSha,ptReflectionP1Sha256)==0;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault=GetExceptionCode();exact=false;
        }
        PTReflectionDescribeStoredBlob(ptReflectionP1Bytes.data(),ptReflectionP1Bytes.size(),
            ptReflectionP1Sha256,resident,id,"P1");
    }
    ptReflectionP1StorageVerified=id>=0&&resident&&exact&&fault==0;
    Log("PT_REFLECTION_P1_STORAGE_REGISTER id=%d resident=%p bytes=%llu source_sha256=%s exact=%u verified=%u exception=0x%08lX activation=disabled delayed_dispatch_gate=240",
        id,resident,static_cast<unsigned long long>(ptReflectionP1Bytes.size()),
        ptReflectionP1Sha256,unsigned(exact),unsigned(ptReflectionP1StorageVerified),fault);
}

static void PTReflectionProbeCachedTechniqueOnce(HMODULE renderer,HMODULE d3d) noexcept {
    if(ptReflectionStorageRoundtripDone.load(std::memory_order_acquire)){
        const auto dispatch=ptReflectionDispatchCount.load(std::memory_order_relaxed);
        if(dispatch&&dispatch%240ull==0)
            Log("PT_REFLECTION_P1_STORAGE_ROUNDTRIP_SUMMARY dispatch=%llu technique=%p shader=%p stage=%u native_id=%d persist_result=%d is_persisted_before=%u same_id=%u valid=%u variant=%u sidecar_loaded=%u sidecar_bytes=%llu sidecar_sha256=%s activation=disabled",
                dispatch,ptReflectionStorageRoundtripTechnique,ptReflectionStorageRoundtripShader,
                ptReflectionStorageRoundtripStage,ptReflectionStorageRoundtripNativeId,
                ptReflectionStorageRoundtripPersistResult,unsigned(ptReflectionStorageRoundtripBefore),
                unsigned(ptReflectionStorageRoundtripSame),unsigned(ptReflectionStorageRoundtripValid),
                ptReflectionStorageRoundtripVariant,unsigned(ptReflectionP2Loaded),
                static_cast<unsigned long long>(ptReflectionP2Bytes.size()),ptReflectionP2Sha256);
        return;
    }
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d||!RRAlbedoShader::residentGetter||
       !ptReflectionStoragePersist||!ptReflectionStorageIsPersisted)return;
    void* technique=nullptr;
    __try {
        technique=*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(renderer)+0x128D900u);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    }
    if(!technique)return;

    unsigned char* table=nullptr;unsigned count=0;
    __try {
        table=*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(technique)+0x238);
        count=*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(technique)+0x240);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return;
    }
    if(!table||count==0||count>128)return;

    for(unsigned i=0;i<count;++i){
        auto* shader=table+static_cast<size_t>(i)*0xA8u;
        PTReflectionRayIdentity identity{};
        if(!PTReflectionReadRayIdentity(technique,shader,&identity))continue;

        const auto& d=identity.descriptor;
        const char* resident=nullptr;
        bool before=false;int persisted=-0x7fffffff;DWORD fault=0;
        unsigned char descriptorBytes[32]{};
        char descriptorHex[65]{};
        __try {
            resident=RRAlbedoShader::residentGetter(d.identifier);
            if(!resident)continue;
            before=ptReflectionStorageIsPersisted(resident);
            memcpy(descriptorBytes,reinterpret_cast<const void*>(d.shaderBase),sizeof(descriptorBytes));
            persisted=ptReflectionStoragePersist(resident,static_cast<unsigned long long>(d.size),d.stage);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            fault=GetExceptionCode();
        }
        for(unsigned n=0;n<sizeof(descriptorBytes);++n)
            sprintf_s(descriptorHex+n*2,3,"%02X",descriptorBytes[n]);

        const bool same=persisted==d.identifier;
        const bool valid=persisted>=0&&fault==0;
        if(valid){
            const char* persistedResident=nullptr;
            __try { persistedResident=RRAlbedoShader::residentGetter(persisted); }
            __except(EXCEPTION_EXECUTE_HANDLER) { persistedResident=nullptr; }
            if(persistedResident){
                unsigned char* nativeCopy=static_cast<unsigned char*>(HeapAlloc(GetProcessHeap(),0,static_cast<size_t>(d.size)));
                if(nativeCopy){
                    bool copied=false;
                    __try {
                        memcpy(nativeCopy,resident,static_cast<size_t>(d.size));
                        copied=true;
                    } __except(EXCEPTION_EXECUTE_HANDLER) { copied=false; }
                    if(copied) PTReflectionDescribeStoredBlob(nativeCopy,static_cast<size_t>(d.size),
                        identity.sha256,persistedResident,persisted,"NATIVE");
                    HeapFree(GetProcessHeap(),0,nativeCopy);
                }
            }
        }
        ptReflectionStorageRoundtripPersistResult=persisted;
        ptReflectionStorageRoundtripNativeId=d.identifier;
        ptReflectionStorageRoundtripStage=d.stage;
        ptReflectionStorageRoundtripVariant=identity.variant>=0?static_cast<unsigned>(identity.variant):0xffffffffu;
        ptReflectionStorageRoundtripBefore=before;
        ptReflectionStorageRoundtripSame=same;
        ptReflectionStorageRoundtripValid=valid;
        ptReflectionStorageRoundtripTechnique=technique;
        ptReflectionStorageRoundtripShader=shader;
        ptReflectionStorageRoundtripDone.store(1u,std::memory_order_release);
        Log("PT_REFLECTION_P1_STORAGE_ROUNDTRIP technique=%p table_index=%u shader=%p descriptor=%p stage=%u native_id=%d bytes=%llu resident=%p is_persisted_before=%u persist_result=%d same_id=%u valid=%u exception=0x%08lX descriptor32=%s native_sha256=%s variant=%03d sidecar_loaded=%u sidecar_bytes=%llu sidecar_sha256=%s activation=disabled",
            technique,identity.tableIndex,shader,reinterpret_cast<void*>(d.shaderBase),d.stage,d.identifier,
            static_cast<unsigned long long>(d.size),resident,unsigned(before),persisted,unsigned(same),unsigned(valid),
            fault,descriptorHex,identity.sha256,identity.variant,unsigned(ptReflectionP2Loaded),
            static_cast<unsigned long long>(ptReflectionP2Bytes.size()),ptReflectionP2Sha256);
        return;
    }
}

static bool PTReflectionInstallShaderLookupHooks(HMODULE renderer,HMODULE d3d) noexcept {
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d)return false;
    PTReflectionLogShaderCodeStorageExports(d3d);
    PTReflectionLogShaderCodeStorageGetterBytes(d3d);
    PTReflectionLogShaderCodeStorageGetterCallsites(d3d);
    ptReflectionStoragePersist=reinterpret_cast<PTShaderCodeStoragePersistFn>(
        GetProcAddress(d3d,"?persist@ShaderCodeStorage@d3d@@YAHPEBD_KW4ShaderType@2@@Z"));
    ptReflectionStorageIsPersisted=reinterpret_cast<PTShaderCodeStorageIsPersistedFn>(
        GetProcAddress(d3d,"?isPersisted@ShaderCodeStorage@d3d@@YA_NPEBD@Z"));
    ptReflectionStorageRoundtripPersistResult=-0x7fffffff;
    ptReflectionStorageRoundtripNativeId=-1;
    ptReflectionStorageRoundtripStage=0;
    ptReflectionStorageRoundtripVariant=0xffffffffu;
    ptReflectionStorageRoundtripBefore=false;
    ptReflectionStorageRoundtripSame=false;
    ptReflectionStorageRoundtripValid=false;
    ptReflectionStorageRoundtripTechnique=nullptr;
    ptReflectionStorageRoundtripShader=nullptr;
    ptReflectionStorageRoundtripDone.store(0u,std::memory_order_release);
    Log("PT_REFLECTION_P1_STORAGE_API persist=%p is_persisted=%p ready=%u usage=disabled_visual_build reason=private_packed_representation",
        reinterpret_cast<void*>(ptReflectionStoragePersist),reinterpret_cast<void*>(ptReflectionStorageIsPersisted),
        unsigned(ptReflectionStoragePersist&&ptReflectionStorageIsPersisted));
    PTReflectionLoadP1Sidecar();
    PTReflectionLoadP2Sidecar();
    PTReflectionLoadP2VerifySidecar();
    PTReflectionLoadP2LocalLightProbeSidecar();
    PTReflectionLoadP2CompositeSidecar();
    PTReflectionLoadP2DiffuseBridgeSidecar();
    ptReflectionCompositePsoCaptured.store(0u,std::memory_order_release);
    ptReflectionNativeCompositePso=nullptr;
    ptReflectionP2CompositePso=nullptr;
    ptReflectionP2VerifyPso=nullptr;
    ptReflectionP2LocalLightProbePso=nullptr;
    ptReflectionTwoLayerContract.store(0u,std::memory_order_release);
    ptReflectionTwoLayerAttempts.store(0u,std::memory_order_release);
    ptReflectionUserState.fetch_and(1u,std::memory_order_acq_rel);
    ptReflectionVerifyKeyDown.store(0u,std::memory_order_release);
    Log("PT_REFLECTION_DIFFUSE_PRESERVATION native_diffuse_gi=1 native_direct_emission=1 specular_only_replacement=1 vndf_weight=F_G1 native_parity_gate=1");
    Log("PT_REFLECTION_NATIVE_SECONDARY_SHADING enabled=1 material_brdf=1 light_cells=1 point_shadow_atlas=1 spot_shadow_atlases=1 gi_volumes=1 native_shader_sha=4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9");
    Log("PT_REFLECTION_MATERIAL_BOUNDS source=bound_t4_GetDimensions native_b6_count_ignored=1 expected_stride=336");
    Log("PT_REFLECTION_REJECTION_DIAGNOSTIC enabled=1 F11_mode=1 red=first_hit magenta=second_hit yellow=normal cyan=segment white=material_range orange=native_parity green=accepted");
    Log("PT_REFLECTION_HIT_UAV_COHERENCE enabled=1 globallycoherent_uavs=3 raygen_publish=1 closesthit_publish=1 miss_publish=1");
    Log("PT_REFLECTION_SECONDARY_MATERIAL_FILTER enabled=1 uv_steps=31 footprint_mip=1 emission_texture_slot=5 primary_ray_change=0");
    Log("PT_REFLECTION_SECONDARY_ORIGIN_PRECISION enabled=1 source=ray_origin_direction_payload distance_quantum_m=0.0001 fp16_position_reuse=0 native_samples=1");
    Log("PT_REFLECTION_TWO_LAYER_PROOF_CONFIG ray_paths=1 first_layer=0 second_layer=1 native_material_sentinel=65534 secondary_id_storage=numeric_half secondary_uv_storage=numeric_5bit_each native_first_sample_tradeoff=1 experimental=1");
    Log("PT_REFLECTION_VERIFY_INIT mode=0 hotkey=F11 cycle=normal,composite_quadrants,diffuse_gi_native_copy_proof,native_point_light_probe verify_sidecar=%u local_probe_sidecar=%u",
        unsigned(ptReflectionP2VerifyLoaded),unsigned(ptReflectionP2LocalLightProbeLoaded));
    ptReflectionCompositeRoot=nullptr;
    ptReflectionP1ShaderRecordReady=false;
    ptReflectionP2RayShaderRecordReady=false;
    ptReflectionP2CompositeShaderRecordReady=false;
    ptReflectionNativeDeferredShaderRecord=nullptr;
    ptReflectionRecordMapLogged.store(0,std::memory_order_release);
    ptReflectionDeferredRecordMapLogged.store(0,std::memory_order_release);
    ptReflectionDeferredDxbcDumped.store(0,std::memory_order_release);
    ptReflectionP1GetIntercepts.store(0,std::memory_order_release);
    ptReflectionP2RayGetIntercepts.store(0,std::memory_order_release);
    ptReflectionP2CompositeGetIntercepts.store(0,std::memory_order_release);
    ptReflectionP2PostDispatches.store(0,std::memory_order_release);
    memset(ptReflectionNativeCompositeCacheTail,0,sizeof(ptReflectionNativeCompositeCacheTail));
    memset(ptReflectionP2CompositeCacheTail,0,sizeof(ptReflectionP2CompositeCacheTail));
    ptReflectionNativeCompositeCacheTailReady=false;
    ptReflectionP2CompositeCacheTailReady=false;
    ptReflectionP2InjectionEnabled=false;
    const bool storageGetHooksReady=PTReflectionInstallStorageGetHooks(d3d);
    wchar_t p1EnableValue[8]{};
    const DWORD p1EnableLength=GetEnvironmentVariableW(
        L"CONTROLFG_PT_REFLECTION_P1",p1EnableValue,_countof(p1EnableValue));
    const bool explicitDisable=p1EnableLength>0&&p1EnableLength<_countof(p1EnableValue)&&p1EnableValue[0]==L'0';
    ptReflectionP1InjectionEnabled=ptReflectionP1Loaded&&storageGetHooksReady&&!explicitDisable;
    Log("PT_REFLECTION_P1_INJECTION_CONFIG enabled=%u sidecar=%u get_hooks=%u variant_gate=000 exact_native_sha=1 env=CONTROLFG_PT_REFLECTION_P1 default=enabled disable_with_0 fail_closed=1",
        unsigned(ptReflectionP1InjectionEnabled),unsigned(ptReflectionP1Loaded),unsigned(storageGetHooksReady));
    ptReflectionP1StorageId=-1;
    ptReflectionP1StorageVerified=false;
    ptReflectionP1StorageAttempted=false;
    ptReflectionForcedNativeVariant=PTReflectionReadForcedNativeVariant();
    ptReflectionSwapTechnique=nullptr;
    ptReflectionSwapShader=nullptr;
    ptReflectionSwapVariant=-1;
    Log("PT_REFLECTION_P1_NATIVE_SWAP_CONFIG forced_variant=%d env=CONTROLFG_PT_REFLECTION_NATIVE_VARIANT default=disabled",
        ptReflectionForcedNativeVariant);
    if(!RRAlbedoShader::Initialize(d3d)){
        Log("PT_REFLECTION_P1_LOOKUP_HOOKS ready=0 reason=resident_shader_getter");
        return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    ptReflectionOriginalRfxLookup=reinterpret_cast<PTReflectionRfxLookupFn>(base+0x1D9960);
    ptReflectionOriginalTechniqueLookup=reinterpret_cast<PTReflectionTechniqueLookupFn>(base+0x1D2CE0);
    ptReflectionOriginalVariantLookup=reinterpret_cast<PTReflectionVariantLookupFn>(base+0x1DD450);
    ptReflectionBindCompute=reinterpret_cast<PTReflectionBindComputeFn>(base+0x1DB750);
    ptReflectionOriginalDispatch=reinterpret_cast<PTReflectionDispatchFn>(
        GetProcAddress(d3d,"?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z"));

    bool p2DispatchHookReady=false;
    auto** dispatchSlot=reinterpret_cast<void**>(base+0x5DFCC0);
    if(ptReflectionOriginalDispatch&&dispatchSlot&&
       ptReflectionDeferredDispatchPatch.Prepare(base+0x129714,dispatchSlot,
            reinterpret_cast<void*>(ptReflectionOriginalDispatch),
            reinterpret_cast<void*>(&PTReflectionHookDeferredDispatch))&&
       ptReflectionDeferredDispatchPatch.Exchange(true)&&
       ptReflectionDeferredDispatchPatch.healthy){
        p2DispatchHookReady=true;
    }
    wchar_t p2EnableValue[8]{};
    const DWORD p2EnableLength=GetEnvironmentVariableW(
        L"CONTROLFG_PT_REFLECTION_P2",p2EnableValue,_countof(p2EnableValue));
    const bool p2ExplicitDisable=p2EnableLength>0&&p2EnableLength<_countof(p2EnableValue)&&p2EnableValue[0]==L'0';
    ptReflectionP2InjectionEnabled=ptReflectionP2Loaded&&ptReflectionP2CompositeLoaded&&
        ptReflectionP2DiffuseBridgeLoaded&&storageGetHooksReady&&p2DispatchHookReady&&!p2ExplicitDisable;
    Log("PT_REFLECTION_P2_INJECTION_CONFIG enabled=%u ray_sidecar=%u composite_sidecar=%u diffuse_bridge_sidecar=%u get_hooks=%u dispatch_hook=%u rough_route=diffuse_gi variant_gate=000 native_composite_sha=4fc0a0e9cd3706bcb86e29444d86fa32616138e1712c53b1b5103b30d7a1ecc9 env=CONTROLFG_PT_REFLECTION_P2 default=enabled disable_with_0 fail_closed=1",
        unsigned(ptReflectionP2InjectionEnabled),unsigned(ptReflectionP2Loaded),
        unsigned(ptReflectionP2CompositeLoaded),unsigned(ptReflectionP2DiffuseBridgeLoaded),
        unsigned(storageGetHooksReady),unsigned(p2DispatchHookReady));

    if(!RRAlbedoPrepareCallPatch(base+0x12953C,
        reinterpret_cast<void*>(ptReflectionOriginalRfxLookup),
        reinterpret_cast<void*>(&PTReflectionHookRfxLookup),
        &ptReflectionLookupPatches[0]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x12954B,
        reinterpret_cast<void*>(ptReflectionOriginalTechniqueLookup),
        reinterpret_cast<void*>(&PTReflectionHookTechniqueLookup),
        &ptReflectionLookupPatches[1]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x12956C,
        reinterpret_cast<void*>(ptReflectionOriginalVariantLookup),
        reinterpret_cast<void*>(&PTReflectionHookVariantLookup),
        &ptReflectionLookupPatches[2]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x1296B8,
        reinterpret_cast<void*>(ptReflectionOriginalRfxLookup),
        reinterpret_cast<void*>(&PTReflectionHookRfxLookup),
        &ptReflectionLookupPatches[3]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x1296C7,
        reinterpret_cast<void*>(ptReflectionOriginalTechniqueLookup),
        reinterpret_cast<void*>(&PTReflectionHookTechniqueLookup),
        &ptReflectionLookupPatches[4]))return false;
    if(!RRAlbedoPrepareCallPatch(base+0x1296E8,
        reinterpret_cast<void*>(ptReflectionOriginalVariantLookup),
        reinterpret_cast<void*>(&PTReflectionHookVariantLookup),
        &ptReflectionLookupPatches[5]))return false;

    unsigned applied=0;
    for(;applied<6;++applied){
        if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],true))break;
        if(!ptReflectionLookupPatches[applied].writeHealthy){++applied;break;}
    }
    if(applied!=6){
        while(applied){
            --applied;
            if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],false))
                Log("PT_REFLECTION_P1_HOOK_ROLLBACK_FAILED index=%u",applied);
        }
        return false;
    }
    Log("PT_REFLECTION_P1_LOOKUP_HOOKS ready=1 ray_calls=12953C,12954B,12956C deferred_calls=1296B8,1296C7,1296E8 rfx_helper=0x1D9960 technique_helper=0x1D2CE0 variant_helper=0x1DD450 native_hashes=32 native_swap_probe=%s forced_variant=%d gpu_work=%s",
        ptReflectionForcedNativeVariant>=0?"enabled":"disabled",ptReflectionForcedNativeVariant,
        ptReflectionForcedNativeVariant>=0?"native_reflection_permutation_may_change":"unchanged");
    return true;
}
