#pragma once
// CPU recording provenance only. Never dereference or retain resource pointers.
// No GPU work, barriers, waits, resource selection, or FG policy changes.
static std::atomic<unsigned long long> fgAlignUntil{240};
static std::atomic<unsigned long long> fgAlignEpoch{0}, fgAlignPresentCalls{0};
static SRWLOCK fgAlignLock=SRWLOCK_INIT;
struct FGAlignStamp {
    void* resource=nullptr;
    unsigned long long target=0, sequence=0, epoch=0;
};
static FGAlignStamp fgAlignStamps[64]{};
static FGAlignStamp fgAlignLatest{};
static unsigned long long fgAlignSequence=0;
static bool FGAlignActive(unsigned long long present) noexcept {
    return present<=fgAlignUntil.load(std::memory_order_relaxed);
}
static void FGAlignArm(unsigned long long present,const char* reason) noexcept {
    auto desired=present+1800, old=fgAlignUntil.load();
    while(old<desired && !fgAlignUntil.compare_exchange_weak(old,desired)) {}
    Log("ALIGN_ARM present=%llu epoch=%llu until=%llu reason=%s",present,fgAlignEpoch.load(),fgAlignUntil.load(),reason);
}
static void FGAlignPoll(unsigned long long present) noexcept {
    static bool held=false;
    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    if(down&&!held) FGAlignArm(present,"F8_user_marker");
    held=down;
}
static void FGAlignProduced(unsigned long long target,void* resource,void* command) noexcept {
    if(!resource) return;
    AcquireSRWLockExclusive(&fgAlignLock);
    const auto sequence=++fgAlignSequence;
    FGAlignStamp stamp{};stamp.resource=resource;stamp.target=target;stamp.sequence=sequence;stamp.epoch=fgAlignEpoch.load();
    fgAlignStamps[sequence%64]=stamp;fgAlignLatest=stamp;
    ReleaseSRWLockExclusive(&fgAlignLock);
    if(FGAlignActive(target)) Log("ALIGN_HUD_POST target=%llu sequence=%llu epoch=%llu color=%p command=%p aa=%llu begin=%llu",target,sequence,stamp.epoch,resource,command,aaCount.load(),beginCount.load());
}
static void FGAlignBridge(unsigned long long present,UINT index,void* source,void* dest,void* queue) noexcept {
    if(!FGAlignActive(present)) return;
    FGAlignStamp matched{},latest{};
    AcquireSRWLockShared(&fgAlignLock);
    latest=fgAlignLatest;
    for(const auto& stamp:fgAlignStamps) if(stamp.resource==source&&stamp.sequence>matched.sequence) matched=stamp;
    ReleaseSRWLockShared(&fgAlignLock);
    Log("ALIGN_BRIDGE present=%llu epoch=%llu index=%u source=%p destination=%p queue=%p matched_sequence=%llu matched_target=%llu matched_epoch=%llu latest_color=%p latest_target=%llu latest_sequence=%llu source_is_latest=%u target_matches=%u",
        present,fgAlignEpoch.load(),index,source,dest,queue,matched.sequence,matched.target,matched.epoch,latest.resource,latest.target,latest.sequence,unsigned(source==latest.resource),unsigned(matched.sequence!=0&&matched.target==present));
}
static unsigned long long FGAlignPresentEnter(unsigned long long present,IDXGISwapChain* swap,const char* kind,UINT flags) noexcept {
    const auto serial=++fgAlignPresentCalls;
    if(FGAlignActive(present)) Log("ALIGN_DXGI_ENTER serial=%llu present=%llu epoch=%llu kind=%s swap=%p flags=0x%X begin=%llu aa=%llu",serial,present,fgAlignEpoch.load(),kind,swap,flags,beginCount.load(),aaCount.load());
    return serial;
}
static void FGAlignPresentExit(unsigned long long serial,unsigned long long present,IDXGISwapChain* swap,HRESULT hr) noexcept {
    if(!FGAlignActive(present)) return;
    Log("ALIGN_DXGI_EXIT serial=%llu present=%llu epoch=%llu swap=%p hr=0x%08lX",serial,present,fgAlignEpoch.load(),swap,static_cast<unsigned long>(hr));
}
