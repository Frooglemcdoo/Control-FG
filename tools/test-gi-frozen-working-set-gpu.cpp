#include "gpu_test_device.h"
#include "../src/gi_reuse/d3d12_frozen_working_set.h"
#include "../src/gi_reuse/d3d12_split_owner_retirement.h"
#include "../src/gi_reuse/d3d12_frozen_completion.h"
#include "../src/gi_reuse/d3d12_frozen_pipeline_bundle.h"
struct Invocation {gi_reuse::D3D12FrozenWorkingSet scratch;int* deaths;explicit Invocation(int& n):deaths(&n){}~Invocation(){++*deaths;}};
int main()try {
 using namespace gi_reuse;Device d(false);
 auto layout=D3D12FrozenWorkingSet::Describe(18,3,16);Require(bool(layout)&&layout.count==27,"bad compact count");
 Require(!D3D12FrozenWorkingSet::Describe(17,3,16),"odd width admitted");
 Require(!D3D12FrozenWorkingSet::Describe(16384,16384,16),"oversized training admitted");
 auto fourK=D3D12FrozenWorkingSet::Describe(3840,2160,65536);
 Require(fourK.bytes[0]==149299200&&fourK.bytes[1]==16588800,"4K working allocation changed");
 int deaths=0;auto invocation=std::make_unique<Invocation>(deaths);auto& scratch=invocation->scratch;
 D3D12SplitOwnerRetirement<Invocation,2,8*1024*1024> owners;Check(owners.Initialize(d.queue.Get()));
 const auto generation=+[](ID3D12CommandList*)noexcept->UINT64{return 1;};
 Require(FAILED(scratch.Initialize(d.device.Get(),layout,1))&&!scratch.Get(D3D12FrozenWorkingSet::Training),"budget failure published partial set");
 Check(scratch.Initialize(d.device.Get(),layout,4*1024*1024));
 Require(scratch.Matches(18,3,16)&&!scratch.Matches(18,4,16),"wrong extent reuse admitted");
 Require(scratch.AllocationBytes()>=4*65536,"committed allocation not accounted");
 Require(FAILED(scratch.Initialize(d.device.Get(),layout,4*1024*1024)),"live allocation overwritten");
 // Distinct command lists/submissions: producer writes sentinel data; consumer
 // reads it only after producer completes. CPU ownership waits for BOTH resets.
 const UINT count=layout.count;std::vector<float> values(count);for(UINT i=0;i<count;++i)values[i]=float(i)+.25f;
 auto upload=d.Upload(values),read=d.Buffer(count*sizeof(float),D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
 auto* weights=scratch.Get(D3D12FrozenWorkingSet::Weights);
 using Recording=decltype(owners)::Recording;
 const Recording producer{reinterpret_cast<UINT64>(d.list.Get()),1};
 const auto id=owners.Admit(1,producer,scratch.AllocationBytes(),invocation);Require(id&&!invocation,"owner admission failed");
 std::unique_ptr<Invocation> retired;
 d.Transition(weights,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
 d.list->CopyBufferRegion(weights,0,upload.Get(),0,count*sizeof(float));
 ID3D12CommandList* producerLists[]={d.list.Get()};
 auto first=owners.Before(d.queue.Get(),1,producerLists,generation);Require(first.count==1,"producer submit refused");d.Submit();
 // Device::Submit waits for its actual queue fence. Values here are this
 // fixture's monotonically assigned private completion cuts, not frame IDs.
 Check(d.list->Reset(d.allocator.Get(),nullptr));
 Require(owners.After(d.queue.Get(),first),"producer fence refused");owners.Poll();
 Require(owners.Seal(producer)&&!owners.Take(id,retired)&&deaths==0,"producer completion retired consumer scratch");
 ComPtr<ID3D12CommandAllocator> allocator;ComPtr<ID3D12GraphicsCommandList4> consumer;
 Check(d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
 Check(d.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&consumer)));
 const Recording shading{reinterpret_cast<UINT64>(consumer.Get()),1};
 Require(owners.Attach(id,1,shading),"consumer attach failed");
 D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 barrier.Transition={weights,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE};
 consumer->ResourceBarrier(1,&barrier);consumer->CopyBufferRegion(read.Get(),0,weights,0,count*sizeof(float));Check(consumer->Close());
 ID3D12CommandList* lists[]={consumer.Get()};auto second=owners.Before(d.queue.Get(),1,lists,generation);Require(second.count==1,"consumer submit refused");d.queue->ExecuteCommandLists(1,lists);
 auto fence=d.fence;
 Require(owners.After(d.queue.Get(),second),"consumer fence failed");Check(d.queue->Signal(fence.Get(),2));
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Require(event!=nullptr,"event failed");
 Check(fence->SetEventOnCompletion(2,event));Require(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"consumer timed out");CloseHandle(event);
 owners.Poll();Require(!owners.Take(id,retired)&&deaths==0,"closed list retired before reset");
 Check(consumer->Reset(allocator.Get(),nullptr));Require(owners.Seal(shading)&&owners.Take(id,retired)&&owners.RetainedBytes()==0,"completed/reset consumer not retired");
 retired.reset();Require(deaths==1,"invocation resources not released exactly once");
 float* mapped=nullptr;Check(read->Map(0,nullptr,reinterpret_cast<void**>(&mapped)));
 for(UINT i=0;i<count;++i)Require(mapped[i]==values[i],"split scratch data changed");D3D12_RANGE empty{};read->Unmap(0,&empty);
 std::cout<<"PASS split working set: 4K allocation sizing, budget rejection, two-list data preservation, last-consumer lifetime\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
