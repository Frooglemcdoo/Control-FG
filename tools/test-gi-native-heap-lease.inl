static void TestNativeHeapLease(Device& d,ID3D12RootSignature* root,ID3D12PipelineState* pipeline){
 using namespace gi_reuse;
 ComPtr<ID3D12DescriptorHeap> sampler;D3D12_DESCRIPTOR_HEAP_DESC hd{};
 hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;hd.NumDescriptors=4;hd.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
 Check(d.device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&sampler)));
 ComPtr<ID3D12Fence> fence;Check(d.device->CreateFence(5,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));
 ComputeBindingSnapshot bound;bound.root=root;bound.pipeline=pipeline;bound.arguments.Root(reinterpret_cast<UINT64>(root));
 bound.heaps={d.views,sampler};
 alignas(8) unsigned char nativeFence[16]{},records[2][0x40]{};
 auto put=[](unsigned char* p,UINT64 value){std::memcpy(p,&value,8);};
 put(nativeFence,6);put(nativeFence+8,reinterpret_cast<UINT64>(fence.Get()));
 NativeGIRecording source;source.device=d.device.Get();source.queue=d.queue.Get();source.list=d.list.Get();
 source.heaps.frame=100;source.heaps.deviceEpoch=6;source.heaps.completed=UINT64_MAX;
 source.heaps.deviceFenceRecord=reinterpret_cast<UINT64>(nativeFence);source.heaps.deviceFenceObject=reinterpret_cast<UINT64>(fence.Get());
 for(unsigned i=0;i<2;++i){auto& h=source.heaps.heaps[i];
  h.record=reinterpret_cast<UINT64>(records[i]);h.object=reinterpret_cast<UINT64>(bound.heaps[i].Get());h.epoch=6;h.generation=9;h.cursor=2;
  put(records[i],h.object);put(records[i]+0x18,2);put(records[i]+0x20,6);put(records[i]+0x30,9);
 }
 D3D12NativeGIHeapLease lease;
 auto wrong=source;wrong.heaps.heaps[0].object=0;
 Require(!lease.Acquire(wrong,bound)&&!lease.Queue(),"failed heap acquisition published ownership");
 Require(lease.Acquire(source,bound)&&lease.Queue()==d.queue.Get()&&lease.Epoch()==6,"original heap/fence acquisition failed");
 Require(!lease.Acquire(source,bound),"live heap lease acquired twice");
 Require(lease.Validate(100,d.queue.Get()),"saved native heap did not validate");
 Require(!lease.Validate(101,d.queue.Get())&&!lease.Validate(100,nullptr),"wrong frame/queue validated");
 put(records[0]+0x18,3);Require(lease.Validate(100,d.queue.Get()),"append-only native descriptor allocation rejected");
 put(records[0]+0x30,10);Require(!lease.Validate(100,d.queue.Get()),"recycled descriptor generation validated");put(records[0]+0x30,9);
 Check(fence->Signal(6));Require(!lease.Validate(100,d.queue.Get()),"native retirement fence completion ignored");
 D3D12NativeGIHeapLease expired;source.heaps.completed=0;
 Require(!expired.Acquire(source,bound),"fabricated completed=0 admitted already retired heap");
 std::cout<<"PASS original native heap lease: actual device/fence/queue, saved descriptors, append versus recycle, transactional acquisition, native retirement rejection\n";
}
