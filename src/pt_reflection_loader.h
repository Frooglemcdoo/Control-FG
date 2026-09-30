#pragma once
// Experimental PT reflection P1 discovery scaffold.
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

static PTReflectionRfxLookupFn ptReflectionOriginalRfxLookup=nullptr;
static PTReflectionTechniqueLookupFn ptReflectionOriginalTechniqueLookup=nullptr;
static PTReflectionVariantLookupFn ptReflectionOriginalVariantLookup=nullptr;
static RRAlbedoCallPatch ptReflectionLookupPatches[3]{};

static thread_local bool ptReflectionLookupScope=false;
static thread_local void* ptReflectionLastRfxObject=nullptr;
static thread_local void* ptReflectionTechniqueObject=nullptr;

static std::atomic<unsigned long long> ptReflectionRfxLookups{0};
static std::atomic<unsigned long long> ptReflectionTechniqueLookups{0};
static std::atomic<unsigned long long> ptReflectionVariantLookups{0};
static std::atomic<unsigned long long> ptReflectionVariantRejects{0};

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

static bool PTReflectionInspectRayLibrary(void* technique,void* shader,unsigned key) noexcept {
    if(!technique||!shader)return false;
    unsigned tableIndex=0xffffffffu,tableCount=0,actualKey=0;
    if(!PTReflectionTechniqueOwnsShader(technique,shader,&tableIndex,&tableCount,&actualKey)){
        const auto reject=++ptReflectionVariantRejects;
        Log("PT_REFLECTION_P1_VARIANT_REJECT count=%llu key=0x%X technique=%p shader=%p reason=technique_table_membership",
            reject,key,technique,shader);
        return false;
    }
    RRAlbedoShader::NativeDescriptor before{},after{};
    if(!RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(shader),0x70,&before)||
       before.identifier<0||before.size<32||before.size>RRAlbedoShader::kMaximumBlobBytes){
        const auto reject=++ptReflectionVariantRejects;
        Log("PT_REFLECTION_P1_VARIANT_REJECT count=%llu key=0x%X actual_key=0x%X table_index=%u table_count=%u shader=%p reason=ray_descriptor",reject,key,actualKey,tableIndex,tableCount,shader);
        return false;
    }
    const size_t bytes=static_cast<size_t>(before.size);
    unsigned char* copy=static_cast<unsigned char*>(HeapAlloc(GetProcessHeap(),0,bytes));
    if(!copy){
        const auto reject=++ptReflectionVariantRejects;
        Log("PT_REFLECTION_P1_VARIANT_REJECT count=%llu key=0x%X actual_key=0x%X table_index=%u table_count=%u shader=%p reason=allocation bytes=%zu",reject,key,actualKey,tableIndex,tableCount,shader,bytes);
        return false;
    }
    bool copied=RRAlbedoShader::CopyResident(&before,copy,bytes);
    bool stable=RRAlbedoShader::ReadDescriptor(reinterpret_cast<std::uintptr_t>(shader),0x70,&after) &&
        before.shaderBase==after.shaderBase&&before.stage==after.stage&&
        before.identifier==after.identifier&&before.size==after.size;
    uint32_t declared=0;
    if(copied&&bytes>=28)memcpy(&declared,copy+24,sizeof(declared));
    const bool container=copied&&stable&&bytes>=32&&memcmp(copy,"DXBC",4)==0&&declared==bytes;
    char sha[65]{};
    const bool hashed=container&&PTReflectionSha256(copy,bytes,sha);
    const int variant=hashed?PTReflectionKnownVariant(sha):-1;
    HeapFree(GetProcessHeap(),0,copy);
    if(!container||!hashed||variant<0){
        const auto reject=++ptReflectionVariantRejects;
        Log("PT_REFLECTION_P1_VARIANT_REJECT count=%llu key=0x%X actual_key=0x%X table_index=%u table_count=%u shader=%p stage=%u identifier=%d bytes=%llu stable=%u container=%u hashed=%u sha256=%s reason=unknown_native_library",
            reject,key,actualKey,tableIndex,tableCount,shader,before.stage,before.identifier,static_cast<unsigned long long>(before.size),
            unsigned(stable),unsigned(container),unsigned(hashed),hashed?sha:"<none>");
        return false;
    }
    Log("PT_REFLECTION_P1_VARIANT_ADMIT key=0x%X actual_key=0x%X table_index=%u table_count=%u shader=%p ray_descriptor=%p stage=%u identifier=%d bytes=%llu sha256=%s variant=%03d exact_native_hash=1 replacement=disabled",
        key,actualKey,tableIndex,tableCount,shader,reinterpret_cast<void*>(before.shaderBase),before.stage,before.identifier,
        static_cast<unsigned long long>(before.size),sha,variant);
    return true;
}

