#pragma once
#include "pt_glass_capture_policy.h"

// Observe bytes already requested by the game through the existing storage-get
// hooks. No new hooks, root signatures, pipeline replacements, or GPU commands.
static SRWLOCK ptGlassCaptureLock = SRWLOCK_INIT;
static int ptGlassCaptureSeen[4096]{};
static unsigned ptGlassCaptureSeenCount = 0;
static char ptGlassCaptureHashes[24][65]{};
static unsigned ptGlassCaptureCount = 0;
static size_t ptGlassCaptureBytes = 0;

static bool PTGlassResidentIsCandidate(const char* resident) noexcept {
    __try {
        const auto* b = reinterpret_cast<const unsigned char*>(resident);
        if (!b || memcmp(b, "DXBC", 4)) return false;
        const auto size = PTGlassCapture::Word(b + 24);
        if (size < 32 || size > PTGlassCapture::kMaximumBytes) return false;
        return PTGlassCapture::IsGlassLibrary(b,size)||PTGlassCapture::IsHitLighting(b,size);
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    return false;
}

static void PTGlassCaptureResident(int id, const char* resident) noexcept {
    if (id < 0 || !PTGlassResidentIsCandidate(resident)) return;
    bool inspect = true;
    AcquireSRWLockExclusive(&ptGlassCaptureLock);
    for (unsigned i = 0; i < ptGlassCaptureSeenCount; ++i)
        if (ptGlassCaptureSeen[i] == id) { inspect = false; break; }
    if (ptGlassCaptureSeenCount >= _countof(ptGlassCaptureSeen) ||
        ptGlassCaptureCount >= _countof(ptGlassCaptureHashes)) inspect = false;
    if (inspect) ptGlassCaptureSeen[ptGlassCaptureSeenCount++] = id;
    ReleaseSRWLockExclusive(&ptGlassCaptureLock);
    if (!inspect) return;

    unsigned char* copy = nullptr;
    size_t size = 0;
    bool copied = false;
    __try {
        if (memcmp(resident, "DXBC", 4) == 0) {
            size = PTGlassCapture::Word(reinterpret_cast<const unsigned char*>(resident) + 24);
            if (size >= 32 && size <= PTGlassCapture::kMaximumBytes) {
                copy = static_cast<unsigned char*>(HeapAlloc(GetProcessHeap(), 0, size));
                if (copy) { memcpy(copy, resident, size); copied = true; }
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { copied = false; }
    if (!copied || !(PTGlassCapture::IsGlassLibrary(copy, size)||PTGlassCapture::IsHitLighting(copy,size))) {
        if (copy) HeapFree(GetProcessHeap(), 0, copy);
        return;
    }
    char sha[65]{};
    bool capture = PTReflectionSha256(copy, size, sha);
    unsigned index = 0;
    AcquireSRWLockExclusive(&ptGlassCaptureLock);
    for (unsigned i = 0; i < ptGlassCaptureCount; ++i)
        if (!strcmp(sha, ptGlassCaptureHashes[i])) capture = false;
    if (ptGlassCaptureCount >= _countof(ptGlassCaptureHashes) ||
        size > 4u * 1024u * 1024u - ptGlassCaptureBytes) capture = false;
    if (capture) {
        index = ptGlassCaptureCount++;
        strcpy_s(ptGlassCaptureHashes[index], sha);
        ptGlassCaptureBytes += size;
    }
    ReleaseSRWLockExclusive(&ptGlassCaptureLock);
    if (capture) {
        constexpr size_t chunkBytes = 384;
        Log("PT_GLASS_DXIL_BEGIN index=%u id=%d bytes=%llu sha256=%s chunks=%llu native_passthrough=1",
            index, id, static_cast<unsigned long long>(size), sha,
            static_cast<unsigned long long>((size + chunkBytes - 1) / chunkBytes));
        Log("PT_GLASS_SHADER_KIND index=%u kind=%s",index,
            PTGlassCapture::IsGlassLibrary(copy,size)?"glass_ray_library":"hit_lighting_candidate");
        for (size_t offset = 0; offset < size; offset += chunkBytes) {
            const size_t count = (std::min)(chunkBytes, size - offset);
            char hex[chunkBytes * 2 + 1]{};
            for (size_t i = 0; i < count; ++i)
                sprintf_s(hex + i * 2, 3, "%02X", copy[offset + i]);
            Log("PT_GLASS_DXIL_CHUNK index=%u offset=%llu bytes=%llu hex=%s",
                index, static_cast<unsigned long long>(offset),
                static_cast<unsigned long long>(count), hex);
        }
        Log("PT_GLASS_DXIL_END index=%u bytes=%llu sha256=%s", index,
            static_cast<unsigned long long>(size), sha);
    }
    HeapFree(GetProcessHeap(), 0, copy);
}
