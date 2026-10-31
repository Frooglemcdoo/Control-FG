#pragma once
// Startup-only direct call patch helper.
struct NativeCpuCallPatch {
    unsigned char* site;
    unsigned char original[5];
    unsigned char replacement[5];
    void* relay;
    bool writeHealthy;
};
static bool NativeCpuPrepareCallPatch(unsigned char* site, void* original, void* hook,
    NativeCpuCallPatch* patch, unsigned char opcode=0xE8) noexcept {
    if (!site || !original || !hook || !patch || (opcode!=0xE8&&opcode!=0xE9) || site[0] != opcode) return false;
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
    patch->replacement[0] = opcode;
    const auto delta = static_cast<int>(reinterpret_cast<std::intptr_t>(patch->relay) -
        reinterpret_cast<std::intptr_t>(site + 5));
    memcpy(patch->replacement + 1, &delta, 4);
    return true;
}
static bool NativeCpuExchangeCall(NativeCpuCallPatch* patch, bool install) noexcept {
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
        Log("NATIVE_CPU_R218_MEMORY_WARNING installed=%u flush=%u protection=%u error=%lu",
            unsigned(install), unsigned(flushed), unsigned(protectedAgain), GetLastError());
    }
    // The bytes did change even if protection restoration failed. The caller
    // must count this patch as applied and preserve its relay for any rollback.
    return true;
}
