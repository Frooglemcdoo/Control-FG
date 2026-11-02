#pragma once
#include "compute_root_arguments.h"
#include <d3d12.h>
#include <utility>
#include <wrl/client.h>

namespace gi_reuse {
// Owned immutable value snapshot. This retains binding objects, not the resources
// referenced by descriptors/GPU addresses; recording and submission leases must
// separately retain those. Restore never switches descriptor heaps.
struct ComputeBindingSnapshot {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline;
    Microsoft::WRL::ComPtr<ID3D12StateObject> rays;
    std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>,2> heaps;
    ComputeRootArguments arguments;
    bool Ready() const noexcept { return root && (pipeline || rays) && arguments.Ready(); }
    bool Restore(ID3D12GraphicsCommandList4* list) const noexcept {
        if (!list || !Ready()) return false;
        list->SetComputeRootSignature(root.Get());
        struct Sink {
            ID3D12GraphicsCommandList4* list;
            void Constants(unsigned index,unsigned offset,unsigned count,const std::uint32_t* values) noexcept {
                list->SetComputeRoot32BitConstants(index,count,values,offset);
            }
            void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address) noexcept {
                switch(kind) {
                case RootArgumentKind::Table: list->SetComputeRootDescriptorTable(index,{address}); break;
                case RootArgumentKind::CBV: list->SetComputeRootConstantBufferView(index,address); break;
                case RootArgumentKind::SRV: list->SetComputeRootShaderResourceView(index,address); break;
                case RootArgumentKind::UAV: list->SetComputeRootUnorderedAccessView(index,address); break;
                default: break;
                }
            }
        } sink{list};
        arguments.Replay(sink);
        if (rays) list->SetPipelineState1(rays.Get());
        else list->SetPipelineState(pipeline.Get());
        return true;
    }
};
// One instance belongs to one command-list recording. Not installed into game
// hooks yet. Hook installation must establish full coverage before admission,
// exclude injected calls, and call Reset only after a successful native reset.
class ComputeBindingTracker {
    ComputeBindingSnapshot state_;
public:
    void Reset(ID3D12PipelineState* initial=nullptr) noexcept {
        state_ = {}; state_.pipeline = initial;
    }
    void Invalidate() noexcept { state_.arguments.Invalidate(); }
    void Root(ID3D12RootSignature* root) noexcept {
        state_.arguments.Root(reinterpret_cast<std::uintptr_t>(root)); state_.root = root;
    }
    void Pipeline(ID3D12PipelineState* pipeline) noexcept {
        state_.pipeline = pipeline; state_.rays.Reset();
    }
    void Rays(ID3D12StateObject* rays) noexcept {
        state_.rays = rays; state_.pipeline.Reset();
    }
    void Descriptor(unsigned index,RootArgumentKind kind,std::uint64_t address) noexcept {
        state_.arguments.Descriptor(index,kind,address);
    }
    void Constants(unsigned index,unsigned offset,unsigned count,const std::uint32_t* values) noexcept {
        state_.arguments.Constants(index,offset,count,values);
    }
    void Heaps(unsigned count,ID3D12DescriptorHeap* const* heaps) noexcept {
        if (count>2 || (count && !heaps)) { Invalidate(); return; }
        std::array<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>,2> next;
        for (unsigned i=0;i<count;++i) {
            if (!heaps[i]) { Invalidate(); return; }
            const auto desc=heaps[i]->GetDesc();
            if (!(desc.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)) { Invalidate(); return; }
            const auto type=desc.Type;
            if (type!=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV && type!=D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) { Invalidate(); return; }
            const unsigned slot=type==D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER?1u:0u;
            if (next[slot]) { Invalidate(); return; }
            next[slot]=heaps[i];
        }
        if (next[0].Get()!=state_.heaps[0].Get() || next[1].Get()!=state_.heaps[1].Get()) state_.arguments.HeapChange();
        state_.heaps=std::move(next);
    }
    bool Ready() const noexcept { return state_.Ready(); }
    bool Capture(ComputeBindingSnapshot& snapshot) const noexcept {
        snapshot = {};
        if (!state_.Ready()) return false;
        snapshot = state_; return true;
    }
};
}
