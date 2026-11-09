#pragma once
// Included after RRPart1ReadResource, RRAlbedoRead, and RRAlbedoCapture exist.
// All native reads and commands stay on the authenticated capture thread.
static bool RRPart1EmitCopy(control_rr_part1::CopyOp op,RRGuideJob* job,
    const RRGuideInputSnapshot* context,DWORD* fault) noexcept {
    __try {
        auto count=reinterpret_cast<unsigned long long*>(static_cast<unsigned char*>(context->commandContext)+8);
        if(op==control_rr_part1::CopyOp::CopyBytes) {
            ++*count;
            context->commandList->CopyBufferRegion(job->part1Readback,0,job->part1Source,0,job->part1Bytes);
        } else {
            D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Transition.pResource=job->part1Source;
            barrier.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
            const auto shaderRead=static_cast<D3D12_RESOURCE_STATES>(0xC0);
            const auto copyRead=static_cast<D3D12_RESOURCE_STATES>(0xC0|D3D12_RESOURCE_STATE_COPY_SOURCE);
            barrier.Transition.StateBefore=op==control_rr_part1::CopyOp::ToCopySource?shaderRead:copyRead;
            barrier.Transition.StateAfter=op==control_rr_part1::CopyOp::ToCopySource?copyRead:shaderRead;
            ++*count;context->commandList->ResourceBarrier(1,&barrier);
        }
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { *fault=GetExceptionCode();return false; }
}
// Each emit is an SEH-contained leaf; this ordinary C++ orchestration never
// unwinds across a native lock and is also tested by the portable failure matrix.
static bool RRPart1RecordCommands(RRGuideJob* job,const RRGuideInputSnapshot* context,
    control_rr_part1::CopyProgress* progress,DWORD* fault) noexcept {
    return control_rr_part1::RecordCopy([&](control_rr_part1::CopyOp op) noexcept {
        return RRPart1EmitCopy(op,job,context,fault);
    },*progress);
}
#ifndef CONTROL_RR_PART1_EMIT_ONLY
static bool RRPart1SameContext(const RRGuideInputSnapshot& a,const RRGuideInputSnapshot& b) noexcept {
    return a.captureThreadId==b.captureThreadId && a.engineFrame==b.engineFrame &&
        a.presentToken==b.presentToken && a.commandContext==b.commandContext && a.commandList==b.commandList &&
        a.recordingLock==b.recordingLock && a.engineQueue==b.engineQueue && a.queue==b.queue &&
        a.staticTlsBlock==b.staticTlsBlock;
}
static bool RRPart1CopyImpl(RRGuideJob* job,const RRAlbedoCapture* capture,
    const char** reason,control_rr_part1::CopyProgress* progress,DWORD* fault) noexcept {
    bool locked=false,finished=false;
    RRGuideInputSnapshot current{},again{};
    control_rr_part1::Sample sample{};
    RRPart1ResourceObservation resource{};
    if(!job->part1Readback || job->part1Source || job->part1Copied) {
        *reason="part1_readback_unavailable_or_already_attempted";return true;
    }
    __try {
        if(!RRGuideReadRendererContext(&current,reason) || !RRPart1SameContext(current,job->input)) {
            *reason="part1_context_changed";__leave;
        }
        locked=RRGuideTryAcquireNativeLock(current.recordingLock);
        if(!locked) {*reason="part1_recording_lock_busy";__leave;}
        if(!RRGuideReadRendererContext(&again,reason) || !RRPart1SameContext(again,current)) {
            *reason="part1_locked_context_changed";__leave;
        }
        sample=control_rr_part1::Observe(&RRAlbedoRead,capture->renderer);
        sample.frameMatched=current.engineFrame==capture->frame;
        if(!control_rr_part1::PairMatched(capture->part1Preparation,sample)) {
            *reason="part1_preparation_identity_changed";__leave;
        }
        resource=RRPart1ReadResource(sample);
        if(!resource.read || !resource.descriptorMatched || resource.fault || resource.heapResult<0 ||
           !control_rr_part1::CopyLayoutAllowed(sample.metadata.bytes,sample.metadata.stride,
               resource.nativeState,resource.heapType,sample.mappingMode)) {
            *reason="part1_resource_or_state_preflight_rejected";__leave;
        }
        auto* source=reinterpret_cast<ID3D12Resource*>(resource.resource);
        const auto dst=job->part1Readback->GetDesc();
        if(source==job->part1Readback || dst.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER ||
           dst.Width<sample.metadata.bytes) {*reason="part1_destination_mismatch";__leave;}
        RRGuideInputSnapshot devices=current;
        devices.gbuffer1.resource=source;devices.gbuffer2.resource=source;
        if(!RRGuideResourcesShareDevice(&devices,job->part1Readback,nullptr)) {
            *reason="part1_device_mismatch";__leave;
        }
        source->AddRef();job->part1Source=source; // retained on all recording uncertainty
        job->part1Bytes=sample.metadata.bytes;
        const auto finalSample=control_rr_part1::Observe(&RRAlbedoRead,capture->renderer);
        const auto finalResource=RRPart1ReadResource(finalSample);
        if(!finalSample.repeatedReadMatched || !control_rr_part1::Equal(sample.metadata,finalSample.metadata) ||
           !finalResource.descriptorMatched || finalResource.resource!=resource.resource ||
           finalResource.nativeState!=resource.nativeState || !RRGuideReadRendererContext(&again,reason) ||
           !RRPart1SameContext(again,current)) {
            *reason="part1_final_identity_changed";__leave;
        }
        finished=RRPart1RecordCommands(job,&current,progress,fault);
        job->part1Copied=finished;
        *reason=finished?"part1_copy_recorded":"part1_recording_ambiguous";
    } __finally {
        if(locked) RRGuideReleaseNativeLock(current.recordingLock);
    }
    // A preflight rejection does not invalidate the normal/albedo diagnostic.
    return !progress->started || finished;
}
static bool RRPart1CopyForGuide(RRGuideJob* job,const RRAlbedoCapture* capture,const char** reason) noexcept {
    control_rr_part1::CopyProgress progress{};DWORD fault=0;bool okay=false;
    __try { okay=RRPart1CopyImpl(job,capture,reason,&progress,&fault); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        fault=GetExceptionCode();*reason="part1_readback_exception";
        okay=!progress.started;
    }
    Log("RR_GUIDE_G12_PART1_COPY frame=%llu copied=%u bytes=%llu started=%u initial_barrier=%u copy_call=%u restore_attempted=%u restored=%u reason=%s fault=0x%08lX rr_eval=disabled",
        job->input.engineFrame,unsigned(job->part1Copied),job->part1Bytes,unsigned(progress.started),
        unsigned(progress.initial),unsigned(progress.copy),unsigned(progress.restoreAttempted),
        unsigned(progress.restored),*reason,fault);
    return okay;
}
#endif
