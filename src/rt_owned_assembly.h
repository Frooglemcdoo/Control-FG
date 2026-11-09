#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
// Recovered renderer A8CE0: only the object-to-record assembly routine.
// Native selection, geometry updates, material resolution, allocation growth and
// downstream AS construction remain native. No persistent pointer caching.
namespace rt_owned_assembly {
template<class T>inline T Read(const void* p,std::size_t offset=0)noexcept{T v;std::memcpy(&v,static_cast<const unsigned char*>(p)+offset,sizeof(v));return v;}
template<class T>inline void Write(void* p,std::size_t offset,T v)noexcept{std::memcpy(static_cast<unsigned char*>(p)+offset,&v,sizeof(v));}
// Bind only byte-verified renderer getters. Unknown or replaced methods stay virtual.
struct ProviderBinding {void* table=nullptr;void* address=nullptr;void* count=nullptr;void* geometry=nullptr;void* mask=nullptr;};
struct Api {void(*append64)(void*,const void*);void(*append8)(void*,const void*);ProviderBinding staticMesh{},skinnedMesh{};};
struct Result {std::uint32_t geometry=0,fastAppends=0,growthAppends=0,materialFastAppends=0,materialGrowthAppends=0,providerKind=0,avoidedCalls=0,reusedGeometry=0,reservedObjects=0;};
inline void Append64(const Api& api,void* vector,const void* source,Result& result){
 const auto count=Read<std::uint32_t>(vector,8),capacity=Read<std::uint32_t>(vector,12)&0x3fffffffu;
 if(count>=capacity){api.append64(vector,source);++result.growthAppends;return;}
 auto* data=Read<unsigned char*>(vector);Write(vector,8,count+1);std::memcpy(data+std::size_t(count)*64,source,64);++result.fastAppends;
}
// Native 22730 fast path: increment count, then copy the pointer bits.
inline void AppendMaterial(const Api& api,void* vector,const void* source,Result& result){
 const auto count=Read<std::uint32_t>(vector,8),capacity=Read<std::uint32_t>(vector,12)&0x3fffffffu;
 if(count>=capacity){api.append8(vector,source);++result.materialGrowthAppends;return;}
 auto* data=Read<unsigned char*>(vector);Write(vector,8,count+1);std::memcpy(data+std::size_t(count)*8,source,8);++result.materialFastAppends;
}
template<class F>inline F Method(void* object,std::size_t offset)noexcept{return Read<F>(Read<void*>(object),offset);}
inline bool MatchesProvider(void* provider,const ProviderBinding& binding)noexcept{
 const auto* table=Read<void*>(provider);
 return binding.table&&table==binding.table&&Read<void*>(table,0x40)==binding.address&&Read<void*>(table,0x48)==binding.count&&Read<void*>(table,0x50)==binding.geometry&&Read<void*>(table,0x60)==binding.mask;
}
template<unsigned Kind>inline std::int32_t GeometryCount(void* p){
 if constexpr(Kind==1)return Read<std::int32_t>(p,0x28);
 else if constexpr(Kind==2)return Read<std::int32_t>(p,0x30);
 else return Method<std::int32_t(*)(void*)>(p,0x48)(p);
}
template<unsigned Kind>inline const void* GeometryRecord(void* p,std::int32_t index){
 if constexpr(Kind==1)return Read<unsigned char*>(p,0x20)+std::ptrdiff_t(index)*64;
 else if constexpr(Kind==2)return Read<unsigned char*>(p,0x18)+std::ptrdiff_t(index)*80+16;
 else return Method<const void*(*)(void*,std::int32_t)>(p,0x50)(p,index);
}
template<unsigned Kind>inline std::uint32_t InstanceMask(void* p){
 if constexpr(Kind==1)return Read<std::uint32_t>(p,0x60);
 else if constexpr(Kind==2)return Read<std::uint32_t>(p,0x54);
 else return Method<std::uint32_t(*)(void*)>(p,0x60)(p);
}
template<unsigned Kind>inline std::uint64_t BlasAddress(void* p){
 if constexpr(Kind==1){
  const bool alternate=Read<std::int32_t>(p,0xac)>=3;
  const auto* allocation=Read<void*>(p,alternate?0x88:0x68);
  const auto offset=Read<std::int32_t>(p,alternate?0x90:0x70);
  return Read<std::uint64_t>(allocation,0x10)+static_cast<std::uint64_t>(static_cast<std::int64_t>(offset));
 }
 else if constexpr(Kind==2){
  const auto* state=Read<void*>(p,0x88);const auto* allocation=Read<void*>(state,0x28);
  return Read<std::uint64_t>(allocation,0x10)+static_cast<std::uint64_t>(static_cast<std::int64_t>(Read<std::int32_t>(state,0x30)));
 }
 else return Method<std::uint64_t(*)(void*)>(p,0x40)(p);
}
#if defined(_MSC_VER)
#define RT_RESERVED_NOINLINE __declspec(noinline)
#else
#define RT_RESERVED_NOINLINE __attribute__((noinline))
#endif
template<unsigned Kind>RT_RESERVED_NOINLINE bool TryAssembleReserved(const Api&,void*,const void*,void*,std::int32_t,void*,unsigned char,unsigned char,std::uint32_t*,Result&);
template<unsigned Kind,bool AllowReserved=false>inline Result AssembleProvider(const Api& api,void* context,const void* transform,void* materials,std::int32_t variant,
                       void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 Result result;auto* ctx=static_cast<unsigned char*>(context);
 const auto count=GeometryCount<Kind>(provider);
 if constexpr(AllowReserved&&Kind!=0){if(count>=8&&TryAssembleReserved<Kind>(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix,result))return result;}
 result.providerKind=Kind;if constexpr(Kind!=0)result.avoidedCalls=3u+static_cast<std::uint32_t>(count>0?count:0);
 bool alpha=false,cull=disableCull!=0;
 for(std::int32_t index=0;index<count;++index){
  const auto* geometry=GeometryRecord<Kind>(provider,index);
  Append64(api,ctx+0x80,geometry,result);
  const auto used=Read<std::uint32_t>(ctx,0x88);auto* record=Read<unsigned char*>(ctx,0x80)+std::size_t(used-1)*64;
  const auto materialIndex=Read<std::int32_t>(record);
  void* table;
  if(variant!=0&&Read<std::uint32_t>(materials,0x158)!=0){
   auto* variants=Read<unsigned char*>(materials,0x150);
   table=Read<void*>(variants+(std::ptrdiff_t(variant)*3-2)*8);
  }else table=Read<void*>(materials,0x130);
  auto* material=Read<void*>(static_cast<unsigned char*>(table)+std::ptrdiff_t(materialIndex)*8);
  AppendMaterial(api,ctx+0x90,&material,result);
  if(Read<std::uint32_t>(material,0x204)!=2)cull=true;
  bool geometryAlpha=false;
  if(!ignoreAlpha){
   const auto* flags=Read<void*>(material,0x1f8);
   geometryAlpha=((Read<std::uint32_t>(flags)&Read<std::uint32_t>(material,0x1f0))&0x80000000u)!=0;
  }
  alpha|=geometryAlpha;Write(record,4,std::uint32_t(geometryAlpha));
 }
 std::uint32_t instance[16]{};
 // Transpose by copying bits: preserve signed zero, infinities and NaN payloads.
 for(unsigned column=0;column<4;++column)for(unsigned row=0;row<3;++row)instance[row*4+column]=Read<std::uint32_t>(transform,(column*3+row)*4);
 const auto mask=InstanceMask<Kind>(provider);
 instance[12]=mask<<24;instance[13]=(*prefix*2u)&0xffffffu;
 if(!alpha){instance[12]|=0x04000000u;instance[13]|=0x04000000u;}
 if(cull)instance[13]|=0x01000000u;
 const auto address=BlasAddress<Kind>(provider);std::memcpy(instance+14,&address,8);
 Append64(api,ctx+0x70,instance,result);*prefix+=static_cast<std::uint32_t>(count);result.geometry=static_cast<std::uint32_t>(count);return result;
}
inline bool FitsBatch(const void* vector,std::uint64_t amount)noexcept{
 const auto used=Read<std::uint32_t>(vector,8),capacity=Read<std::uint32_t>(vector,12)&0x3fffffffu;
 return used<=capacity&&amount<=std::uint64_t(capacity-used);
}

// Reserve the whole object's existing capacity once. With verified leaf getters
// and no allocation callbacks, vector counts need publication only at completion.
// Every geometry/material/transform input is still read fresh for this call.
template<unsigned Kind>RT_RESERVED_NOINLINE bool TryAssembleReserved(const Api& api,void* context,const void* transform,void* materials,std::int32_t variant,
 void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix,Result& result){
 static_assert(Kind==1||Kind==2,"verified live providers only");(void)api;
 const auto count=GeometryCount<Kind>(provider);if(count<0)return false;
 auto* ctx=static_cast<unsigned char*>(context);
 if(!FitsBatch(ctx+0x70,1)||!FitsBatch(ctx+0x80,static_cast<std::uint32_t>(count))||!FitsBatch(ctx+0x90,static_cast<std::uint32_t>(count)))return false;
 const auto instances=Read<std::uint32_t>(ctx,0x78),geometries=Read<std::uint32_t>(ctx,0x88),materialCount=Read<std::uint32_t>(ctx,0x98);
 auto* records=count?Read<unsigned char*>(ctx,0x80)+std::size_t(geometries)*64:nullptr;
 auto* pointers=count?Read<unsigned char*>(ctx,0x90)+std::size_t(materialCount)*8:nullptr;
 void* table=nullptr;
 if(count){
  if(variant!=0&&Read<std::uint32_t>(materials,0x158)!=0)table=Read<void*>(Read<unsigned char*>(materials,0x150)+(std::ptrdiff_t(variant)*3-2)*8);
  else table=Read<void*>(materials,0x130);
 }
 bool alpha=false,cull=disableCull!=0;
 for(std::int32_t i=0;i<count;++i){
  auto* record=records+std::size_t(i)*64;std::memcpy(record,GeometryRecord<Kind>(provider,i),64);
  auto* material=Read<void*>(static_cast<unsigned char*>(table)+std::ptrdiff_t(Read<std::int32_t>(record))*8);
  Write(pointers,std::size_t(i)*8,material);cull|=Read<std::uint32_t>(material,0x204)!=2;
  const bool a=!ignoreAlpha&&((Read<std::uint32_t>(Read<void*>(material,0x1f8))&Read<std::uint32_t>(material,0x1f0)&0x80000000u)!=0);
  alpha|=a;Write(record,4,std::uint32_t(a));
 }
 std::uint32_t instance[16]{};
 for(unsigned column=0;column<4;++column)for(unsigned row=0;row<3;++row)instance[row*4+column]=Read<std::uint32_t>(transform,(column*3+row)*4);
 instance[12]=InstanceMask<Kind>(provider)<<24;instance[13]=(*prefix*2u)&0xffffffu;
 if(!alpha){instance[12]|=0x04000000u;instance[13]|=0x04000000u;}if(cull)instance[13]|=0x01000000u;
 const auto address=BlasAddress<Kind>(provider);std::memcpy(instance+14,&address,8);
 std::memcpy(Read<unsigned char*>(ctx,0x70)+std::size_t(instances)*64,instance,64);
 Write(ctx,0x78,instances+1);Write(ctx,0x88,geometries+static_cast<std::uint32_t>(count));Write(ctx,0x98,materialCount+static_cast<std::uint32_t>(count));*prefix+=static_cast<std::uint32_t>(count);
 result={};result.geometry=static_cast<std::uint32_t>(count);result.fastAppends=result.geometry+1;result.materialFastAppends=result.geometry;result.providerKind=Kind;result.avoidedCalls=result.geometry+3;result.reservedObjects=1;return true;
}
#undef RT_RESERVED_NOINLINE
#if defined(_MSC_VER)
__forceinline
#else
inline __attribute__((always_inline))
#endif
Result Assemble(const Api& api,void* context,const void* transform,void* materials,std::int32_t variant,
 void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 if(MatchesProvider(provider,api.staticMesh))return AssembleProvider<1,true>(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix);
 if(MatchesProvider(provider,api.skinnedMesh))return AssembleProvider<2,true>(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix);
 return AssembleProvider<0>(api,context,transform,materials,variant,provider,ignoreAlpha,disableCull,prefix);
}
// Reuse is scoped to one native instanced group. No readiness/update callback
// occurs inside that loop. Require a known static provider and enough existing
// output capacity for the WHOLE group, so replay invokes no native allocator.
inline bool TryAssembleBatch(const Api& api,void* context,const void* transforms,std::uint32_t instances,
 void* materials,void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix,Result& total){
 if(instances<2||!MatchesProvider(provider,api.staticMesh))return false;
 const auto geometry=GeometryCount<1>(provider);if(geometry<0)return false;
 const auto records=std::uint64_t(static_cast<std::uint32_t>(geometry))*instances;
 auto* ctx=static_cast<unsigned char*>(context);
 if(!FitsBatch(ctx+0x70,instances)||!FitsBatch(ctx+0x80,records)||!FitsBatch(ctx+0x90,records))return false;
 const auto instanceStart=Read<std::uint32_t>(ctx,0x78),geometryStart=Read<std::uint32_t>(ctx,0x88),materialStart=Read<std::uint32_t>(ctx,0x98);
 const auto first=AssembleProvider<1>(api,context,transforms,materials,0,provider,ignoreAlpha,disableCull,prefix);
 auto* instanceData=Read<unsigned char*>(ctx,0x70);
 auto* geometryData=Read<unsigned char*>(ctx,0x80);
 auto* materialData=Read<unsigned char*>(ctx,0x90);
 const auto* instanceTemplate=instanceData+std::size_t(instanceStart)*64;
 // No pointer is retained after this call. Output order and duplicated records
 // remain exactly native; downstream material-ID patching still visits each one.
 for(std::uint32_t i=1;i<instances;++i){
  const auto destination=std::size_t(i)*static_cast<std::uint32_t>(geometry);
  if(geometry){
   std::memcpy(geometryData+(geometryStart+destination)*64,geometryData+std::size_t(geometryStart)*64,std::size_t(geometry)*64);
   std::memcpy(materialData+(materialStart+destination)*8,materialData+std::size_t(materialStart)*8,std::size_t(geometry)*8);
  }
  auto* instance=instanceData+(std::size_t(instanceStart)+i)*64;std::memcpy(instance,instanceTemplate,64);
  const auto* transform=static_cast<const unsigned char*>(transforms)+std::size_t(i)*48;
  for(unsigned column=0;column<4;++column)for(unsigned row=0;row<3;++row)Write(instance,(row*4+column)*4,Read<std::uint32_t>(transform,(column*3+row)*4));
  Write(instance,52,(Read<std::uint32_t>(instance,52)&0xff000000u)|((*prefix*2u)&0xffffffu));
  *prefix+=static_cast<std::uint32_t>(geometry);
 }
 Write(ctx,0x78,instanceStart+instances);Write(ctx,0x88,geometryStart+static_cast<std::uint32_t>(records));Write(ctx,0x98,materialStart+static_cast<std::uint32_t>(records));
 total=first;total.reusedGeometry=static_cast<std::uint32_t>(geometry)*(instances-1);total.geometry*=instances;total.fastAppends*=instances;total.materialFastAppends*=instances;total.avoidedCalls*=instances;
 return true;
}

}
