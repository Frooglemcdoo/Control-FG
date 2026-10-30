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
static void Log(const char*,...){}
static bool HashMatches(HMODULE,const char*){return false;}
static void* AllocateExecutableRelayNear(const void*){return nullptr;}
static bool Rel32Fits(const void*,const void*){return false;}
static HMODULE selfModule=nullptr;
static std::wstring ModulePath(HMODULE){return L"";}
#include "../../src/native_cpu_call_patch.h"
#include "../../src/native_loader_trace_runtime.h"
#include "../../src/native_texture_contract_runtime.h"
#include "../../src/native_gpu_storage_runtime.h"
using Microsoft::WRL::ComPtr;
using Batch=control_storage::PackedTextureBatch;
static void Check(bool value,const char* name){if(!value){printf("FAIL %s error=%lu\n",name,GetLastError());ExitProcess(1);}}
static void Hr(HRESULT hr,const char* name){if(FAILED(hr)){printf("FAIL %s hr=%08lx\n",name,static_cast<unsigned long>(hr));ExitProcess(2);}}
static ComPtr<ID3D12Resource> Resource(ID3D12Device* d,const D3D12_RESOURCE_DESC& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
 D3D12_HEAP_PROPERTIES h{};h.Type=type;ComPtr<ID3D12Resource> r;Hr(d->CreateCommittedResource(&h,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&r)),"resource");return r;
}
static unsigned refs=0,published=0,native=0;
static int enginePending=0;
static void Adjust(void*,int delta){enginePending+=delta;Check(enginePending>=0,"native pending count never underflows");}
static ComPtr<ID3D12Resource> testResource;
static void Add(void*){++refs;}
static void Remove(void*){Check(refs!=0,"balanced engine reference");--refs;}
static void Finish(void*,int level,bool success){Check(level==2&&success&&refs==1&&enginePending==1,"publish while reference held");++published;}
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
static void Admission(){
 namespace n=native_gpu_storage;n::adjustPending=&Adjust;n::describe=&Describe;n::resolve=&Resolve;n::addRef=&Add;n::removeRef=&Remove;n::finish=&Finish;n::original=&Native;
 n::wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);n::available=true;n::cursor=0;
 auto texture=reinterpret_cast<void*>(0x1234);auto user=reinterpret_cast<void*>(2);
 SetLastError(101);Check(n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user)==123&&GetLastError()==456&&native==1,"existing resource remains native");
 n::allocation={texture,2,0x9999};SetLastError(101);const auto id=n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user);
 Check(id<0&&GetLastError()==101&&refs==1&&native==1&&n::pending==1&&enginePending==1&&n::accepted==1,"accepted request bypasses native payload read");
 auto job=std::move(n::jobs[0]);n::jobs[0]={};Check(n::active[0]==testResource.Get(),"active target stays reserved after dequeue");
 n::Retire(job,true,false);Check(refs==0&&enginePending==0&&published==1&&n::pending==0&&n::active[0]==nullptr,"publication and retirement");
 n::cursor=0;n::allocation={texture,2,0x9999};n::RequestHook(nullptr,"fixture",texture,17,64,nullptr,14,7,false,user);
 job=std::move(n::jobs[0]);n::jobs[0]={};n::Retire(job,false,true);Check(native==2&&refs==0&&enginePending==0&&n::pending==0&&published==1,"retired failure retries native exactly once");
 CloseHandle(n::wake);n::wake=nullptr;n::available=false;
}
int main(){
 ComPtr<IDXGIFactory4> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<ID3D12Device> device;
 Hr(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"DXGI");Hr(dxgi->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP");Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)),"device");
 ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;Hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
 ComPtr<IDStorageFactory> factory;Hr(DStorageGetFactory(IID_PPV_ARGS(&factory)),"factory");Batch transfer;Hr(transfer.Initialize(factory.Get(),queue.Get()),"packed backend");
 struct Fixture {ComPtr<ID3D12Resource> texture;D3D12_RESOURCE_DESC desc{};native_texture_layout::Layout layout{};std::vector<unsigned char> compact;};
 Fixture fixtures[8];const DXGI_FORMAT formats[]={DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_BC1_TYPELESS,DXGI_FORMAT_BC3_TYPELESS,DXGI_FORMAT_BC4_UNORM,DXGI_FORMAT_BC5_UNORM,DXGI_FORMAT_B8G8R8A8_UNORM,DXGI_FORMAT_B8G8R8A8_TYPELESS,DXGI_FORMAT_BC7_UNORM};
 std::vector<unsigned char> archive(713,0x5a);Batch::Input inputs[8]{};
 for(unsigned t=0;t<8;++t){auto& f=fixtures[t];auto& d=f.desc;d.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;d.Width=t==0?129:128;d.Height=t==0?65:64;d.DepthOrArraySize=1;d.MipLevels=8;d.Format=formats[t];d.SampleDesc.Count=1;
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
 for(unsigned pass=0;pass<2;++pass){Hr(transfer.Submit(inputs,8),"packed batch submit");Check(!transfer.Close(),"pending resources cannot retire");HRESULT result=E_PENDING;
  const auto deadline=GetTickCount64()+30000;while(!transfer.Poll(result)){Check(GetTickCount64()<deadline,"GPU completion deadline");WaitForSingleObject(transfer.Event(),100);}
  Hr(result,"packed completion");Check(transfer.CurrentPhase()==Batch::Phase::Idle,"batch reuse");}
 // Independent lanes overlap storage/GPU work while retaining disjoint targets.
 Batch parallel;Hr(parallel.Initialize(factory.Get(),queue.Get()),"parallel lane");
 Hr(transfer.Submit(inputs,4),"lane zero");Hr(parallel.Submit(inputs+4,4),"lane one");
 bool done[2]{};Batch* lanes[]={&transfer,&parallel};const auto deadline=GetTickCount64()+30000;
 while(!done[0]||!done[1]){for(unsigned i=0;i<2;++i)if(!done[i]){HRESULT result=E_PENDING;done[i]=lanes[i]->Poll(result);if(done[i])Hr(result,"overlapped completion");}
  Check(GetTickCount64()<deadline,"parallel deadline");if(!done[0])WaitForSingleObject(transfer.Event(),10);else if(!done[1])WaitForSingleObject(parallel.Event(),10);}
 Check(parallel.Close(),"parallel retired");
 // Readback belongs exclusively to this test, never to runtime payload handling.
 ComPtr<ID3D12Fence> fence;Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"readback fence");HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 for(unsigned t=0;t<8;++t){auto& f=fixtures[t];D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[8]{};UINT rows[8]{};UINT64 rowBytes[8]{},total=0;device->GetCopyableFootprints(&f.desc,0,8,0,fp,rows,rowBytes,&total);
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
 AllocationGate();ResolveRange(factory.Get(),path);testResource=fixtures[0].texture;Admission();testResource.Reset();Check(transfer.Close(),"drained close");source.Reset();CloseHandle(event);Check(DeleteFileW(path)!=FALSE,"file ownership released");
 puts("PASS R241: file-to-GPU packed rows, eight formats / 64 mips, repeated batches, native state restoration, bounds, native archive bias resolution, overlap rejection, admission ownership and deferred publication; runtime CPU payload bytes=0");
}
