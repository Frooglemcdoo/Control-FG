// A real D3D destruction callback seals recording lifetime only. It does not
// supply GPU completion and is not triggered by replacing private metadata.
static void TestGIListDestruction(Device& d){
 using namespace gi_reuse;
 using Pool=D3D12SplitOwnerRetirement<Invocation,2,1024*1024>;
 Pool owners;Check(owners.Initialize(d.queue.Get()));int deaths=0;
 static Pool* active=nullptr;active=&owners;
 static std::atomic<unsigned> notifications{0};notifications=0;
 fixture::gi_compute_hooks::onFrozenRecordingDestroyed=+[](ID3D12CommandList* list,UINT64 generation)noexcept{
  ++notifications;active->Seal({reinterpret_cast<UINT64>(list),generation});
 };
 auto make=[&](ComPtr<ID3D12CommandAllocator>& allocator,ComPtr<ID3D12GraphicsCommandList4>& list){
  Check(d.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
  Check(d.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));
  fixture::gi_compute_hooks::InstallList(list.Get());fixture::gi_compute_hooks::Begin(list.Get(),nullptr);
  const auto generation=fixture::gi_compute_hooks::RecordingGeneration(list.Get());
  Require(fixture::gi_compute_hooks::ObserveDestruction(list.Get(),generation),"D3D list destruction notification unavailable");
  Require(fixture::gi_compute_hooks::ObserveDestruction(list.Get(),generation),"existing destruction notification lost");
  return Pool::Recording{reinterpret_cast<UINT64>(list.Get()),generation};
 };
 const auto contexts=fixture::gi_compute_hooks::destructionContexts.load();
 ComPtr<ID3D12CommandAllocator> a,b;ComPtr<ID3D12GraphicsCommandList4> first,second;
 const auto producer=make(a,first);const auto consumer=make(b,second);
 Require(fixture::gi_compute_hooks::destructionContexts==contexts+2,"duplicate destruction context allocated");
 auto owner=std::make_unique<Invocation>(deaths);std::unique_ptr<Invocation> retired;
 const auto id=owners.Admit(11,producer,64,owner);Require(id&&owners.Attach(id,11,consumer),"destruction owner admission failed");
 const auto generation=&fixture::gi_compute_hooks::RecordingGeneration;
 Check(first->Close());ID3D12CommandList* lists[]={first.Get()};auto batch=owners.Before(d.queue.Get(),1,lists,generation);
 d.queue->ExecuteCommandLists(1,lists);Require(owners.After(d.queue.Get(),batch),"destruction producer submit failed");
 Require(owners.ClaimConsumerRecording(id,11,d.queue.Get(),consumer),"destruction consumer not admitted");
 Check(second->Close());lists[0]=second.Get();batch=owners.Before(d.queue.Get(),1,lists,generation);
 d.queue->ExecuteCommandLists(1,lists);Require(owners.After(d.queue.Get(),batch),"destruction consumer submit failed");
 // Removing private metadata cannot seal a still-replayable command list.
 Check(first->SetPrivateDataInterface(fixture::gi_compute_hooks::stateKey,nullptr));
 Require(notifications==0&&!owners.Take(id,retired),"private state removal faked list destruction");
 first.Reset();second.Reset();
 Require(!owners.Take(id,retired)&&deaths==0,"list destruction faked GPU completion");
 ComPtr<ID3D12Fence> fence;Check(d.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
 Check(d.queue->Signal(fence.Get(),1));HANDLE done=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 Require(done!=nullptr,"destruction event failed");Check(fence->SetEventOnCompletion(1,done));
 Require(WaitForSingleObject(done,30000)==WAIT_OBJECT_0,"destruction GPU timeout");CloseHandle(done);
 Require(notifications==2&&fixture::gi_compute_hooks::destructionContexts==contexts,"list destruction notification/context cleanup missing");
 owners.Poll();Require(owners.Take(id,retired),"destroyed and completed recordings did not retire");retired.reset();
 Require(deaths==1,"destroyed owner not released exactly once");
 // A successful Reset changes the identity reported at eventual destruction.
 ComPtr<ID3D12CommandAllocator> c;ComPtr<ID3D12GraphicsCommandList4> third;
 const auto old=make(c,third);Check(third->Close());Check(third->Reset(c.Get(),nullptr));
 const auto fresh=fixture::gi_compute_hooks::RecordingGeneration(third.Get());Require(fresh!=old.generation,"Reset kept destruction identity");
 owner=std::make_unique<Invocation>(deaths);
 const auto cancelled=owners.Admit(12,{reinterpret_cast<UINT64>(third.Get()),fresh},64,owner);
 Require(cancelled&&owners.CancelUnattached(cancelled),"cancelled destruction admission failed");
 Check(third->Close());third.Reset();
 Require(notifications==3&&owners.Take(cancelled,retired),"destruction used pre-Reset identity");retired.reset();
 fixture::gi_compute_hooks::onFrozenRecordingDestroyed=nullptr;active=nullptr;
 std::cout<<"PASS actual D3D list destruction: metadata replacement not destruction, latest Reset identity, independent GPU fence, no retention cycle\n";
}
