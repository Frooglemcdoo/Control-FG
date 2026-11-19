// Included inside ControlNrdRuntime: shares the native queue/fence owner with GI.
static UINT64 specularGuideFrame=0,specularGuideView=0,specularGuideGeneration=0;
static ID3D12GraphicsCommandList* specularGuideList=nullptr;
static LONGLONG specularLastTicks=0;static UINT64 specularLastFrame=0;
static bool SpecularReady(UINT64 frame) noexcept {
 std::lock_guard<std::recursive_mutex> lock(mutex);
 return Requested()&&specularEpoch&&!specularEpoch->poisoned&&specularGuideFrame==frame&&
  specularEpoch->rrRequested==control_rr::RRUserRequested();
}
static void SpecularReason(const char* why)noexcept {
 static const char* last=nullptr;if(last!=why){last=why;Log("REBLUR_SPECULAR fallback=%s",why);}
 control_reblur::reason=why;
 control_reblur::status=control_reblur::Status::Fallback;
}
static void SpecularProducer(void* owner) noexcept {
 std::lock_guard<std::recursive_mutex> lock(mutex);Mute mute;
 specularGuideFrame=0;
 if(!Requested())return;
 try {
  Poll();auto access=rrReflectionAccess;access.current=&RRReflectionCurrent;access.user=nullptr;
  control_rr_reflection::Snapshot source;
  if(!control_rr_reflection::ReadBound(access.Callbacks(),reinterpret_cast<UINT64>(verifiedRenderer),control_rr_reflection::BindCallRva,reinterpret_cast<UINT64>(owner),source)){SpecularReason("specular_producer_binding");return;}
  auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(source.context.list);
  gi_reuse::ComputeBindingSnapshot binding;UINT64 generation=0;gi_reuse::GraphicsTableState graphics;
  ComPtr<ID3D12GraphicsCommandList4> nativeList;
  const auto reset=gi_dual_bridge::resetReady.load();
  if(!reset||!reset(list)){SpecularReason("specular_reset_coverage");return;}
  if(!hooks::Capture(list,binding,generation)||!binding.Ready()){SpecularReason("specular_binding_snapshot");return;}
  if(FAILED(list->QueryInterface(IID_PPV_ARGS(&nativeList)))){SpecularReason("specular_command_interface");return;}
  if(!Initialize(reinterpret_cast<ID3D12CommandQueue*>(source.context.queue))){SpecularReason("specular_queue_initialization");return;}
  hooks::CaptureGraphics(list,graphics);if(!graphics.known){SpecularReason("specular_graphics_state");return;}
  RRDistanceInput distance;const char* why=nullptr;
  if(!RRDistanceReadInput(source,distance,&why)||!distance.valid){SpecularReason("specular_distance_contract");return;}
  control_nrd_native::Texture depth;
  if(!NamedTexture("g_tClipDepth",depth,true)||reinterpret_cast<UINT64>(depth.resource)!=distance.depthResource){SpecularReason("specular_clip_depth");return;}
  RRGuideInputTexture normal;DWORD fault=0;
  if(!RRGuideReadTexture(kRRGuideGBuffer1ShaderRva,&normal,&why,&fault)){SpecularReason("specular_primary_normal");return;}
  const UINT w=UINT(source.material.shape.width),h=source.material.shape.height;
  // Guides cannot be overwritten while a prior list has not been submitted.
  for(auto& r:recordings)if(!r->reset&&!r->submitted&&r->specularEpoch&&r->specularGuides){SpecularReason("specular_submission_pending");return;}
  if(!specularEpoch||specularEpoch->poisoned||specularEpoch->width!=w||specularEpoch->height!=h||specularEpoch->rrRequested!=control_rr::RRUserRequested()||specularEpoch->modeRevision!=control_reblur::revision.load()){
   std::vector<SpecularEpoch*> held;for(auto& r:recordings)if(r->specularEpoch&&std::find(held.begin(),held.end(),r->specularEpoch.get())==held.end())held.push_back(r->specularEpoch.get());
   if(held.size()>=3){SpecularReason("specular_epoch_retirement");return;}
   auto next=std::make_shared<SpecularEpoch>();next->width=w;next->height=h;next->rrRequested=control_rr::RRUserRequested();next->modeRevision=control_reblur::revision.load();
   next->guides=std::make_unique<control_nrd::SpecularGuides>(PTGlassRuntime::device.Get(),w,h,D3D12_SHADER_BYTECODE{kNrd_specular_guides,sizeof(kNrd_specular_guides)});
   next->filter=std::make_unique<control_nrd::DiffusePipeline>(PTGlassRuntime::device.Get(),w,h,D3D12_SHADER_BYTECODE{kNrd_specular_native_prepare,sizeof(kNrd_specular_native_prepare)},D3D12_SHADER_BYTECODE{kNrd_finish,sizeof(kNrd_finish)},control_nrd::Signal::Specular);
   specularEpoch=next;
  }
  auto* record=GetRecording(nativeList.Get(),generation);if(!record||record->specularGuides){SpecularReason("specular_recording_capacity");return;}
  record->specularEpoch=specularEpoch;record->specularGuides=std::make_unique<control_nrd::SpecularGuides::Work>();
  using E=control_nrd::D3D12Backend::External;
  const std::array<E,4> inputs{{{reinterpret_cast<ID3D12Resource*>(source.material.resource),static_cast<D3D12_RESOURCE_STATES>(source.material.state)},
   {reinterpret_cast<ID3D12Resource*>(source.position.resource),static_cast<D3D12_RESOURCE_STATES>(source.position.state)},
   {depth.resource,depth.state},{normal.resource,static_cast<D3D12_RESOURCE_STATES>(normal.trackedState)}}};
  if(!specularEpoch->guides->Record(nativeList.Get(),binding,graphics,inputs,&distance.constants,*record->specularGuides)){SpecularReason("specular_guide_record");return;}
  specularGuideFrame=source.context.frame;specularGuideView=source.context.view;specularGuideList=list;specularGuideGeneration=generation;
 }catch(...){if(specularEpoch)specularEpoch->poisoned=true;SpecularReason("specular_producer_exception");}
}
static bool SpecularDispatch()noexcept {
 std::lock_guard<std::recursive_mutex> lock(mutex);Mute mute;
 try {
  Poll();auto* scope=rrNativeSpecularScope;if(!scope||!SpecularReady(scope->frame.frame)||specularGuideView!=scope->frame.view)return false;
  control_rr_reflection::Context current;if(!RRReflectionCurrent(nullptr,current)||!control_rr_specular::SameFrameOwner(scope->frame,current))return false;
  auto access=rrReflectionAccess;access.current=&RRReflectionCurrent;access.user=nullptr;control_rr_specular::Pair pair;
  if(control_rr_specular::ReadPair(access.Callbacks(),reinterpret_cast<UINT64>(verifiedRenderer),pair)||pair.source!=scope->sourceNative)return false;
  auto* list=reinterpret_cast<ID3D12GraphicsCommandList*>(current.list);
  gi_reuse::ComputeBindingSnapshot binding;UINT64 generation=0;gi_reuse::GraphicsTableState graphics;ComPtr<ID3D12GraphicsCommandList4> nativeList;
  if(!hooks::Capture(list,binding,generation)||!binding.Ready()||FAILED(list->QueryInterface(IID_PPV_ARGS(&nativeList))))return false;
  hooks::CaptureGraphics(list,graphics);if(!graphics.known)return false;
  bool ordered=false;for(auto& r:recordings)if(r->list.Get()==specularGuideList&&r->generation==specularGuideGeneration&&r->specularGuides&&r->specularGuides->recorded&&r->specularEpoch==specularEpoch)
   ordered=r->submitted||(r->list.Get()==list&&!r->reset);
  if(!ordered){SpecularReason("specular_guide_order");return false;}
  control_nrd_native::Texture raw,target,motion;
  if(!NamedTexture("g_DLF_rwtColorSource",raw)||!NamedTexture("g_DLF_rwtColorTarget",target)||!NamedTexture("g_tGeometryVelocityTexture",motion)||
     reinterpret_cast<UINT64>(raw.resource)!=pair.sourceResource||reinterpret_cast<UINT64>(target.resource)!=pair.targetResource||target.state!=D3D12_RESOURCE_STATE_UNORDERED_ACCESS)return false;
  CameraSnapshot camera;DWORD fault=0;const char* why=nullptr;control_nrd::CameraMatrices matrices;double shift[2]{};std::array<float,2> jitter;
  const UINT w=specularEpoch->width,h=specularEpoch->height;
  if(!ReadCamera(&camera,&fault,&why)||camera.engineFrame!=current.frame||reinterpret_cast<UINT64>(camera.view)!=current.view||!control_nrd::ConvertCamera(camera,matrices)||
     !control_rr_reflection::WindowsAccess::Read(nullptr,reinterpret_cast<UINT64>(camera.renderer)+0x480,shift,sizeof(shift))||!control_nrd::NativeClipShiftToNrdJitter(shift[0],shift[1],w,h,jitter))return false;
  control_nrd::Frame frame;frame.frame=current.frame;frame.view=current.view;frame.producer=1;frame.width=std::uint16_t(w);frame.height=std::uint16_t(h);
  frame.sameFrameGuides=frame.worldNormals=frame.linearViewDepth=frame.unjitteredMotion=frame.radianceContract=frame.hitDistanceContract=true;
  std::memcpy(frame.settings.worldToViewMatrix,matrices.worldToView.data(),64);std::memcpy(frame.settings.viewToClipMatrix,matrices.viewToClip.data(),64);
  frame.settings.cameraJitter[0]=jitter[0];frame.settings.cameraJitter[1]=jitter[1];frame.settings.timeDeltaBetweenFrames=16.6667f;
  LARGE_INTEGER now{},frequency{};QueryPerformanceCounter(&now);QueryPerformanceFrequency(&frequency);
  if(specularLastTicks&&specularLastFrame+1==frame.frame&&frequency.QuadPart>0&&now.QuadPart>specularLastTicks)frame.settings.timeDeltaBetweenFrames=float(std::fmax(1.0,std::fmin(250.0,1000.0*(now.QuadPart-specularLastTicks)/frequency.QuadPart)));
  using E=control_nrd::D3D12Backend::External;auto& guides=*specularEpoch->guides;
  control_nrd::DiffusePipeline::Bindings inputs;inputs.input={E{raw.resource,raw.state},E{guides.Resource(0),guides.State(0)},E{guides.Resource(1),guides.State(1)},E{motion.resource,motion.state},E{guides.Resource(2),guides.State(2)},E{guides.Resource(3),guides.State(3)}};inputs.output={target.resource,target.state};
  auto* record=GetRecording(nativeList.Get(),generation);if(!record||record->specularWork||(record->specularEpoch&&record->specularEpoch!=specularEpoch))return false;
  record->specularEpoch=specularEpoch;record->specularWork=std::make_unique<control_nrd::NativeTransaction::Work>();
  struct Reservation{control_nrd::NativeTransaction::Work* work;UINT maximum;};Reservation reservation{record->specularWork.get(),specularEpoch->filter->MaxDescriptors()};
  auto reserve=[](const void* data,UINT count,control_nrd::D3D12Backend::Arena& arena)->const char*{
   const auto& r=*static_cast<const Reservation*>(data);if(count>r.maximum)return "specular_descriptor_count";
   D3D12_DESCRIPTOR_HEAP_DESC desc{};desc.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;desc.NumDescriptors=r.maximum;desc.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
   if(FAILED(PTGlassRuntime::device->CreateDescriptorHeap(&desc,IID_PPV_ARGS(&r.work->privateHeap))))return "specular_descriptor_allocation";
   arena={r.work->privateHeap.Get(),0,r.maximum};return nullptr;
  };
  control_nrd::DiffusePipeline::Constants conversion;conversion.sharpening=control_reblur::Sharpness();
  if(!control_nrd::NativeTransaction::Record(*specularEpoch->filter,nativeList.Get(),binding,frame,current.queue,inputs,conversion, {binding.heaps[0].Get(),0,0},*record->specularWork,reserve,&reservation,&graphics)){
   if(record->specularWork->diffuse.commandsRecorded)specularEpoch->poisoned=true;SpecularReason("specular_filter_record");return false;
  }
  record->specularFiltered=true;specularGuideFrame=0;specularLastTicks=now.QuadPart;specularLastFrame=frame.frame;
  static int reportedMode=-1;const int mode=int(control_rr::RRUserRequested());if(reportedMode!=mode){reportedMode=mode;Log("REBLUR_SPECULAR recorded=1 rr=%u native_temporal=0 native_spatial=0",unsigned(mode));}
  control_reblur::status=control_reblur::Status::Recorded;control_reblur::reason="specular_recorded";return true;
 }catch(...){if(specularEpoch)specularEpoch->poisoned=true;SpecularReason("specular_dispatch_exception");return false;}
}
static const control_nrd_bridge::Specular specularCallbacks{&SpecularProducer,&SpecularReady,&SpecularDispatch};
static const bool specularRegistered=[] {control_nrd_bridge::specular=&specularCallbacks;return true;}();
