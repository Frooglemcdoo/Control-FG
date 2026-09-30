#pragma once
// PT reflections P1: reflection-library identification and scoped ShaderCodeStorage
// interception only. No replacement bytes are returned in this milestone.
//
// The hook is limited to renderer+0x129577, the verified call immediately after
// selecting rt_reflectionDeferredShading and immediately before
// reflectionRayGeneration is registered. All unrelated shader binds pass through.

static constexpr size_t kPTReflectionBindCallRva = 0x129577;
static constexpr size_t kPTReflectionBindFunctionRva = 0x1DB810;
static constexpr size_t kPTReflectionTechniquePointerRva = 0x128D900;
static constexpr size_t kPTShaderStride = 0xA8;
static constexpr char kPTShaderCodeGetSymbol[] = "?get@ShaderCodeStorage@d3d@@YAPEBDH@Z";

using PTReflectionBindFn = void (*)(void*, void*);
using PTShaderCodeGetFn = const char* (__cdecl*)(int);

static PTReflectionBindFn ptReflectionOriginalBind = nullptr;
static PTShaderCodeGetFn ptReflectionOriginalShaderGet = nullptr;
static RRAlbedoCallPatch ptReflectionBindCallPatch{};
static Patch ptReflectionShaderGetPatch{};
static thread_local bool ptReflectionBindActive = false;
static thread_local int ptReflectionTargetIdentifier = -1;
static std::atomic<unsigned long long> ptReflectionP1BindCalls{0};
static std::atomic<unsigned long long> ptReflectionP1GetterCalls{0};
static std::atomic<unsigned long long> ptReflectionP1Matches{0};

struct PTReflectionDescriptor {
    std::uintptr_t base = 0;
    unsigned stage = 0;
    int identifier = -1;
    std::uint64_t size = 0;
    size_t slot = 0;
};

