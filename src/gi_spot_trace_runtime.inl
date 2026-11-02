// Included within native_lighting, after GI binding audit. Diagnostic-only.
namespace gi_spot_trace {
using Microsoft::WRL::ComPtr;
constexpr UINT stride=256;
inline UINT Capacity() noexcept;
inline UINT Bytes() noexcept {return 16+Capacity()*stride;}
inline std::atomic<UINT> savedFrames{0},gridWidth{0},gridHeight{0};
inline UINT64 nextFrameAt=0;
inline UINT FrameCount() noexcept;
inline const GUID stateKey={0x274a0001,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
inline const GUID rootKey={0x274a0002,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
inline const GUID psoKey={0x274a0003,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
inline const GUID variantKey={0x274a0004,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
inline const GUID traceRootKey={0x274a0005,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
inline const GUID nativeRootKey={0x274a0006,0x7310,0x4fd1,{1,2,3,4,5,6,7,8}};
struct State {UINT64 values[8]{},heaps[2]{},generation=0;UINT mask=0;ID3D12RootSignature* root=nullptr;};
using Root=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12RootSignature*);
using Table=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_DESCRIPTOR_HANDLE);
using Cbv=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,D3D12_GPU_VIRTUAL_ADDRESS);
using Heaps=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,UINT,ID3D12DescriptorHeap*const*);
using Bundle=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12GraphicsCommandList*);
using Indirect=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*,ID3D12CommandSignature*,UINT,ID3D12Resource*,UINT64,ID3D12Resource*,UINT64);
struct Hooks{Bundle bundle=nullptr;Indirect indirect=nullptr;void** table=nullptr;Root root=nullptr;Table descriptor=nullptr;Cbv cbv=nullptr;Heaps heaps=nullptr;};
inline Hooks hooks[8]{};inline SRWLOCK hooksLock=SRWLOCK_INIT,lock=SRWLOCK_INIT;
inline std::atomic<bool> enabledReady{false};
enum class Phase{Idle,Countdown,Armed,Recording,Recorded,Submitted,Exporting,Gap,Complete,Failed};
inline std::atomic<Phase> phase{Phase::Idle};
inline ComPtr<ID3D12Device> device;
inline ComPtr<ID3D12Resource> output,readback,zero;
inline ComPtr<ID3D12Fence> fence;
inline ComPtr<ID3D12GraphicsCommandList> recordedList;
inline ComPtr<ID3D12PipelineState> retainedPso,retainedTrace;
inline ComPtr<ID3D12RootSignature> retainedRoot,retainedTraceRoot;
inline ComPtr<ID3D12CommandQueue> submittedQueue;
inline UINT64 generation=0,capturedPresent=0,armedAt=0;
inline bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_SPOT_TRACE",b,4)==1&&b[0]=='1';}();return value;}
inline bool JointTrace() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_JOINT_TRACE",b,4)==1&&b[0]=='1';}();return value;}
inline UINT Capacity() noexcept {return JointTrace()?262144:65536;}
inline UINT FrameCount() noexcept {return JointTrace()?8:1;}
inline bool RayTrace() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_RAY_TRACE",b,4)==1&&b[0]=='1';}();return value;}
inline UINT groups[3]{};
inline std::atomic<const wchar_t*> failure{L"Trace failed. Restart the diagnostic launcher; send logs."};
inline std::atomic<const wchar_t*> notice{L"Press F7 or use ARM GI TRACE in F10."};
inline void Fail(const char* code,const wchar_t* message) noexcept {
 if(phase.exchange(Phase::Failed)==Phase::Failed)return;failure.store(message);Trace("GI_SPOT_R275_FAILED reason=%s resources_retained=1",code);
}
inline const wchar_t* Status() noexcept {
 switch(phase.load()) {
 case Phase::Idle:return enabledReady.load()?notice.load():L"WAITING: GI probe is not ready. Load the level.";
 case Phase::Countdown:{
  static wchar_t text[128]{};const auto elapsed=GetTickCount64()-armedAt;
  if(elapsed>=5000)return L"ARMED: return to Control to start capture. Keep zoom held.";
  swprintf_s(text,L"ARMED: capture in %llu seconds. Hold your zoom and keep still.",(5000-elapsed+999)/1000);return text;
 }
 case Phase::Armed:return L"ARMED: waiting for the matching GI dispatch. Keep still.";
 case Phase::Recording:return L"CAPTURING: keep the camera and zoom steady.";
 case Phase::Recorded:return L"CAPTURE RECORDED: waiting for GPU submission.";
 case Phase::Submitted:return L"CAPTURE SUBMITTED: waiting for GPU completion.";
 case Phase::Exporting:return L"SAVING: writing this frame. Keep still.";
 case Phase::Gap:{static wchar_t text[128]{};swprintf_s(text,L"CAPTURING: %u / %u frames saved. Keep camera and zoom steady.",savedFrames.load(),FrameCount());return text;}
 case Phase::Complete:return L"SAVED: exit normally to package the trace and logs.";
 case Phase::Failed:return failure.load();
 }return L"Unknown capture state.";
}
using InstanceRequest=void (*)() noexcept;
inline std::atomic<InstanceRequest> instanceRequest{nullptr};
inline bool Request(const char* source) noexcept {
 if(!Enabled())return false;
 if(!enabledReady.load()){notice.store(L"NOT ARMED: probe not ready. Load the level first.");Trace("GI_SPOT_R275_REQUEST accepted=0 source=%s reason=not_ready",source);return false;}
 AcquireSRWLockExclusive(&lock);
 const bool accepted=phase.load()==Phase::Idle;
 if(accepted){armedAt=GetTickCount64();phase.store(Phase::Countdown);}
 ReleaseSRWLockExclusive(&lock);
 Trace("GI_SPOT_R275_REQUEST accepted=%u source=%s phase=%u delay_ms=5000",unsigned(accepted),source,unsigned(phase.load()));
 return accepted;
}
inline void AdvanceCountdown(UINT64 now,bool foreground) noexcept {
 if(phase.load()==Phase::Gap&&now>=nextFrameAt&&foreground){armedAt=now;phase.store(Phase::Armed);}
 if(phase.load()==Phase::Countdown&&now-armedAt>=5000&&foreground){
  Phase expected=Phase::Countdown;if(phase.compare_exchange_strong(expected,Phase::Armed)){armedAt=now;Trace("GI_SPOT_R275_ARMED countdown_complete=1");if(const auto request=instanceRequest.load())request();}
 }
}
// Called every 50 ms by the diagnostic UI, independent of disk/log draining.
inline void InputTick() noexcept {
 if(!Enabled())return;
 static bool held=false;const bool down=(GetAsyncKeyState(VK_F7)&0x8000)!=0;
 DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
 const bool foreground=foregroundPid==GetCurrentProcessId();
 const bool control=(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0,shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
 if(down&&!held){
  Trace("GI_SPOT_R275_KEY foreground=%u phase=%u ready=%u ctrl=%u shift=%u left=%u right=%u",unsigned(foreground),unsigned(phase.load()),unsigned(enabledReady.load()),unsigned(control),unsigned(shift),unsigned((GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0),unsigned((GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0));
  if(foreground&&!(control&&shift))Request("F7");
  else notice.store(control&&shift?L"NOT ARMED: release Ctrl+Shift and press F7 again.":L"NOT ARMED: focus Control and press F7 again.");
 }held=down;
 AdvanceCountdown(GetTickCount64(),foreground);
}


inline State Read(ID3D12GraphicsCommandList* list) noexcept {State s{};UINT n=sizeof(s);if(FAILED(list->GetPrivateData(stateKey,&n,&s))||n!=sizeof(s))s={};return s;}
inline void Save(ID3D12GraphicsCommandList* list,const State& s) noexcept {const HRESULT hr=list->SetPrivateData(stateKey,sizeof(s),&s);if(FAILED(hr))Fail("binding_state_storage",L"FAILED: could not store command-list state. Send logs.");}
template<class T> inline bool Attached(ID3D12Object* o,REFGUID key,ComPtr<T>& p) noexcept {UINT n=sizeof(T*);return o&&SUCCEEDED(o->GetPrivateData(key,&n,p.ReleaseAndGetAddressOf()))&&n==sizeof(T*)&&p;}
inline Hooks LookupHooks(ID3D12GraphicsCommandList* list) noexcept {Hooks h{};auto** table=*reinterpret_cast<void***>(list);AcquireSRWLockShared(&hooksLock);for(const auto& x:hooks)if(x.table==table){h=x;break;}ReleaseSRWLockShared(&hooksLock);return h;}
inline void STDMETHODCALLTYPE RootHook(ID3D12GraphicsCommandList* list,ID3D12RootSignature* root){
 auto h=LookupHooks(list);h.root(list,root);State s=Read(list);if(s.root!=root){s.mask=0;s.root=root;list->SetPrivateDataInterface(rootKey,root);}Save(list,s);
}
inline void STDMETHODCALLTYPE TableHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_DESCRIPTOR_HANDLE value){
 LookupHooks(list).descriptor(list,index,value);if(index==0||index==1||index==6||index==7){State s=Read(list);s.values[index]=value.ptr;s.mask|=1u<<index;Save(list,s);}
}
inline void STDMETHODCALLTYPE CbvHook(ID3D12GraphicsCommandList* list,UINT index,D3D12_GPU_VIRTUAL_ADDRESS value){
 LookupHooks(list).cbv(list,index,value);if(index>=2&&index<=5){State s=Read(list);s.values[index]=value;s.mask|=1u<<index;Save(list,s);}
}
inline void STDMETHODCALLTYPE HeapsHook(ID3D12GraphicsCommandList* list,UINT count,ID3D12DescriptorHeap*const* heaps){
 LookupHooks(list).heaps(list,count,heaps);State s=Read(list);UINT64 next[2]{};
 for(UINT i=0;i<count;++i){const auto type=heaps[i]->GetDesc().Type;if(type<=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)next[unsigned(type)]=reinterpret_cast<UINT64>(heaps[i]);}
 if(next[0]!=s.heaps[0]||next[1]!=s.heaps[1])s.mask&=~0xc3u;
 memcpy(s.heaps,next,sizeof(next));Save(list,s);
}
// Bundles and indirect commands can change root arguments without invoking
// the public root setters. Invalidate our observation rather than reuse it.
inline void STDMETHODCALLTYPE BundleHook(ID3D12GraphicsCommandList* list,ID3D12GraphicsCommandList* bundle){
 LookupHooks(list).bundle(list,bundle);State s=Read(list);s.mask=0;s.root=nullptr;s.heaps[0]=s.heaps[1]=0;Save(list,s);list->SetPrivateDataInterface(rootKey,nullptr);list->SetPrivateDataInterface(psoKey,nullptr);
}
inline void STDMETHODCALLTYPE IndirectHook(ID3D12GraphicsCommandList* list,ID3D12CommandSignature* signature,UINT count,ID3D12Resource* arguments,UINT64 offset,ID3D12Resource* counter,UINT64 counterOffset){
 LookupHooks(list).indirect(list,signature,count,arguments,offset,counter,counterOffset);State s=Read(list);s.mask=0;Save(list,s);
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled())return;auto** table=*reinterpret_cast<void***>(list);AcquireSRWLockExclusive(&hooksLock);
 for(const auto& h:hooks)if(h.table==table){ReleaseSRWLockExclusive(&hooksLock);return;}
 for(auto& h:hooks)if(!h.table){h={reinterpret_cast<Bundle>(table[27]),reinterpret_cast<Indirect>(table[59]),table,reinterpret_cast<Root>(table[29]),reinterpret_cast<Table>(table[31]),reinterpret_cast<Cbv>(table[37]),reinterpret_cast<Heaps>(table[28])};
 bool ok=PatchSlot(&table[27],reinterpret_cast<void*>(h.bundle),reinterpret_cast<void*>(&BundleHook));
 ok=PatchSlot(&table[59],reinterpret_cast<void*>(h.indirect),reinterpret_cast<void*>(&IndirectHook))&&ok;
 ok=PatchSlot(&table[29],reinterpret_cast<void*>(h.root),reinterpret_cast<void*>(&RootHook))&&ok;
 ok=PatchSlot(&table[31],reinterpret_cast<void*>(h.descriptor),reinterpret_cast<void*>(&TableHook))&&ok;
 ok=PatchSlot(&table[37],reinterpret_cast<void*>(h.cbv),reinterpret_cast<void*>(&CbvHook))&&ok;
 ok=PatchSlot(&table[28],reinterpret_cast<void*>(h.heaps),reinterpret_cast<void*>(&HeapsHook))&&ok;
 if(!ok)Fail("hook_install",L"FAILED: binding hook setup. Restart and send logs.");Trace("GI_SPOT_R274_HOOKS ready=%u",unsigned(ok));ReleaseSRWLockExclusive(&hooksLock);return;}
 Fail("hook_capacity",L"FAILED: binding hook capacity. Send logs.");ReleaseSRWLockExclusive(&hooksLock);
}
inline void Bind(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {if(Enabled())list->SetPrivateDataInterface(psoKey,pso);}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled())return;AcquireSRWLockExclusive(&lock);
 if(recordedList.Get()==list&&(phase.load()==Phase::Recorded||phase.load()==Phase::Recording)){Fail("discarded_before_submission",L"FAILED: recording reset before submission. Send logs.");}
 State s=Read(list);const UINT64 next=s.generation+1;s={};s.generation=next;Save(list,s);list->SetPrivateDataInterface(rootKey,nullptr);ReleaseSRWLockExclusive(&lock);
}
inline HRESULT Buffer(ID3D12Device* d,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags,ComPtr<ID3D12Resource>& out) noexcept {
 D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;hp.CreationNodeMask=hp.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=heap==D3D12_HEAP_TYPE_UPLOAD?256:Bytes();rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 return d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&out));
}
inline bool Initialize(ID3D12Device* d) noexcept {
 if(device)return device.Get()==d&&enabledReady.load();device=d;
 HRESULT hr=Buffer(d,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,output);
 if(SUCCEEDED(hr))hr=Buffer(d,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,readback);
 if(SUCCEEDED(hr))hr=Buffer(d,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,zero);
 if(SUCCEEDED(hr))hr=d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
 if(SUCCEEDED(hr)){void* p=nullptr;D3D12_RANGE r{0,0};hr=zero->Map(0,&r,&p);if(SUCCEEDED(hr)){memset(p,0,256);zero->Unmap(0,nullptr);}}
 enabledReady.store(SUCCEEDED(hr));Trace("GI_SPOT_R274_SETUP ready=%u bytes=%u capacity=%u hr=0x%08lX",unsigned(SUCCEEDED(hr)),Bytes(),Capacity(),static_cast<unsigned long>(hr));return SUCCEEDED(hr);
}
inline void Observe(ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native,CreateCompute create) noexcept {
 if(!Enabled()||!desc||!desc->pRootSignature||!desc->CS.pShaderBytecode)return;
 const auto& shader=JointTrace()?gi_spot_trace_shader::jointShader:RayTrace()?gi_spot_trace_shader::rayShader:gi_spot_trace_shader::shaders[0];if(desc->CS.BytecodeLength!=shader.nativeSize)return;
 char sha[65]{};if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||strcmp(sha,shader.nativeSha))return;
 unsigned char data[8192]{};UINT size=sizeof(data);
 if(FAILED(desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&size,data)))return;
 char rootSha[65]{};if(!PTReflectionSha256(data,size,rootSha)||strcmp(rootSha,"451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3")){Trace("GI_SPOT_R274_REJECT reason=unreviewed_root");return;}
 ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
 if(FAILED(D3D12CreateVersionedRootSignatureDeserializer(data,size,IID_PPV_ARGS(&decoder))))return;
 const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* version=nullptr;if(FAILED(decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&version)))return;
 auto rd=version->Desc_1_0;if(rd.NumParameters!=8)return;
 D3D12_ROOT_PARAMETER parameters[10]{};memcpy(parameters,rd.pParameters,8*sizeof(*parameters));
 parameters[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[8].Descriptor={0,2};parameters[8].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 parameters[9].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;parameters[9].Constants={0,2,4};
 rd.NumParameters=JointTrace()?10:9;rd.pParameters=parameters;
 ComPtr<ID3DBlob> blob,error;HRESULT hr=D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error);
 ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pso,owner;
 if(SUCCEEDED(hr))hr=d->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root));
 auto replacement=*desc;replacement.pRootSignature=root.Get();replacement.CS={shader.bytes,shader.size};replacement.CachedPSO={};
 if(SUCCEEDED(hr))hr=create(d,&replacement,IID_PPV_ARGS(&pso));
 if(SUCCEEDED(hr))hr=native->QueryInterface(IID_PPV_ARGS(&owner));
 if(SUCCEEDED(hr))hr=pso->SetPrivateDataInterface(traceRootKey,root.Get());
 if(SUCCEEDED(hr))hr=pso->SetPrivateDataInterface(nativeRootKey,desc->pRootSignature);
 if(SUCCEEDED(hr))hr=owner->SetPrivateDataInterface(variantKey,pso.Get());
 if(SUCCEEDED(hr)){AcquireSRWLockExclusive(&lock);const bool ready=Initialize(d);ReleaseSRWLockExclusive(&lock);if(!ready){owner->SetPrivateDataInterface(variantKey,nullptr);hr=E_FAIL;}}
 Trace("GI_SPOT_R274_PIPELINE variant=6 ready=%u hr=0x%08lX",unsigned(SUCCEEDED(hr)),static_cast<unsigned long>(hr));
}
inline void Rebind(ID3D12GraphicsCommandList* list,const Hooks& h,ID3D12RootSignature* root,const State& s) noexcept {
 h.root(list,root);for(UINT i=0;i<8;++i){if(!(s.mask&(1u<<i)))continue;if(i>=2&&i<=5)h.cbv(list,i,s.values[i]);else h.descriptor(list,i,{s.values[i]});}
}
inline void Transition(ID3D12GraphicsCommandList* list,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) noexcept {
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={output.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
}
// Runs the native dispatch once, with only our extra raw UAV and bytecode stores.
inline bool Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) noexcept {
 if(!Enabled()||!enabledReady.load())return false;
 ComPtr<ID3D12PipelineState> pso,trace;ComPtr<ID3D12RootSignature> root,nativeRoot;
 if(!Attached(list,psoKey,pso)||!Attached(pso.Get(),variantKey,trace)||!Attached(trace.Get(),traceRootKey,root)||!Attached(trace.Get(),nativeRootKey,nativeRoot))return false;
 gridWidth.store(x*8);gridHeight.store(y*16);
 if(phase.load()!=Phase::Armed)return false;
 const State s=Read(list);const auto h=LookupHooks(list);const auto native=Lookup(list);
 if((s.mask&0xfdu)!=0xfdu||s.root!=nativeRoot.Get()||!h.root||!h.descriptor||!h.cbv||!native.set||!native.dispatch){Trace("GI_SPOT_R274_WAIT binding_mask=%u",s.mask);return false;}
 AcquireSRWLockExclusive(&lock);if(phase.load()!=Phase::Armed){ReleaseSRWLockExclusive(&lock);return false;}
 if(savedFrames.load()&&(groups[0]!=x||groups[1]!=y||groups[2]!=z)){Fail("sequence_grid_changed",L"FAILED: resolution changed during capture. Exit and send logs.");ReleaseSRWLockExclusive(&lock);return false;}
 phase.store(Phase::Recording);recordedList=list;retainedPso=pso;retainedTrace=trace;retainedRoot=nativeRoot;retainedTraceRoot=root;generation=s.generation;capturedPresent=presentCount.load();groups[0]=x;groups[1]=y;groups[2]=z;
 UINT roi[4]={(x*8>128)?x*4-64:0,(y*16>128)?y*8-64:0,128,128};
 if(JointTrace()){
  void* initial=nullptr;D3D12_RANGE none{0,0};const HRESULT mapHr=zero->Map(0,&none,&initial);
  if(FAILED(mapHr)){Fail("joint_header_map",L"FAILED: joint trace header. Send logs.");ReleaseSRWLockExclusive(&lock);return false;}
  auto* header=static_cast<UINT*>(initial);header[0]=0;header[1]=0x284;header[2]=roi[0];header[3]=roi[1];D3D12_RANGE written{0,16};zero->Unmap(0,&written);
  Trace("GI_JOINT_R284_ROI x=%u y=%u width=128 height=128 stride=2 dense_center=32 frame=%u grid_center=1",roi[0],roi[1],savedFrames.load()+1);
 }
 if(savedFrames.load())Transition(list,D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COPY_DEST);
 list->CopyBufferRegion(output.Get(),0,zero.Get(),0,16);Transition(list,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 Rebind(list,h,root.Get(),s);list->SetComputeRootUnorderedAccessView(8,output->GetGPUVirtualAddress());if(JointTrace())list->SetComputeRoot32BitConstants(9,4,roi,0);native.set(list,trace.Get());native.dispatch(list,x,y,z);
 Rebind(list,h,nativeRoot.Get(),s);native.set(list,pso.Get());
 Transition(list,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),0,output.Get(),0,Bytes());
 phase.store(Phase::Recorded);Trace("GI_SPOT_R274_RECORDED present=%llu generation=%llu groups=%u,%u,%u root_and_pso_restored=1 native_dispatches=1",capturedPresent,generation,x,y,z);ReleaseSRWLockExclusive(&lock);return true;
}
inline void AfterExecute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled()||phase.load()!=Phase::Recorded)return;AcquireSRWLockExclusive(&lock);
 if(phase.load()==Phase::Recorded)for(UINT i=0;i<count;++i)if(lists[i]==recordedList.Get()){
 if(Read(recordedList.Get()).generation!=generation){Fail("submission_generation",L"FAILED: submission generation changed. Send logs.");break;}
 submittedQueue=queue;const HRESULT hr=queue->Signal(fence.Get(),savedFrames.load()+1);if(SUCCEEDED(hr))phase.store(Phase::Submitted);else Fail("fence_signal",L"FAILED: GPU completion signal. Send logs.");Trace("GI_SPOT_R274_SUBMITTED hr=0x%08lX generation=%llu",static_cast<unsigned long>(hr),generation);break;}
 ReleaseSRWLockExclusive(&lock);
}
inline void Poll() noexcept {
 if(!Enabled())return;
 auto p=phase.load();if((p==Phase::Armed||p==Phase::Recorded||p==Phase::Submitted)&&GetTickCount64()-armedAt>30000){Fail("timeout",L"FAILED: capture timed out. Exit and send logs.");return;}
 if(p!=Phase::Submitted)return;const auto completed=fence->GetCompletedValue();if(completed==UINT64_MAX){Fail("device_removed",L"FAILED: graphics device removed. Send logs.");return;}if(completed<savedFrames.load()+1)return;
 Phase expected=Phase::Submitted;if(!phase.compare_exchange_strong(expected,Phase::Exporting))return;
 void* mapped=nullptr;D3D12_RANGE range{0,Bytes()};HRESULT hr=readback->Map(0,&range,&mapped);
 bool ok=false;UINT records=0;wchar_t path[MAX_PATH]{};
 if(SUCCEEDED(hr)){
  memcpy(&records,mapped,4);AcquireSRWLockExclusive(&reportLock);const bool opened=report.OpenDefault();if(opened)wcscpy_s(path,report.Path());ReleaseSRWLockExclusive(&reportLock);
  if(opened){auto* slash=wcsrchr(path,L'\\');if(slash){*(slash+1)=0;wchar_t name[80]{};if(JointTrace())swprintf_s(name,L"GI-Joint-R284-%lu-frame%02u.bin",GetCurrentProcessId(),savedFrames.load()+1);else swprintf_s(name,RayTrace()?L"GI-Ray-R278-%lu.bin":L"GI-Spot-R274-%lu.bin",GetCurrentProcessId());wcscat_s(path,name);
   const HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(file!=INVALID_HANDLE_VALUE){DWORD written=0;const DWORD size=16+(std::min)(records,Capacity())*stride;ok=WriteFile(file,mapped,size,&written,nullptr)&&written==size;CloseHandle(file);}
  }}D3D12_RANGE none{0,0};readback->Unmap(0,&none);
 }
 Trace("GI_SPOT_R274_EXPORT complete=%u gpu_completion_confirmed=1 records=%u retained=%u overflow=%u stride=%u present=%llu groups=%u,%u,%u variant=6 sampling_step=32",unsigned(ok&&records&&records<=Capacity()),records,(std::min)(records,Capacity()),unsigned(records>Capacity()),stride,capturedPresent,groups[0],groups[1],groups[2]);
 if(JointTrace())Trace("GI_JOINT_R284_EXPORT complete=%u records=%u overflow=%u present=%llu frame=%u frames=8 same_dispatch=1",unsigned(ok&&records&&records<=Capacity()),records,unsigned(records>Capacity()),capturedPresent,savedFrames.load()+1);
 else if(RayTrace())Trace("GI_RAY_R278_EXPORT complete=%u records=%u overflow=%u present=%llu final_sum_and_rays=1 no_reuse_accounting=1",unsigned(ok),records,unsigned(records>Capacity()),capturedPresent);
 if(ok&&!records){Fail("empty_capture",L"FAILED: no GI samples recorded. Exit and send logs.");return;}
 if(ok&&records>Capacity()){Fail("overflow",L"FAILED: record capacity exceeded. Partial frame saved; send logs.");return;}
 if(ok){
  AcquireSRWLockExclusive(&lock);
  if(phase.load()!=Phase::Exporting){ReleaseSRWLockExclusive(&lock);return;}
  const auto saved=++savedFrames;
  recordedList.Reset();retainedPso.Reset();retainedTrace.Reset();retainedRoot.Reset();retainedTraceRoot.Reset();submittedQueue.Reset();
  nextFrameAt=GetTickCount64()+250;phase.store(saved<FrameCount()?Phase::Gap:Phase::Complete);ReleaseSRWLockExclusive(&lock);
 }else Fail("export",L"FAILED: could not save trace. Exit and send logs.");
}
}
