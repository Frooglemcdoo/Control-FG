#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace rt_spot_capture {
using ShaderIdentifier = std::array<unsigned char, 32>;
struct IdentifierReplacement { ShaderIdentifier native; ShaderIdentifier capture; };
inline constexpr std::size_t NativeHitRecordBytes=96,MaximumHitTableBytes=8*1024*1024;
// Validate the entire table before writing. Null identifiers retain their full
// record, including nonzero local arguments; record indices never change.
inline bool TranslateHitTableBytes(const unsigned char* source,std::size_t bytes,
    const IdentifierReplacement* replacements,std::size_t count,unsigned char* destination,std::size_t capacity) noexcept {
    if(!source || !destination || !bytes || bytes>MaximumHitTableBytes || bytes%NativeHitRecordBytes || capacity<bytes || !replacements || !count || count>16)return false;
    const ShaderIdentifier empty{};
    for(std::size_t i=0;i<count;++i) {
        if(replacements[i].native==empty || replacements[i].capture==empty)return false;
        for(std::size_t j=0;j<i;++j)if(replacements[i].native==replacements[j].native)return false;
    }
    for(std::size_t at=0;at<bytes;at+=NativeHitRecordBytes) {
        if(!std::memcmp(source+at,empty.data(),32))continue;
        bool found=false;
        for(std::size_t i=0;i<count;++i)if(!std::memcmp(source+at,replacements[i].native.data(),32)){found=true;break;}
        if(!found)return false;
    }
    // Distinct storage is the runtime contract; memmove also permits exact
    // in-place translation without changing any local-root bytes.
    std::memmove(destination,source,bytes);
    for(std::size_t at=0;at<bytes;at+=NativeHitRecordBytes) {
        if(!std::memcmp(destination+at,empty.data(),32))continue;
        for(std::size_t i=0;i<count;++i)if(!std::memcmp(destination+at,replacements[i].native.data(),32)) {
            std::memcpy(destination+at,replacements[i].capture.data(),32);break;
        }
    }
    return true;
}
inline bool TranslateHitTable(const std::vector<unsigned char>& source,
    const std::vector<IdentifierReplacement>& replacements,std::vector<unsigned char>& destination) {
    if(source.empty() || source.size()>MaximumHitTableBytes)return false;
    std::vector<unsigned char> translated(source.size());
    if(!TranslateHitTableBytes(source.data(),source.size(),replacements.data(),replacements.size(),translated.data(),translated.size()))return false;
    destination.swap(translated);return true;
}
}
