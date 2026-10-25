#pragma once
#include "rt_spot_capture_translation_test.h"

static void TestHitCaptureSubmissionPolicy(ID3D12Device* device) {
    namespace capture=native_lighting::rt_spot_capture_runtime;
    namespace journal=native_lighting::rt_spot_table_audit;
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList4> rayList,signalList;
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&rayList)));Check(rayList->Close());
    Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&signalList)));Check(signalList->Close());
    D3D12_COMMAND_QUEUE_DESC desc{};ComPtr<ID3D12CommandQueue> rayQueue,signalQueue;
    Check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&rayQueue)));Check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&signalQueue)));
    Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&capture::resources.fence)));
    Check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&capture::resources.signalFence)));
    auto wait=[](ID3D12Fence* fence,UINT64 value) {
        HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("stage fence event");
        Check(fence->SetEventOnCompletion(value,event));const auto result=WaitForSingleObject(event,10000);CloseHandle(event);
        if(result!=WAIT_OBJECT_0)throw std::runtime_error("stage fence timeout");
    };
    capture::fenceValue=1;capture::readbackCopied=false;
    capture::recordedList=rayList;capture::recordedGeneration=journal::Token(rayList.Get());
    capture::phase=capture::Phase::Recorded;capture::signalPhase=capture::SignalPhase::Waiting;
    ID3D12CommandList* rays[]={rayList.Get()};ID3D12CommandList* signals[]={signalList.Get()};
    // Reproduce R145's failure: rays submit and finish before composition exists.
    capture::BeforeExecute(rayQueue.Get(),1,rays);
    if(capture::phase!=capture::Phase::Submitting || !capture::WaitingForSignal())throw std::runtime_error("early ray submission rejected");
    rayQueue->ExecuteCommandLists(1,rays);capture::AfterExecute(rayQueue.Get(),1,rays);
    wait(capture::resources.fence.Get(),1);
    if(capture::phase!=capture::Phase::Submitted || capture::BothStagesCompleted() || !capture::WaitingForSignal())throw std::runtime_error("ray completion prematurely completed capture");
    // Native allocator/list rollover is allowed after submission, before signal recording.
    capture::ResetObserved(rayList.Get());journal::Token(rayList.Get(),true);
    if(capture::phase==capture::Phase::Failed)throw std::runtime_error("submitted ray reset rejected");
    capture::recordedSignalList=signalList;capture::signalGeneration=journal::Token(signalList.Get());
    capture::signalPhase=capture::SignalPhase::Recorded;
    capture::BeforeExecute(signalQueue.Get(),1,signals);
    if(capture::signalPhase!=capture::SignalPhase::Submitting || capture::signalQueue.Get()!=signalQueue.Get())throw std::runtime_error("signal submission owner");
    // The wrong queue must not publish completion.
    capture::AfterExecute(rayQueue.Get(),1,signals);
    if(capture::signalPhase!=capture::SignalPhase::Submitting)throw std::runtime_error("wrong signal queue accepted");
    signalQueue->ExecuteCommandLists(1,signals);capture::AfterExecute(signalQueue.Get(),1,signals);
    wait(capture::resources.signalFence.Get(),1);
    if(!capture::BothStagesCompleted())throw std::runtime_error("two-stage completion not recognized");
    capture::BeforeExecute(signalQueue.Get(),1,signals);
    if(capture::phase!=capture::Phase::Failed)throw std::runtime_error("signal replay accepted");
    // Same-list batches remain supported: both fences cover their shared submit.
    capture::fenceValue=2;capture::recordedGeneration=journal::Token(rayList.Get());
    capture::recordedSignalList=rayList;capture::signalGeneration=capture::recordedGeneration;
    capture::phase=capture::Phase::Recorded;capture::signalPhase=capture::SignalPhase::Recorded;
    capture::BeforeExecute(rayQueue.Get(),1,rays);
    if(capture::phase!=capture::Phase::Submitting || capture::signalPhase!=capture::SignalPhase::Submitting)throw std::runtime_error("shared batch not tracked twice");
    rayQueue->ExecuteCommandLists(1,rays);capture::AfterExecute(rayQueue.Get(),1,rays);
    wait(capture::resources.fence.Get(),2);wait(capture::resources.signalFence.Get(),2);
    if(!capture::BothStagesCompleted())throw std::runtime_error("shared batch completion");
    capture::BeforeExecute(rayQueue.Get(),1,rays);
    if(capture::phase!=capture::Phase::Failed)throw std::runtime_error("ray replay accepted");
    // Independently require both completion values, whichever completes first.
    capture::phase=capture::Phase::Submitted;capture::signalPhase=capture::SignalPhase::Submitted;capture::fenceValue=3;
    Check(capture::resources.signalFence->Signal(3));
    if(capture::BothStagesCompleted())throw std::runtime_error("signal-only fence accepted");
    Check(capture::resources.fence->Signal(3));if(!capture::BothStagesCompleted())throw std::runtime_error("both fences rejected");
    capture::fenceValue=4;Check(capture::resources.fence->Signal(4));
    if(capture::BothStagesCompleted())throw std::runtime_error("ray-only fence accepted");
    Check(capture::resources.signalFence->Signal(4));
    // Unsubmitted work cannot be reset/discarded or mistaken for a new generation.
    capture::phase=capture::Phase::Recorded;capture::signalPhase=capture::SignalPhase::Waiting;
    capture::ResetObserved(rayList.Get());if(capture::phase!=capture::Phase::Failed)throw std::runtime_error("ray discard accepted");
    capture::phase=capture::Phase::Submitted;capture::signalPhase=capture::SignalPhase::Recorded;
    capture::ResetObserved(rayList.Get());if(capture::phase!=capture::Phase::Failed)throw std::runtime_error("signal discard accepted");
    capture::phase=capture::Phase::Recorded;capture::signalPhase=capture::SignalPhase::Waiting;
    journal::Token(rayList.Get(),true);capture::BeforeExecute(rayQueue.Get(),1,rays);
    if(capture::phase!=capture::Phase::Recorded)throw std::runtime_error("stale generation accepted");
    capture::inScope=true;capture::EndScope();
    if(capture::phase!=capture::Phase::Failed)throw std::runtime_error("missing composition at scope end accepted");
    capture::recordedList.Reset();capture::recordedSignalList.Reset();capture::submittedQueue.Reset();capture::signalQueue.Reset();
    capture::resources.fence.Reset();capture::resources.signalFence.Reset();capture::recordedGeneration=capture::signalGeneration=0;
    capture::phase=capture::Phase::Idle;capture::signalPhase=capture::SignalPhase::Waiting;capture::fenceValue=1;
    std::cout<<"PASS R146 independent submissions: early ray submit/completion/reset, separate queues, shared batch, both fence orders, stale generations, wrong queue, replay and discard guards\n";
}

