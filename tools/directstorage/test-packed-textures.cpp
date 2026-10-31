#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dstorage.h>
#include <wrl/client.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
static void Log(const char*,...){}
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(const void*){return nullptr;}
static bool Rel32Fits(const void*,const void*){return false;}
static HMODULE selfModule=nullptr;
static std::wstring ModulePath(HMODULE){return L"";}
#include "../../src/native_cpu_call_patch.h"
#include "../../src/native_loader_trace_runtime.h"
#include "../../src/native_texture_contract_runtime.h"
static bool GetFGDirectStorageEnabled(){return true;}
#include "../../src/native_gpu_storage_runtime.h"
using Microsoft::WRL::ComPtr;
using Batch=control_storage::PackedTextureBatch;
static void Check(bool value,const char* name){if(!value){printf("FAIL %s error=%lu\n",name,GetLastError());ExitProcess(1);}}
static void Hr(HRESULT hr,const char* name){if(FAILED(hr)){printf("FAIL %s hr=%08lx\n",name,static_cast<unsigned long>(hr));ExitProcess(2);}}
static ComPtr<ID3D12Resource> Resource(ID3D12Device* d,const D3D12_RESOURCE_DESC& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
 D3D12_HEAP_PROPERTIES h{};h.Type=type;ComPtr<ID3D12Resource> r;Hr(d->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r)),"resource");return r;
}
static unsigned refs=0,published=0,native=0,failedPublished=0;
static bool expectFinishSuccess=true;
static int enginePending=0;
static void Adjust(void*,int delta){enginePending+=delta;Check(enginePending>=0,"native pending count never underflows");}
static ComPtr<ID3D12Resource> testResource;
static void Add(void*){++refs;}
static void Remove(void*){Check(refs!=0,"balanced engine reference");--refs;}
static void Finish(void*,int level,bool success){Check(level==2&&success==expectFinishSuccess&&refs==1&&enginePending==1,"publish while reference held");if(success)++published;else ++failedPublished;}
static std::int64_t Native(void*,const char*,void*,std::uint64_t offset,std::uint64_t size,char*,int thread,int priority,bool,void*){Check(offset==17&&size==64&&thread==14&&priority==7,"native fallback arguments");++native;SetLastError(456);return 123;}
static bool Describe(void*,unsigned,std::uint64_t,native_gpu_storage::Job& job){job.resource=testResource;return true;}
static bool Resolve(const char*,std::uint64_t,std::uint64_t,native_gpu_storage::Job&){return true;}
static unsigned char lookupPack[0x300]{};
static void Put(void* p,unsigned offset,std::uint64_t v){memcpy(static_cast<unsigned char*>(p)+offset,&v,8);}
static void Lookup(void* fs,void* record,std::uint64_t count,int stride){
 Check(fs==reinterpret_cast<void*>(0x8888)&&count==1&&stride==0x188,"native lookup ABI");
 Put(record,0x120,reinterpret_cast<std::uint64_t>(lookupPack));Put(record,0x128,900713);Put(record,0x130,10000);
}
static void ResolveRange(IDStorageFactory* factory,const wchar_t* path){
 namespace n=native_gpu_storage;std::vector<unsigned char> runtime(0x3a3180);n::runtime=runtime.data();n::lookup=&Lookup;n::factory=factory;
 Put(runtime.data(),0x3a3178,0x8888);Put(lookupPack,0x288,900000);
 char host[1024]{};Check(WideCharToMultiByte(CP_UTF8,0,path,-1,host,1024,nullptr,nullptr)!=0,"host path");Put(lookupPack,0x170,reinterpret_cast<std::uint64_t>(host));
 n::Job job;Check(n::Resolve("texture",17,64,job)&&job.sourceOffset==730&&job.file,"archive sort bias is not a physical offset");
 Check(!n::Resolve("texture",9999,64,job),"native asset range checked before archive read");
 job={};for(auto& s:n::sources){s.file.Reset();if(s.guard!=INVALID_HANDLE_VALUE)CloseHandle(s.guard);s={};}n::factory.Reset();n::runtime=nullptr;
}
static unsigned char allocationTexture[0x100]{},allocationLevels[3*0x28]{};
static bool allocationSuccess=true;
static bool Allocate(void* texture,unsigned level){
 Check(texture==allocationTexture&&level==2&&GetLastError()==77,"native allocation ABI");
 if(allocationSuccess)Put(allocationLevels,2*0x28+0x10,0x9999);SetLastError(78);return allocationSuccess;
}
static void AllocationGate(){
 namespace n=native_gpu_storage;n::allocate=&Allocate;Put(allocationTexture,0xe8,reinterpret_cast<std::uint64_t>(allocationLevels));
 SetLastError(77);Check(n::AllocateHook(allocationTexture,2)&&GetLastError()==78&&n::allocation.texture==allocationTexture&&n::allocation.container==0x9999,"new unpublished allocation accepted");
 SetLastError(77);Check(n::AllocateHook(allocationTexture,2)&&!n::allocation.texture&&GetLastError()==78,"existing allocation excluded");
 allocationSuccess=false;SetLastError(77);Check(!n::AllocateHook(allocationTexture,2)&&!n::allocation.texture&&GetLastError()==78,"failed allocation excluded");
}
static void Scheduling(){
 namespace n=native_gpu_storage;
 for(unsigned i=0;i<n::QueueCapacity;++i){auto& j=n::jobs[i];j.used=true;j.priority=int(i%4);j.serial=i+1;j.gpuBytes=64;}
 n::Job out[16];Check(n::Dequeue(out,16)==16,"full burst selects batch");
 for(unsigned i=0;i<16;++i)Check(out[i].priority==3&&out[i].serial==4+4*i,"priority then FIFO ordering");
 unsigned remaining=0;for(const auto& j:n::jobs)remaining+=j.used;Check(remaining==n::QueueCapacity-16,"unselected jobs retained");
 for(auto& j:n::jobs)j={};for(auto& j:out)j={};
 n::jobs[0].used=true;n::jobs[0].gpuBytes=70ull*1024*1024;n::jobs[0].serial=1;
 n::jobs[1].used=true;n::jobs[1].gpuBytes=70ull*1024*1024;n::jobs[1].serial=2;
 Check(n::Dequeue(out,16)==1&&n::jobs[1].used,"batch respects staging budget");
 for(auto& j:n::jobs)j={};
 Check(n::Dequeue(out,16)==0,"empty queue");
}
static void Admission(){
 namespace n=native_gpu_storage;n::adjustPending=&Adjust;n::describe=&Describe;n::resolve=&Resolve;n::addRef=&Add;n::removeRef=&Remove;n::finish=&Finish;n::original=&Native;
 n::wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);n::available=true;n::cursor=0;n::traceLatency=true;
 n::assetTraceEnabled=true;n::assetCapture.Begin(n::ClockUs());n::assetDeadline=n::assetCapture.flushAt;
 auto texture=reinterpret_cast<void*>(0x1234);auto user=reinterpret_cast<void*>(2);
 SetLastError(101);Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)==123&&GetLastError()==456&&native==1,"existing resource remains native");
 n::allocation={texture,2,0x9999};SetLastError(101);const auto id=n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user);
 Check(id<0&&GetLastError()==101&&refs==1&&native==1&&n::pending==1&&enginePending==1&&n::accepted==1,"accepted request bypasses native payload read");
 auto job=std::move(n::jobs[0]);n::jobs[0]={};Check(n::active[0]==testResource.Get(),"active target stays reserved after dequeue");
 n::Retire(job,true,false);Check(refs==0&&enginePending==0&&published==1&&n::pending==0&&n::active[0]==nullptr,"publication and retirement");
 Check(n::endToEndTime.count==1&&n::handoffTime.count==1&&n::levelLatency[2].count==1,"successful native handoff latency recorded");
 n::cursor=0;n::allocation={texture,2,0x9999};n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user);
 job=std::move(n::jobs[0]);n::jobs[0]={};n::Retire(job,false,true);Check(native==2&&refs==0&&enginePending==0&&n::pending==0&&published==1,"retired failure retries native exactly once");
 // Fill the bounded descriptor queue to its final slot. Synthetic
 // reservations avoid allocating thousands
 // of GPU resources: this fixture tests admission, not the GPU memory budget.
 static_assert(n::QueueCapacity==4096,"bounded descriptor capacity");
 const auto oldCapacity=n::capacityRejected.load(),oldDuplicate=n::duplicateRejected.load();
 for(unsigned i=0;i<n::QueueCapacity-1;++i)n::active[i]=reinterpret_cast<ID3D12Resource*>(1);
 n::pending=n::QueueCapacity-1;n::cursor=n::QueueCapacity-1;
 n::allocation={texture,2,0x9999};SetLastError(101);
 Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)<0&&GetLastError()==101,"final queue slot admitted");
 Check(n::pending==n::QueueCapacity&&n::peakPending==n::QueueCapacity&&refs==1&&enginePending==1,"burst peak and lifetime accounting");
 n::allocation={texture,2,0x9999};
 Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)==123&&n::capacityRejected==oldCapacity+1,"full queue falls back without reservation");
 const unsigned last=n::QueueCapacity-1;job=std::move(n::jobs[last]);n::jobs[last]={};
 // Free one synthetic reservation while the real target stays active. This
 // must count as duplicate rejection, independently of capacity pressure.
 n::active[0]=nullptr;--n::pending;n::allocation={texture,2,0x9999};
 Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)==123&&n::duplicateRejected==oldDuplicate+1&&n::capacityRejected==oldCapacity+1,"duplicate has distinct rejection reason");
 n::Retire(job,true,false);Check(refs==0&&enginePending==0&&n::reservedBytes==0,"burst retirement balances ownership");
 for(auto& target:n::active)target=nullptr;n::pending=0;
 n::cursor=last;n::allocation={texture,2,0x9999};
 Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)<0,"retired slot reusable");
 job=std::move(n::jobs[last]);n::jobs[last]={};n::Retire(job,true,false);
 Check(refs==0&&enginePending==0&&n::pending==0,"slot reuse balances pending");
 // Terminal failure must finish once, without retry or successful latency data.
 const auto beforeNative=native,beforePublished=published;
 const auto beforeLatency=n::endToEndTime.count.load();
 n::cursor=0;n::allocation={texture,2,0x9999};
 Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)<0,"terminal fixture admitted");
 job=std::move(n::jobs[0]);n::jobs[0]={};expectFinishSuccess=false;
 n::Retire(job,false,false);expectFinishSuccess=true;
 Check(failedPublished==1&&native==beforeNative&&published==beforePublished,"terminal failure does not retry or publish success");
 Check(refs==0&&enginePending==0&&n::pending==0&&n::reservedBytes==0&&!n::active[0],"terminal failure balances ownership");
 Check(n::endToEndTime.count==beforeLatency,"terminal failure excluded from success latency");
 Check(n::assetCapture.count==8,"all request attempts captured including native and queue-full fallbacks");
 Check(n::assetCapture.rows[0].path==2&&n::assetCapture.rows[1].path==1&&n::assetCapture.rows[2].path==2,"native, GPU and retry routes distinguished");
 Check(n::assetCapture.rows[0].texture==0x1234&&n::assetCapture.rows[0].offset==17,"identity and archive offset preserved");
 n::assetTraceEnabled=false;n::assetDeadline=0;n::assetCapture.active=false;
 CloseHandle(n::wake);n::wake=nullptr;n::available=false;n::traceLatency=false;
}
static void PublicationObservation(){
 namespace n=native_gpu_storage;
 storage_publication::Tracker tracker;
 Check(tracker.Begin(16,2,99,100),"publication tracked");
 Check(tracker.Take(16,3,99,200)==0,"different mip does not match");
 Check(tracker.Take(16,2,99,200)==100&&tracker.Take(16,2,99,201)==0,"publication consumes once");
 Check(tracker.Begin(16,2,99,100)&&tracker.Take(16,2,100,200)==0,"replaced container rejected");
 Check(tracker.Begin(16,2,99,100)&&tracker.Begin(16,2,100,200)&&tracker.Take(16,2,100,300)==200,"new generation replaces pending identity");
 Check(tracker.Begin(16,2,99,100)&&tracker.Take(16,2,99,30000101)==0,"stale identity expires");
 Check(tracker.Begin(16,2,99,100)&&tracker.Take(16,2,99,99)==0,"clock regression rejected");
 for(unsigned i=0;i<8;++i)Check(tracker.Begin(16+65536ull*i,2,99,100),"bounded collision slot");
 Check(!tracker.Begin(16+65536ull*8,2,99,100),"full probe range drops observation");
 unsigned char texture[256]{},levels[3*0x28]{},record[16]{};
 Put(texture,0xe8,reinterpret_cast<std::uint64_t>(levels));Put(texture,0xf0,3);Put(levels,2*0x28+0x10,99);
 Put(record,0,reinterpret_cast<std::uint64_t>(texture));unsigned level=2;memcpy(record+8,&level,4);record[12]=1;
 n::consume=[](void* r){Check(GetLastError()==123,"publication observer preserves incoming LastError");Check(static_cast<unsigned char*>(r)[12]==1,"native completion forwarded unchanged");SetLastError(456);};
 n::publicationInstalled=true;n::Job job;job.texture=texture;job.level=2;job.container=99;
 const auto before=n::publicationDelay.count.load();n::PublicationBegin(job,n::ClockUs());
 SetLastError(123);n::PublicationHook(record);Check(GetLastError()==456&&n::publicationDelay.count==before+1,"matched publication measured and native LastError preserved");
 SetLastError(123);n::PublicationHook(record);Check(GetLastError()==456&&n::publicationDelay.count==before+1,"unmatched native completion still forwarded without sample");
 n::publicationInstalled=false;n::consume=nullptr;
 puts("PASS R250 publication: mip/container matching, replacement, expiry, collisions, one-shot consumption, native forwarding and LastError");
}
static void AssetTraceObservation(){
 static storage_asset_trace::Capture c;
 Check(c.Begin(100)&&!c.Begin(200),"capture cannot restart while active");
 Check(!c.Add("early",99,100,0,0,1),"pre-window request excluded");
 const auto token=c.Add("sign\nname",100,110,2,7,4096);
 Check(token&&c.count==1&&strcmp(c.rows[0].name,"sign_name")==0,"metadata captured and control characters sanitized");
 c.Submit(token,120);c.Retire(token,130,1);c.Publish(token,140);
 Check(c.rows[0].submitted==120&&c.rows[0].retired==130&&c.rows[0].published==140&&c.rows[0].result==1,"events correlated");
 Check(!c.Add("late",30000100,30000101,0,0,1),"window end excluded");
 char longName[600];memset(longName,'x',sizeof(longName));longName[599]=0;
 c.Add(longName,101,102,0,0,1);Check(c.rows[1].truncated&&strlen(c.rows[1].name)==511,"bounded name copy");
 for(unsigned i=c.count;i<c.Capacity;++i)Check(c.Add("fill",102,103,0,0,1)!=0,"capture slot");
 Check(!c.Add("overflow",102,103,0,0,1)&&c.overflow==1,"capture capacity bounded");
 Check(!c.Finish(40000099)&&c.Finish(40000100),"completion grace enforced");
 Check(!c.Find(token)&&c.Begin(50000000),"completed capture can rearm");
 const auto next=c.Add("new",50000000,50000001,0,0,1);c.Publish(token,50000002);
 Check(next!=token&&!c.rows[0].published,"old generation cannot mutate new capture");
 storage_publication::Tracker tracker;std::uint64_t out=999;
 Check(tracker.Begin(16,2,99,100,next)&&tracker.Take(16,2,99,200,&out)==100&&out==next,"publication carries asset token");
 Check(!tracker.Take(16,2,99,201,&out)&&out==0,"unmatched publication clears token");
 puts("PASS R252 asset trace: bounded capture, time window, grace, sanitization, event correlation and stale-token rejection");
}
static void Put32(void* p,unsigned offset,unsigned value){memcpy(static_cast<unsigned char*>(p)+offset,&value,4);}
static void SelectionObservation(){
 namespace n=native_gpu_storage;
 static unsigned char rendererFixture[0x9186a4]{};unsigned char texture[0x160]{},levels[3*0x28]{};
 auto* oldRenderer=n::renderer;n::renderer=rendererFixture;
 Put(texture,0xe8,reinterpret_cast<std::uint64_t>(levels));Put32(texture,0xf0,3);texture[0x111]=1;
 Put32(texture,0xb4,2);Put32(texture,0xb8,2);Put32(texture,0xc4,1);Put32(levels,0x28+0x20,0);
 n::assetTraceEnabled=true;n::assetCapture.Begin(n::ClockUs());n::assetDeadline=n::assetCapture.flushAt;n::selectionTrace.Reset();
 n::RegisterSelection(texture,"data_pc\\textures\\props\\signs\\wayfinding_text_luck_probability_a.tex");
 Check(n::selectionTrace.Contains(reinterpret_cast<std::uint64_t>(texture)),"wayfinding target registered before scan");
 n::requestLevel=[](void* t,int level){Check(GetLastError()==123&&level==1,"budget call ABI and incoming error");Put32(t,0xc0,9);SetLastError(456);return false;};
 SetLastError(123);Check(!n::RequestLevelHook(texture,1)&&GetLastError()==456,"budget result and outgoing error preserved");
 Check(n::selectionTrace.count==1&&n::selectionTrace.rows[0].kind==1&&n::selectionTrace.rows[0].before.delta==0&&n::selectionTrace.rows[0].after.delta==9,"budget before/after state captured");
 n::selectTexture=[](void* t,bool a,bool b,bool c){Check(GetLastError()==123&&a&&!b&&c,"selector arguments forwarded");Put32(t,0xc4,2);SetLastError(789);};
 SetLastError(123);n::SelectTextureHook(texture,true,false,true);Check(GetLastError()==789&&n::selectionTrace.count==2,"selector forwarded exactly once");
 Check(n::selectionTrace.rows[1].before.wanted==1&&n::selectionTrace.rows[1].after.wanted==2,"target change captured");
 n::RegisterSelection(texture,"other.tex");Check(!n::selectionTrace.Contains(reinterpret_cast<std::uint64_t>(texture)),"reused identity invalidated");
 n::assetTraceEnabled=false;n::assetDeadline=0;n::assetCapture.active=false;n::renderer=oldRenderer;n::selectTexture=nullptr;n::requestLevel=nullptr;
 puts("PASS R253 selection: target registration, identity invalidation, before/after snapshots, selector/budget forwarding, return and LastError");
}
int main(){
 setvbuf(stdout,nullptr,_IONBF,0);
 SelectionObservation();
 AssetTraceObservation();
 PublicationObservation();
 ComPtr<IDXGIFactory4> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
 Hr(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"DXGI");Hr(dxgi->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP");Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)),"device");
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;Hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
 // Prewarm mailbox: invalid queue rejection, concurrent single admission,
 // retained ownership and one-shot consumption without issuing GPU work.
 namespace n=native_gpu_storage;
 Check(!n::PostWarmQueue(nullptr),"null warm queue rejects");
 ComPtr<ID3D12CommandQueue> copyQueue;auto copyDesc=q;copyDesc.Type=D3D12_COMMAND_LIST_TYPE_COPY;
 Hr(device->CreateCommandQueue(&copyDesc,IID_PPV_ARGS(&copyQueue)),"copy queue");
 Check(!n::PostWarmQueue(copyQueue.Get())&&!n::warmPosted,"copy warm queue rejects without consuming slot");
 n::wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(n::wake!=nullptr,"warm wake event");
 std::atomic<unsigned> posted{0};std::thread posters[8];
 for(auto& thread:posters)thread=std::thread([&](){if(n::PostWarmQueue(queue.Get()))++posted;});
 for(auto& thread:posters)thread.join();
 Check(posted==1&&n::warmPosted&&WaitForSingleObject(n::wake,0)==WAIT_OBJECT_0,"one warm post wakes worker");
 auto warm=n::TakeWarmQueue();Check(warm.Get()==queue.Get()&&!n::TakeWarmQueue(),"warm queue retained and consumed once");
 Check(!n::PostWarmQueue(queue.Get()),"no repeated initialization after consume");
 warm.Reset();CloseHandle(n::wake);n::wake=nullptr;n::warmPosted=false;
 ComPtr<IDStorageFactory> factory;Hr(DStorageGetFactory(IID_PPV_ARGS(&factory)),"factory");Batch transfer;Hr(transfer.Initialize(factory.Get(),queue.Get()),"packed backend");transfer.SetLatencyTracking(true);
 struct Fixture {ComPtr<ID3D12Resource> texture;D3D12_RESOURCE_DESC desc{};native_texture_layout::Layout layout{};std::vector<unsigned char> compact;};
 Fixture fixtures[9];const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_BC1_TYPELESS,DXGI_FORMAT_BC3_TYPELESS,DXGI_FORMAT_BC4_UNORM,DXGI_FORMAT_BC5_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_B8G8R8A8_TYPELESS,DXGI_FORMAT_BC7_UNORM,DXGI_FORMAT_R8G8B8A8_UNORM};
 std::vector<unsigned char> archive(713,0x5a);Batch::Input inputs[9]{};
 for(unsigned t=0;t<9;++t){auto& f=fixtures[t];auto& d=f.desc;d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=t==8?1024:(t==0?129:128);d.Height=t==8?1024:(t==0?65:64);d.DepthOrArraySize=1;d.MipLevels=8;d.Format=formats[t];d.SampleDesc.Count=1;
  f.texture=Resource(device.Get(),d,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATES(0xc0));Check(native_texture_layout::Build(device.Get(),d,f.layout),"layout");
  f.compact.resize(static_cast<size_t>(f.layout.compactBytes));
  for(unsigned m=0;m<8;++m){const auto& l=f.layout.mips[m];for(unsigned r=0;r<l.rows;++r)for(UINT64 c=0;c<l.rowBytes;++c)f.compact[size_t(l.compactOffset+r*l.rowBytes+c)]=static_cast<unsigned char>((t*31+m*17+r*7+c*3)%251);}
  inputs[t]={nullptr,f.texture.Get(),archive.size(),static_cast<UINT>(f.compact.size()),D3D12_RESOURCE_STATES(0xc0)};archive.insert(archive.end(),f.compact.begin(),f.compact.end());archive.insert(archive.end(),13,0x77);
 }
 wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};Check(GetTempPathW(MAX_PATH,temp)&&GetTempFileNameW(temp,L"cfp",0,path),"temp path");
 HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);Check(file!=INVALID_HANDLE_VALUE,"fixture create");DWORD written=0;
 Check(WriteFile(file,archive.data(),static_cast<DWORD>(archive.size()),&written,nullptr)&&written==archive.size(),"fixture write");CloseHandle(file);
 ComPtr<IDStorageFile> source;Hr(factory->OpenFile(path,IID_PPV_ARGS(&source)),"source");for(auto& in:inputs)in.file=source.Get();
 auto bad=inputs[0];++bad.bytes;Check(FAILED(transfer.Submit(&bad,1)),"wrong compact size rejects");bad=inputs[0];bad.offset=archive.size();Check(FAILED(transfer.Submit(&bad,1)),"EOF rejects");
 Batch::Input duplicate[]={inputs[0],inputs[0]};Check(FAILED(transfer.Submit(duplicate,2)),"duplicate target rejects");
 // Model loading code that cannot submit the next frame until textures finish.
 // The consumer queue is deliberately blocked until the transfer has retired.
 ComPtr<ID3D12Fence> frameGate;Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&frameGate)),"frame gate");
 Hr(queue->Wait(frameGate.Get(),1),"block native frame queue");
 for(unsigned pass=0;pass<2;++pass){Hr(transfer.Submit(inputs,8),"packed batch submit");Check(!transfer.Close(),"pending resources cannot retire");HRESULT result=E_PENDING;
  const auto deadline=GetTickCount64()+30000;while(!transfer.Poll(result)){Check(GetTickCount64()<deadline,"GPU completion deadline");WaitForSingleObject(transfer.Event(),100);}
  Hr(result,"packed completion");Check(transfer.LastBatchTiming().storageDone&&transfer.LastBatchTiming().gpuDone,"observed completion stages recorded");if(pass==0)Hr(frameGate->Signal(1),"release native frame after GPU transfer");Check(transfer.CurrentPhase()==Batch::Phase::Idle,"batch reuse");}
 // Independent lanes overlap storage/GPU work while retaining disjoint targets.
 Batch parallel;Hr(parallel.Initialize(factory.Get(),queue.Get(),&transfer),"parallel lane");
 Check(parallel.InitializationTiming().sharedPipeline&&!transfer.InitializationTiming().sharedPipeline,"second lane shares same-device compiled pipeline");
 Hr(transfer.Submit(inputs,4),"lane zero");Hr(parallel.Submit(inputs+4,4),"lane one");
 bool done[2]{};Batch* lanes[]={&transfer,&parallel};const auto deadline=GetTickCount64()+30000;
 while(!done[0]||!done[1]){for(unsigned i=0;i<2;++i)if(!done[i]){HRESULT result=E_PENDING;done[i]=lanes[i]->Poll(result);if(done[i])Hr(result,"overlapped completion");}
  Check(GetTickCount64()<deadline,"parallel deadline");if(!done[0])WaitForSingleObject(transfer.Event(),10);else if(!done[1])WaitForSingleObject(parallel.Event(),10);}
 Check(!parallel.LastBatchTiming().storageDone&&!parallel.LastBatchTiming().gpuDone,"disabled latency leaves transfer behavior intact");
 Check(parallel.Close()&&parallel.RetainedBytes()==0,"parallel retired and pool released");
 Check(transfer.BufferAllocations()==2&&transfer.ReusedBatches()==2,"small repeated batches reuse two arenas");
 for(unsigned pass=0;pass<3;++pass){
  Hr(transfer.Submit(pass==2?inputs:inputs+8,pass==2?8:1),"growth and smaller reuse submit");
  HRESULT result=E_PENDING;const auto until=GetTickCount64()+30000;
  while(!transfer.Poll(result)){Check(GetTickCount64()<until,"growth completion deadline");WaitForSingleObject(transfer.Event(),100);}
  Hr(result,"growth completion");Check(transfer.RetainedBytes()<=Batch::StagingBudget,"retained pool bounded");
 }
 Check(transfer.BufferAllocations()==4&&transfer.ReusedBatches()==4,"growth once then reuse for large and small batches");
 const auto allocations=transfer.BufferAllocations();bad=inputs[0];++bad.bytes;
 Check(FAILED(transfer.Submit(&bad,1))&&transfer.BufferAllocations()==allocations,"invalid batch does not resize arenas");
 // Exercise fresh resource turnover while the transport holds the only
 // application-owned COM references during its Storage phase. Recover the
 // caller references before retirement for the existing byte-equality checks.
 for(unsigned pass=0;pass<32;++pass){
  const unsigned t=pass%9;auto& f=fixtures[t];
  f.texture=Resource(device.Get(),f.desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATES(0xc0));
  inputs[t].texture=f.texture.Get();
  Batch::Input invalid[]={inputs[t],inputs[(t+1)%9]};++invalid[1].bytes;
  Check(FAILED(transfer.Submit(invalid,2))&&transfer.CurrentPhase()==Batch::Phase::Idle,"partial preparation rejection returns idle");
  Hr(transfer.Submit(inputs+t,1),"lifetime turnover submit");
  Check(transfer.CurrentPhase()==Batch::Phase::Storage,"storage owns submitted references");
  Check(FAILED(transfer.Submit(inputs+t,1))&&!transfer.Close(),"busy submit and close preserve live transfer");
  auto* rawTexture=f.texture.Get();auto* rawFile=source.Get();f.texture.Reset();source.Reset();
  Check(rawTexture->GetDesc().Width==f.desc.Width,"batch retains destination after caller releases");
  BY_HANDLE_FILE_INFORMATION info{};Hr(rawFile->GetFileInformation(&info),"batch retains source after caller releases");
  f.texture=rawTexture;source=rawFile;
  HRESULT result=E_PENDING;const auto until=GetTickCount64()+30000;
  while(!transfer.Poll(result)){Check(GetTickCount64()<until,"turnover completion deadline");WaitForSingleObject(transfer.Event(),100);}
  Hr(result,"turnover completion");
  Check(transfer.CurrentPhase()==Batch::Phase::Idle&&transfer.BufferAllocations()==allocations,"turnover reuses arenas without allocation growth");
 }
 puts("PASS R248 lifetime: 32 fresh-resource turnover cycles, caller reference release, busy-operation rejection, partial preparation recovery, no arena growth");
 // Readback belongs exclusively to this test, never to runtime payload handling.
 ComPtr<ID3D12Fence> fence;Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"readback fence");HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 for(unsigned t=0;t<9;++t){auto& f=fixtures[t];D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[8]{};UINT rows[8]{};UINT64 rowBytes[8]{},total=0;device->GetCopyableFootprints(&f.desc,0,8,0,fp,rows,rowBytes,&total);
  D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=total;d.Height=1;d.DepthOrArraySize=1;d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  auto readback=Resource(device.Get(),d,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList> list;
  Hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");Hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={f.texture.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATES(0xc0),D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&b);
  for(unsigned m=0;m<8;++m){D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=fp[m];src.pResource=f.texture.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=m;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
  Hr(list->Close(),"close");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);Hr(queue->Signal(fence.Get(),t+1),"signal");Hr(fence->SetEventOnCompletion(t+1,event),"event");Check(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"readback completion");
  void* mapped=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(total)};Hr(readback->Map(0,&range,&mapped),"test readback");
  for(unsigned m=0;m<8;++m)for(unsigned r=0;r<rows[m];++r)Check(memcmp(static_cast<unsigned char*>(mapped)+fp[m].Offset+size_t(r)*fp[m].Footprint.RowPitch,f.compact.data()+f.layout.mips[m].compactOffset+r*rowBytes[m],static_cast<size_t>(rowBytes[m]))==0,"packed GPU mip byte equality");
  D3D12_RANGE empty{};readback->Unmap(0,&empty);
 }
 storage_latency::Timing metric;metric.Record(100);metric.Record(16000);metric.Record(100000);Check(metric.count==3&&metric.total==116100&&metric.maximum==100000&&metric.over16ms==2&&metric.over100ms==1,"latency totals and threshold boundaries");
 Scheduling();AllocationGate();ResolveRange(factory.Get(),path);testResource=fixtures[0].texture;Admission();puts("PASS R248 terminal retirement: failure completion once, no native retry, balanced references/reservations, no success latency");testResource.Reset();Check(transfer.Close(),"drained close");source.Reset();CloseHandle(event);Check(DeleteFileW(path)!=FALSE,"file ownership released");
 puts("PASS R247: prewarm single-admission race, queue-type guard, retained handoff, wake and one-shot consumption; file-to-GPU packed rows, eight formats / 72 mips, repeated and growing arena reuse, bounded staging, native state restoration, blocked consumer queue independence, bounds, native archive bias resolution, overlap rejection, optional stage timings, threshold accounting, embedded shader, shared pipeline, priority/FIFO burst scheduling, bounded queue admission, capacity/duplicate separation, slot reuse, ownership and deferred publication; runtime CPU payload bytes=0");
}
