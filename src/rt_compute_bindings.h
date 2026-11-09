#pragma once
#include "rt_owned_assembly.h"
#include <array>
namespace rt_compute_bindings {
using rt_owned_assembly::Read;using rt_owned_assembly::Write;
struct Cbv {std::uint64_t address;std::uint32_t bytes,padding=0;};
struct Api {
 void(*updateConstants)(void*);void*(*threadData)();void*(*state)(void*);
 void(*createCbv)(const Cbv*,std::uintptr_t);void(*copyOne)(std::uintptr_t,std::uintptr_t);
 void(*copyMany)(unsigned,const std::uintptr_t*,const std::uintptr_t*);
 const unsigned char* registry=nullptr;const std::uint32_t* increment=nullptr;void* const* nullDescriptors=nullptr;
};
struct Result {bool handled=false,batched=false;unsigned descriptors=0;};
// Metadata is native compiled-shader metadata; no pointers are retained after this call.
inline bool Eligible(const void* table){
 const auto stage=Read<std::uint32_t>(table);if(stage!=5&&stage!=6)return false;
 if(Read<std::uint32_t>(table,0x10)||Read<std::uint32_t>(table,0x20))return false; // texture/sampler lifetime callbacks stay native
 const auto ranges=Read<std::uint32_t>(table,0x40);if(!ranges||ranges>36)return false;
 auto* list=Read<unsigned char*>(table,0x38);if(!list)return false;
 unsigned total=0;for(unsigned i=0;i<ranges;++i){const auto* r=list+24*i;const auto slot=Read<std::int32_t>(r,4),n=Read<std::int32_t>(r,8);
  if(n<0||slot<0||slot>35||n>36-slot||Read<std::int32_t>(r)<0||Read<std::int32_t>(r,12)<0)return false;
  total+=static_cast<unsigned>(n);if(total>36)return false;
 }
 if(total<2)return false;
 for(auto off:{std::size_t(0x70),std::size_t(0x80)}){
  const auto n=Read<std::uint32_t>(table,off);if(n>16)return false;
  auto* records=Read<unsigned char*>(table,off-8);if(n&&!records)return false;
  for(unsigned i=0;i<n;++i){auto* r=records+40*i;const auto slot=Read<std::int32_t>(r);if(slot<0||slot>8||Read<std::int32_t>(r,off==0x70?0x20:0x24)<0)return false;}
 }
 return true;
}
inline Result TryBind(const Api& api,void* table){
 if(!Eligible(table))return {};
 // Preserve native constant-buffer dirty guards, allocations and uploads exactly once.
 api.updateConstants(table);auto* tls=static_cast<unsigned char*>(api.threadData());auto* state=api.state(tls);
 const auto step=*api.increment;const auto base=Read<std::uintptr_t>(state,0x40);
 auto* list=Read<unsigned char*>(table,0x68);auto count=Read<std::uint32_t>(table,0x70);
 for(std::uint32_t i=0;i<count;++i){auto* r=list+40*i;const auto slot=Read<std::int32_t>(r);const auto index=Read<std::int32_t>(r,0x20);
  auto* allocation=tls+0x456c0+std::ptrdiff_t(index)*40;
  Cbv desc{Read<std::uint64_t>(allocation,8)+(std::uint64_t(Read<std::uint32_t>(allocation,0x20))<<5),Read<std::uint32_t>(allocation,0x24)<<4,0};
  state=api.state(tls);if(slot<4)Write(state,0xf8+std::size_t(slot)*8,desc.address);else api.createCbv(&desc,Read<std::uintptr_t>(state,0x40)+std::uintptr_t(slot-4)*step);
 }
 list=Read<unsigned char*>(table,0x78);count=Read<std::uint32_t>(table,0x80);
 for(std::uint32_t i=0;i<count;++i){auto* r=list+40*i;const auto slot=Read<std::int32_t>(r),id=Read<std::int32_t>(r,0x24);
  auto* desc=tls+0x4250+Read<std::int32_t>(api.registry,std::size_t(id)*16);state=api.state(tls);
  if(slot<4)Write(state,0xf8+std::size_t(slot)*8,Read<std::uint64_t>(desc));else api.createCbv(reinterpret_cast<const Cbv*>(desc),Read<std::uintptr_t>(state,0x40)+std::uintptr_t(slot-4)*step);
 }
 // The native function snapshots the SRV destination base before its CBV loops.
 const auto srvBase=base+std::uintptr_t(step)*5;
 std::array<std::uintptr_t,36> dst{},src{};unsigned n=0;std::uint64_t used=0;bool safe=step!=0;
 list=Read<unsigned char*>(table,0x38);count=Read<std::uint32_t>(table,0x40);
 for(std::uint32_t i=0;i<count;++i){auto* r=list+24*i;const auto id=Read<std::int32_t>(r),slot=Read<std::int32_t>(r,4),size=Read<std::int32_t>(r,8),offset=Read<std::int32_t>(r,12);
  auto* values=tls+0x4250+Read<std::int32_t>(api.registry,std::size_t(id)*16)+std::ptrdiff_t(offset)*8;
  for(std::int32_t j=0;j<size;++j){const auto bit=std::uint64_t(1)<<(slot+j);safe=safe&&!(used&bit);used|=bit;
   dst[n]=srvBase+std::uintptr_t(slot+j)*step;src[n]=Read<std::uintptr_t>(values,std::size_t(j)*8);if(!src[n])src[n]=Read<std::uintptr_t>(*api.nullDescriptors,0x90);++n;
  }
 }
 // A batched copy must not overlap any written range with any read range.
 // Native heap handles are opaque; use descriptor increments, not guessed byte sizes.
 for(unsigned i=0;i<n&&safe;++i)for(unsigned j=0;j<n;++j){const auto distance=dst[i]>src[j]?dst[i]-src[j]:src[j]-dst[i];if(distance<step){safe=false;break;}}
 if(safe)api.copyMany(n,dst.data(),src.data());else for(unsigned i=0;i<n;++i)api.copyOne(dst[i],src[i]);
 return {true,safe,n};
}
struct ShaderApi {void*(*retrieve)();void(*setShaders)(void*,const void* const*,bool);void(*nativeUse)(void*,void*);void(*finishUse)(void*,void*);};
struct ShaderResult {std::uint64_t handled=0,batches=0,descriptors=0,native=0;};
inline ShaderResult ApplyShader(const Api& api,const ShaderApi& shaderApi,void* material){
 ShaderResult stats;const void* shaders[7]{};for(unsigned i=0;i<7;++i)shaders[i]=Read<void*>(material,0x10+i*16);
 auto* state=shaderApi.retrieve();shaderApi.setShaders(state,shaders,Read<unsigned char>(material,1)!=0);
 for(unsigned i=0;i<7;++i)if(Read<void*>(material,0x10+i*16)){
  auto* table=Read<void*>(material,0x18+i*16);const auto result=TryBind(api,table);
  if(result.handled){shaderApi.finishUse(table,state);++stats.handled;stats.batches+=result.batched?1u:0u;stats.descriptors+=result.descriptors;}
  else{shaderApi.nativeUse(table,state);++stats.native;}
 }
 return stats;
}

}
