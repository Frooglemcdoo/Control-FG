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
int main() try {
    Lifetime();
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
    Check(gpu.queue->Signal(gpu.fence.Get(), 2));
    HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(event != nullptr, "Fixture event creation failed");
    Check(gpu.fence->SetEventOnCompletion(2, event));
    const DWORD waited = WaitForSingleObject(event, 15000); CloseHandle(event);
    Require(waited == WAIT_OBJECT_0, "Fixture fence timed out");
    pool.Poll();
    Check(gpu.allocator->Reset()); Check(gpu.list->Reset(gpu.allocator.Get(), nullptr));
    auto* reused = pool.Acquire(gpu.list.Get(), 32, 16);
    Require(reused == slot, "Retired recording was not reusable");
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
