#pragma once
#include <cstdint>
#include <cstddef>
namespace control_fg_native_source {
// Verified against d3d SHA256 cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5.
constexpr std::uintptr_t IndexOffset=0x138,TexturesOffset=0x140,ChainOffset=0x178,ResourceOffset=0x88;
enum class Status { Matched, UnverifiedModule, UnsupportedCount, ReadFailed, WrongChain, BadIndex, NoResourceMatch, AmbiguousResource };
struct Result {unsigned source=0,nativeIndex=~0u;std::uintptr_t resource=0;Status status=Status::ReadFailed;};
template<class Reader> Result Select(std::uintptr_t devicePointerAddress,std::uintptr_t expectedChain,
    const std::uintptr_t* shadows,unsigned count,unsigned fallback,Reader read) noexcept {
    Result result;result.source=fallback;
    if(!devicePointerAddress){result.status=Status::UnverifiedModule;return result;}
    if(count!=2||!shadows){result.status=Status::UnsupportedCount;return result;}
    std::uintptr_t device=0,chain=0,texture=0;
    if(!read(devicePointerAddress,&device,sizeof(device))||!device)return result;
    if(!read(device+ChainOffset,&chain,sizeof(chain)))return result;
    if(!expectedChain||chain!=expectedChain){result.status=Status::WrongChain;return result;}
    if(!read(device+IndexOffset,&result.nativeIndex,sizeof(result.nativeIndex)))return result;
    if(result.nativeIndex>=2){result.status=Status::BadIndex;return result;}
    if(!read(device+TexturesOffset+result.nativeIndex*sizeof(std::uintptr_t),&texture,sizeof(texture))||!texture)return result;
    if(!read(texture+ResourceOffset,&result.resource,sizeof(result.resource))||!result.resource)return result;
    unsigned match=~0u;
    for(unsigned i=0;i<count;++i)if(shadows[i]==result.resource){
        if(match!=~0u){result.status=Status::AmbiguousResource;return result;}
        match=i;
    }
    if(match==~0u){result.status=Status::NoResourceMatch;return result;}
    result.source=match;result.status=Status::Matched;return result;
}
inline const char* Name(Status status) noexcept {
    switch(status){case Status::Matched:return "matched";case Status::UnverifiedModule:return "module_unverified";
    case Status::UnsupportedCount:return "unsupported_count";case Status::ReadFailed:return "read_failed";
    case Status::WrongChain:return "wrong_chain";case Status::BadIndex:return "bad_native_index";
    case Status::NoResourceMatch:return "no_resource_match";case Status::AmbiguousResource:return "ambiguous_resource";}
    return "unknown";
}
}
