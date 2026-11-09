#pragma once
// Primary preparation owns this fixed cache. Records are retained until exit,
// like native shader storage; workers never observe a reused record.
struct RRWorldAlbedoRecord {
    std::uintptr_t selected=0;
    int variant=-1;
    alignas(16) unsigned char record[0xA8]{};
    alignas(16) unsigned char descriptor[0x100]{};
};
static RRWorldAlbedoRecord rrWorldAlbedoRecords[64]{};
static unsigned rrWorldAlbedoRecordCount=0;
static bool RRWorldAlbedoRequested() noexcept {
    return native_lighting::UsesWorldAlbedo(native_lighting::LiveSelection::GetMode(native_lighting::selection.Read()));
}
static std::uintptr_t RRWorldAlbedoReplacement(std::uintptr_t selected,int variant) noexcept {
    if(!rrWorldAlbedoStorageReady.load(std::memory_order_acquire)||!selected||variant<0||
       static_cast<unsigned>(variant)>=sizeof(kRRWorldVariants)/sizeof(kRRWorldVariants[0]))return 0;
    for(unsigned i=0;i<rrWorldAlbedoRecordCount;++i) {
        const auto& record=rrWorldAlbedoRecords[i];
        if(record.selected==selected&&record.variant==variant)return reinterpret_cast<std::uintptr_t>(record.record);
    }
    if(rrWorldAlbedoRecordCount>=64)return 0;
    auto& record=rrWorldAlbedoRecords[rrWorldAlbedoRecordCount];
    const auto& shader=kRRWorldVariants[variant];
    if(!PTReflectionCloneShaderRecord(reinterpret_cast<void*>(selected),0x50,4,
        kRRWorldAlbedoStorageBase+variant,shader.replaySize,record.record,record.descriptor))return 0;
    record.selected=selected;record.variant=variant;++rrWorldAlbedoRecordCount;
    return reinterpret_cast<std::uintptr_t>(record.record);
}

static bool RRWorldPerspectiveGuideRequested() noexcept {
    return native_lighting::UsesPerspectiveGuide(
        native_lighting::LiveSelection::GetMode(native_lighting::selection.Read()));
}
