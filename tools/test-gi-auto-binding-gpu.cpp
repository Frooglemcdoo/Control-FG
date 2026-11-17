#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_target_binding.h"
#include "../src/gi_reuse/d3d12_snapshot_retirement.h"
#include <limits>
int main()try {
 using namespace gi_reuse;Device d(false);
 GIAutoTargets t{};t.schema=0x41544731;t.version=7;t.frame=12;t.validMask=1;
 t.bounds[0]={{1,2,3},.5f};t.counts[0]=42;t.scores[0]=123;
 GIAutoSelection s{7,12,10,.1f};Require(GIAutoUsable(t,s),"valid selection");
 auto invalid=s;invalid.fraction=.41f;Require(!GIAutoSelectionValid(invalid),"balance mixture bound");
 invalid=s;invalid.rayCount=1;Require(!GIAutoSelectionValid(invalid),"one ray admitted");
 invalid=s;invalid.fraction=std::numeric_limits<float>::quiet_NaN();Require(!GIAutoSelectionValid(invalid),"NaN fraction");
 auto bad=t;bad.bounds[0].radius=-1;Require(!GIAutoUsable(bad,s),"invalid bounds admitted");
 auto target=d.Upload(std::vector<GIAutoTargets>{t});
 D3D12AutoTargetBinding untracked;Require(!untracked.Initialize(target.Get(),s),"unproven storage admitted");
 Require(RetainCreatedStorage(target.Get()),"storage registration");
 struct Invocation {std::array<std::unique_ptr<D3D12AutoTargetBinding>,3> bindings;};
 auto owner=std::make_unique<Invocation>();
 std::array<std::array<unsigned char,64>,3> records{};std::array<unsigned char,32> identifier{};identifier.fill(0x65);
 for(UINT i=0;i<3;++i){auto select=s;if(i==1)++select.sourceFrame;if(i==2)++select.version;
  auto& binding=owner->bindings[i];binding=std::make_unique<D3D12AutoTargetBinding>();
  Require(binding->Initialize(target.Get(),select),"binding initialize");
  Require(!binding->Initialize(target.Get(),s),"immutable selection overwritten");
  Require(binding->EncodeRaygen(identifier.data(),records[i]),"encode raygen");
  UINT64 address=0;GIAutoSelection decoded{};std::memcpy(&address,records[i].data()+32,8);std::memcpy(&decoded,records[i].data()+40,16);
  Require(address==target->GetGPUVirtualAddress()&&std::memcmp(&decoded,&select,16)==0,"raygen binding identity");
  for(UINT j=56;j<64;++j)Require(records[i][j]==0,"shader record padding");
 }
 auto* invocation=owner.get();using Retirement=D3D12SnapshotRetirement<Invocation,8,2,4096>;Retirement retirement;
 const auto id=retirement.Admit(101,3*sizeof(GIAutoTargets),owner);Require(id&&!owner,"retirement admission");
 Retirement::Recording recording{id,101};std::unique_ptr<Invocation> released;
 Require(!retirement.Take(id,released),"unsealed recording retired");
 auto params=D3D12AutoTargetBinding::Parameters();D3D12_ROOT_SIGNATURE_DESC rd{2,params.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE};
 ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> localRoot;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&localRoot)));
 std::array<D3D12_ROOT_PARAMETER,3> global{params[0],params[1],{}};global[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;global[2].Descriptor={0,0};
 rd={3,global.data(),0,nullptr,D3D12_ROOT_SIGNATURE_FLAG_NONE};blob.Reset();error.Reset();Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 auto code=Read("gi-auto-binding.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS={code.data(),code.size()};ComPtr<ID3D12PipelineState> pso;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&pso)));
 auto output=d.Buffer(96,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);auto read=d.Buffer(96,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->SetComputeRootSignature(root.Get());d.list->SetPipelineState(pso.Get());
 for(UINT i=0;i<3;++i){Require(invocation->bindings[i]->BindEvaluator(d.list.Get(),0,1),"evaluator bind");
  d.list->SetComputeRootUnorderedAccessView(2,output->GetGPUVirtualAddress()+32*i);d.list->Dispatch(1,1,1);}
 d.Transition(output.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyResource(read.Get(),output.Get());
 // Drop all external target references BEFORE submission. The retirement owner
 // now owns the only target leases; fixture storage must not mask a lifetime bug.
 target.Reset();d.retained.clear();
 auto batch=retirement.Before(d.queue.Get(),&recording,1);Require(batch.count==1,"submission lease");
 Require(!retirement.Take(id,released),"pending submission retired");d.Submit();Require(retirement.After(batch),"completion signal");
 Require(!retirement.Take(id,released),"replay-eligible recording retired");
 // Successful Reset ends replay eligibility; Close alone must not seal it.
 Check(d.list->Reset(d.allocator.Get(),nullptr));Require(retirement.Seal(recording),"seal reset generation");
 // Test-only bounded wait. Production retirement uses nonblocking Poll().
 Check(d.queue->Signal(d.fence.Get(),2));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
 Check(d.fence->SetEventOnCompletion(2,event));auto wait=WaitForSingleObject(event,15000);CloseHandle(event);Require(wait==WAIT_OBJECT_0,"retirement timeout");
 retirement.Poll();Require(retirement.Take(id,released)&&retirement.RetainedBytes()==0,"completed lease not retired");
 UINT* result=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&result)));
 for(UINT i=0;i<3;++i){Require(result[i*8]==(i==0?1u:0u),"GPU validity differs");
  Require(result[i*8+1]==7&&result[i*8+2]==12,"GPU target differs");
  Require(std::memcmp(result+i*8+4,records[i].data()+40,16)==0,"producer/evaluator selection differs");}
 D3D12_RANGE empty{};read->Unmap(0,&empty);
 std::cout<<"PASS immutable target selection, local-root serialization, ray-record layout, GPU evaluator identity, stale rejection and completion retirement. DXR dispatch and live hooks not tested.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
