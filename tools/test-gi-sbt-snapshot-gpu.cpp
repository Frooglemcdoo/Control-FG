#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_sbt_snapshot.h"
#include "../src/gi_reuse/d3d12_native_sbt_source.h"
#include "../src/gi_reuse/d3d12_frozen_raygen.h"
int main()try {
 using namespace gi_reuse;Device d(false);std::vector<unsigned char> bytes(1024);
 for(UINT i=0;i<bytes.size();++i)bytes[i]=static_cast<unsigned char>(i*73+19);
 auto source=d.Upload(bytes);const auto base=source->GetGPUVirtualAddress();
 D3D12_DISPATCH_RAYS_DESC native{};native.Width=2560;native.Height=1440;native.Depth=1;
 native.RayGenerationShaderRecord={base,32};native.MissShaderTable={base+64,32,32};native.HitGroupTable={base+128,384,96};
 D3D12GISbtSnapshot copy;auto wrong=native;wrong.HitGroupTable.SizeInBytes=1056;
 Require(FAILED(copy.Initialize(d.device.Get(),source.Get(),wrong)),"out of range accepted");
 wrong=native;wrong.HitGroupTable.StrideInBytes=64;Require(FAILED(copy.Initialize(d.device.Get(),source.Get(),wrong)),"wrong ABI accepted");
 Check(copy.Initialize(d.device.Get(),source.Get(),native));Require(!copy.Dispatch()&&!copy.Resource(),"unrecorded snapshot exposed");
 Require(copy.Record(d.list.Get())&&!copy.Record(d.list.Get()),"snapshot repeated");
 const auto* desc=copy.Dispatch();Require(desc&&desc->Width==native.Width&&desc->Height==native.Height&&copy.Bytes()==512,"copy extent/shape changed");
 Require(desc->HitGroupTable.StartAddress-desc->RayGenerationShaderRecord.StartAddress==128&&desc->MissShaderTable.StartAddress-desc->RayGenerationShaderRecord.StartAddress==64,"table offsets changed");
 // Native Control uses GPU-local SBT storage, with GPU upload already ordered.
 auto local=d.Buffer(bytes.size(),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyResource(local.Get(),source.Get());d.Transition(local.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 auto localDesc=native;const auto localBase=local->GetGPUVirtualAddress();
 localDesc.RayGenerationShaderRecord.StartAddress=localBase;localDesc.MissShaderTable.StartAddress=localBase+64;localDesc.HitGroupTable.StartAddress=localBase+128;
 std::vector<unsigned char> wrapper(0x50);auto* resource=local.Get();const UINT64 capacity=bytes.size();
 std::memcpy(wrapper.data(),&resource,8);std::memcpy(wrapper.data()+0x30,&capacity,8);std::memcpy(wrapper.data()+0x48,&localBase,8);
 ComPtr<ID3D12Resource> acquired;
 Require(AcquireNativeGISbtSource(wrapper.data(),localBase,capacity,acquired.GetAddressOf())&&acquired.Get()==local.Get(),"native wrapper source acquisition failed");
 ID3D12Resource* rejected=nullptr;
 Require(!AcquireNativeGISbtSource(wrapper.data(),localBase+64,capacity,&rejected)&&!rejected,"wrong wrapper identity admitted");
 Require(!AcquireNativeGISbtSource(wrapper.data(),localBase,capacity+256,&rejected),"wrong native capacity admitted");
 D3D12GISbtSnapshot localCopy;
 Require(FAILED(localCopy.Initialize(d.device.Get(),local.Get(),localDesc)),"DEFAULT state guessed as GENERIC_READ");
 Check(localCopy.Initialize(d.device.Get(),local.Get(),localDesc,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
 Require(localCopy.Record(d.list.Get()),"GPU-local SBT snapshot failed");
 auto frozenCode=Read("gi-frozen-raygen.dxil");D3D12FrozenRaygenAugment augment;
 Check(augment.Initialize(d.device.Get(),{frozenCode.data(),frozenCode.size()}));
 std::vector<D3D12_STATE_SUBOBJECT> objects;objects.reserve(3);Require(augment.Append(objects)&&objects.size()==3,"raygen augmentation failed");
 const auto* association=static_cast<const D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION*>(objects[2].pDesc);
 Require(association->pSubobjectToAssociate==objects.data()+1&&association->NumExports==1&&!wcscmp(association->pExports[0],D3D12FrozenRaygenAugment::Export),"frozen local-root association wrong");
 const auto* library=static_cast<const D3D12_DXIL_LIBRARY_DESC*>(objects[0].pDesc);
 Require(library->NumExports==1&&!wcscmp(library->pExports[0].ExportToRename,L"giFrozenRaygen"),"frozen export alias wrong");
 D3D12FrozenRaygenRecords records;
 Require(FAILED(records.Initialize(d.device.Get(),bytes.data(),base,localBase,2559,1440,100,7,65536,0,.125f)),"odd frozen width admitted");
 Check(records.Initialize(d.device.Get(),bytes.data(),base,localBase,2560,1440,100,7,65536,0,.125f));
 Require(records.Matches(2560,1440,100,7,65536,0,base,localBase)&&!records.Matches(2560,1440,101,7,65536,0,base,localBase),"stale frozen identity admitted");
 Require(records.At(0).SizeInBytes==96&&records.At(1).StartAddress-records.At(0).StartAddress==128&&!records.At(2).StartAddress,"frozen record stride wrong");
 Require(records.NativeComplement().StartAddress-records.At(0).StartAddress==256,"complement record missing");
 auto recordBack=d.Buffer(D3D12FrozenRaygenRecords::Bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);d.list->CopyResource(recordBack.Get(),records.Resource());
 d.Submit();
 // Simulate native upload-buffer reuse AFTER the original recording completed.
 void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(source->Map(0,&empty,&mapped));std::memset(mapped,0xee,1024);source->Unmap(0,nullptr);
 Check(d.allocator->Reset());Check(d.list->Reset(d.allocator.Get(),nullptr));Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(d.fence.ReleaseAndGetAddressOf())));
 // Reuse native DEFAULT table too, after its original submission completed.
 d.Transition(local.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyResource(local.Get(),source.Get());
 d.Transition(local.Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
 auto localBack=d.Buffer(localCopy.Bytes(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(localCopy.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyBufferRegion(localBack.Get(),0,localCopy.Resource(),0,localCopy.Bytes());
 auto back=d.Buffer(copy.Bytes(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(copy.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyBufferRegion(back.Get(),0,copy.Resource(),0,copy.Bytes());d.Submit();
 D3D12_RANGE range{0,static_cast<SIZE_T>(copy.Bytes())};Check(back->Map(0,&range,&mapped));
 Require(!std::memcmp(mapped,bytes.data(),static_cast<size_t>(copy.Bytes())),"SBT changed when native upload reused");back->Unmap(0,&empty);
 Check(localBack->Map(0,nullptr,&mapped));Require(!std::memcmp(mapped,bytes.data(),static_cast<size_t>(localCopy.Bytes())),"SBT changed when native DEFAULT reused");localBack->Unmap(0,&empty);
 Check(recordBack->Map(0,nullptr,&mapped));const auto* recordBytes=static_cast<const unsigned char*>(mapped);
 for(UINT phase=0;phase<3;++phase){const auto* record=recordBytes+phase*128;Require(!memcmp(record,bytes.data(),32),"frozen identifier changed");
  UINT64 address=0;std::memcpy(&address,record+32,8);Require(address==base,"frozen guide root wrong");std::memcpy(&address,record+40,8);Require(address==localBase,"frozen weight root wrong");
  UINT control[8]{};std::memcpy(control,record+48,32);float fraction=0;std::memcpy(&fraction,record+76,4);
  Require(control[0]==2560&&control[1]==1440&&control[2]==100&&control[3]==7&&control[4]==65536&&control[5]==(phase?1u:0u)&&control[6]==(phase==1?1u:0u)&&fraction==.125f,"frozen phase constants wrong");
  for(UINT i=80;i<128;++i)Require(record[i]==0,"frozen record padding nonzero");
 }recordBack->Unmap(0,&empty);
 std::cout<<"PASS GPU SBT snapshot: native DEFAULT/UPLOAD reuse and explicit source-state restoration, ABI/range rejection, preserved offsets/bytes, one-shot recording. Referenced geometry lifetime remains external.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
