#pragma once
#include <array>
#include <cstdint>

namespace gi_reuse {
enum class RootArgumentKind { None, Table, CBV, SRV, UAV, Constants };
// Per-recording state, with no allocations in setters. The observer must cover
// every setter and invalidate after unmodelled bundle/indirect state changes.
class ComputeRootArguments {
public:
    struct Argument {
        RootArgumentKind kind = RootArgumentKind::None;
        std::uint64_t address = 0, knownConstants = 0;
        std::array<std::uint32_t,64> constants{};
    };
private:
    std::array<Argument,64> arguments_{};
    std::uint64_t root_ = 0;
    bool healthy_ = true;
    bool Accept(unsigned index, RootArgumentKind kind) noexcept {
        if (!healthy_) return false;
        if (!root_ || index >= arguments_.size()) { Invalidate(); return false; }
        auto& a = arguments_[index];
        if (a.kind != RootArgumentKind::None && a.kind != kind) { Invalidate(); return false; }
        a.kind = kind;
        return true;
    }
public:
    void Reset() noexcept { arguments_ = {}; root_ = 0; healthy_ = true; }
    void Invalidate() noexcept { healthy_ = false; }
    bool Ready() const noexcept { return healthy_ && root_ != 0; }
    void Root(std::uint64_t identity) noexcept {
        if (identity != root_) { arguments_ = {}; root_ = identity; }
    }
    void Descriptor(unsigned index, RootArgumentKind kind, std::uint64_t address) noexcept {
        if (kind == RootArgumentKind::None || kind == RootArgumentKind::Constants) { Invalidate(); return; }
        if (Accept(index,kind)) arguments_[index].address = address;
    }
    void Constants(unsigned index, unsigned offset, unsigned count, const std::uint32_t* values) noexcept {
        if (!count) return;
        if (!values || offset >= 64 || count > 64-offset) { Invalidate(); return; }
        if (!Accept(index,RootArgumentKind::Constants)) return;
        auto& a = arguments_[index];
        for (unsigned i=0; i<count; ++i) {
            a.constants[offset+i] = values[i];
            a.knownConstants |= std::uint64_t(1) << (offset+i);
        }
    }
    void HeapChange() noexcept {
        for (auto& a : arguments_) if (a.kind == RootArgumentKind::Table) a = {};
    }
    // Restore only arguments that were defined. Unknown constants stay unknown;
    // never fill holes with zero or replay stale arguments after a root switch.
    template<class Sink> bool Replay(Sink& sink) const noexcept {
        if (!Ready()) return false;
        for (unsigned index=0; index<arguments_.size(); ++index) {
            const auto& a = arguments_[index];
            if (a.kind == RootArgumentKind::Constants) {
                unsigned start=0;
                while (start<64) {
                    if (!(a.knownConstants & (std::uint64_t(1)<<start))) { ++start; continue; }
                    unsigned end=start+1;
                    while (end<64 && (a.knownConstants & (std::uint64_t(1)<<end))) ++end;
                    sink.Constants(index,start,end-start,a.constants.data()+start);
                    start=end;
                }
            } else if (a.kind != RootArgumentKind::None) sink.Descriptor(index,a.kind,a.address);
        }
        return true;
    }
};
}
