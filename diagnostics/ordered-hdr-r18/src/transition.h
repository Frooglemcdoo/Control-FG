#pragma once
#include <cstdint>
namespace hdrguard {
enum class Phase { Idle, Stopping, Drain, ReplayReady, AwaitDisplay, Warming, WarmFence, ResumeReady, Confirming, Failed };
struct Frame {
    uint64_t now{}, present{}, generation{}, settled{}, frees{}, generated{};
    bool displayValid{}, hdr{}, bridgeHdr{}, foreground{}, presentOK{}, coreOff{}, allInputs{}, coreResetIdle{};
    bool stopFenceDone{}, warmFenceDone{};
    uint32_t selection{}, colorWidth{}, colorHeight{}, colorFormat{}, hudlessFormat{};
};
struct Transition {
    Phase phase{Phase::Idle};
    uint64_t begun{}, changedAt{}, stableAt{}, lastWarmPresent{}, baseFree{}, baseGenerated{}, stopPresent{}, lastOffPresent{};
    uint64_t fingerprintGen{};
    uint32_t fingerprintWidth{}, fingerprintHeight{}, fingerprintFormat{}, goodFrames{}, offPresents{};
    bool oldHdr{}, targetHdr{}, stopCommitted{}, replayed{};
    const char* error{"none"};
    bool active() const { return phase != Phase::Idle; }
    bool hold() const { return phase != Phase::Idle && phase != Phase::Confirming; }
    void fail(const char* why) { phase=Phase::Failed; error=why; }
    bool start(const Frame& f) {
        if(active() || !f.foreground || !f.displayValid) return false;
        *this=Transition{}; phase=Phase::Stopping; begun=f.now; oldHdr=f.hdr; targetHdr=!f.hdr;
        baseFree=f.frees; baseGenerated=f.generated; return true;
    }
    bool timeout(uint64_t now) {
        if(active() && phase!=Phase::Failed && now-begun>25000) {fail("transition_timeout"); return true;} return false;
    }
    bool beforeReplay(const Frame& f) {
        if(replayed) return true;
        if(!f.foreground) {fail("focus_lost_before_replay"); return false;}
        if(f.displayValid && f.hdr!=oldHdr) {fail("display_changed_before_controlled_replay"); return false;}
        return true;
    }
    bool validNewFrame(const Frame& f) const {
        return replayed && f.displayValid && f.hdr==targetHdr && f.bridgeHdr==targetHdr && f.foreground &&
            f.presentOK && f.coreOff && f.allInputs && f.coreResetIdle && f.settled==f.generation &&
            f.colorWidth>0 && f.colorHeight>0 && f.colorFormat!=0 && f.hudlessFormat==f.colorFormat;
    }
    // Called only after the real Present callback and the original core frame have returned.
    void observe(const Frame& f) {
        if(!active() || phase==Phase::Failed || timeout(f.now)) return;
        if(!replayed && !beforeReplay(f)) return;
        if(phase==Phase::Stopping && stopCommitted && f.coreOff && f.presentOK && f.frees>baseFree) {
            stopPresent=lastOffPresent=f.present; offPresents=1; phase=Phase::Drain; return;
        }
        if(phase==Phase::Drain) {
            if(!f.coreOff) {fail("fg_reactivated_before_replay");return;}
            if(f.presentOK && f.present>lastOffPresent) {++offPresents;lastOffPresent=f.present;}
            if(!f.presentOK) offPresents=0;
            if(offPresents>=3 && f.stopFenceDone && f.presentOK && f.displayValid) phase=Phase::ReplayReady;
            return;
        }
        if(phase==Phase::ReplayReady && !f.coreOff) {fail("fg_reactivated_before_replay");return;}
        if(phase==Phase::AwaitDisplay || phase==Phase::Warming || phase==Phase::WarmFence || phase==Phase::ResumeReady) {
            if(!validNewFrame(f)) {goodFrames=0; stableAt=0; phase=Phase::AwaitDisplay; return;}
            const bool newFingerprint = f.generation!=fingerprintGen || f.colorWidth!=fingerprintWidth ||
                f.colorHeight!=fingerprintHeight || f.colorFormat!=fingerprintFormat;
            if(!stableAt || newFingerprint) {
                stableAt=f.now; goodFrames=0; lastWarmPresent=0; fingerprintGen=f.generation;
                fingerprintWidth=f.colorWidth; fingerprintHeight=f.colorHeight; fingerprintFormat=f.colorFormat;
                phase=Phase::Warming;
            }
            if(f.present!=lastWarmPresent) {++goodFrames; lastWarmPresent=f.present;}
            if(phase==Phase::Warming && goodFrames>=8 && f.now-stableAt>=750) phase=Phase::WarmFence;
            if(phase==Phase::WarmFence && f.warmFenceDone) phase=Phase::ResumeReady;
        }
        if(phase==Phase::Confirming && (f.selection==0 || (f.generated>baseGenerated && !f.coreOff))) phase=Phase::Idle;
    }
    void replayResult(bool success, uint64_t now) {
        if(phase!=Phase::ReplayReady) return;
        if(!success){fail("hdr_shortcut_replay_failed");return;}
        replayed=true; changedAt=now; phase=Phase::AwaitDisplay;
    }
    bool releaseForCurrentFrame(const Frame& f) {
        if(phase==Phase::ResumeReady && validNewFrame(f) && f.warmFenceDone &&
            f.generation==fingerprintGen && f.colorWidth==fingerprintWidth &&
            f.colorHeight==fingerprintHeight && f.colorFormat==fingerprintFormat) {
            phase=Phase::Confirming; baseGenerated=f.generated; return true;
        }
        return false;
    }
    void manualOffRecovery(uint32_t selection) {
        if(phase==Phase::Failed && selection==0) *this=Transition{};
    }
};
inline const char* name(Phase p) {
    switch(p) {
    case Phase::Idle:return "idle"; case Phase::Stopping:return "fg_off_requested";
    case Phase::Drain:return "off_committed_wait_fence"; case Phase::ReplayReady:return "hdr_replay_ready";
    case Phase::AwaitDisplay:return "await_new_display_and_inputs";case Phase::Warming:return "warm_complete_ui_frames";
    case Phase::WarmFence:return "wait_warmup_fence";case Phase::ResumeReady:return "restore_ready";
    case Phase::Confirming:return "await_generated_frame_confirmation";case Phase::Failed:return "failed_fg_held_off";
    }return "unknown";
}
}
