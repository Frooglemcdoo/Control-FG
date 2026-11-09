#pragma once
// Installed only during the existing hash-verified renderer initialization.
// Two exact E8 instructions and two native callback slots; no prologue copies.
struct RRAlbedoCallPatch {
    unsigned char* site;
    unsigned char original[5];
    unsigned char replacement[5];
    void* relay;
    bool writeHealthy;
};
static RRAlbedoCallPatch rrAlbedoCallPatches[2]{};
static bool RRAlbedoPrepareCallPatch(unsigned char* site, void* original, void* hook,
    RRAlbedoCallPatch* patch) noexcept {
    if (!site || !original || !hook || !patch || site[0] != 0xE8) return false;
    int displacement = 0; memcpy(&displacement, site + 1, 4);
    if (site + 5 + displacement != original) return false;
    patch->site = site; memcpy(patch->original, site, 5);
    patch->relay = AllocateExecutableRelayNear(site);
    if (!patch->relay) return false;
    unsigned char stub[14]{0xFF, 0x25, 0, 0, 0, 0};
    memcpy(stub + 6, &hook, 8); memcpy(patch->relay, stub, sizeof(stub));
    DWORD previous = 0;
    if (!VirtualProtect(patch->relay, 4096, PAGE_EXECUTE_READ, &previous) ||
        !FlushInstructionCache(GetCurrentProcess(), patch->relay, sizeof(stub)) ||
        !Rel32Fits(site + 5, patch->relay)) return false;
    patch->replacement[0] = 0xE8;
    const auto delta = static_cast<int>(reinterpret_cast<std::intptr_t>(patch->relay) -
        reinterpret_cast<std::intptr_t>(site + 5));
    memcpy(patch->replacement + 1, &delta, 4);
    return true;
}
static bool RRAlbedoExchangeCall(RRAlbedoCallPatch* patch, bool install) noexcept {
    const auto* expected = install ? patch->original : patch->replacement;
    const auto* next = install ? patch->replacement : patch->original;
    if (memcmp(patch->site, expected, 5)) return false;
    DWORD previous = 0;
    if (!VirtualProtect(patch->site, 5, PAGE_EXECUTE_READWRITE, &previous)) return false;
    // Initialization runs before these native draw callbacks are armed. As for
    // the existing native call hooks, this is not a live hot-patch API.
    memcpy(patch->site, next, 5);
    const bool flushed = FlushInstructionCache(GetCurrentProcess(), patch->site, 5) != FALSE;
    DWORD ignored = 0;
    const bool protectedAgain = VirtualProtect(patch->site, 5, previous, &ignored) != FALSE;
    patch->writeHealthy = flushed && protectedAgain;
    if (!flushed || !protectedAgain) {
        Log("RR_GUIDE_G3_HOOK_MEMORY_WARNING installed=%u flush=%u protection=%u error=%lu",
            unsigned(install), unsigned(flushed), unsigned(protectedAgain), GetLastError());
    }
    // The bytes did change even if protection restoration failed. The caller
    // must count this patch as applied and preserve its relay for any rollback.
    return true;
}
static bool RRAlbedoInstallHooks(HMODULE renderer, HMODULE d3d) noexcept {
    if (renderer != verifiedRenderer || d3d != verifiedD3d || !renderer || !d3d) return false;
    auto base = reinterpret_cast<unsigned char*>(renderer);
    rrAlbedoOriginalPrepare = reinterpret_cast<RRAlbedoCallback>(base + 0x13E4D0);
    rrAlbedoOriginalJoin = reinterpret_cast<RRAlbedoCallback>(base + 0x13EBB0);
    rrAlbedoOriginalDraw = reinterpret_cast<control_rr_albedo::DrawOpaque>(base + 0x1C1AE0);
    rrAlbedoLookupShader = reinterpret_cast<RRAlbedoLookup>(base + 0x1DD3F0);
    if (!RRAlbedoNativeInitialize(d3d, &rrAlbedoAPI) || !RRAlbedoShader::Initialize(d3d)) return false;
    Patch slots[2]{
        {reinterpret_cast<void**>(base + 0x632008), reinterpret_cast<void*>(rrAlbedoOriginalPrepare),
            reinterpret_cast<void*>(&RRAlbedoHookPrepare), "G3_PRIMARY_PREPARE"},
        {reinterpret_cast<void**>(base + 0x631C18), reinterpret_cast<void*>(rrAlbedoOriginalJoin),
            reinterpret_cast<void*>(&RRAlbedoHookJoin), "G3_PRIMARY_JOIN"}
    };
    for (const auto& slot : slots) if (*slot.slot != slot.original) return false;
    const size_t rvas[2]{0x12E29E, 0x126591};
    for (unsigned int i = 0; i < 2; ++i) {
        if (!RRAlbedoPrepareCallPatch(base + rvas[i], reinterpret_cast<void*>(rrAlbedoOriginalDraw),
            reinterpret_cast<void*>(&RRAlbedoHookDraw), &rrAlbedoCallPatches[i])) return false;
    }
    unsigned int callsApplied = 0, slotsApplied = 0;
    bool callsHealthy = true;
    for (; callsApplied < 2;) {
        if (!RRAlbedoExchangeCall(&rrAlbedoCallPatches[callsApplied], true)) { callsHealthy = false; break; }
        const bool healthy = rrAlbedoCallPatches[callsApplied].writeHealthy;
        ++callsApplied;
        if (!healthy) { callsHealthy = false; break; }
    }
    if (callsApplied == 2 && callsHealthy) {
        for (; slotsApplied < 2; ++slotsApplied) if (!Exchange(slots[slotsApplied], true)) break;
    }
    if (!callsHealthy || callsApplied != 2 || slotsApplied != 2) {
        while (slotsApplied) {
            --slotsApplied;
            if (!Exchange(slots[slotsApplied], false)) Log("RR_GUIDE_G3_HOOK_ROLLBACK_FAILED label=%s", slots[slotsApplied].label);
        }
        while (callsApplied) {
            --callsApplied;
            if (!RRAlbedoExchangeCall(&rrAlbedoCallPatches[callsApplied], false))
                Log("RR_GUIDE_G3_HOOK_ROLLBACK_FAILED call=%u", callsApplied);
        }
        return false;
    }
    rrAlbedoStage.store(RRAlbedoStage::Waiting, std::memory_order_release);
    Log("RR_GUIDE_G3_ALBEDO_HOOKS_READY prepare=0x632008 join=0x631C18 serial=0x12E29E worker=0x126591 captures=1 rr_eval=disabled");
    return true;
}
