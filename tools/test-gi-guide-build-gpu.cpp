#include "gpu_test_device.h"
#include "../src/gi_reuse/frozen_guide_training.h"
#include "../src/gi_reuse/d3d12_frozen_guide_build.h"
int main()try {
 using namespace gi_reuse;Device d(false);constexpr UINT slots=16;
 GIWorldGuideKey a{0,0,0,7,1},b{1,0,0,7,1},c{2,0,0,7,1};
 while(GITrainingHash(b)%slots==GITrainingHash(a)%slots)++b.x;
 while(GITrainingHash(c)%slots==GITrainingHash(a)%slots||GITrainingHash(c)%slots==GITrainingHash(b)%slots)++c.x;
 auto collision=a;++collision.x;while(GITrainingHash(collision)%slots!=GITrainingHash(a)%slots)++collision.x;
 std::vector<GITrainingRecord> records;
 for(UINT i=0;i<1024;++i){
  auto key=i<512?a:i<767?b:c;
  const UINT bin=i%64;const float phi=6.28318530718f*(float(bin%8)+.5f)/8;
  const float z=2*(float(bin/8)+.5f)/8-1,rad=std::sqrt(1-z*z);
  records.push_back({key,{rad*std::cos(phi),rad*std::sin(phi),z},1.0f});
 }
 auto input=d.Upload(records);std::vector<GITrainingRecord> collisionRecords{{collision,{0,0,1},100000}};auto colliding=d.Upload(collisionRecords);
 std::vector<GITrainingRecord> invalidRecords{{c,{0,0,1},-1}};auto invalid=d.Upload(invalidRecords);
 auto cells=d.Buffer(slots*sizeof(GITrainingCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto frozen=d.Buffer(slots*sizeof(GIKeyedGuide),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 const UINT bytes=slots*(sizeof(GITrainingCell)+sizeof(GIKeyedGuide));auto back=d.Buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER params[4]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[0].Descriptor={0,0};
 params[1].ParameterType=params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};params[2].Descriptor={1,0};
 params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[3].Constants={0,0,4};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=4;rd.pParameters=params;
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 ComPtr<ID3D12PipelineState> clear,claim,accumulate,freeze;
 const char* names[]={"gi-guide-Clear.dxil","gi-guide-Claim.dxil","gi-guide-Accumulate.dxil","gi-guide-Freeze.dxil"};ID3D12PipelineState** out[]={clear.GetAddressOf(),claim.GetAddressOf(),accumulate.GetAddressOf(),freeze.GetAddressOf()};
 for(UINT i=0;i<4;++i){auto code=Read(names[i]);D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};Check(d.device->CreateComputePipelineState(&pd,__uuidof(ID3D12PipelineState),reinterpret_cast<void**>(out[i])));}
 d.list->SetComputeRootSignature(root.Get());d.list->SetComputeRootShaderResourceView(0,input->GetGPUVirtualAddress());
 d.list->SetComputeRootUnorderedAccessView(1,cells->GetGPUVirtualAddress());d.list->SetComputeRootUnorderedAccessView(2,frozen->GetGPUVirtualAddress());
 UINT control[]={1024,slots,100,7};d.list->SetComputeRoot32BitConstants(3,4,control,0);
 d.list->SetPipelineState(clear.Get());d.list->Dispatch(1,1,1);d.Uav(cells.Get());d.Uav(frozen.Get());
 d.list->SetPipelineState(claim.Get());d.list->Dispatch(16,1,1);d.Uav(cells.Get());
 d.list->SetPipelineState(accumulate.Get());d.list->Dispatch(16,1,1);d.Uav(cells.Get());
 // A later colliding key must not contaminate the original cell's energy/key.
 d.list->SetComputeRootShaderResourceView(0,colliding->GetGPUVirtualAddress());control[0]=1;d.list->SetComputeRoot32BitConstants(3,4,control,0);
 d.list->Dispatch(1,1,1);d.Uav(cells.Get());
 d.list->SetComputeRootShaderResourceView(0,invalid->GetGPUVirtualAddress());d.list->Dispatch(1,1,1);d.Uav(cells.Get());
 d.list->SetPipelineState(freeze.Get());d.list->Dispatch(1,1,1);
 // Exercise the production four-stage recorder, not only manually issued stages.
 std::array<std::vector<char>,4> buildCode;std::array<D3D12_SHADER_BYTECODE,4> buildShaders;
 for(UINT i=0;i<4;++i){buildCode[i]=Read(names[i]);buildShaders[i]={buildCode[i].data(),buildCode[i].size()};}
 D3D12FrozenGuideBuild builder;Check(builder.Initialize(d.device.Get(),buildShaders));
 auto ownedCells=d.Buffer(slots*sizeof(GITrainingCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto ownedGuides=d.Buffer(slots*sizeof(GIKeyedGuide),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto ownedRead=d.Buffer(slots*sizeof(GIKeyedGuide),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 ComputeBindingTracker tracker;tracker.Root(root.Get());tracker.Pipeline(freeze.Get());
 tracker.Descriptor(0,RootArgumentKind::SRV,invalid->GetGPUVirtualAddress());tracker.Descriptor(1,RootArgumentKind::UAV,cells->GetGPUVirtualAddress());
 tracker.Descriptor(2,RootArgumentKind::UAV,frozen->GetGPUVirtualAddress());tracker.Constants(3,0,4,control);
 ComputeBindingSnapshot restore;Require(tracker.Capture(restore),"builder restore snapshot failed");
 Require(!builder.Record(d.list.Get(),input.Get(),1024,ownedCells.Get(),ownedCells.Get(),slots,101,8,restore),"aliased builder outputs admitted");
 Require(!builder.Record(d.list.Get(),input.Get(),2048,ownedCells.Get(),ownedGuides.Get(),slots,101,8,restore),"short training buffer admitted");
 Require(builder.Record(d.list.Get(),input.Get(),1024,ownedCells.Get(),ownedGuides.Get(),slots,101,8,restore),"production builder failed");
 d.Transition(ownedGuides.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyResource(ownedRead.Get(),ownedGuides.Get());
 d.Transition(cells.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.Transition(frozen.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);
 d.list->CopyBufferRegion(back.Get(),0,cells.Get(),0,slots*sizeof(GITrainingCell));d.list->CopyBufferRegion(back.Get(),slots*sizeof(GITrainingCell),frozen.Get(),0,slots*sizeof(GIKeyedGuide));d.Submit();
 void* mapped=nullptr;D3D12_RANGE range{0,bytes};Check(back->Map(0,&range,&mapped));
 auto* bins=static_cast<GITrainingCell*>(mapped);auto* guides=reinterpret_cast<GIKeyedGuide*>(static_cast<unsigned char*>(mapped)+slots*sizeof(GITrainingCell));
 auto ai=GITrainingHash(a)%slots,bi=GITrainingHash(b)%slots,ci=GITrainingHash(c)%slots;
 std::cout<<"training cell: state="<<bins[ai].state<<" count="<<bins[ai].count<<" poison="<<bins[ai].poison<<" valid="<<guides[ai].guide.valid<<"\n";
 Require(guides[ai].guide.valid==1&&GIWorldGuideKeysMatch(guides[ai].key,a),"Training guide unavailable or wrong key");
 float energy[64],total=0;for(UINT i=0;i<64;++i){std::memcpy(&energy[i],&bins[ai].energy[i],4);total+=energy[i];}
 Require(total==float(bins[ai].count)&&total>=256&&total<=512,"Training count/energy mismatch or collision contamination");
 auto ref=GIFreezeWorldGuide(energy,bins[ai].count,100,7);for(UINT i=0;i<65;++i)Require(std::abs(ref.cdf[i]-guides[ai].guide.cdf[i])<.000001f,"Frozen CDF mismatch");
 Require(!guides[bi].guide.valid&&bins[bi].count<=255,"Insufficient training accepted");
 Require(bins[ci].poison==1&&!guides[ci].guide.valid,"Invalid training not rejected");
 auto native=GISampleKeyedFrozenWorld(guides[ai].guide,guides[ai].key,collision,{0,0,1},100,7,.125f,.3f,.8f);
 Require(native.valid&&native.weight==1,"Collision failed native pre-sample fallback");
 D3D12_RANGE empty{0,0};back->Unmap(0,&empty);
 GIKeyedGuide* actual=nullptr;Check(ownedRead->Map(0,nullptr,reinterpret_cast<void**>(&actual)));
 Require(actual[ai].guide.valid&&actual[ai].guide.frame==101&&actual[ai].guide.epoch==8,"production builder did not freeze current identity");
 Require(!actual[bi].guide.valid&&actual[ci].guide.valid,"production builder count validation failed");
 Require(GIWorldGuideKeysMatch(actual[ai].key,a)&&actual[ai].guide.samples<=512&&actual[ai].guide.samples>=256,"production builder lost key/count");
 ownedRead->Unmap(0,&empty);
 std::cout<<"PASS GPU training: atomic energy/count, frozen CPU/HLSL CDF, full-key collision isolation, insufficient/invalid native fallback. Not live or hardware timing.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
