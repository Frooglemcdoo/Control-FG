#pragma once
#include "pt_scene_admission.h"
#include <array>
#include <cstring>
#include <vector>
// Conversion only: source records are read from a caller-owned CPU snapshot.
// Native shader identifiers are supplied by the SAME native state object.
// Position bindings must come from a verified BLAS/attribute correspondence.
namespace PTShaderTableAdapter {
constexpr unsigned NativeStride=96,PrivateStride=128;
using Identifier=std::array<unsigned char,32>;
using Bytes=std::array<unsigned char,PrivateStride>;
struct Exports {Identifier nativeOpaque,nativeAlpha,privateOpaque,privateAlpha;};
enum class Status {ready,record_size,unknown_export,unverified_mapping,native_mismatch,invalid_geometry,invalid_exports};
inline Status Convert(const unsigned char* source,std::size_t bytes,const Exports& exports,
                      const PTSceneAdmission::Geometry* mapping,Bytes& output){
    output.fill(0);
    if(!source||bytes!=NativeStride)return Status::record_size;
    if(exports.nativeOpaque==Identifier{}||exports.nativeAlpha==Identifier{}||exports.privateOpaque==Identifier{}||exports.privateAlpha==Identifier{}||exports.nativeOpaque==exports.nativeAlpha)return Status::invalid_exports;
    const Identifier* identifier=nullptr;
    if(!std::memcmp(source,exports.nativeOpaque.data(),32))identifier=&exports.privateOpaque;
    else if(!std::memcmp(source,exports.nativeAlpha.data(),32))identifier=&exports.privateAlpha;
    else return Status::unknown_export;
    if(!mapping)return Status::unverified_mapping;
    std::uint64_t indexBase=0,attributeBase=0;std::uint32_t constants[10]{};
    std::memcpy(&indexBase,source+32,8);std::memcpy(&attributeBase,source+40,8);std::memcpy(constants,source+48,40);
    PTSceneAdmission::Record admitted;
    if(PTSceneAdmission::Admit(*mapping,admitted)!=PTSceneAdmission::Error::none)return Status::invalid_geometry;
    // Check the full native bounds and material contract, not just two addresses.
    if(indexBase!=admitted.indices||attributeBase!=admitted.attributes)return Status::native_mismatch;
    for(unsigned i=0;i<10;++i){
        // Tangent/color fields are unused by the admitted geometric+UV decoder.
        if(i==4||i==7)continue;
        // Native R16 views can end at a halfword. The private raw Load needs
        // the final DWORD, whose backing bytes were proved by Admit. Only the
        // exact round-up is permitted; never silently enlarge an arbitrary view.
        if(i==8){
            const auto logical=std::uint64_t(constants[i]);
            const auto required=std::uint64_t(admitted.constants[0])+std::uint64_t(mapping->indexCount)*mapping->indexStride;
            if(logical<required||((logical+3)&~std::uint64_t(3))!=admitted.constants[i])return Status::native_mismatch;
            continue;
        }
        // Native attribute views may contain vertices unused by this BLAS.
        // Preserve the declared bound only after checking the backing view.
        if(i==9){
            if(constants[i]<admitted.constants[i]||!PTSceneAdmission::Contains(mapping->attributes,constants[i]))return Status::native_mismatch;
            continue;
        }
        if(constants[i]!=admitted.constants[i])return Status::native_mismatch;
    }
    admitted.constants[4]=constants[4];admitted.constants[7]=constants[7];admitted.constants[9]=constants[9];
    std::memcpy(output.data(),identifier->data(),32);
    std::memcpy(output.data()+32,&admitted.indices,8);
    std::memcpy(output.data()+40,&admitted.attributes,8);
    std::memcpy(output.data()+48,&admitted.positions,8);
    std::memcpy(output.data()+56,admitted.constants,48);
    return Status::ready;
}
struct Binding {const Exports* exports;const PTSceneAdmission::Geometry* geometry;};
inline Status ConvertTable(const unsigned char* source,std::size_t bytes,const std::vector<Binding>& bindings,
                           std::vector<Bytes>& output,std::size_t& failedRecord){
    output.clear();failedRecord=0;
    if(!source||!bytes||bytes%NativeStride||bytes/NativeStride!=bindings.size()||bindings.size()>131072)return Status::record_size;
    std::vector<Bytes> pending(bindings.size());
    for(std::size_t i=0;i<bindings.size();++i){
        failedRecord=i;if(!bindings[i].exports)return Status::invalid_exports;
        auto status=Convert(source+i*NativeStride,NativeStride,*bindings[i].exports,bindings[i].geometry,pending[i]);
        if(status!=Status::ready)return status;
    }
    output.swap(pending);failedRecord=bindings.size();return Status::ready;
}
// Record numbers (including TLAS instance contributions) stay unchanged. The
// private DispatchRays table uses 128-byte stride; never patch the native table.
}
