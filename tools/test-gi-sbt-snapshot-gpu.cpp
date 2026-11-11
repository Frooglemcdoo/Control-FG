#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_sbt_snapshot.h"
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
 d.Submit();
 // Simulate native upload-buffer reuse AFTER the original recording completed.
 void* mapped=nullptr;D3D12_RANGE empty{0,0};Check(source->Map(0,&empty,&mapped));std::memset(mapped,0xee,1024);source->Unmap(0,nullptr);
 Check(d.allocator->Reset());Check(d.list->Reset(d.allocator.Get(),nullptr));Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(d.fence.ReleaseAndGetAddressOf())));
 auto back=d.Buffer(copy.Bytes(),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.Transition(copy.Resource(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyBufferRegion(back.Get(),0,copy.Resource(),0,copy.Bytes());d.Submit();
 D3D12_RANGE range{0,static_cast<SIZE_T>(copy.Bytes())};Check(back->Map(0,&range,&mapped));
 Require(!std::memcmp(mapped,bytes.data(),static_cast<size_t>(copy.Bytes())),"SBT changed when native upload reused");back->Unmap(0,&empty);
 std::cout<<"PASS GPU SBT snapshot: native upload reuse, ABI/range rejection, preserved offsets/bytes, one-shot recording. Referenced geometry lifetime remains external.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
