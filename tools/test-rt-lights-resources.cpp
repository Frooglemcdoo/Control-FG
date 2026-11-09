#include "gpu_test_device.h"
#include "../src/rt_lights/frame_pool.h"
#include "../src/rt_lights/frame_recorder.h"
using namespace rt_lights;
static void Lifetime() {
    RecordingLifetime life;
    Require(life.Begin(1) && !life.Begin(2), "Aliased recording lease");
    Require(life.Submit(1, 10) && !life.Retire(10), "Fence completion released a resubmittable list");
    Require(life.Submit(1, 11) && !life.Submit(1, 11), "Repeated submission fence was not ordered");
    Require(life.Reset(1) && !life.Retire(10) && !life.Retire(UINT64_MAX), "Premature/device-removed retirement");
    Require(life.Retire(11) && life.Available(), "Completed reset recording did not retire");
    Require(life.Begin(2) && life.Reset(2) && life.Retire(0), "Unsubmitted discard retained its lease");
    Require(life.Begin(3), "Could not reuse discarded slot");
    life.Quarantine();
    Require(!life.Reset(3) && !life.Retire(100) && !life.Available(), "Quarantined work was reused");
}
static void Handoff() {
    Device gpu(false);
    FramePool pool; Check(pool.Initialize(gpu.device.Get()));
    Check(pool.Prepare(32,16,768ull*1024*1024));
    ComPtr<ID3D12CommandAllocator> consumerAllocator;
    ComPtr<ID3D12GraphicsCommandList> consumer;
    Check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&consumerAllocator)));
    Check(gpu.device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,consumerAllocator.Get(),nullptr,IID_PPV_ARGS(&consumer)));
    auto* slot = pool.Acquire(gpu.list.Get(),32,16);
    Require(slot != nullptr,"Missing handoff lease");
    const UINT value = 83;
    const auto address = slot->resources->StoreConstants(&value,sizeof(value));
    slot->resources->CopyStored(gpu.list.Get(),Surface::Counter,address,4);
    Check(gpu.list->Close());
    Require(pool.AttachConsumer(slot,consumer.Get()),"Second native list could not join lease");
    slot->resources->Transition(consumer.Get(),Surface::Counter,D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto readback = gpu.Buffer(4,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
    consumer->CopyBufferRegion(readback.Get(),0,slot->resources->Get(Surface::Counter),0,4);
    Check(consumer->Close()); pool.ReleaseRecording(slot);
    ID3D12CommandList* pair[] = {gpu.list.Get(),consumer.Get()};
    gpu.queue->ExecuteCommandLists(2,pair);
    Require(pool.Submitted(gpu.queue.Get(),2,pair),"Paired submission was rejected");
    auto wait = [&](UINT64 serial) {
        Check(gpu.queue->Signal(gpu.fence.Get(),serial));
        HANDLE event = CreateEventW(nullptr,FALSE,FALSE,nullptr);
        Require(event != nullptr,"Handoff fixture event failed");
        Check(gpu.fence->SetEventOnCompletion(serial,event));
        DWORD result = WaitForSingleObject(event,15000); CloseHandle(event);
        Require(result == WAIT_OBJECT_0,"Handoff fixture fence timed out");
    };
    wait(1);
    void* mapped = nullptr; Check(readback->Map(0,nullptr,&mapped));
    Require(*static_cast<UINT*>(mapped) == value,"Cross-list producer data was not preserved");
    readback->Unmap(0,nullptr);
    Check(gpu.list->Reset(gpu.allocator.Get(),nullptr)); pool.ResetSucceeded(gpu.list.Get());
    Require(!pool.Acquire(gpu.list.Get(),32,16),"Producer reset released a resubmittable consumer");
    gpu.queue->ExecuteCommandLists(1,pair+1);
    Require(pool.Submitted(gpu.queue.Get(),1,pair+1),"Consumer resubmission lost its resource lease");
    wait(2);
    Check(consumer->Reset(consumerAllocator.Get(),nullptr)); pool.ResetSucceeded(consumer.Get());
    auto* reused = pool.Acquire(gpu.list.Get(),32,16);
    Require(reused == slot,"Completed reset pair did not retire");
    Require(pool.AttachConsumer(reused,consumer.Get()),"Discard pair attach failed");
    Check(gpu.list->Close()); Check(consumer->Close());
    Check(gpu.list->Reset(gpu.allocator.Get(),nullptr)); pool.ResetSucceeded(gpu.list.Get());
    pool.ReleaseRecording(reused);
    Require(!pool.Acquire(gpu.list.Get(),32,16),"Partial discard released an attached consumer");
    Check(consumer->Reset(consumerAllocator.Get(),nullptr)); pool.ResetSucceeded(consumer.Get());
    pool.Poll(true);
    Require(!pool.Acquire(gpu.list.Get(),32,16),"Discarded pair was not trimmed");
    std::cout << "PASS: two-list GPU handoff, consumer resubmission, paired retirement and discard\n";
}

int main() try {
    Handoff();
    Lifetime();
    constexpr auto prefix = DescriptorOverlap(1024,49,992,3,32);
    static_assert(prefix.tableIndex == 0 && prefix.sourceSkip == 1 && prefix.count == 2);
    constexpr auto suffix = DescriptorOverlap(1024,49,2560,8,32);
    static_assert(suffix.tableIndex == 48 && suffix.count == 1);
    constexpr auto covering = DescriptorOverlap(1024,49,0,128,32);
    static_assert(covering.sourceSkip == 32 && covering.count == 49);
    static_assert(DescriptorOverlap(1024,49,1025,8,32).count == 0);
    static_assert(DescriptorOverlap(1024,49,2592,1,32).count == 0);
    Device gpu(false);
    FramePool pool; Check(pool.Initialize(gpu.device.Get()));
    constexpr UINT64 budget = 768ull * 1024 * 1024;
    Check(pool.Prepare(32, 16, budget));
    auto* slot = pool.Acquire(gpu.list.Get(), 32, 16);
    Require(slot && !pool.Acquire(gpu.list.Get(), 64, 16), "Unprepared extent was accepted");
    auto& frame = *slot->resources;
    const UINT first[] = {1, 2, 3, 4}, second[] = {5, 6, 7, 8};
    auto a = frame.StoreConstants(first, sizeof(first)), b = frame.StoreConstants(second, sizeof(second));
    Require(a && b == a + 256, "Constant slices overlap or are misaligned");
    frame.CopyStored(gpu.list.Get(), Surface::Counter, a, 4);
    frame.Transition(gpu.list.Get(), Surface::Counter, D3D12_RESOURCE_STATE_COPY_SOURCE);
    auto copied = gpu.Buffer(4, D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST);
    gpu.list->CopyBufferRegion(copied.Get(), 0, frame.Get(Surface::Counter), 0, 4);
    const ReceiverRange range{0,1,84,0};
    const Lighting light{{1,2,3},1,{4,5,6},1};
    const Visibility visibility{1,0,1,0,0,84,0,0};
    const Contribution contribution{0,0,0,84,{1,2,3},0,{4,5,6},0};
    const auto uploadEvidence = [&](Surface surface, const void* value, UINT bytes) {
        const auto address = frame.StoreConstants(value,bytes);
        Require(address != 0,"Evidence fixture constants exhausted");
        frame.CopyStored(gpu.list.Get(),surface,address,bytes);
    };
    uploadEvidence(Surface::Ranges,&range,sizeof(range));
    uploadEvidence(Surface::Lighting,&light,sizeof(light));
    uploadEvidence(Surface::Visibility,&visibility,sizeof(visibility));
    uploadEvidence(Surface::Contributions,&contribution,sizeof(contribution));
    frame.CaptureEvidence(gpu.list.Get(),0,1,84);
    FrameResources::EvidenceSummary early;
    Require(frame.EvidencePending() && !frame.ReadEvidence(early),"Unretired GPU evidence was readable");
    pool.ReleaseRecording(slot);
    gpu.Submit(); // Fixture-only blocking wait; production pool has none.
    ID3D12CommandList* submitted[] = {gpu.list.Get()};
    Require(pool.Submitted(gpu.queue.Get(), 1, submitted), "Actual submission was rejected");
    pool.Poll();
    Require(!pool.Acquire(gpu.list.Get(), 32, 16), "Completed but unreset command list released its slot");
    void* data = nullptr; Check(copied->Map(0, nullptr, &data));
    Require(*static_cast<UINT*>(data) == 1, "Later constants overwrote an earlier dispatch's upload slice");
    copied->Unmap(0, nullptr);
    Check(gpu.allocator->Reset()); Check(gpu.list->Reset(gpu.allocator.Get(), nullptr));
    pool.ResetSucceeded(gpu.list.Get());
    // Submit the new empty recording so the fixture's completion fence also
    // orders the pool's earlier signal. Only the fixture waits on this fence.
    Check(gpu.list->Close()); gpu.queue->ExecuteCommandLists(1, submitted);
    Require(pool.Submitted(gpu.queue.Get(), 1, submitted), "Reset list submission was charged to its old recording");
    Check(gpu.queue->Signal(gpu.fence.Get(), 2));
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(event != nullptr, "Fixture event creation failed");
    Check(gpu.fence->SetEventOnCompletion(2, event));
    const DWORD waited = WaitForSingleObject(event, 15000); CloseHandle(event);
    Require(waited == WAIT_OBJECT_0, "Fixture fence timed out");
    // Completed evidence must not hold the render slot hostage to the worker timer.
    auto* reused = pool.Acquire(gpu.list.Get(),32,16);
    Require(reused == slot,"GPU evidence blocked scratch reuse");
    static bool reported = false;
    pool.Poll(false, [](const FrameResources::EvidenceSummary& evidence) {
        Require(evidence.tasks==1 && evidence.validRanges==1 && evidence.litRanges==1 &&
            evidence.validLighting==1 && evidence.validVisibility==1 && evidence.validContributions==1,
            "Completed GPU evidence did not match recorded data");
        reported = true;
    });
    Require(reported,"Worker did not report completed GPU evidence");
    Check(gpu.allocator->Reset()); Check(gpu.list->Reset(gpu.allocator.Get(), nullptr));
    Require(reused == slot, "Completed recording was not reusable without worker polling");
    // Discard an unsubmitted recording with a recorded state change. Subsequent
    // recording must start from the actual last submitted COPY_SOURCE state.
    frame.Transition(gpu.list.Get(), Surface::Counter, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    Check(gpu.list->Close()); Check(gpu.list->Reset(gpu.allocator.Get(), nullptr));
    pool.ResetSucceeded(gpu.list.Get()); pool.ReleaseRecording(reused); pool.Poll();
    pool.Poll(true);
    Require(!pool.Acquire(gpu.list.Get(), 32, 16), "Disabled feature retained an idle allocation");
    std::cout << "PASS: recording retirement, resubmission ownership, immutable upload slices, extent gating and worker cleanup\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
