// Included within native_lighting, after GI binding audit. Diagnostic-only.
namespace gi_spot_trace {
using Microsoft::WRL::ComPtr;
constexpr UINT capacity=65536,stride=256,bytes=16+capacity*stride;
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
enum class Phase{Idle,Armed,Recording,Recorded,Submitted,Exporting,Complete,Failed};
inline std::atomic<Phase> phase{Phase::Idle};
inline ComPtr<ID3D12Device> device;
inline ComPtr<ID3D12Resource> output,readback,zero;
inline ComPtr<ID3D12Fence> fence;
inline ComPtr<ID3D12GraphicsCommandList> recordedList;
inline ComPtr<ID3D12PipelineState> retainedPso,retainedTrace;
inline ComPtr<ID3D12RootSignature> retainedRoot,retainedTraceRoot;
inline ComPtr<ID3D12CommandQueue> submittedQueue;
inline UINT64 generation=0,capturedPresent=0,armedAt=0;
inline UINT groups[3]{};
inline bool Enabled() noexcept {static const bool value=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_SPOT_TRACE",b,4)==1&&b[0]=='1';}();return value;}
inline State Read(ID3D12GraphicsCommandList* list) noexcept {State s{};UINT n=sizeof(s);if(FAILED(list->GetPrivateData(stateKey,&n,&s))||n!=sizeof(s))s={};return s;}
inline void Save(ID3D12GraphicsCommandList* list,const State& s) noexcept {if(FAILED(list->SetPrivateData(stateKey,sizeof(s),&s)))phase.store(Phase::Failed);}
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
 if(!ok)phase.store(Phase::Failed);Trace("GI_SPOT_R274_HOOKS ready=%u",unsigned(ok));ReleaseSRWLockExclusive(&hooksLock);return;}
 phase.store(Phase::Failed);ReleaseSRWLockExclusive(&hooksLock);
}
inline void Bind(ID3D12GraphicsCommandList* list,ID3D12PipelineState* pso) noexcept {if(Enabled())list->SetPrivateDataInterface(psoKey,pso);}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled())return;AcquireSRWLockExclusive(&lock);
 if(recordedList.Get()==list&&(phase.load()==Phase::Recorded||phase.load()==Phase::Recording)){phase.store(Phase::Failed);Trace("GI_SPOT_R274_FAILED reason=discarded_before_submission");}
 State s=Read(list);const UINT64 next=s.generation+1;s={};s.generation=next;Save(list,s);list->SetPrivateDataInterface(rootKey,nullptr);ReleaseSRWLockExclusive(&lock);
}
inline HRESULT Buffer(ID3D12Device* d,D3D12_HEAP_TYPE heap,D3D12_RESOURCE_STATES state,D3D12_RESOURCE_FLAGS flags,ComPtr<ID3D12Resource>& out) noexcept {
 D3D12_HEAP_PROPERTIES hp{};hp.Type=heap;hp.CreationNodeMask=hp.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=bytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;rd.Flags=flags;
 return d->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,state,nullptr,IID_PPV_ARGS(&out));
}
inline bool Initialize(ID3D12Device* d) noexcept {
 if(device)return device.Get()==d&&enabledReady.load();device=d;
 HRESULT hr=Buffer(d,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,output);
 if(SUCCEEDED(hr))hr=Buffer(d,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_FLAG_NONE,readback);
 if(SUCCEEDED(hr))hr=Buffer(d,D3D12_HEAP_TYPE_UPLOAD,D3D12_RESOURCE_STATE_GENERIC_READ,D3D12_RESOURCE_FLAG_NONE,zero);
 if(SUCCEEDED(hr))hr=d->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
 if(SUCCEEDED(hr)){void* p=nullptr;D3D12_RANGE r{0,0};hr=zero->Map(0,&r,&p);if(SUCCEEDED(hr)){memset(p,0,bytes);zero->Unmap(0,nullptr);}}
 enabledReady.store(SUCCEEDED(hr));Trace("GI_SPOT_R274_SETUP ready=%u bytes=%u capacity=%u hr=0x%08lX",unsigned(SUCCEEDED(hr)),bytes,capacity,static_cast<unsigned long>(hr));return SUCCEEDED(hr);
}
inline void Observe(ID3D12Device* d,const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc,IUnknown* native,CreateCompute create) noexcept {
 if(!Enabled()||!desc||!desc->pRootSignature||!desc->CS.pShaderBytecode)return;
 const auto& shader=gi_spot_trace_shader::shaders[0];if(desc->CS.BytecodeLength!=shader.nativeSize)return;
 char sha[65]{};if(!PTReflectionSha256(static_cast<const unsigned char*>(desc->CS.pShaderBytecode),desc->CS.BytecodeLength,sha)||strcmp(sha,shader.nativeSha))return;
 unsigned char data[8192]{};UINT size=sizeof(data);
 if(FAILED(desc->pRootSignature->GetPrivateData(rt_spot_capture_audit::kRootBytes,&size,data)))return;
 char rootSha[65]{};if(!PTReflectionSha256(data,size,rootSha)||strcmp(rootSha,"451162124dcb4e610eb1ee4939b9f0d7963ec3be5d475ed8986799846a22bee3")){Trace("GI_SPOT_R274_REJECT reason=unreviewed_root");return;}
 ComPtr<ID3D12VersionedRootSignatureDeserializer> decoder;
 if(FAILED(D3D12CreateVersionedRootSignatureDeserializer(data,size,IID_PPV_ARGS(&decoder))))return;
 const D3D12_VERSIONED_ROOT_SIGNATURE_DESC* version=nullptr;if(FAILED(decoder->GetRootSignatureDescAtVersion(D3D_ROOT_SIGNATURE_VERSION_1_0,&version)))return;
 auto rd=version->Desc_1_0;if(rd.NumParameters!=8)return;
 D3D12_ROOT_PARAMETER parameters[9]{};memcpy(parameters,rd.pParameters,8*sizeof(*parameters));
 parameters[8].ParameterType=D3D12_ROOT_PARAMETER_TYPE_UAV;parameters[8].Descriptor={0,2};parameters[8].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
 rd.NumParameters=9;rd.pParameters=parameters;
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
 h.root(list,root);for(UINT i=0;i<8;++i){if(i>=2&&i<=5)h.cbv(list,i,s.values[i]);else h.descriptor(list,i,{s.values[i]});}
}
inline void Transition(ID3D12GraphicsCommandList* list,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) noexcept {
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={output.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,before,after};list->ResourceBarrier(1,&b);
}
// Runs the native dispatch once, with only our extra raw UAV and bytecode stores.
inline bool Dispatch(ID3D12GraphicsCommandList* list,UINT x,UINT y,UINT z) noexcept {
 if(!Enabled()||phase.load()!=Phase::Armed||!enabledReady.load())return false;
 ComPtr<ID3D12PipelineState> pso,trace;ComPtr<ID3D12RootSignature> root,nativeRoot;
 if(!Attached(list,psoKey,pso)||!Attached(pso.Get(),variantKey,trace)||!Attached(trace.Get(),traceRootKey,root)||!Attached(trace.Get(),nativeRootKey,nativeRoot))return false;
 const State s=Read(list);const auto h=LookupHooks(list);const auto native=Lookup(list);
 if(s.mask!=255||s.root!=nativeRoot.Get()||!h.root||!h.descriptor||!h.cbv||!native.set||!native.dispatch){Trace("GI_SPOT_R274_WAIT binding_mask=%u",s.mask);return false;}
 AcquireSRWLockExclusive(&lock);if(phase.load()!=Phase::Armed){ReleaseSRWLockExclusive(&lock);return false;}
 phase.store(Phase::Recording);recordedList=list;retainedPso=pso;retainedTrace=trace;retainedRoot=nativeRoot;retainedTraceRoot=root;generation=s.generation;capturedPresent=presentCount.load();groups[0]=x;groups[1]=y;groups[2]=z;
 list->CopyBufferRegion(output.Get(),0,zero.Get(),0,bytes);Transition(list,D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
 Rebind(list,h,root.Get(),s);list->SetComputeRootUnorderedAccessView(8,output->GetGPUVirtualAddress());native.set(list,trace.Get());native.dispatch(list,x,y,z);
 Rebind(list,h,nativeRoot.Get(),s);native.set(list,pso.Get());
 Transition(list,D3D12_RESOURCE_STATE_UNORDERED_ACCESS,D3D12_RESOURCE_STATE_COPY_SOURCE);list->CopyBufferRegion(readback.Get(),0,output.Get(),0,bytes);
 phase.store(Phase::Recorded);Trace("GI_SPOT_R274_RECORDED present=%llu generation=%llu groups=%u,%u,%u root_and_pso_restored=1 native_dispatches=1",capturedPresent,generation,x,y,z);ReleaseSRWLockExclusive(&lock);return true;
}
inline void AfterExecute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled()||phase.load()!=Phase::Recorded)return;AcquireSRWLockExclusive(&lock);
 if(phase.load()==Phase::Recorded)for(UINT i=0;i<count;++i)if(lists[i]==recordedList.Get()){
 if(Read(recordedList.Get()).generation!=generation){phase.store(Phase::Failed);break;}
 submittedQueue=queue;const HRESULT hr=queue->Signal(fence.Get(),1);phase.store(SUCCEEDED(hr)?Phase::Submitted:Phase::Failed);Trace("GI_SPOT_R274_SUBMITTED hr=0x%08lX generation=%llu",static_cast<unsigned long>(hr),generation);break;}
 ReleaseSRWLockExclusive(&lock);
}
inline void Poll() noexcept {
 if(!Enabled())return;
 static bool held=false;const bool down=(GetAsyncKeyState(VK_F7)&0x8000)!=0&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_SHIFT)&0x8000);
 DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
 if(down&&!held&&foregroundPid==GetCurrentProcessId()&&enabledReady.load()&&phase.load()==Phase::Idle){armedAt=GetTickCount64();phase.store(Phase::Armed);Trace("GI_SPOT_R274_ARMED bounded_capture=1 key=F7");}held=down;
 auto p=phase.load();if((p==Phase::Armed||p==Phase::Recorded||p==Phase::Submitted)&&GetTickCount64()-armedAt>30000){phase.store(Phase::Failed);Trace("GI_SPOT_R274_FAILED reason=timeout resources_retained=1");return;}
 if(p!=Phase::Submitted)return;const auto completed=fence->GetCompletedValue();if(completed==UINT64_MAX){phase.store(Phase::Failed);return;}if(completed<1)return;
 phase.store(Phase::Exporting);void* mapped=nullptr;D3D12_RANGE range{0,bytes};HRESULT hr=readback->Map(0,&range,&mapped);
 bool ok=false;UINT records=0;wchar_t path[MAX_PATH]{};
 if(SUCCEEDED(hr)){
  memcpy(&records,mapped,4);AcquireSRWLockExclusive(&reportLock);const bool opened=report.OpenDefault();if(opened)wcscpy_s(path,report.Path());ReleaseSRWLockExclusive(&reportLock);
  if(opened){auto* slash=wcsrchr(path,L'\\');if(slash){*(slash+1)=0;wchar_t name[80]{};swprintf_s(name,L"GI-Spot-R274-%lu.bin",GetCurrentProcessId());wcscat_s(path,name);
   const HANDLE file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
   if(file!=INVALID_HANDLE_VALUE){DWORD written=0;const DWORD size=16+(std::min)(records,capacity)*stride;ok=WriteFile(file,mapped,size,&written,nullptr)&&written==size;CloseHandle(file);}
  }}D3D12_RANGE none{0,0};readback->Unmap(0,&none);
 }
 Trace("GI_SPOT_R274_EXPORT complete=%u gpu_completion_confirmed=1 records=%u retained=%u overflow=%u stride=%u present=%llu groups=%u,%u,%u variant=6 sampling_step=32",unsigned(ok),records,(std::min)(records,capacity),unsigned(records>capacity),stride,capturedPresent,groups[0],groups[1],groups[2]);
 phase.store(ok?Phase::Complete:Phase::Failed);
}
}
