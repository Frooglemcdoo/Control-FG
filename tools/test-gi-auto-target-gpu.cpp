#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_auto_target_build.h"
#include <limits>
#include "../src/gi_reuse/pilot_guide_resolve.h"
int main()try {
 using namespace gi_reuse;Device d(false);constexpr UINT slots=256;
 std::vector<GIAutoTraining> records;
 GIAutoTraining a{{.2f,.2f,3.2f},2,1,7,0,0},b{{1.2f,.2f,3.2f},1,1,7,0,0};
 for(UINT i=0;i<400;++i)records.push_back(i<200?a:b);
 auto bad=a;bad.version=6;bad.energy=10000;for(UINT i=0;i<50;++i)records.push_back(bad);
 bad=a;bad.energy=std::numeric_limits<float>::quiet_NaN();records.push_back(bad);
 bad=a;bad.position.x=std::numeric_limits<float>::infinity();records.push_back(bad);
 bad=a;bad.energy=-1;records.push_back(bad);
 auto input=d.Upload(records);auto empty=d.Upload(std::vector<GIAutoTraining>(1));
 auto cells=d.Buffer(slots*sizeof(GIAutoCell),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto frozen=d.Buffer(sizeof(GIAutoTargets),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto back=d.Buffer(7*sizeof(GIAutoTargets),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 std::array<std::vector<char>,4> code;std::array<D3D12_SHADER_BYTECODE,4> shaders;
 const char* names[]={"gi-auto-Clear.dxil","gi-auto-Claim.dxil","gi-auto-Accumulate.dxil","gi-auto-Freeze.dxil"};
 for(UINT i=0;i<4;++i){code[i]=Read(names[i]);shaders[i]={code[i].data(),code[i].size()};}
 D3D12AutoTargetBuild builder;Check(builder.Initialize(d.device.Get(),shaders));
 // A real previous compute root/PSO is required for tested state restoration.
 D3D12_ROOT_PARAMETER params[4]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;params[0].Descriptor={0,0};
 params[1].ParameterType=params[2].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;params[1].Descriptor={0,0};params[2].Descriptor={1,0};
 params[3].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[3].Constants={0,0,8};
 D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=4;rd.pParameters=params;ComPtr<ID3DBlob> blob,error;Check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));
 ComPtr<ID3D12RootSignature> root;Check(d.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.CS=shaders[3];ComPtr<ID3D12PipelineState> prior;Check(d.device->CreateComputePipelineState(&pd,IID_PPV_ARGS(&prior)));
 ComputeBindingTracker tracker;tracker.Root(root.Get());tracker.Pipeline(prior.Get());tracker.Descriptor(0,RootArgumentKind::SRV,input->GetGPUVirtualAddress());tracker.Descriptor(1,RootArgumentKind::UAV,cells->GetGPUVirtualAddress());tracker.Descriptor(2,RootArgumentKind::UAV,frozen->GetGPUVirtualAddress());UINT control[8]{};tracker.Constants(3,0,8,control);ComputeBindingSnapshot restore;Require(tracker.Capture(restore),"restore snapshot");
 Require(!builder.Record(d.list.Get(),input.Get(),65536,cells.Get(),frozen.Get(),slots,1,7,.5f,512,8,restore),"unsafe count admitted");
 Require(!builder.Record(d.list.Get(),input.Get(),400,cells.Get(),cells.Get(),slots,1,7,.5f,512,8,restore),"alias admitted");
 Require(!builder.Record(d.list.Get(),empty.Get(),400,cells.Get(),frozen.Get(),slots,1,7,.5f,512,8,restore),"short input admitted");
 Require(builder.Record(d.list.Get(),input.Get(),UINT(records.size()),cells.Get(),frozen.Get(),slots,1,7,.5f,512,8,restore),"first build");
 auto copy=[&](UINT index){d.Transition(frozen.Get(),D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,D3D12_RESOURCE_STATE_COPY_SOURCE);d.list->CopyBufferRegion(back.Get(),index*sizeof(GIAutoTargets),frozen.Get(),0,sizeof(GIAutoTargets));d.Transition(frozen.Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);};copy(0);
 Require(builder.Record(d.list.Get(),empty.Get(),0,cells.Get(),frozen.Get(),slots,2,7,.5f,512,8,restore),"empty build");copy(1);
 Require(builder.Record(d.list.Get(),input.Get(),UINT(records.size()),cells.Get(),frozen.Get(),slots,3,8,.5f,512,8,restore),"new epoch build");copy(2);
 // Competing cells with an identical hash may lose training, but must never merge.
 int collisionX=1;while(GIAutoHash(collisionX,0,6)%slots!=GIAutoHash(0,0,6)%slots)++collisionX;
 auto colliding=a;colliding.position.x=float(collisionX)*.5f+.2f;colliding.energy=1;
 std::vector<GIAutoTraining> collisionRecords(200,a);collisionRecords.insert(collisionRecords.end(),50,colliding);auto collisionInput=d.Upload(collisionRecords);
 Require(builder.Record(d.list.Get(),collisionInput.Get(),250,cells.Get(),frozen.Get(),slots,4,7,.5f,512,8,restore),"collision build");copy(3);
 auto hdr=a;hdr.energy=1e30f;auto hdrInput=d.Upload(std::vector<GIAutoTraining>(65535,hdr));
 Require(builder.Record(d.list.Get(),hdrInput.Get(),65535,cells.Get(),frozen.Get(),slots,5,7,.5f,512,8,restore),"HDR bound build");copy(4);
 // One rare bright hit must defeat a frequent dim collision and remain ahead
 // of a different cell with greater aggregate energy but lower peak energy.
 auto dim=a;dim.energy=.01f;auto rare=colliding;rare.energy=4;
 std::vector<GIAutoTraining> rareRecords(200,dim);rareRecords.push_back(rare);
 rareRecords.insert(rareRecords.end(),1000,b);auto rareInput=d.Upload(rareRecords);
 Require(builder.Record(d.list.Get(),rareInput.Get(),UINT(rareRecords.size()),cells.Get(),frozen.Get(),slots,6,7,.5f,512,1,restore),"rare-hit build");copy(5);
 Require(builder.Record(d.list.Get(),rareInput.Get(),UINT(rareRecords.size()),cells.Get(),frozen.Get(),slots,7,7,.5f,512,8,restore),"old threshold control");copy(6);d.Submit();
 GIAutoTargets* mapped=nullptr;Check(back->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
 Require(GIAutoMatches(mapped[0],7,1)&&mapped[0].validMask==3,"target header");
 Require(mapped[0].counts[0]==200&&mapped[0].counts[1]==200,"invalid training contaminated count");
 Require(mapped[0].scores[0]==204800&&mapped[0].scores[1]==102400,"target ordering/scores");
 Require(std::abs(mapped[0].bounds[0].center.x-.25f)<1e-6&&std::abs(mapped[0].bounds[1].center.x-1.25f)<1e-6,"target centers");
 Require(std::abs(mapped[0].bounds[0].radius-.4330127f)<1e-6,"enclosing radius");
 Require(!GIAutoMatches(mapped[0],8,1)&&!GIAutoMatches(mapped[0],7,2),"wrong identity admitted");
 Require(mapped[1].frame==2&&mapped[1].validMask==0&&mapped[1].scores[0]==0,"empty frame retained stale target");
 Require(mapped[2].version==8&&mapped[2].validMask==0,"epoch retained stale target");
 Require(mapped[3].validMask==1&&mapped[3].counts[0]==200,"hash collision merged unrelated targets");
 Require(mapped[4].validMask==1&&mapped[4].counts[0]==65535&&mapped[4].scores[0]==4294836225u,"HDR accumulation wrapped");
 Require(mapped[5].validMask==3&&mapped[5].counts[0]==1&&mapped[5].scores[0]==2048&&mapped[5].counts[1]==1000,"rare peak lost to frequency/collision");
 Require(std::abs(mapped[5].bounds[0].center.x-(float(collisionX)*.5f+.25f))<1e-6,"collision selected wrong location");
 Require(mapped[6].validMask==1&&mapped[6].counts[0]==1000,"eight-record threshold control did not reject rare hit");
 D3D12_RANGE none{};back->Unmap(0,&none);
 // Execute the actual shared pilot-resolve shader, not only its CPU counterpart.
 Device pilotDevice(false);struct V{float x,y,z,w;};constexpr UINT cases=4;
 std::vector<V> values(cases*65);std::vector<GIPilotResult> expected;
 for(UINT c=0;c<cases;++c){
  UINT n=c==2?64:10,version=14;float alpha=c==3?.2f:1.f/float(n);
  values[c*65]={float(n),alpha,0,0};std::memcpy(&values[c*65].z,&version,4);
  GIPoint pilot{.25f,2,100};values[c*65+1]={pilot.x,pilot.y,pilot.z,0};auto state=GIPilotBegin(pilot,n,alpha,version);
  for(UINT i=1;i<n;++i){float w=i%2?1/(1-alpha):0;GIPoint color=c==0?GIPoint{float(i),.1f,2}:pilot;
   values[c*65+1+i]={color.x,color.y,color.z,w};state=GIPilotAdd(state,color,w,version);}
  expected.push_back(GIPilotFinish(state));
 }
 auto pilotInput=pilotDevice.Upload(values);auto pilotOutput=pilotDevice.Buffer(cases*sizeof(V),D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,true);
 auto pilotRead=pilotDevice.Buffer(cases*sizeof(V),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 D3D12_ROOT_PARAMETER pp[2]{};pp[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_SRV;pp[0].Descriptor={0,0};pp[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;pp[1].Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC prd{};prd.NumParameters=2;prd.pParameters=pp;blob.Reset();error.Reset();Check(D3D12SerializeRootSignature(&prd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));ComPtr<ID3D12RootSignature> pilotRoot;Check(pilotDevice.device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&pilotRoot)));
 auto pilotCode=Read("gi-pilot-resolve.dxil");D3D12_COMPUTE_PIPELINE_STATE_DESC pdesc{};pdesc.pRootSignature=pilotRoot.Get();pdesc.CS={pilotCode.data(),pilotCode.size()};ComPtr<ID3D12PipelineState> pilotPso;Check(pilotDevice.device->CreateComputePipelineState(&pdesc,IID_PPV_ARGS(&pilotPso)));
 pilotDevice.list->SetComputeRootSignature(pilotRoot.Get());pilotDevice.list->SetPipelineState(pilotPso.Get());pilotDevice.list->SetComputeRootShaderResourceView(0,pilotInput->GetGPUVirtualAddress());pilotDevice.list->SetComputeRootUnorderedAccessView(1,pilotOutput->GetGPUVirtualAddress());pilotDevice.list->Dispatch(cases,1,1);pilotDevice.Transition(pilotOutput.Get(),D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);pilotDevice.list->CopyResource(pilotRead.Get(),pilotOutput.Get());pilotDevice.Submit();
 V* pv=nullptr;Check(pilotRead->Map(0,nullptr,reinterpret_cast<void**>(&pv)));
 for(UINT c=0;c<cases;++c){Require(pv[c].w==float(expected[c].valid),"GPU pilot validity");const float actual[]={pv[c].x,pv[c].y,pv[c].z},ref[]={expected[c].rgb.x,expected[c].rgb.y,expected[c].rgb.z};for(UINT k=0;k<3;++k)Require(std::abs(actual[k]-ref[k])<.00001f*(1+std::abs(ref[k])),"GPU pilot result");}
 pilotRead->Unmap(0,&none);
 std::cout<<"PASS GPU target build: score ordering, invalid inputs, empty expiry, epoch isolation, collision separation, bounded HDR accumulation, GPU pilot resolve and production recording. No live integration or GPU timing claim.\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
