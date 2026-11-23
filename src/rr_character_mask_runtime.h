#pragma once
#ifdef CONTROL_CHARACTER_GI_BLEND
// Character surface variants are matched by native PS bytes. A hair variant
// additionally needs the audited slot-26 opacity ABI. Cloth is admitted by
// the caller only for a known character mesh and instance span.
static std::uintptr_t RRCharacterMaskShader(std::uintptr_t original, std::uintptr_t selected,
                                           unsigned family) noexcept {
    if(!rrHairOpacityStorageReady.load() || !original || !selected) return 0;
    struct Record {
        RRAlbedoShader::NativeDescriptor source{}, replay{};
        unsigned family=0;
        alignas(16) unsigned char record[0xA8]{};
        alignas(16) unsigned char descriptor[0x100]{};
        bool matched=false;
    };
    static Record records[128]{};
    static size_t count=0; // primary preparation thread
    RRAlbedoShader::NativeDescriptor src{}, dst{};
    if(!RRAlbedoShader::ReadDescriptor(original,0x50,&src) || !RRAlbedoShader::ReadDescriptor(selected,0x50,&dst) ||
       src.stage!=4 || dst.stage!=4 || src.size<32 || src.size>RRAlbedoShader::kMaximumBlobBytes) return 0;
    // Other stages of a record can differ even when sharing a pixel shader.
    struct Key { std::uintptr_t original=0,selected=0; };
    static Key keys[128]{};
    for(size_t i=0;i<count;++i) {
        const auto& r=records[i];
        if(keys[i].original==original && keys[i].selected==selected && r.family==family &&
           r.source.identifier==src.identifier && r.source.shaderBase==src.shaderBase && r.source.size==src.size &&
           r.replay.identifier==dst.identifier && r.replay.shaderBase==dst.shaderBase && r.replay.size==dst.size)
            return r.matched?reinterpret_cast<std::uintptr_t>(r.record):0;
    }
    if(count==128) return 0;
    try {
        const bool hair=family==5;
        std::vector<uint8_t> bytes(size_t(src.size));
        if(!RRAlbedoShader::CopyResident(&src,bytes.data(),bytes.size())) return 0;
        const int known=native_lighting::character_surface::Family({bytes.data(),bytes.size()});
        const size_t index=count++;
        auto& r=records[index];keys[index]={original,selected};
        r.source=src;r.replay=dst;r.family=family;
        if(known<0 || native_lighting::character_surface::kCharacterGroups[known]!=(hair?2u:family==3?1u:0u)) return 0;
        char sha[65]{};
        if(hair && (!PTReflectionSha256(bytes.data(),bytes.size(),sha) ||
            strcmp(sha,"33fde7037525c2b9aa6cca58bd45d3151ecc8f1f91bfa4ff6529909179dc41ca"))) return 0;
        r.matched=PTReflectionCloneShaderRecord(reinterpret_cast<void*>(selected),0x50,4,
            hair?kCharacterMaskHairId:kCharacterMaskOpaqueId,hair?sizeof(kCharacterMaskHair):sizeof(kCharacterMaskOpaque),
            r.record,r.descriptor);
        return r.matched?reinterpret_cast<std::uintptr_t>(r.record):0;
    } catch(...) {return 0;}
}
#endif
