#include <windows.h>
#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include "../../src/nrd_diffuse_d3d12.h"
#ifdef CONTROL_NRD_PIPELINE_TEST
#include "../../src/nrd_diffuse_native_transaction.h"
#include "nrd_restore_probe.h"
#include "../../src/nrd_diffuse_receiver_d3d12.h"
#include "nrd_prepare.h"
#include "nrd_finish.h"
#include "nrd_receiver.h"
#include "../pt-glass-native-root-fixture.h"
#endif
#include <cstdio>
#include <array>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT h){if(FAILED(h))throw std::runtime_error("GPU test failure");}
static void Require(bool good,const char*s){if(!good)throw std::runtime_error(s);}
constexpr UINT W=128,H=64;
#ifdef CONTROL_NRD_PIPELINE_TEST
constexpr UINT InputCount=6,OutputIndex=6;
#else
constexpr UINT InputCount=4,OutputIndex=4;
#endif
int main(){try{
 ComPtr<ID3D12Debug> debug;Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));debug->EnableDebugLayer();
 ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter> adapter;Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
 ComPtr<ID3D12Device> device;Check(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)));
 ComPtr<ID3D12InfoQueue> messages;Check(device.As(&messages));
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC qd{};Check(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue)));
 ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 ComPtr<ID3D12GraphicsCommandList4> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
 ComPtr<ID3D12Fence> fence;Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event");
 auto buffer=[&](UINT64 size,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){ComPtr<ID3D12Resource> p;D3D12_HEAP_PROPERTIES hp{};hp.Type=type;D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=size;d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&d,state,nullptr,IID_PPV_ARGS(&p)));return p;};
 std::array<ComPtr<ID3D12Resource>,InputCount+1> textures;
 D3D12_RESOURCE_DESC td{};td.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;td.Width=W;td.Height=H;td.DepthOrArraySize=td.MipLevels=1;td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
 for(auto&t:textures)Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&td,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&t)));
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 bytes=0;device->GetCopyableFootprints(&td,0,1,0,&footprint,nullptr,nullptr,&bytes);
 auto readback=buffer(bytes,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
#ifdef CONTROL_NRD_PIPELINE_TEST
 // Compile the real receiver decoder against the audited native root ABI.
 // This checks PSO compatibility, not in-game receiver values.
 ComPtr<ID3D12RootSignature> nativeRoot;Check(CreateGlassNativeRootFixture(device.Get(),&nativeRoot));
 control_nrd::NativeReceiver receiver(device.Get(),nativeRoot.Get(),{kNrd_receiver,sizeof(kNrd_receiver)},W,H);
 control_nrd::DiffusePipeline pipeline(device.Get(),W,H,{kNrd_prepare,sizeof(kNrd_prepare)},{kNrd_finish,sizeof(kNrd_finish)});
 D3D12_ROOT_PARAMETER probeParameters[2]{};
 probeParameters[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;probeParameters[0].Constants={0,0,4};
 probeParameters[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;probeParameters[1].Descriptor={0,0};
 D3D12_ROOT_SIGNATURE_DESC probeDesc{};probeDesc.NumParameters=2;probeDesc.pParameters=probeParameters;
 ComPtr<ID3DBlob> probeBytes,probeErrors;Check(D3D12SerializeRootSignature(&probeDesc,D3D_ROOT_SIGNATURE_VERSION_1,&probeBytes,&probeErrors));
 ComPtr<ID3D12RootSignature> probeRoot;Check(device->CreateRootSignature(0,probeBytes->GetBufferPointer(),probeBytes->GetBufferSize(),IID_PPV_ARGS(&probeRoot)));
 D3D12_COMPUTE_PIPELINE_STATE_DESC probePS{};probePS.pRootSignature=probeRoot.Get();probePS.CS={kNrd_restore_probe,sizeof(kNrd_restore_probe)};
 ComPtr<ID3D12PipelineState> probePipeline;Check(device->CreateComputePipelineState(&probePS,IID_PPV_ARGS(&probePipeline)));
 D3D12_RESOURCE_DESC probeBuffer{};probeBuffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;probeBuffer.Width=16;probeBuffer.Height=1;probeBuffer.DepthOrArraySize=probeBuffer.MipLevels=1;probeBuffer.SampleDesc.Count=1;probeBuffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;probeBuffer.Flags=D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
 ComPtr<ID3D12Resource> probeOutput;Check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&probeBuffer,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,nullptr,IID_PPV_ARGS(&probeOutput)));
 auto probeReadback=buffer(16,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 control_nrd::DiffusePipeline::Bindings bindings;
 for(UINT i=0;i<InputCount;++i)bindings.input[i]={textures[i].Get(),D3D12_RESOURCE_STATE_COMMON};
 bindings.output={textures[OutputIndex].Get(),D3D12_RESOURCE_STATE_COMMON};
 auto input=[&](UINT i)->control_nrd::D3D12Backend::External&{return bindings.input[i];};
 auto& output=bindings.output;
#else
 control_nrd::Session session;control_nrd::D3D12Backend backend(device.Get(),session.Description(),W,H);
 control_nrd::D3D12Backend::Inputs inputs;
 const nrd::ResourceType names[]={nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST,nrd::ResourceType::IN_NORMAL_ROUGHNESS,nrd::ResourceType::IN_VIEWZ,nrd::ResourceType::IN_MV,nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST};
 for(UINT i=0;i<InputCount+1;++i)inputs[names[i]]={textures[i].Get(),D3D12_RESOURCE_STATE_COMMON};
 auto input=[&](UINT i)->control_nrd::D3D12Backend::External&{return inputs[names[i]];};
 auto& output=inputs[names[OutputIndex]];
#endif
 control_nrd::Frame f;f.view=1;f.width=W;f.height=H;f.sameFrameGuides=f.worldNormals=f.linearViewDepth=f.unjitteredMotion=f.radianceContract=f.hitDistanceContract=true;
 auto&s=f.settings;s.timeDeltaBetweenFrames=16.6667f;for(UINT i=0;i<16;++i)s.worldToViewMatrix[i]=s.worldToViewMatrixPrev[i]=i%5==0?1.f:0.f;
 s.viewToClipMatrix[0]=s.viewToClipMatrix[5]=1;s.viewToClipMatrix[10]=1.001f;s.viewToClipMatrix[11]=1;s.viewToClipMatrix[14]=-.1001f;std::memcpy(s.viewToClipMatrixPrev,s.viewToClipMatrix,64);
 auto transition=[&](control_nrd::D3D12Backend::External&t,D3D12_RESOURCE_STATES next){if(t.state==next)return;D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={t.resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,t.state,next};list->ResourceBarrier(1,&b);t.state=next;};
 double finalMean=0,spotMean=0,quietMean=0,worstTrail=0;
#ifdef CONTROL_NRD_PIPELINE_TEST
 control_nrd::NativeTransaction::Work transaction;
 ID3D12Resource* previousRelax=nullptr;ID3D12Resource* previousConversion=nullptr;
 UINT restartDescriptors=0,normalDescriptors=0;
 control_nrd::Session descriptorContract;
#endif
 for(UINT frame=1;frame<=128;++frame){
  Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));
  std::vector<ComPtr<ID3D12Resource>> uploads;
  for(UINT i=0;i<InputCount;++i){auto upload=buffer(bytes,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ);void*p=nullptr;Check(upload->Map(0,nullptr,&p));
   for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){auto*pixel=reinterpret_cast<float*>(static_cast<unsigned char*>(p)+footprint.Offset+y*footprint.Footprint.RowPitch)+x*4;
    std::array<float,4> value=i==0?std::array<float,4>{.4f,.4f,.4f,2.f}:i==1?std::array<float,4>{0,0,-1,1}:i==2?std::array<float,4>{10,0,0,0}:std::array<float,4>{0,0,0,0};
    if(i==0 && frame>16){
     const bool spot=x>=W/4&&x<3*W/4&&y>=H/4&&y<3*H/4;
     float light=frame>80?.01f:(spot?.8f:.05f);
     // Sparse, moving single-frame outliers: the old hybrid path mistook these
     // for illumination changes and repeatedly reset to the noisy sample.
     if(frame>48&&frame<=80&&((x*17+y*31+frame*13)%509)==0)light=4;
     value={light,light,light,2};
    }
    if(frame>96){
     // A foreground surface moves right, exposing a stationary background.
     // This is geometry motion with matching depth/MV, not a moving light.
     const UINT left=16+2*(frame-97);
     const bool foreground=x>=left&&x<left+24&&y>=16&&y<48;
     if(i==0){const float light=foreground?.8f:.05f;value={light,light,light,2};}
     if(i==2)value={foreground?9.f:10.f,0,0,0};
     if(i==3)value={foreground?-2.f/W:0.f,0,0,0};
    }
#ifdef CONTROL_NRD_PIPELINE_TEST
    const float material=x<W/2?.25f:.75f;
    if(i==0){value[0]*=material;value[1]*=.5f;value[2]*=.8f;}
    if(i==1&&x<4)value={0,0,0,1}; // rejected guide must preserve raw energy
    if(i==4)value={2,0,0,0};
    if(i==5)value={material,.5f,.8f,1};
#endif
    std::memcpy(pixel,value.data(),16);
   }upload->Unmap(0,nullptr);transition(input(i),D3D12_RESOURCE_STATE_COPY_DEST);
   D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=textures[i].Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);uploads.push_back(upload);
  }
  f.frame=frame;
