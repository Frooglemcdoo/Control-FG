#pragma once
#include "auto_target_binding.h"
#include "d3d12_storage_lease.h"
#include <array>
#include <cstring>

namespace gi_reuse {
// One per-recording frozen selection. Host admits this owner to submission
// retirement BEFORE recording commands. Both stages borrow THIS owner, never a
// global latest-target pointer. Retention does not establish queue dependencies,
// immutable GPU contents, resource state, residency, or recording replay safety.
class D3D12AutoTargetBinding {
 BufferStorageLease targets_;
 GIAutoSelection selection_{};
public:
 static constexpr UINT RegisterSpace=19,RecordBytes=64;
 D3D12AutoTargetBinding()=default;
 D3D12AutoTargetBinding(const D3D12AutoTargetBinding&)=delete;
 D3D12AutoTargetBinding& operator=(const D3D12AutoTargetBinding&)=delete;
 bool Initialize(ID3D12Resource* targets,GIAutoSelection selection) noexcept {
  if(targets_.resource||!GIAutoSelectionValid(selection))return false;
  BufferStorageLease lease;
  if(!AcquireStorageLease(targets,lease)||lease.bytes<GIAutoPublicationBytes||
     lease.address%8||(targets->GetDesc().Flags&D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE))return false;
  targets_=std::move(lease);selection_=selection;return true;
 }
 // Root parameter layouts for a private raygen LOCAL root and evaluator GLOBAL
 // extension are identical. Caller must audit register-space collisions and root
 // DWORD budget, and associate the local root with only the new raygen export.
 static std::array<D3D12_ROOT_PARAMETER,2> Parameters() noexcept {
  std::array<D3D12_ROOT_PARAMETER,2> p{};
  p[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;p[0].Descriptor={0,RegisterSpace};
  p[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;p[1].Constants={0,RegisterSpace,5};
  return p;
 }
 // Upload/record ownership belongs to the invocation, not this CPU byte array.
 // Identifier MUST come from the same augmented state object being dispatched.
 bool EncodeRaygen(const void* identifier,std::array<unsigned char,RecordBytes>& record)const noexcept {
  record={};if(!targets_.resource||!identifier)return false;
  std::memcpy(record.data(),identifier,D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
  std::memcpy(record.data()+32,&targets_.address,8);
  std::memcpy(record.data()+40,&selection_,sizeof(selection_));return true;
 }
 // All setup/admission must succeed BEFORE ray generation. Once guided rays
 // exist, the host cannot select an ordinary evaluator as an error fallback.
 bool BindEvaluator(ID3D12GraphicsCommandList* list,UINT srvSlot,UINT constantsSlot)const noexcept {
  if(!targets_.resource||!list||srvSlot>=64||constantsSlot>=64||srvSlot==constantsSlot)return false;
  list->SetComputeRootShaderResourceView(srvSlot,targets_.address);
  list->SetComputeRoot32BitConstants(constantsSlot,5,&selection_,0);return true;
 }
 const GIAutoSelection& Selection()const noexcept{return selection_;}
 UINT64 Address()const noexcept{return targets_.address;}
 UINT64 RetainedBytes()const noexcept{return targets_.bytes;}
 ID3D12Resource* Resource()const noexcept{return targets_.resource.Get();}
};
}
