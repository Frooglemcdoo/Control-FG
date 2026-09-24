#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>

// r21x: native Control specular-energy clamp.
//
// Control's original 0x600347E7 temporal pre-accumulator performs two jobs:
//   (1) a current-frame mip-based firefly/energy clamp, and
//   (2) temporal history accumulation.
// The exact native shader multiplies temporal confidence by
//     (1 - g_uCameraCut) * 0.6
// so forcing the already-bound g_uCameraCut provider to 1 only across the
// temporal technique bind+dispatch preserves (1) while making (2) contribute
// zero. The provider is restored immediately after that one dispatch.
//
// No D3D12 device vtable, PSO clone, ReShade or loose DLSS file is involved.
namespace control_rr_native_clamp {

inline constexpr std::uintptr_t ProviderResolverRva = 0x210270;
inline constexpr std::uintptr_t ProviderResolverProofCallRva = 0x17e2e9; // g_mClipToView wrapper
inline constexpr std::uintptr_t ProviderMetadataRva = 0x112840;
inline constexpr std::uintptr_t ProviderTlsIndexRva = 0x1115fc;
inline constexpr std::uintptr_t ProviderArenaOffset = 0x4250;
inline constexpr std::size_t ProviderArenaBytes = 0x40000;
inline constexpr std::uintptr_t SetProviderIatRva = 0x5dff90;
inline constexpr std::uintptr_t CompareProviderRva = 0x16a40;
inline constexpr char CameraCutName[] = "g_uCameraCut";
inline constexpr char SetProviderName[] = "?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z";
inline constexpr char CompareProviderName[] = "?cmpProviderData@ConstantValueProviderRegistry@d3d@@SA_NHPEBXPEAH@Z";

struct Entry { std::int32_t offset, reserved, size, revisionIndex; };
static_assert(sizeof(Entry) == 16, "native provider entry stride");

struct Lease {
    std::int32_t provider = -1;
    std::uint32_t original = 0;
    bool armed = false;
    bool bindReturned = false;
    bool dispatched = false;
    bool restored = false;
    const char* reason = "not_attempted";
};

struct Api {
    using ResolveFn = void (*)(void*, const char*);
    using SetFn = void (*)(int, const void*, const char*);
    using CompareFn = bool (*)(int, const void*, int*);
    unsigned char* renderer = nullptr;
    unsigned char* d3d = nullptr;
    ResolveFn resolve = nullptr;
    SetFn set = nullptr;
    CompareFn compare = nullptr;
    bool ready = false;

    bool Initialize(HMODULE rendererModule, HMODULE d3dModule) noexcept {
        renderer = reinterpret_cast<unsigned char*>(rendererModule);
        d3d = reinterpret_cast<unsigned char*>(d3dModule);
        resolve = nullptr; set = nullptr; compare = nullptr; ready = false;
        if (!renderer || !d3d || rendererModule != verifiedRenderer || d3dModule != verifiedD3d) return false;
        __try {
            // Prove the internal name->provider resolver target from the exact
            // hash-locked renderer using an already audited g_mClipToView wrapper.
            auto* proof = renderer + ProviderResolverProofCallRva;
            if (proof[0] != 0xE8) return false;
            std::int32_t rel = 0; std::memcpy(&rel, proof + 1, sizeof(rel));
            if (proof + 5 + rel != renderer + ProviderResolverRva) return false;
            resolve = reinterpret_cast<ResolveFn>(renderer + ProviderResolverRva);

            set = reinterpret_cast<SetFn>(GetProcAddress(d3dModule, SetProviderName));
            compare = reinterpret_cast<CompareFn>(GetProcAddress(d3dModule, CompareProviderName));
            if (!set || !compare || reinterpret_cast<unsigned char*>(compare) != d3d + CompareProviderRva) return false;
            // The renderer import slot is an independent identity check for setProviderData.
            if (*reinterpret_cast<void**>(renderer + SetProviderIatRva) != reinterpret_cast<void*>(set)) return false;
            ready = true; return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            resolve = nullptr; set = nullptr; compare = nullptr; ready = false; return false;
        }
    }

    bool EntryFor(std::int32_t id, Entry& out) const noexcept {
        out = {};
        if (!ready || id < 0 || id >= 4096) return false;
        __try {
            std::memcpy(&out, d3d + ProviderMetadataRva + std::uintptr_t(id) * sizeof(Entry), sizeof(out));
            return out.offset >= 0 && out.size == 4 &&
                static_cast<std::size_t>(out.offset) <= ProviderArenaBytes - 4;
        } __except (EXCEPTION_EXECUTE_HANDLER) { out = {}; return false; }
    }