#ifdef CONTROL_NRD_PIPELINE_TEST
  auto& recording=transaction.diffuse;
  control_nrd::DiffusePipeline::Constants conversion;
  // Mimic the native renderer's already-bound heap with a reserved prefix.
  ComPtr<ID3D12DescriptorHeap> nativeHeap;D3D12_DESCRIPTOR_HEAP_DESC arenaDesc{};
  arenaDesc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;arenaDesc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  arenaDesc.NumDescriptors=31+pipeline.MaxDescriptors();Check(device->CreateDescriptorHeap(&arenaDesc,IID_PPV_ARGS(&nativeHeap)));
  ID3D12DescriptorHeap* boundHeap=nativeHeap.Get();list->SetDescriptorHeaps(1,&boundHeap);
  control_nrd::D3D12Backend::Arena arena{nativeHeap.Get(),31,pipeline.MaxDescriptors()};
  conversion.radianceScale=frame<64?2.f:1.f; // exercise scale-epoch restart
  if(frame==1){
   auto alias=bindings;alias.output=alias.input[0];control_nrd::DiffusePipeline::Work rejected;
   Require(!pipeline.Record(list.Get(),f,1,alias,conversion,rejected),"aliased native output accepted");
  }
  gi_reuse::ComputeBindingTracker tracker;tracker.Reset(probePipeline.Get());tracker.Root(probeRoot.Get());tracker.Heaps(1,&boundHeap);
  const std::uint32_t expected[]={frame,17,93,421};tracker.Constants(0,0,4,expected);tracker.Descriptor(1,gi_reuse::RootArgumentKind::UAV,probeOutput->GetGPUVirtualAddress());
  gi_reuse::ComputeBindingSnapshot snapshot;Require(tracker.Capture(snapshot),"native state snapshot rejected");Require(snapshot.Restore(list.Get()),"initial native bindings");
  struct Reservation {control_nrd::D3D12Backend::Arena available;UINT* requested;unsigned mode;};
  UINT requested=0;Reservation reservation{arena,&requested,0};
  auto reserve=[](const void* context,UINT count,control_nrd::D3D12Backend::Arena& result)->const char* {
   const auto& r=*static_cast<const Reservation*>(context);*r.requested=count;
   if(r.mode==1)return "test_reservation_unavailable";
   if(count>r.available.count)return "test_reservation_capacity";
   result={r.available.heap,r.available.first,count};
   if(r.mode==2)--result.count;
   if(r.mode==3)result.heap=nullptr;
   return nullptr;
  };
  if(frame==1){
   const char* reasons[]={"test_reservation_unavailable","filter_descriptor_arena_bounds","filter_heap_changed"};
   for(unsigned mode=1;mode<=3;++mode){
    control_nrd::NativeTransaction::Work rejected;reservation.mode=mode;
    Require(!control_nrd::NativeTransaction::Record(pipeline,list.Get(),snapshot,f,1,bindings,conversion,arena,rejected,reserve,&reservation),"invalid descriptor reservation accepted");
    Require(std::strcmp(pipeline.LastFailure(),reasons[mode-1])==0,"descriptor failure mislabeled");
    Require(!rejected.diffuse.commandsRecorded&&!rejected.diffuse.heap&&!rejected.diffuse.constants,"failed reservation recorded or allocated GPU work");
    Require(!pipeline.Submitted(1,1),"failed reservation left pending history");
    for(UINT i=0;i<InputCount;++i)Require(input(i).state==D3D12_RESOURCE_STATE_COPY_DEST,"failed reservation altered resource states");
   }
   reservation.mode=0;
  }
  // Once the normal plan is known, expose exactly that many descriptors.
  // A regression to worst-case reservation fails here instead of silently
  // relying on a large test heap. Restart at frame 64 retains full capacity.
  if(frame>2&&frame<64)reservation.available.count=normalDescriptors;
  Require(control_nrd::NativeTransaction::Record(pipeline,list.Get(),snapshot,f,1,bindings,conversion,arena,transaction,reserve,&reservation),"complete native transaction rejected");
  Require(requested==22+UINT(pipeline.DispatchCount())*(descriptorContract.Description().descriptorPoolDesc.perSetTexturesMaxNum+descriptorContract.Description().descriptorPoolDesc.perSetStorageTexturesMaxNum),"reservation did not match dispatch plan");
  if(frame==1)restartDescriptors=requested;
  if(frame==2){normalDescriptors=requested;Require(normalDescriptors<restartDescriptors&&normalDescriptors<pipeline.MaxDescriptors(),"steady frame retained worst-case reservation");std::printf("Descriptor reservation: restart %u, normal %u, maximum %u\n",restartDescriptors,normalDescriptors,pipeline.MaxDescriptors());}
  if(frame==64)Require(requested==restartDescriptors,"scale restart did not reserve clear passes");
  if(frame>1){Require(recording.constants.Get()==previousConversion,"conversion upload buffer was not reused");Require(recording.relax.constants.Get()==previousRelax,"NRD upload buffer was not reused");}
  previousConversion=recording.constants.Get();previousRelax=recording.relax.constants.Get();
  // Dispatch without rebinding: only the transaction's restoration can supply
  // the correct PSO, root signature, constants and root UAV here.
  list->Dispatch(1,1,1);
  D3D12_RESOURCE_BARRIER probeBarrier{};probeBarrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;probeBarrier.Transition={probeOutput.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&probeBarrier);
  list->CopyBufferRegion(probeReadback.Get(),0,probeOutput.Get(),0,16);
  std::swap(probeBarrier.Transition.StateBefore,probeBarrier.Transition.StateAfter);list->ResourceBarrier(1,&probeBarrier);
  Require(recording.relax.borrowed&&recording.relax.heap.Get()==nativeHeap.Get(),"native descriptor heap not retained");
  {control_nrd::DiffusePipeline::Work duplicate;Require(!pipeline.Record(list.Get(),f,1,bindings,conversion,duplicate),"pending frame accepted twice");}
  for(UINT i=0;i<InputCount;++i)Require(input(i).state==D3D12_RESOURCE_STATE_COPY_DEST,"input state not restored");
