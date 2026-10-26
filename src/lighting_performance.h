#pragma once
// Included inside native_lighting. Timestamp resources and list references are
// retained for the process lifetime: a native list may be submitted again.
// Query indices are never recycled, including after a cancelled capture.
namespace performance {
enum class Stage : unsigned { None, ShadowRays, OtherRays, Selector, History, Spatial0, Spatial1, Composition, OtherCompute, Count };
inline constexpr unsigned kStageCount=static_cast<unsigned>(Stage::Count);
inline const char* Name(Stage s) noexcept {
    constexpr const char* names[]={"none","shadow_rays","other_rays","selector","history","spatial0","spatial1","composition","other_compute"};
    return names[static_cast<unsigned>(s)];
}
enum class State : unsigned { Recording, Recorded, Submitting, Submitted, Read, Discarded };
inline constexpr unsigned kCapacity=16384,kSampleInterval=128,kInvalid=~0u;
struct Sample {
    ID3D12GraphicsCommandList* list=nullptr;
    State state=State::Recording;Stage stage=Stage::None;
    UINT64 capture=0,present=0,selection=0,fence=0,frequency=0,tableBytes=0;
    char sourceShader[65]{};
    UINT x=0,y=0,z=0;bool rr=false,current=true;unsigned effects=0;
};
inline SRWLOCK lock=SRWLOCK_INIT;
inline ID3D12Device* device=nullptr;
inline ID3D12QueryHeap* queries=nullptr;
inline ID3D12Resource* readback=nullptr;
inline ID3D12Fence* fence=nullptr;
inline ID3D12CommandQueue* queue=nullptr;
inline UINT64* mapped=nullptr;
inline UINT64 frequency=0,nextFence=0;
inline std::atomic<bool> submitting{false};
inline Sample samples[kCapacity]{};
inline unsigned count=0;
inline std::atomic<bool> enabled{false},watch{false},fault{false};
inline std::atomic<UINT64> capture{0},skipped{0};
inline void RememberDevice(ID3D12Device* value) noexcept {
    AcquireSRWLockExclusive(&lock);
    if(!device && value){device=value;device->AddRef();}
    ReleaseSRWLockExclusive(&lock);
}
// UI thread only. No allocations in Begin/End or submission callbacks.
inline bool Prepare() noexcept {
    if(fault.load() || !device || count+2048>kCapacity)return false;
    if(queries)return mapped && fence;
    D3D12_QUERY_HEAP_DESC q{};q.Type=D3D12_QUERY_HEAP_TYPE_TIMESTAMP;q.Count=kCapacity*2;
    HRESULT hr=device->CreateQueryHeap(&q,IID_PPV_ARGS(&queries));
    D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
    D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=kCapacity*16;
    d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    if(SUCCEEDED(hr))hr=device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback));
    if(SUCCEEDED(hr))hr=device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
    D3D12_RANGE range{0,kCapacity*16};
    if(SUCCEEDED(hr))hr=readback->Map(0,&range,reinterpret_cast<void**>(&mapped));
    if(FAILED(hr)){fault=true;return false;}return true;
}
inline unsigned Begin(ID3D12GraphicsCommandList* list,Stage stage,UINT x,UINT y,UINT z,UINT64 tableBytes=0,const char* sourceShader=nullptr) noexcept {
    if(!enabled.load(std::memory_order_acquire) || fault.load() || stage==Stage::None || !list)return kInvalid;
    const auto present=presentCount.load();if(present%kSampleInterval)return kInvalid;
    if(list->GetType()!=D3D12_COMMAND_LIST_TYPE_DIRECT)return kInvalid;
    if(!TryAcquireSRWLockExclusive(&lock)){++skipped;return kInvalid;}
    if(!enabled.load(std::memory_order_acquire)){ReleaseSRWLockExclusive(&lock);return kInvalid;}
    if(count==kCapacity){++skipped;ReleaseSRWLockExclusive(&lock);return kInvalid;}
    const unsigned index=count++;
    auto& s=samples[index];s.list=list;list->AddRef();s.stage=stage;s.capture=capture.load();s.present=present;
    for(unsigned i=0;i<64;++i)s.sourceShader[i]=sourceShader?sourceShader[i]:0;
    s.sourceShader[64]=0;
    s.selection=selection.Read();s.x=x;s.y=y;s.z=z;s.tableBytes=tableBytes;
    s.rr=control_rr_diagnostic::lastSelected.load();s.effects=control_rr_diagnostic::lastEffects.load();
    watch.store(true,std::memory_order_release);
    ReleaseSRWLockExclusive(&lock);
    list->EndQuery(queries,D3D12_QUERY_TYPE_TIMESTAMP,index*2);
    return index;
}
inline void End(unsigned index) noexcept {
    if(index==kInvalid)return;
    auto& s=samples[index];
    s.list->EndQuery(queries,D3D12_QUERY_TYPE_TIMESTAMP,index*2+1);
    s.list->ResolveQueryData(queries,D3D12_QUERY_TYPE_TIMESTAMP,index*2,2,readback,UINT64(index)*16);
    AcquireSRWLockExclusive(&lock);s.state=State::Recorded;ReleaseSRWLockExclusive(&lock);
}
inline void Reset(ID3D12GraphicsCommandList* list) noexcept {
    if(!watch.load())return;
    AcquireSRWLockExclusive(&lock); // Metadata only; UI never holds this lock during file I/O.
    for(unsigned i=0;i<count;++i)if(samples[i].list==list && samples[i].current){
        samples[i].current=false;
        if(samples[i].state==State::Recorded || samples[i].state==State::Recording)samples[i].state=State::Discarded;
    }
    ReleaseSRWLockExclusive(&lock);
}
// Called before the actual ExecuteCommandLists; only its exact list set is leased.
inline UINT64 BeforeExecute(ID3D12CommandQueue* q,UINT n,ID3D12CommandList*const* lists) noexcept {
    if(!watch.load() || fault.load())return 0;
    AcquireSRWLockExclusive(&lock); // Submission/reset bookkeeping must not be dropped on contention.
    bool found=false;
    for(unsigned i=0;i<count;++i){auto& s=samples[i];
        if(!s.current || s.state==State::Discarded)continue;
        for(UINT j=0;j<n;++j)if(s.list==lists[j]){
            // Read or submitted queries cannot be measured twice. Keep resources
            // alive, invalidate this capture and leave rendering untouched.
            if(s.state!=State::Recorded){fault=true;break;}found=true;
        }
    }
    if(found && !queue){
        queue=q;queue->AddRef();
        if(FAILED(q->GetTimestampFrequency(&frequency)) || !frequency)fault=true;
    }
    if(found && queue!=q)fault=true;
    UINT64 value=0;
    if(found && !fault.load()){
        if(submitting.exchange(true)){fault=true;ReleaseSRWLockExclusive(&lock);return 0;}
        value=++nextFence;
        for(unsigned i=0;i<count;++i){auto& s=samples[i];if(!s.current || s.state!=State::Recorded)continue;
            for(UINT j=0;j<n;++j)if(s.list==lists[j]){s.state=State::Submitting;s.fence=value;s.frequency=frequency;break;}
        }
    }
    ReleaseSRWLockExclusive(&lock);return value;
}
inline void AfterExecute(ID3D12CommandQueue* q,UINT64 value) noexcept {
    if(!value)return;
    const HRESULT hr=q->Signal(fence,value);
    AcquireSRWLockExclusive(&lock);
    if(FAILED(hr))fault=true;
    for(unsigned i=0;i<count;++i)if(samples[i].state==State::Submitting && samples[i].fence==value)
        samples[i].state=SUCCEEDED(hr)?State::Submitted:State::Discarded;
    ReleaseSRWLockExclusive(&lock);submitting=false;
}
// UI polling only. No queue waits, disk writes, or polling on a render thread.
template<class Emit> inline unsigned Poll(UINT64 session,Emit emit) noexcept {
    if(!fence || fault.load())return 0;
    unsigned pending=0;
    const auto completed=fence->GetCompletedValue();
    if(completed==UINT64_MAX){fault=true;return 0;}
    for(unsigned i=0;;++i){
        if(!TryAcquireSRWLockExclusive(&lock))return pending+1;
        if(i>=count){ReleaseSRWLockExclusive(&lock);break;}
        auto& s=samples[i];Sample copy{};double ms=0;bool output=false;
        if(s.capture==session){
            if(s.state==State::Submitted && s.fence<=completed){
                const UINT64 a=mapped[i*2],b=mapped[i*2+1];
                if(b>=a && s.frequency){copy=s;ms=1000.0*double(b-a)/double(s.frequency);output=true;}
                else fault=true;
                s.state=State::Read;
            }else if(s.state==State::Recording || s.state==State::Recorded || s.state==State::Submitting || s.state==State::Submitted)++pending;
        }
        ReleaseSRWLockExclusive(&lock);
        if(output)emit(copy,ms); // File I/O occurs after releasing the metadata lock.
    }
    return pending;
}

inline thread_local ID3D12GraphicsCommandList* computeList=nullptr;
inline thread_local Stage computeStage=Stage::None;
inline thread_local char computeSourceShader[65]{};
inline void Bind(ID3D12GraphicsCommandList* list,Stage stage,const char* shader=nullptr) noexcept {
    computeList=list;computeStage=stage;
    for(unsigned i=0;i<64;++i)computeSourceShader[i]=shader?shader[i]:0;
    computeSourceShader[64]=0;
}
inline const char* ComputeShader(ID3D12GraphicsCommandList* list) noexcept {
    return computeList==list?computeSourceShader:nullptr;
}
inline Stage ComputeStage(ID3D12GraphicsCommandList* list) noexcept {return computeList==list?computeStage:Stage::None;}
} // namespace performance