static void* PTReflectionHookRfxLookup(void* manager,const char* name) {
    char safeName[128]{};
    RRSafeCopyCString(name,safeName,sizeof(safeName));
    const bool target=strcmp(safeName,"rt_reflection.rfx")==0;
    void* result=ptReflectionOriginalRfxLookup(manager,name);
    if(target){
        ptReflectionLookupScope=true;
        ptReflectionLastRfxObject=result;
        ptReflectionTechniqueObject=nullptr;
        const auto call=++ptReflectionRfxLookups;
        if(call<=16 || (call%240)==0)
            Log("PT_REFLECTION_P1_RFX_LOOKUP call=%llu manager=%p name=%s result=%p mode=observe_only replacement=disabled native_result_preserved=1",
                call,manager,safeName,result);
    } else {
        ptReflectionLookupScope=false;
        ptReflectionLastRfxObject=nullptr;
        ptReflectionTechniqueObject=nullptr;
    }
    return result;
}

static void* PTReflectionHookTechniqueLookup(void* rfx,const char* technique) {
    char safeTechnique[128]{};
    RRSafeCopyCString(technique,safeTechnique,sizeof(safeTechnique));
    const bool target=ptReflectionLookupScope &&
        rfx==ptReflectionLastRfxObject &&
        strcmp(safeTechnique,"rt_reflectionDeferredShading")==0;
    void* result=ptReflectionOriginalTechniqueLookup(rfx,technique);
    if(target){
        ptReflectionTechniqueObject=result;
        const auto call=++ptReflectionTechniqueLookups;
        Log("PT_REFLECTION_P1_TECHNIQUE call=%llu rfx=%p technique=%s technique_object=%p mode=observe_only replacement=disabled native_result_preserved=1",
            call,rfx,safeTechnique,result);
    }else{
        ptReflectionTechniqueObject=nullptr;
    }
    ptReflectionLookupScope=false;
    ptReflectionLastRfxObject=nullptr;
    return result;
}

static void* PTReflectionHookVariantLookup(void* technique,unsigned key) {
    void* result=ptReflectionOriginalVariantLookup(technique,key);
    if(technique&&technique==ptReflectionTechniqueObject){
        const auto call=++ptReflectionVariantLookups;
        const bool admitted=PTReflectionInspectRayLibrary(technique,result,key);
        Log("PT_REFLECTION_P1_VARIANT call=%llu technique=%p key=0x%X shader=%p admitted=%u mode=observe_only native_result_preserved=1",
            call,technique,key,result,unsigned(admitted));
        ptReflectionTechniqueObject=nullptr;
    }
    return result;
}

static bool PTReflectionInstallShaderLookupHooks(HMODULE renderer,HMODULE d3d) noexcept {
    if(!renderer||renderer!=verifiedRenderer||!d3d||d3d!=verifiedD3d)return false;
    if(!RRAlbedoShader::Initialize(d3d)){
        Log("PT_REFLECTION_P1_LOOKUP_HOOKS ready=0 reason=resident_shader_getter");
        return false;
    }
    auto* base=reinterpret_cast<unsigned char*>(renderer);
    ptReflectionOriginalRfxLookup=reinterpret_cast<PTReflectionRfxLookupFn>(base+0x1D9960);
    ptReflectionOriginalTechniqueLookup=reinterpret_cast<PTReflectionTechniqueLookupFn>(base+0x1D2CE0);
    ptReflectionOriginalVariantLookup=reinterpret_cast<PTReflectionVariantLookupFn>(base+0x1DD450);

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

    unsigned applied=0;
    for(;applied<3;++applied){
        if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],true))break;
        if(!ptReflectionLookupPatches[applied].writeHealthy){++applied;break;}
    }
    if(applied!=3){
        while(applied){
            --applied;
            if(!RRAlbedoExchangeCall(&ptReflectionLookupPatches[applied],false))
                Log("PT_REFLECTION_P1_HOOK_ROLLBACK_FAILED index=%u",applied);
        }
        return false;
    }
    Log("PT_REFLECTION_P1_LOOKUP_HOOKS ready=1 rfx_callsite=0x12953C technique_callsite=0x12954B variant_callsite=0x12956C rfx_helper=0x1D9960 technique_helper=0x1D2CE0 variant_helper=0x1DD450 native_hashes=32 replacement=disabled gpu_work=unchanged");
    return true;
}