#else
  Require(session.Prepare(f,1),"plan rejected");control_nrd::D3D12Backend::Recording recording;backend.Record(list.Get(),session.Plan(),inputs,recording);
#endif
  transition(output,D3D12_RESOURCE_STATE_COPY_SOURCE);
  D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;src.pResource=textures[OutputIndex].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
  Check(list->Close());ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Check(queue->Signal(fence.Get(),frame));
#ifdef CONTROL_NRD_PIPELINE_TEST
  Require(pipeline.Submitted(1,frame),"pipeline submission rejected");
#else
  Require(session.Submitted(1,frame),"submission rejected");
#endif
  Check(fence->SetEventOnCompletion(frame,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"GPU timeout");
#ifdef CONTROL_NRD_PIPELINE_TEST
  void* probeData=nullptr;Check(probeReadback->Map(0,nullptr,&probeData));Require(std::memcmp(probeData,expected,16)==0,"native bindings were not restored");probeReadback->Unmap(0,nullptr);
#endif
  void*p=nullptr;Check(readback->Map(0,nullptr,&p));double sum=0,center=0,trail=0,movingCenter=0;unsigned centerCount=0,trailCount=0,movingCount=0;for(UINT y=0;y<H;++y)for(UINT x=0;x<W;++x){const auto*pixel=reinterpret_cast<const float*>(static_cast<unsigned char*>(p)+footprint.Offset+y*footprint.Footprint.RowPitch)+x*4;Require(std::isfinite(pixel[0])&&std::isfinite(pixel[1])&&std::isfinite(pixel[2]),"nonfinite RELAX output");float light=pixel[0];
#ifdef CONTROL_NRD_PIPELINE_TEST
  const float factor=x<W/2?.25f:.75f;light/=factor;
  Require(pixel[3]==pixel[0],"native alpha contract changed");
  if(frame==16)Require(std::abs(light-.4f)<.02f&&std::abs(pixel[1]/.5f-.4f)<.02f&&std::abs(pixel[2]/.8f-.4f)<.02f,"receiver shading or brightness changed");
#endif
  if(frame>96){const UINT left=16+2*(frame-97);
   if(x>=left-2&&x<left&&y>=24&&y<40){trail+=light;++trailCount;}
   if(x>=left+8&&x<left+16&&y>=24&&y<40){movingCenter+=light;++movingCount;}
  }
  sum+=light;if(x>=7*W/16&&x<9*W/16&&y>=7*H/16&&y<9*H/16){center+=light;++centerCount;}}readback->Unmap(0,nullptr);finalMean=sum/(W*H);
  if(frame==16){std::printf("Uniform mean %.8f\n",finalMean);Require(std::abs(finalMean-.4)<.015,"RELAX lost uniform light");}
  if(frame==48){spotMean=center/centerCount;quietMean=finalMean;std::printf("Persistent light center %.8f mean %.8f\n",spotMean,quietMean);Require(spotMean>.65,"RELAX removed persistent local light");}
  if(frame==80){std::printf("Sparse outliers center %.8f mean %.8f\n",center/centerCount,finalMean);Require(std::abs(finalMean-quietMean)<.06,"Sparse outliers caused excessive light fluctuation");}
  if(frame==96){std::printf("Light off mean %.8f\n",finalMean);Require(finalMean<.025,"Stale illumination persisted after light removal");}
  if(frame>100){const double trailMean=trail/trailCount;worstTrail=(std::max)(worstTrail,trailMean);
   Require(trailMean<.14,"moving foreground left stale illumination on exposed background");
   Require(movingCenter/movingCount>.6,"moving foreground lost illumination");
  }
  if(frame==128)std::printf("Moving surface: worst exposed background %.8f (target 0.05), foreground %.8f (target 0.8)\n",worstTrail,movingCenter/movingCount);
  // Remove all recorded references before releasing this frame's descriptors.
  Check(allocator->Reset());Check(list->Reset(allocator.Get(),nullptr));Check(list->Close());
#ifdef CONTROL_NRD_PIPELINE_TEST
  transaction.RetireForReuse();
  Require(!transaction.native.root&&!recording.heap&&!recording.relax.heap,"retired work kept stale descriptors");
#endif
 }
 CloseHandle(event);
 unsigned errors=0;for(UINT64 i=0;i<messages->GetNumStoredMessages();++i){SIZE_T bytes=0;Check(messages->GetMessage(i,nullptr,&bytes));std::vector<unsigned char> storage(bytes);auto* message=reinterpret_cast<D3D12_MESSAGE*>(storage.data());Check(messages->GetMessage(i,message,&bytes));
  if(message->Severity==D3D12_MESSAGE_SEVERITY_ERROR||message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION){++errors;std::fprintf(stderr,"D3D12: %s\n",message->pDescription);}}
 Require(errors==0,"D3D12 validation errors");
 std::puts("PASS actual RELAX_DIFFUSE D3D12 dispatch chain and independent history; clean debug layer");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