    bool ReadCurrent(std::int32_t id, std::uint32_t& value, Entry* metadata = nullptr) const noexcept {
        value = 0; Entry before{}, after{};
        if (!EntryFor(id, before)) return false;
        __try {
            const auto tlsIndex = *reinterpret_cast<const std::uint32_t*>(d3d + ProviderTlsIndexRva);
            if (tlsIndex >= 4096) return false;
            auto** tlsArray = reinterpret_cast<unsigned char**>(__readgsqword(0x58));
            if (!tlsArray || !tlsArray[tlsIndex]) return false;
            std::memcpy(&value, tlsArray[tlsIndex] + ProviderArenaOffset + std::uintptr_t(before.offset), sizeof(value));
            int difference = 1;
            if (!compare(id, &value, &difference) || difference != 0 || !EntryFor(id, after) ||
                std::memcmp(&before, &after, sizeof(before)) != 0) {
                value = 0; return false;
            }
            if (metadata) *metadata = before;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) { value = 0; return false; }
    }

    bool ResolveCameraCut(std::int32_t& id) const noexcept {
        id = -1;
        if (!ready || !resolve) return false;
        struct Slot { std::int32_t id; std::int32_t callerTag; } slot{-1, 0};
        __try { resolve(&slot, CameraCutName); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        Entry meta{}; std::uint32_t current = 0;
        if (slot.id < 0 || slot.id >= 4096 || !EntryFor(slot.id, meta) || !ReadCurrent(slot.id, current)) return false;
        // The shader declares this provider as uint. Normal engine values are 0/1.
        if (current > 1u) return false;
        id = slot.id; return true;
    }

    bool SetVerified(std::int32_t id, std::uint32_t value) const noexcept {
        Entry before{}, after{}; std::uint32_t observed = 0;
        if (!ready || !set || !EntryFor(id, before)) return false;
        __try { set(id, &value, nullptr); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
        if (!ReadCurrent(id, observed, &after) || observed != value || std::memcmp(&before, &after, sizeof(before)) != 0) return false;
        return true;
    }

    bool Arm(Lease& lease) const noexcept {
        lease = {}; lease.reason = "camera_cut_resolve";
        std::int32_t id = -1;
        if (!ResolveCameraCut(id)) return false;
        lease.reason = "camera_cut_snapshot";
        std::uint32_t original = 0;
        if (!ReadCurrent(id, original) || original > 1u) return false;
        lease.provider = id; lease.original = original;
        lease.reason = "camera_cut_force";
        if (!SetVerified(id, 1u)) {
            // setProviderData may have completed even if the subsequent metadata/
            // compare verification failed. Never leave a possibly modified TLS
            // provider behind on a rejected arm: best-effort restore the exact
            // snapshot before returning failure.
            (void)SetVerified(id, original);
            lease.provider = -1;
            lease.reason = "camera_cut_force_or_verify_failed_restored";
            return false;
        }
        lease.armed = true; lease.reason = "camera_cut_forced"; return true;
    }

    bool StillForced(Lease& lease) const noexcept {
        lease.reason = "camera_cut_verify_before_dispatch";
        if (!lease.armed || lease.provider < 0) return false;
        std::uint32_t current = 0;
        if (!ReadCurrent(lease.provider, current) || current != 1u) return false;
        lease.reason = "camera_cut_ready_for_dispatch"; return true;
    }

    bool Restore(Lease& lease) const noexcept {
        if (!lease.armed) { lease.restored = true; return true; }
        lease.reason = "camera_cut_restore";
        // A verification failure does not prove the native set failed. Retrying
        // the idempotent original value is safer than carrying CameraCut=1 into
        // later renderer work. Leave `armed` set if all verification attempts
        // fail so the outer filter finally can make one more restoration pass.
        for (unsigned attempt = 0; attempt != 3; ++attempt) {
            if (SetVerified(lease.provider, lease.original)) {
                lease.armed = false; lease.restored = true; lease.reason = "camera_cut_restored"; return true;
            }
        }
        lease.reason = "camera_cut_restore_failed";
        return false;
    }
};

} // namespace control_rr_native_clamp
