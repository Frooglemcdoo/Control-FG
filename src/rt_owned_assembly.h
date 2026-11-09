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
struct Api {void(*append64)(void*,const void*);void(*append8)(void*,const void*);};
struct Result {std::uint32_t geometry=0,fastAppends=0,growthAppends=0;};
inline void Append64(const Api& api,void* vector,const void* source,Result& result){
 const auto count=Read<std::uint32_t>(vector,8),capacity=Read<std::uint32_t>(vector,12)&0x3fffffffu;
 if(count>=capacity){api.append64(vector,source);++result.growthAppends;return;}
 auto* data=Read<unsigned char*>(vector);Write(vector,8,count+1);std::memcpy(data+std::size_t(count)*64,source,64);++result.fastAppends;
}
template<class F>inline F Method(void* object,std::size_t offset)noexcept{return Read<F>(Read<void*>(object),offset);}
inline Result Assemble(const Api& api,void* context,const void* transform,void* materials,std::int32_t variant,
                       void* provider,unsigned char ignoreAlpha,unsigned char disableCull,std::uint32_t* prefix){
 Result result;auto* ctx=static_cast<unsigned char*>(context);
 const auto count=Method<std::int32_t(*)(void*)>(provider,0x48)(provider);
 bool alpha=false,cull=disableCull!=0;
 for(std::int32_t index=0;index<count;++index){
  const auto* geometry=Method<const void*(*)(void*,std::int32_t)>(provider,0x50)(provider,index);
  Append64(api,ctx+0x80,geometry,result);
  const auto used=Read<std::uint32_t>(ctx,0x88);auto* record=Read<unsigned char*>(ctx,0x80)+std::size_t(used-1)*64;
  const auto materialIndex=Read<std::int32_t>(record);
  void* table;
  if(variant!=0&&Read<std::uint32_t>(materials,0x158)!=0){
   auto* variants=Read<unsigned char*>(materials,0x150);
   table=Read<void*>(variants+(std::ptrdiff_t(variant)*3-2)*8);
  }else table=Read<void*>(materials,0x130);
  auto* material=Read<void*>(static_cast<unsigned char*>(table)+std::ptrdiff_t(materialIndex)*8);
  api.append8(ctx+0x90,&material);
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
 const auto mask=Method<std::uint32_t(*)(void*)>(provider,0x60)(provider);
 instance[12]=mask<<24;instance[13]=(*prefix*2u)&0xffffffu;
 if(!alpha){instance[12]|=0x04000000u;instance[13]|=0x04000000u;}
 if(cull)instance[13]|=0x01000000u;
 const auto address=Method<std::uint64_t(*)(void*)>(provider,0x40)(provider);std::memcpy(instance+14,&address,8);
 Append64(api,ctx+0x70,instance,result);*prefix+=static_cast<std::uint32_t>(count);result.geometry=static_cast<std::uint32_t>(count);return result;
}
}
