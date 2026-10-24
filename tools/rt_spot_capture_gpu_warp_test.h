#pragma once
#include "rt_spot_capture_translation_test.h"

static void TestHitCaptureSubmissionPolicy(ID3D12Device* device) {
    namespace capture=native_lighting::rt_spot_capture_runtime;
    namespace journal=native_lighting::rt_spot_table_audit;
    ComPtr<ID3D12CommandAllocator> allocator;Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
    ComPtr<ID3D12GraphicsCommandList4> list;Check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));Check(list->Close());
    D3D12_COMMAND_QUEUE_DESC desc{};ComPtr<ID3D12CommandQueue> queue;Check(device->CreateCommandQueue(&desc,IID_PPV_ARGS(&queue)));
    capture::recordedList=list;capture::recordedGeneration=journal::Token(list.Get());capture::readbackCopied=false;
    ID3D12CommandList* lists[]={list.Get()};
    capture::phase.store(capture::Phase::Recorded);
    // A reused command-list object is insufficient: its generation must match.
    journal::Token(list.Get(),true);capture::BeforeExecute(queue.Get(),1,lists);
    if(capture::phase.load()!=capture::Phase::Recorded)throw std::runtime_error("stale generation submitted capture");
    capture::recordedGeneration=journal::Token(list.Get());capture::BeforeExecute(queue.Get(),1,lists);
    if(capture::phase.load()!=capture::Phase::Submitting || capture::submittedQueue.Get()!=queue.Get())throw std::runtime_error("capture submission identity");
    capture::BeforeExecute(queue.Get(),1,lists);
    if(capture::phase.load()!=capture::Phase::Failed)throw std::runtime_error("capture replay not rejected");
    capture::phase.store(capture::Phase::Recorded);capture::ResetObserved(list.Get());
    if(capture::phase.load()!=capture::Phase::Failed)throw std::runtime_error("discarded capture not rejected");
    if(capture::recordedList.Get()!=list.Get())throw std::runtime_error("uncertain capture reference released");
    capture::recordedList.Reset();capture::submittedQueue.Reset();capture::recordedGeneration=0;capture::phase.store(capture::Phase::Idle);
    std::cout<<"PASS capture submission policy: exact generation, actual queue, replay rejection, reset-before-submit rejection and retained uncertain references\n";
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
    capture::ready.store(true);capture::RequestFresh();capture::RequestFresh();
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
