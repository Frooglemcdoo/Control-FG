#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace PTGlassCapture {
constexpr std::size_t kMaximumBytes = 2u * 1024u * 1024u;
inline std::uint32_t Word(const unsigned char* p) noexcept {
    std::uint32_t v; std::memcpy(&v, p, sizeof(v)); return v;
}
// Only shader libraries exporting the observed glass ray-generation entry
// qualify. Search RDAT, never arbitrary strings in a pixel/compute shader.
inline bool IsGlassLibrary(const unsigned char* bytes, std::size_t size) noexcept {
    if (!bytes || size < 32 || size > kMaximumBytes ||
        std::memcmp(bytes, "DXBC", 4) || Word(bytes + 24) != size) return false;
    const auto count = Word(bytes + 28);
    if (!count || count > (size - 32) / 4) return false;
    bool library = false, glass = false;
    const char entry[] = "transparentRayGeneration";
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto offset = Word(bytes + 32 + i * 4);
        if (offset < 32 + count * 4 || offset > size - 8) return false;
        const auto length = Word(bytes + offset + 4);
        if (length > size - offset - 8) return false;
        const auto* data = bytes + offset + 8;
        if (!std::memcmp(bytes + offset, "DXIL", 4) && length >= 16)
            library = (Word(data) >> 16) == 6u;
        if (!std::memcmp(bytes + offset, "RDAT", 4)) {
            for (std::size_t j = 0; j + sizeof(entry) - 1 <= length; ++j)
                if (!std::memcmp(data + j, entry, sizeof(entry) - 1)) glass = true;
        }
    }
    return library && glass;
}
inline bool Contains(const unsigned char* bytes, std::size_t size, const char* name) noexcept {
    const auto length=std::strlen(name)+1;
    for(std::size_t i=0;i+length<=size;++i)
        if(!std::memcmp(bytes+i,name,length))return true;
    return false;
}
// Collect the native hit-lighting candidates, including the known opaque
// reference. RDEF resource names are required; arbitrary instruction strings
// and vertex shaders must not qualify. The recovered ABI decides which
// candidate belongs to glass; this predicate never activates a replacement.
inline bool IsHitLighting(const unsigned char* bytes, std::size_t size) noexcept {
    if(!bytes||size<32||size>kMaximumBytes||std::memcmp(bytes,"DXBC",4)||Word(bytes+24)!=size)return false;
    const auto count=Word(bytes+28);
    if(!count||count>(size-32)/4)return false;
    bool stage=false,resources=false;
    for(std::uint32_t i=0;i<count;++i){
        const auto offset=Word(bytes+32+i*4);
        if(offset<32+count*4||offset>size-8)return false;
        const auto length=Word(bytes+offset+4);
        if(length>size-offset-8)return false;
        const auto* data=bytes+offset+8;
        if((!std::memcmp(bytes+offset,"SHEX",4)||!std::memcmp(bytes+offset,"SHDR",4))&&length>=4){
            const auto kind=Word(data)>>16;stage=kind==0u||kind==5u;
        }
        if(!std::memcmp(bytes+offset,"RDEF",4))
            resources=Contains(data,length,"g_tMaterialId")&&
                Contains(data,length,"g_tNormal_TexcoordX")&&Contains(data,length,"g_tPosition_TexcoordY");
    }
    return stage&&resources;
}
}
