#pragma once
// F9 evidence only. Copy bounded resident bytecode on the native preparation
// thread; a worker owns each published slot and performs all file operations.
static std::atomic<bool> rrHairShaderAuditRequested{false};
namespace RRMaterialAudit {
constexpr unsigned SlotCount = 4, PairLimit = 24, StandardLimit = 16;
constexpr unsigned FrameLimit = 120, ShaderCount = 4;
struct Slot {
    std::atomic<bool> busy{false};
    unsigned long long session = 0, frame = 0;
    unsigned family = 0, source = 0;
    RRAlbedoShader::NativeDescriptor descriptors[ShaderCount]{};
    unsigned char bytes[ShaderCount][RRAlbedoShader::kMaximumBlobBytes]{};
};
static Slot slots[SlotCount];
struct Key {
    std::uintptr_t bases[ShaderCount]{};
    int identifiers[ShaderCount]{};
    unsigned family = 0;
};
// The following state belongs exclusively to native primary preparation.
static Key seen[PairLimit];
static unsigned count = 0, standardCount = 0, framesLeft = 0;
static unsigned long long session = 0;
static bool copiedThisFrame = false;

static DWORD WINAPI Export(void* context) noexcept {
    auto& slot = *static_cast<Slot*>(context);
    bool okay = false;
    try {
        wchar_t env[32768]{};
        const DWORD length = GetEnvironmentVariableW(L"LOCALAPPDATA", env, 32768);
        if (length && length < 32768) {
            const auto root = std::wstring(env, length) + L"\\ControlFGProbe";
            CreateDirectoryW(root.c_str(), nullptr);
            const auto directory = root + L"\\rr-material-shaders-" +
                std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(slot.session);
            if (CreateDirectoryW(directory.c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS) {
                const auto prefix = directory + L"\\family-" + std::to_wstring(slot.family) +
                    L"-batch-" + std::to_wstring(slot.source) + L"-frame-" + std::to_wstring(slot.frame);
                const wchar_t* names[]{L"-original-vs.dxbc", L"-original-ps.dxbc",
                                       L"-candidate-vs.dxbc", L"-candidate-ps.dxbc"};
                okay = true;
                for (unsigned i = 0; i < ShaderCount; ++i) {
                    RRGuideExportDetail::File file(prefix + names[i]);
                    const bool written = file.Write(slot.bytes[i], static_cast<size_t>(slot.descriptors[i].size)) && file.Finish();
                    okay = written && okay;
                }
                // A complete marker is published only after every bytecode file closes.
                if (okay) {
                    RRGuideExportDetail::File marker(prefix + L".complete");
                    const char note[] = "R101 native material evidence; candidate semantics unverified; no replay admission changed.\n";
                    okay = marker.Write(note, sizeof(note) - 1) && marker.Finish();
                }
            }
        }
    } catch (...) { okay = false; }
    Log("RR_MATERIAL_SHADER_AUDIT_R101 session=%llu frame=%llu family=%u source_batch=%u exported=%u",
        slot.session, slot.frame, slot.family, slot.source, unsigned(okay));
    slot.busy.store(false, std::memory_order_release);
    return 0;
}

static void BeginFrame(unsigned long long frame) noexcept {
    if (rrHairShaderAuditRequested.exchange(false, std::memory_order_acq_rel)) {
        session = frame;
        count = standardCount = 0;
        framesLeft = FrameLimit;
    }
    copiedThisFrame = false;
    if (framesLeft) --framesLeft;
}

static bool Wants(unsigned family) noexcept {
    return framesLeft && !copiedThisFrame && count < PairLimit && family >= 1 && family <= 6 &&
        (family == 1 ? standardCount < StandardLimit : count - standardCount < PairLimit - StandardLimit);
}

static void Capture(unsigned long long frame, unsigned family, unsigned source,
                    std::uintptr_t original, std::uintptr_t candidate) noexcept {
    if (!Wants(family)) return;
    const std::uintptr_t records[]{original, original, candidate, candidate};
    const size_t offsets[]{0x10, 0x50, 0x10, 0x50};
    RRAlbedoShader::NativeDescriptor descriptors[ShaderCount]{};
    Key key{}; key.family = family;
    for (unsigned i = 0; i < ShaderCount; ++i) {
        auto& d = descriptors[i];
        if (!RRAlbedoShader::ReadDescriptor(records[i], offsets[i], &d) ||
            d.stage != (i % 2 ? 4u : 0u) || d.identifier < 0 || d.size < 32 ||
            d.size > RRAlbedoShader::kMaximumBlobBytes) return;
        key.bases[i] = d.shaderBase; key.identifiers[i] = d.identifier;
    }
    for (unsigned n = 0; n < count; ++n) {
        bool same = seen[n].family == family;
        for (unsigned i = 0; i < ShaderCount; ++i)
            same = same && seen[n].bases[i] == key.bases[i] && seen[n].identifiers[i] == key.identifiers[i];
        if (same) return;
    }
    Slot* slot = nullptr;
    for (auto& entry : slots) {
        bool free = false;
        if (entry.busy.compare_exchange_strong(free, true, std::memory_order_acquire)) { slot = &entry; break; }
    }
    if (!slot) return; // Never wait for disk or an exporter.
    copiedThisFrame = true; // At most one copy attempt (1 MiB) per preparation frame.
    for (unsigned i = 0; i < ShaderCount; ++i) {
        RRAlbedoShader::NativeDescriptor after{};
        const auto& d = descriptors[i];
        if (!RRAlbedoShader::CopyResident(&d, slot->bytes[i], static_cast<size_t>(d.size)) ||
            !RRAlbedoShader::ReadDescriptor(records[i], offsets[i], &after) ||
            d.shaderBase != after.shaderBase || d.identifier != after.identifier ||
            d.size != after.size || d.stage != after.stage) {
            slot->busy.store(false, std::memory_order_release); return;
        }
        slot->descriptors[i] = d;
    }
    slot->session = session; slot->frame = frame; slot->family = family; slot->source = source;
    if (!QueueUserWorkItem(&Export, slot, WT_EXECUTEDEFAULT)) {
        slot->busy.store(false, std::memory_order_release); return;
    }
    seen[count++] = key;
    if (family == 1) ++standardCount;
}
} // namespace RRMaterialAudit
