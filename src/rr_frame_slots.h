#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include "rr_dimensions.h"
namespace control_rr {
// Single-owner policy; caller holds the render/retirement lock. GPU objects are
// owned outside this class, one set per slot. No allocation or CPU wait occurs.
enum class SlotState {Free,Recording,Ready,Submitted,Retained};
struct Lease {std::size_t index=3;std::uint64_t serial=0;};
struct FrameKey {std::uint64_t frame=0,epoch=0;std::uint32_t width=0,height=0;};
struct FenceKey {std::uintptr_t device=0,queue=0,fence=0;};
inline bool SameFence(FenceKey a,FenceKey b) noexcept {return a.device==b.device && a.queue==b.queue && a.fence==b.fence;}
inline bool SameFrame(FrameKey a,FrameKey b) noexcept {return a.frame==b.frame && a.epoch==b.epoch && a.width==b.width && a.height==b.height;}
struct FrameSlot {SlotState state=SlotState::Free;FrameKey key{};std::uint64_t serial=0,fenceValue=0;bool commands=false,evaluationTaken=false;};
class FrameSlots {
    std::array<FrameSlot,3> slots_{};
    FenceKey fence_{};std::uint64_t serial_=0,lastSignal_=0,completed_=0;
    FrameKey latest_{};
    bool lost_=false;
    FrameSlot* Match(Lease l) noexcept {
        if(l.index>=slots_.size() || !l.serial || slots_[l.index].serial!=l.serial) return nullptr;
        return &slots_[l.index];
    }
public:
    bool Bind(FenceKey key) noexcept {
        if(!key.device || !key.queue || !key.fence) return false;
        if(SameFence(fence_,key)) return !lost_;
        for(const auto& s:slots_) if(s.state!=SlotState::Free) return false;
        fence_=key;lastSignal_=completed_=0;latest_={};lost_=false;return true;
    }
    bool Collect(FenceKey key,std::uint64_t completed) noexcept {
        if(!SameFence(fence_,key) || !key.fence || lost_) return false;
        if(completed==UINT64_MAX) {lost_=true;return false;}
        if(completed<completed_ || completed>lastSignal_) return false;
        completed_=completed;
        for(auto& s:slots_) if(s.state==SlotState::Submitted && s.fenceValue<=completed) s.state=SlotState::Free;
        return true;
    }
    bool Begin(FrameKey key,Lease& lease) noexcept {
        lease={};
        if(lost_ || !fence_.fence || !key.frame || !key.epoch || !key.width || !key.height ||
           !RenderExtent(key.width,key.height) || serial_==UINT64_MAX) return false;
        if(key.epoch<latest_.epoch || (key.epoch==latest_.epoch &&
           (key.frame<=latest_.frame || key.width!=latest_.width || key.height!=latest_.height))) return false;
        for(const auto& s:slots_) if(s.state!=SlotState::Free && SameFrame(s.key,key)) return false;
        for(std::size_t i=0;i<slots_.size();++i) if(slots_[i].state==SlotState::Free) {
            slots_[i]={SlotState::Recording,key,++serial_,0,false};lease={i,serial_};latest_=key;return true;
        }
        return false; // All resources busy: original AA must remain the fallback.
    }
    bool CommandsStarted(Lease l) noexcept {
        auto* s=Match(l);if(!s || s->state!=SlotState::Recording || lost_) return false;
        s->commands=true;return true;
    }
    bool GuidesReady(Lease l) noexcept {
        auto* s=Match(l);if(!s || s->state!=SlotState::Recording || !s->commands || lost_) return false;
        s->state=SlotState::Ready;return true;
    }
    bool CanEvaluate(Lease l,FrameKey current) const noexcept {
        if(lost_ || l.index>=slots_.size() || !l.serial || !SameFrame(current,latest_)) return false;
        const auto& s=slots_[l.index];return s.serial==l.serial && s.state==SlotState::Ready && !s.evaluationTaken && SameFrame(s.key,current);
    }
    bool TakeEvaluation(Lease l,FrameKey current) noexcept {
        if(!CanEvaluate(l,current)) return false;
        slots_[l.index].evaluationTaken=true;return true;
    }
    bool Submit(Lease l,std::uint64_t value) noexcept {
        auto* s=Match(l);
        if(!s || !s->commands || (s->state!=SlotState::Recording && s->state!=SlotState::Ready) || lost_) return false;
        if(!value || value==UINT64_MAX || value<=lastSignal_) {s->state=SlotState::Retained;return false;}
        // Caller invokes only after a successful signal on the bound fence.
        s->state=SlotState::Submitted;s->fenceValue=value;lastSignal_=value;return true;
    }
    bool Abort(Lease l) noexcept {
        auto* s=Match(l);if(!s || (s->state!=SlotState::Recording && s->state!=SlotState::Ready)) return false;
        s->state=s->commands?SlotState::Retained:SlotState::Free;return true;
    }
    const std::array<FrameSlot,3>& Inspect() const noexcept {return slots_;}
};
}