static void TestHitCaptureExportRetry() {
    namespace capture=native_lighting::rt_spot_capture_runtime;
    auto& queue=native_lighting::diagnosticLog;
    std::vector<char> bytes(native_lighting::DiagnosticLogQueue::Capacity(),'x');
    auto drain=[&](){return queue.Drain(bytes.data(),bytes.size());};
    auto fill=[&](){drain();if(!queue.Push(bytes.data(),bytes.size()))throw std::runtime_error("capture log fill");};
    memset(capture::completedBytes,0,sizeof(capture::completedBytes));
    const UINT valid=1;memcpy(capture::completedBytes+76,&valid,4);
    capture::phase.store(capture::Phase::Complete);capture::exportStage=0;capture::exportCursor=0;
    fill();capture::Poll();
    if(capture::exportStage!=0 || capture::exportCursor!=0)throw std::runtime_error("busy log discarded capture header");
    drain();capture::Poll();
    if(capture::exportStage!=2 || capture::exportCursor!=64)throw std::runtime_error("capture header retry did not resume");
    auto length=drain();std::string text(bytes.data(),length);
    if(text.find("rt_hit_gpu_recorded ")==std::string::npos || text.find("rt_hit_gpu_complete ")==std::string::npos || text.find("rt_hit_gpu_record index=0 ")==std::string::npos)throw std::runtime_error("capture headers missing after retry");
    capture::exportCursor=0;fill();capture::Poll();
    if(capture::exportCursor!=0)throw std::runtime_error("busy log discarded capture record");
    drain();capture::Poll();if(capture::exportCursor!=64)throw std::runtime_error("capture record retry failed");
    capture::exportCursor=4096;fill();capture::Poll();
    if(capture::exportStage!=2)throw std::runtime_error("busy log discarded completion marker");
    drain();capture::Poll();if(capture::exportStage!=3)throw std::runtime_error("capture completion retry failed");
    drain();queue.TakeDropped();capture::exportCursor=capture::exportStage=0;capture::phase.store(capture::Phase::Idle);
    std::cout<<"PASS capture log retry: full queue preserves header, record cursor and final completion marker without render-thread waits\n";
}

