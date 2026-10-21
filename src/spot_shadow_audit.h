#pragma once
// Included inside native_lighting. All observations are CPU-side.
namespace spot_shadow_audit {
struct RequestRecord {
    void* record = nullptr;
    uint64_t sequence = 0;
    unsigned light = 0;
    int nativeWidth = 0, requestedWidth = 0, requestedHeight = 0;
};
inline SRWLOCK lock = SRWLOCK_INIT;
inline RequestRecord requests[2048]{};
inline unsigned requestCount = 0, overflowCount = 0;
inline std::atomic<uint64_t> packedSequence{~uint64_t(0)};
inline uint64_t lastReportSequence = ~uint64_t(0);
inline unsigned pathReports = 0;
inline uint64_t pathSequence = ~uint64_t(0);
struct PathKey { unsigned light; unsigned path; };
inline PathKey paths[512]{};

inline void RecordRequest(void* record, uint64_t state, unsigned light,
                          int nativeWidth, int width, int height) noexcept {
    if (packedSequence.load()==LiveSelection::Sequence(state)) return;
    AcquireSRWLockExclusive(&lock);
    unsigned index = 0;
    while (index < requestCount && requests[index].record != record) ++index;
    if (index < 2048) {
        requests[index] = {record, LiveSelection::Sequence(state), light, nativeWidth, width, height};
        if (index == requestCount) ++requestCount;
    } else ++overflowCount;
    ReleaseSRWLockExclusive(&lock);
}
inline void RecordPath(uint64_t state, unsigned light, unsigned path, int width, int height) noexcept {
    AcquireSRWLockExclusive(&lock);
    const auto sequence = LiveSelection::Sequence(state);
    if (pathSequence != sequence) { pathSequence = sequence; pathReports = 0; }
    bool found = false;
    for (unsigned i=0; i<pathReports; ++i)
        if (paths[i].light == light && paths[i].path == path) { found = true; break; }
    if (!found && pathReports < 512) {
        paths[pathReports++] = {light,path};
        Trace("spot_shadow_path seq=%llu light=%u path=%s requested=%dx%d shader_sample_confirmed=0",
              sequence,light,path==0 ? "dynamic_pcf" : path==1 ? "static_vsm" : "dynamic_vsm",width,height);
    }
    ReleaseSRWLockExclusive(&lock);
}
struct PackedTile { int x, y, width, height; void* texture; };
inline bool ReadTile(void* record, PackedTile& tile) noexcept {
    __try {
        auto bytes = static_cast<const unsigned char*>(record);
        std::memcpy(&tile.x,bytes+0x34,4);
        std::memcpy(&tile.y,bytes+0x38,4);
        std::memcpy(&tile.width,bytes+0x3c,4);
        std::memcpy(&tile.height,bytes+0x40,4);
        std::memcpy(&tile.texture,bytes+8,sizeof(void*));
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline bool ReadPool(void* pool, void*** records, int& count, int& width, int& height) noexcept {
    __try {
        auto bytes=static_cast<const unsigned char*>(pool);
        std::memcpy(records,bytes+0x18,sizeof(void*));
        std::memcpy(&count,bytes+0x5c,4);
        std::memcpy(&width,bytes+0x64,4);
        std::memcpy(&height,bytes+0x60,4);
        return count>=0 && count<=4096 && (count==0 || *records) && width>0 && height>0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
inline void* ReadRecord(void** records, int index) noexcept {
    __try { return records[index]; }
    __except(EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
// Called from the native post-pack clear callback while the pool's own lock
// remains held. Records are not retained for later dereferencing.
inline void Packed(void* pool) noexcept {
    const auto state=selection.Read();
    const auto sequence=LiveSelection::Sequence(state);
    if (packedSequence.load()==sequence) return;
    void** records=nullptr;int count=0,width=0,height=0;
    if (!ReadPool(pool,&records,count,width,height)) {
        Trace("spot_pack_read_failed seq=%llu",sequence); return;
    }
    AcquireSRWLockExclusive(&lock);
    unsigned matched=0, exact=0, smaller=0, invalid=0;
    const bool report=lastReportSequence!=sequence;
    if (!report) { requestCount=0; overflowCount=0; ReleaseSRWLockExclusive(&lock); return; }
    for(int i=0;i<count;++i) {
        void* record=ReadRecord(records,i);
        const RequestRecord* request=nullptr;
        for(unsigned j=0;j<requestCount;++j)
            if(requests[j].record==record && requests[j].sequence==sequence) {request=&requests[j];break;}
        if(!request)continue;
        ++matched;
        PackedTile tile{};
        const bool valid=ReadTile(record,tile) && tile.texture && tile.x>=0 && tile.y>=0 &&
            tile.width>0 && tile.height>0 && tile.width<=width && tile.height<=height &&
            tile.x<=width-tile.width && tile.y<=height-tile.height;
        const bool same=valid && tile.width==request->requestedWidth && tile.height==request->requestedHeight;
        if(!valid)++invalid; else if(same)++exact; else ++smaller;
        if(report && matched<=128)
            Trace("spot_tile_packed seq=%llu light=%u native=%d requested=%dx%d packed=%dx%d origin=%d,%d valid=%u exact=%u gpu_confirmed=0",
                  sequence,request->light,request->nativeWidth,request->requestedWidth,request->requestedHeight,
                  tile.width,tile.height,tile.x,tile.y,unsigned(valid),unsigned(same));
    }
    if(matched && !invalid && !smaller && !overflowCount) packedSequence.store(sequence);
    if(report && matched) {
        Trace("spot_pack_summary seq=%llu atlas=%dx%d active=%d matched=%u exact=%u resized=%u invalid=%u overflow=%u gpu_confirmed=0",
              sequence,width,height,count,matched,exact,smaller,invalid,overflowCount);
        lastReportSequence=sequence;
    }
    requestCount=0;overflowCount=0;
    ReleaseSRWLockExclusive(&lock);
}
inline bool Observed(uint64_t state) noexcept { return packedSequence.load()==LiveSelection::Sequence(state); }
} // namespace spot_shadow_audit
