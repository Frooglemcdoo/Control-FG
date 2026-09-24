#pragma once
// Borrowed, synchronous observation only. No ownership, GPU copy, or state change.
struct RRPart1ResourceObservation {
    std::uintptr_t resource=0;
    unsigned long long width=0, gpuAddress=0;
    unsigned dimension=0, height=0, depth=0, mips=0, format=0, samples=0, layout=0, flags=0;
    unsigned nativeState=0, heapType=0;
    long heapResult=0;
    bool read=false, pointerMatched=false, descriptorMatched=false;
    DWORD fault=0;
};
static RRPart1ResourceObservation RRPart1ReadResource(const control_rr_part1::Sample& sample) noexcept {
    RRPart1ResourceObservation out{};
    if(sample.result!=control_rr_part1::Result::Candidate || !sample.repeatedReadMatched ||
       !sample.mappingReadable || sample.mappingMode!=2) return out;
    const auto buffer=static_cast<std::uintptr_t>(sample.metadata.buffer);
    // Metadata validation has already checked buffer+0x48 arithmetic.
    if(!RRAlbedoRead(buffer,&out.resource,sizeof(out.resource)) || out.resource<0x10000 ||
       !RRAlbedoRead(buffer+0x20,&out.nativeState,sizeof(out.nativeState))) return out;
    __try {
        auto* resource=reinterpret_cast<ID3D12Resource*>(out.resource);
        const auto desc=resource->GetDesc();
        out.gpuAddress=resource->GetGPUVirtualAddress();
        D3D12_HEAP_PROPERTIES heap{}; D3D12_HEAP_FLAGS heapFlags{};
        out.heapResult=resource->GetHeapProperties(&heap,&heapFlags);
        if(SUCCEEDED(out.heapResult)) out.heapType=unsigned(heap.Type);
        out.width=desc.Width;out.dimension=unsigned(desc.Dimension);out.height=desc.Height;
        out.depth=desc.DepthOrArraySize;out.mips=desc.MipLevels;out.format=unsigned(desc.Format);
        out.samples=desc.SampleDesc.Count;out.layout=unsigned(desc.Layout);out.flags=unsigned(desc.Flags);
        std::uintptr_t again=0;
        out.pointerMatched=RRAlbedoRead(buffer,&again,sizeof(again)) && again==out.resource;
        out.read=true;
        out.descriptorMatched=out.pointerMatched &&
            desc.Dimension==D3D12_RESOURCE_DIMENSION_BUFFER && desc.Width==sample.metadata.bytes &&
            desc.Height==1 && desc.DepthOrArraySize==1 && desc.MipLevels==1 &&
            desc.Format==DXGI_FORMAT_UNKNOWN && desc.SampleDesc.Count==1 &&
            desc.Layout==D3D12_TEXTURE_LAYOUT_ROW_MAJOR && out.gpuAddress!=0 &&
            out.gpuAddress==sample.metadata.gpuAddress;
    } __except(EXCEPTION_EXECUTE_HANDLER) { out.fault=GetExceptionCode(); }
    return out;
}