// Exercise both ordinary-build isolation and the diagnostic opt-in entry points.
static void TestGpuCapturePolicy(ID3D12Device* device) {
#ifdef CONTROL_CAPTURE_DIAGNOSTIC
    namespace capture=native_lighting::rt_spot_capture_runtime;
    static_assert(rt_spot_capture::GpuCaptureEnabled);
    const bool wasReady=capture::ready.exchange(false);capture::RequestFresh();
    if(capture::phase.load()!=capture::Phase::Idle)throw std::runtime_error("capture armed before installation");
    capture::ready.store(wasReady);capture::Install(device);
    auto* inputs=capture::resources.inputs.Get();
    auto* table=capture::resources.table.Get();
    capture::Install(device);
    if(!capture::ready.load() || !inputs || !table || inputs!=capture::resources.inputs.Get() || table!=capture::resources.table.Get() ||
       !capture::originalCopy || !capture::originalRanges || !capture::originalCbv || !capture::originalSrv || !capture::originalUav)
        throw std::runtime_error("capture installation missing or repeated allocations");
    // The second invocation follows InstallLiveDiagnostic; Install is idempotent.
    capture::ready.store(true);capture::signalReady.store(true);capture::RequestFresh();capture::RequestFresh();
    if(capture::phase.load()!=capture::Phase::Armed)throw std::runtime_error("capture did not arm");
    capture::BeginScope();capture::BeginScope();
    if(!capture::inScope || capture::phase.load()!=capture::Phase::Recording)throw std::runtime_error("capture scope entry");
    capture::EndScope();
    if(capture::inScope || capture::phase.load()!=capture::Phase::Armed)throw std::runtime_error("unrecorded capture retry");
    for(auto phase:{capture::Phase::Recorded,capture::Phase::Submitted,capture::Phase::Complete,capture::Phase::Failed}) {
        capture::phase.store(phase);capture::RequestFresh();capture::BeginScope();
        if(capture::phase.load()!=phase || capture::inScope)throw std::runtime_error("capture rearmed after recording");
    }
    capture::phase.store(capture::Phase::Idle);capture::attempts=0;
    std::cout<<"PASS R132 capture policy: installation idempotent, explicit request, scope retry and incomplete capture rearm rejection\n";
#else
    namespace capture=native_lighting::rt_spot_capture_runtime;
    static_assert(!rt_spot_capture::GpuCaptureEnabled);
    auto** table=*reinterpret_cast<void***>(device);
    constexpr UINT slots[]={17,18,19,23,24};
    void* before[5]{};for(UINT i=0;i<5;++i)before[i]=table[slots[i]];
    capture::Install(device);capture::Install(device);
    if(capture::ready.load() || capture::resources.device || capture::resources.inputs ||
       capture::resources.staging || capture::resources.fence || capture::originalCopy ||
       capture::originalRanges || capture::originalCbv || capture::originalSrv || capture::originalUav)
        throw std::runtime_error("disabled capture initialized GPU resources or hooks");
    for(UINT i=0;i<5;++i)if(before[i]!=table[slots[i]])throw std::runtime_error("disabled capture patched descriptor vtable");
    capture::ready.store(true);capture::RequestFresh();capture::BeginScope();
    if(capture::phase.load()!=capture::Phase::Idle || capture::inScope)
        throw std::runtime_error("BOTH armed disabled capture");
    capture::ready.store(false);
    if(native_lighting::rt_spot_capture_gpu::PreparePipeline(nullptr,nullptr,nullptr)!=E_NOTIMPL)
        throw std::runtime_error("disabled capture attempted pipeline preparation");
    capture::Record(nullptr,nullptr,nullptr);
    std::cout<<"PASS R129 capture isolation: no GPU allocations, descriptor mutations, startup compilation or BOTH capture activation\n";
#endif
}