static bool PTReflectionReadDescriptor(void* shader, size_t slot, PTReflectionDescriptor* out) noexcept {
    if (!shader || !out || slot > 0xA0) return false;
    __try {
        const auto address = reinterpret_cast<std::uintptr_t>(shader);
        const auto base = *reinterpret_cast<const std::uintptr_t*>(address + slot);
        if (base <= 0xFFFFu || base > UINTPTR_MAX - 24u) return false;
        PTReflectionDescriptor d{};
        d.base = base;
        d.stage = *reinterpret_cast<const unsigned*>(base + 8);
        d.identifier = *reinterpret_cast<const int*>(base + 12);
        d.size = *reinterpret_cast<const std::uint64_t*>(base + 16);
        d.slot = slot;
        if (d.identifier < 0 || d.stage > 32u || d.size < 32u || d.size > 4ull * 1024ull * 1024ull) return false;
        *out = d;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static bool PTReflectionContains(const char* bytes, size_t size, const char* needle) noexcept {
    if (!bytes || !needle || !*needle) return false;
    const size_t n = strlen(needle);
    if (n > size) return false;
    __try {
        for (size_t i = 0; i + n <= size; ++i)
            if (bytes[i] == needle[0] && memcmp(bytes + i, needle, n) == 0) return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    return false;
}

static bool PTReflectionDescribeVariant(void* shader, unsigned* indexOut, unsigned* keyOut,
    PTReflectionDescriptor* descriptorOut) noexcept {
    if (indexOut) *indexOut = ~0u;
    if (keyOut) *keyOut = 0;
    if (descriptorOut) *descriptorOut = {};
    if (!shader || !verifiedRenderer || !ptReflectionOriginalShaderGet) return false;

    unsigned key = 0, index = ~0u;
    __try {
        key = *reinterpret_cast<const unsigned*>(static_cast<unsigned char*>(shader) + 4);
        auto* technique = *reinterpret_cast<unsigned char**>(
            reinterpret_cast<unsigned char*>(verifiedRenderer) + kPTReflectionTechniquePointerRva);
        if (technique) {
            auto* table = *reinterpret_cast<unsigned char**>(technique + 0x238);
            const unsigned count = *reinterpret_cast<const unsigned*>(technique + 0x240);
            auto* selected = static_cast<unsigned char*>(shader);
            if (table && count && count <= 256 && selected >= table &&
                selected < table + size_t(count) * kPTShaderStride) {
                const size_t delta = size_t(selected - table);
                if ((delta % kPTShaderStride) == 0) index = static_cast<unsigned>(delta / kPTShaderStride);
            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    PTReflectionDescriptor found{};
    bool matched = false;
    for (size_t slot = 0x10; slot <= 0xA0; slot += sizeof(void*)) {
        PTReflectionDescriptor d{};
        if (!PTReflectionReadDescriptor(shader, slot, &d)) continue;
        const char* bytes = nullptr;
        __try { bytes = ptReflectionOriginalShaderGet(d.identifier); }
        __except(EXCEPTION_EXECUTE_HANDLER) { bytes = nullptr; }
        if (!bytes) continue;
        bool dxbc = false;
        __try { dxbc = d.size >= 4 && memcmp(bytes, "DXBC", 4) == 0; }
        __except(EXCEPTION_EXECUTE_HANDLER) { dxbc = false; }
        if (!dxbc) continue;
        if (!PTReflectionContains(bytes, static_cast<size_t>(d.size), "reflectionRayGeneration")) continue;
        if (!PTReflectionContains(bytes, static_cast<size_t>(d.size), "reflectionClosestHit")) continue;
        if (!PTReflectionContains(bytes, static_cast<size_t>(d.size), "reflectionMiss")) continue;
        if (matched) {
            Log("PT_REFLECTION_P1_REJECT reason=ambiguous_library shader=%p key=0x%08X first_slot=0x%zX second_slot=0x%zX",
                shader,key,found.slot,d.slot);
            return false;
        }
        found = d;
        matched = true;
    }
    if (!matched) return false;
    if (indexOut) *indexOut = index;
    if (keyOut) *keyOut = key;
    if (descriptorOut) *descriptorOut = found;
    return true;
}

static const char* __cdecl PTReflectionHookShaderGet(int identifier) {
    const char* bytes = ptReflectionOriginalShaderGet(identifier);
    if (ptReflectionBindActive && identifier == ptReflectionTargetIdentifier) {
        const auto call = ++ptReflectionP1GetterCalls;
        if (call <= 16 || (call % 240) == 0) {
            Log("PT_REFLECTION_P1_GETTER call=%llu identifier=%d bytes=%p mode=identity_passthrough replacement=0",
                call,identifier,bytes);
        }
    }
    return bytes;
}

static void PTReflectionHookBind(void* shader, void* context) {
    const auto call = ++ptReflectionP1BindCalls;
    unsigned index = ~0u, key = 0;
    PTReflectionDescriptor descriptor{};
    const bool matched = PTReflectionDescribeVariant(shader,&index,&key,&descriptor);
    if (matched) {
        ++ptReflectionP1Matches;
        ptReflectionTargetIdentifier = descriptor.identifier;
        ptReflectionBindActive = true;
        if (call <= 32 || (call % 240) == 0) {
            Log("PT_REFLECTION_P1_LIBRARY bind=%llu shader=%p context=%p key=0x%08X variant_index=%u descriptor_slot=0x%zX descriptor=%p stage=%u identifier=%d size=%llu mode=identity_passthrough exports=raygen_closesthit_miss",
                call,shader,context,key,index,descriptor.slot,reinterpret_cast<void*>(descriptor.base),
                descriptor.stage,descriptor.identifier,static_cast<unsigned long long>(descriptor.size));
        }
    } else if (call <= 16 || (call % 240) == 0) {
        Log("PT_REFLECTION_P1_LIBRARY bind=%llu shader=%p context=%p matched=0 fallback=native",
            call,shader,context);
    }

    ptReflectionOriginalBind(shader,context);
    ptReflectionBindActive = false;
    ptReflectionTargetIdentifier = -1;
}

static bool PTReflectionProbeInstall(HMODULE renderer, HMODULE d3d) noexcept {
    if (!renderer || renderer != verifiedRenderer || !d3d || d3d != verifiedD3d) return false;
    auto* base = reinterpret_cast<unsigned char*>(renderer);
    ptReflectionOriginalBind = reinterpret_cast<PTReflectionBindFn>(base + kPTReflectionBindFunctionRva);
    ptReflectionOriginalShaderGet = reinterpret_cast<PTShaderCodeGetFn>(GetProcAddress(d3d,kPTShaderCodeGetSymbol));
    auto** getterSlot = FindImport(renderer,"d3d_rmdwin10_f.dll",kPTShaderCodeGetSymbol);
    if (!ptReflectionOriginalBind || !ptReflectionOriginalShaderGet || !getterSlot ||
        *getterSlot != reinterpret_cast<void*>(ptReflectionOriginalShaderGet)) {
        Log("PT_REFLECTION_P1_INSTALL ready=0 reason=target_resolution bind=%p getter=%p slot=%p slot_value=%p",
            reinterpret_cast<void*>(ptReflectionOriginalBind),reinterpret_cast<void*>(ptReflectionOriginalShaderGet),
            getterSlot,getterSlot?*getterSlot:nullptr);
        return false;
    }

    ptReflectionShaderGetPatch = {
        getterSlot,
        reinterpret_cast<void*>(ptReflectionOriginalShaderGet),
        reinterpret_cast<void*>(&PTReflectionHookShaderGet),
        "PT_REFLECTION_SHADER_CODE_GET"
    };
    if (!RRAlbedoPrepareCallPatch(base + kPTReflectionBindCallRva,
        reinterpret_cast<void*>(ptReflectionOriginalBind),
        reinterpret_cast<void*>(&PTReflectionHookBind),&ptReflectionBindCallPatch)) {
        Log("PT_REFLECTION_P1_INSTALL ready=0 reason=bind_call_validation");
        return false;
    }

    if (!Exchange(ptReflectionShaderGetPatch,true)) {
        Log("PT_REFLECTION_P1_INSTALL ready=0 reason=getter_iat_exchange");
        return false;
    }
    if (!RRAlbedoExchangeCall(&ptReflectionBindCallPatch,true) || !ptReflectionBindCallPatch.writeHealthy) {
        if (!Exchange(ptReflectionShaderGetPatch,false))
            Log("PT_REFLECTION_P1_ROLLBACK_FAILED target=getter");
        Log("PT_REFLECTION_P1_INSTALL ready=0 reason=bind_call_exchange");
        return false;
    }

    Log("PT_REFLECTION_P1_INSTALL ready=1 bind_call_rva=0x%zX bind_target_rva=0x%zX technique_rva=0x%zX shader_stride=0x%zX mode=identity_passthrough replacement=0",
        kPTReflectionBindCallRva,kPTReflectionBindFunctionRva,kPTReflectionTechniquePointerRva,kPTShaderStride);
    return true;
}
