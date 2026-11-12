#pragma once
// Experimental backend, used by the standalone GPU validation target only.
// Not installed in Control's hooks until native preparation/consumers are owned.
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <nvapi.h>
#include "rt_ptlas_plan.h"
#include <cstddef>
#include <string>

namespace rt_ptlas {
using Microsoft::WRL::ComPtr;
inline void Check(HRESULT hr,const char* operation){if(FAILED(hr))throw std::runtime_error(std::string(operation)+" HRESULT="+std::to_string(static_cast<unsigned long>(hr)));}
inline void CheckNv(NvAPI_Status s,const char* operation){if(s!=NVAPI_OK)throw std::runtime_error(std::string(operation)+" NVAPI="+std::to_string(s));}
inline ComPtr<ID3D12Resource> Buffer(ID3D12Device* device,std::uint64_t bytes,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags=D3D12_RESOURCE_FLAG_NONE){
 if(!bytes)throw std::invalid_argument("zero GPU buffer");
 D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;
 D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=bytes;
 desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;desc.Flags=flags;
 ComPtr<ID3D12Resource> out;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&out)),"Create buffer");return out;
}
inline void UAVBarrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource){D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_UAV;b.UAV.pResource=resource;list->ResourceBarrier(1,&b);}

class NvBackend {
 ComPtr<ID3D12Device5> device_;
 ComPtr<ID3D12Resource> result_,scratch_;
 NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_INPUTS inputs_{};
 bool initialized_=false;
public:
 std::uint64_t recordedBuilds=0;
 NvBackend(ID3D12Device5* device,std::uint32_t capacity,std::uint32_t partitions):device_(device){
  CheckNv(NvAPI_Initialize(),"NvAPI initialize");
  NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAPS caps=NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_NONE;
  CheckNv(NvAPI_D3D12_GetRaytracingCaps(device,NVAPI_D3D12_RAYTRACING_CAPS_TYPE_PARTITIONED_TLAS,&caps,sizeof(caps)),"PTLAS capability");
  if(!(caps&NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_STANDARD))throw std::runtime_error("PTLAS unsupported");
  inputs_.flags=NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_FAST_TRACE;
  inputs_.instanceCount=capacity;inputs_.partitionCount=partitions;
  inputs_.maxInstancePerPartitionCount=capacity;inputs_.maxInstanceInGlobalPartitionCount=capacity;
  NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO sizes{};
  NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS query{};
  query.version=NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS_VER;query.pInput=&inputs_;query.pInfo=&sizes;
  CheckNv(NvAPI_D3D12_GetRaytracingPartitionedTlasIndirectPrebuildInfo(device,&query),"PTLAS prebuild");
  result_=Buffer(device,sizes.resultDataMaxSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
  scratch_=Buffer(device,sizes.scratchDataSizeInBytes,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
 }
 D3D12_GPU_VIRTUAL_ADDRESS Address()const{return result_->GetGPUVirtualAddress();}
 // Caller MUST retain returned upload through command-list execution and fence.
 // On an NVAPI recording failure, discard the entire command list; never submit
 // a partial replacement and retry it through the native builder.
 ComPtr<ID3D12Resource> Record(ID3D12GraphicsCommandList4* list,const Plan& plan){
  static_assert(sizeof(Write)==sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE));
  static_assert(offsetof(Write,blas)==offsetof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE,accelerationStructure));
  static_assert(offsetof(Write,index)==offsetof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE,instanceIndex));
  if(plan.initialize==initialized_)throw std::logic_error("PTLAS initialization order");
  if(plan.writes.empty())return {};
  const std::uint64_t dataBytes=plan.writes.size()*sizeof(Write);
  const std::uint64_t opOffset=(dataBytes+7)&~std::uint64_t(7);
  const auto countOffset=opOffset+sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP);
  auto upload=Buffer(device_.Get(),countOffset+4,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);
  NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP op{};op.type=NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_WRITE_INSTANCE;
  op.count=static_cast<NvU32>(plan.writes.size());op.data.StartAddress=upload->GetGPUVirtualAddress();op.data.StrideInBytes=sizeof(Write);
  void* map=nullptr;D3D12_RANGE noRead{0,0};Check(upload->Map(0,&noRead,&map),"PTLAS upload map");
  std::memcpy(map,plan.writes.data(),dataBytes);std::memcpy(static_cast<char*>(map)+opOffset,&op,sizeof(op));
  const NvU32 count=1;std::memcpy(static_cast<char*>(map)+countOffset,&count,4);upload->Unmap(0,nullptr);
  NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_DESC desc{};desc.inputs=inputs_;
  desc.srcAccelerationStructureData=initialized_?Address():0;desc.destAccelerationStructureData=Address();
  desc.scratchAccelerationStructureData=scratch_->GetGPUVirtualAddress();desc.indirectOps=upload->GetGPUVirtualAddress()+opOffset;
  desc.indirectOpCount=upload->GetGPUVirtualAddress()+countOffset;
  NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS params{};params.version=NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS_VER;params.pDesc=&desc;
  if(initialized_){UAVBarrier(list,result_.Get());UAVBarrier(list,scratch_.Get());}
  CheckNv(NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect(list,&params),"PTLAS record");
  UAVBarrier(list,result_.Get());++recordedBuilds;return upload;
 }
 // Queued caller must establish successful Execute+Signal on the bound queue
 // through SubmissionJournal, retain uploads/dependencies and forbid replay.
 // This advances recording state only; it is NOT a GPU-completion/retirement signal.
 void Submitted(){initialized_=true;}
 // Existing serialized validation path remains supported.
 void Complete(){initialized_=true;}
};
}
