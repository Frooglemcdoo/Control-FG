#pragma once
#include "rr_part1_metadata.h"

namespace control_rr_part1 {
struct Sample {
    Metadata metadata{};
    Result result = Result::InvalidArgument;
    std::uint64_t mappingObject{};
    std::uint32_t mappingMode{};
    bool mappingReadable = false;
    bool repeatedReadMatched = false;
    bool frameMatched = false;
};
inline bool Equal(const Metadata& a, const Metadata& b) noexcept {
    return a.renderer==b.renderer && a.owner==b.owner && a.buffer==b.buffer &&
           a.stride==b.stride && a.bytes==b.bytes && a.gpuAddress==b.gpuAddress;
}
// Reader returns false on inaccessible memory. All saved addresses are numbers,
// never dereferenced later. Two matching reads are NOT an atomic snapshot.
template<class Reader>
Sample Observe(Reader&& read, std::uint64_t renderer) {
    Sample s{};
    auto word = [&](std::uint64_t address, std::uint64_t& value) {
        return read(address, &value, sizeof(value));
    };
    s.result=ReadMetadata(word, renderer, 0, s.metadata);
    if(s.result!=Result::Candidate) return s;
    constexpr auto maximum=(std::numeric_limits<std::uint64_t>::max)();
    const auto buffer=s.metadata.buffer;
    if(buffer<=maximum-0x47 && read(buffer+0x40,&s.mappingObject,sizeof(s.mappingObject)) &&
       s.mappingObject && s.mappingObject<=maximum-0x17) {
        s.mappingReadable=read(s.mappingObject+0x14,&s.mappingMode,sizeof(s.mappingMode));
    }
    Metadata again{};
    s.repeatedReadMatched=ReadMetadata(word,renderer,0,again)==Result::Candidate && Equal(s.metadata,again);
    return s;
}
inline bool PairMatched(const Sample& a,const Sample& b) noexcept {
    return a.result==Result::Candidate && b.result==Result::Candidate &&
        a.frameMatched && b.frameMatched && a.repeatedReadMatched && b.repeatedReadMatched &&
        a.mappingReadable && b.mappingReadable && Equal(a.metadata,b.metadata) &&
        a.mappingObject==b.mappingObject && a.mappingMode==b.mappingMode;
}
} // namespace control_rr_part1
