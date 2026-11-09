#pragma once
#include <cstdint>
#include <cstring>
namespace native_material_resolver {
template<class T> inline T Read(const void* p,std::intptr_t offset=0) noexcept {
    T out;std::memcpy(&out,static_cast<const unsigned char*>(p)+offset,sizeof(out));return out;
}
template<class T> inline void Write(void* p,std::intptr_t offset,T value) noexcept {
    std::memcpy(static_cast<unsigned char*>(p)+offset,&value,sizeof(value));
}
inline void* DescriptorManager(const void* importSlot) noexcept {return Read<void*>(Read<void*>(importSlot));}
// Same live reads, null fallback, slot order and version updates as the native loop.
// No stored resource pointers, retained handles, or cross-frame caching.
inline bool Resolve(void* manager,std::uint64_t entry,std::uint64_t outputBase,const void* descriptorImport) noexcept {
    const auto* tables=Read<void*>(manager,0xa8);
    const auto* slots=Read<void*>(tables,static_cast<std::intptr_t>(entry*8));
    for(std::uint64_t slot=0;slot<35;++slot){
        std::uint64_t descriptor=0;
        if(slots){
            const auto* resource=Read<void*>(slots,static_cast<std::intptr_t>(0x10+slot*0x20));
            if(resource){
                const auto index=Read<std::int32_t>(resource,0xb4);
                const auto* variants=Read<void*>(resource,0xe8);
                const auto offset=std::intptr_t(index)*40+0x10;
                auto* container=Read<void*>(variants,offset+8);
                if(!container)container=Read<void*>(variants,offset);
                if(container){const auto* native=Read<void*>(container,8);if(native)descriptor=Read<std::uint64_t>(native,0x90);}
            }
        }
        if(!descriptor)descriptor=Read<std::uint64_t>(DescriptorManager(descriptorImport),0x80);
        auto* destination=DescriptorManager(descriptorImport);
        Write(destination,static_cast<std::intptr_t>(0x1f8+(outputBase+slot)*8),descriptor);
        Write(destination,8,Read<std::uint64_t>(destination,8)+1);
    }
    return slots!=nullptr;
}
}
