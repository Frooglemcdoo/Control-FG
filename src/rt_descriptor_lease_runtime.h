#pragma once
#include "rt_owned_assembly.h"
#include "rt_descriptor_lease.h"
#include "rt_descriptor_lease_bytes.h"
#include "native_cpu_call_patch.h"
namespace RTDescriptorLease {
inline unsigned char* base=nullptr;inline NativeCpuCallPatch patch{};inline rt_descriptor_lease::Api api{};
inline thread_local rt_descriptor_lease::Lease* job=nullptr;
inline thread_local rt_descriptor_lease::Lease* application=nullptr;
inline thread_local bool allocating=false;
struct JobScope {rt_descriptor_lease::Lease local;rt_descriptor_lease::Lease* previous=job;JobScope(){job=&local;}~JobScope(){job=previous;}JobScope(const JobScope&)=delete;};
struct ApplyScope {rt_descriptor_lease::Lease* previous=application;ApplyScope(){application=job;}~ApplyScope(){application=previous;}ApplyScope(const ApplyScope&)=delete;};
inline std::atomic<UINT64> calls{0},hits{0},reservations{0};
inline rt_descriptor_lease::Snapshot Snapshot(){
 using rt_owned_assembly::Read;
 auto* manager=Read<void*>(base,0x111bb8);auto* frame=Read<void*>(base,0x136d28);
 if(!manager||!frame)return {};
 auto* timeline=Read<void*>(frame,0x88);if(!timeline)return {};
 auto** slots=reinterpret_cast<void**>(__readgsqword(0x58));auto* tls=slots[Read<std::uint32_t>(base,0x1115fc)];
 auto* state=Read<void*>(tls,0x20);if(!state)state=Read<void*>(base,0x111c10);
 return {manager,state,timeline,Read<std::uint64_t>(timeline),Read<std::uint64_t>(manager,8),Read<std::uint32_t>(base,0x111bc0)};
}
inline bool ValidBlock(const rt_descriptor_lease::Block& block,const rt_descriptor_lease::Snapshot& snapshot){
 using rt_owned_assembly::Read;
 return block.owner&&Read<std::uint64_t>(block.owner,0x20)==snapshot.epoch&&Read<std::uint64_t>(block.owner,0x30)==snapshot.version;
}
inline void Allocate(void* argument,std::uint64_t count,std::uint64_t* gpu,std::uint64_t* cpu,void** owner){
 if(!application||allocating){api.allocate(argument,count,gpu,cpu,owner);return;}
 struct Restore {~Restore(){allocating=false;}} restore;allocating=true;
 rt_descriptor_lease::Block output;const bool reused=rt_descriptor_lease::Take(api,*application,argument,count,output);
 *gpu=output.gpu;*cpu=output.cpu;*owner=output.owner;
 const DWORD error=GetLastError();const auto n=calls.fetch_add(1,std::memory_order_relaxed)+1;
 if(reused)hits.fetch_add(1,std::memory_order_relaxed);else reservations.fetch_add(1,std::memory_order_relaxed);
 if(n==1||n%16384==0)Log("RT_LEASE_R407 calls=%llu native_reservations=%llu leased_tables=%llu max_tables_per_reservation=16 table_descriptors=49 job_scoped=1 descriptor_reuse=0",n,reservations.load(),hits.load());
 SetLastError(error);
}
inline bool Install(HMODULE d3d)noexcept{
 if(!d3d||d3d!=verifiedD3d||!HashMatches(d3d,kD3dHash))return false;
 base=reinterpret_cast<unsigned char*>(d3d);
 if(std::memcmp(base+0x1bd50,rt_lease_bytes::Prepare,sizeof(rt_lease_bytes::Prepare))||std::memcmp(base+0x1aa00,rt_lease_bytes::Allocate,sizeof(rt_lease_bytes::Allocate)))return false;
 api={reinterpret_cast<decltype(api.allocate)>(base+0x1aa00),Snapshot,ValidBlock};
 if(!NativeCpuPrepareCallPatch(base+0x1bd85,base+0x1aa00,reinterpret_cast<void*>(&Allocate),&patch))return false;
 if(NativeCpuExchangeCall(&patch,true)&&patch.writeHealthy)return true;
 if(!std::memcmp(patch.site,patch.replacement,5))NativeCpuExchangeCall(&patch,false);return false;
}
}
