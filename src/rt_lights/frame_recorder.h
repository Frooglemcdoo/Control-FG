#pragma once
#include "frame_resources.h"
#include <algorithm>

namespace rt_lights {
// These are copied from one verified native command-list recording. The adapter
// must retain all referenced objects and the native shader-table allocations
// through submission. No field may be supplied from a different frame/list.
struct NativeBindings {
    ID3D12RootSignature* root = nullptr;
    ID3D12PipelineState* pipeline = nullptr;
    ID3D12DescriptorHeap* heap = nullptr;
    D3D12_GPU_DESCRIPTOR_HANDLE table[8]{};
    D3D12_GPU_VIRTUAL_ADDRESS constants[4]{};
    D3D12_CPU_DESCRIPTOR_HANDLE snapshot{};
    UINT64 observed = 0;
    bool Has(UINT64 required) const noexcept {
        return root && heap && snapshot.ptr && table[0].ptr && table[1].ptr && table[6].ptr && table[7].ptr &&
            constants[0] && constants[1] && constants[2] && constants[3] && (observed & required) == required;
    }
    void Apply(ID3D12GraphicsCommandList* list, D3D12_GPU_DESCRIPTOR_HANDLE custom) const noexcept {
        list->SetComputeRootSignature(root);
        for (UINT index : {1u, 6u, 7u}) list->SetComputeRootDescriptorTable(index, table[index]);
        for (UINT index = 0; index < 4; ++index) list->SetComputeRootConstantBufferView(index + 2, constants[index]);
        list->SetComputeRootDescriptorTable(0, custom);
    }
    void Restore(ID3D12GraphicsCommandList* list) const noexcept {
        Apply(list, table[0]);
        if (pipeline) list->SetPipelineState(pipeline);
    }
};

struct Pipelines {
    ID3D12PipelineState* base = nullptr;
    ID3D12PipelineState* point = nullptr;
    ID3D12PipelineState* batch = nullptr;
    ID3D12PipelineState* arguments = nullptr;
    ID3D12PipelineState* extract = nullptr;
    ID3D12PipelineState* pack = nullptr;
    ID3D12PipelineState* compose = nullptr;
    ID3D12PipelineState* commit = nullptr;
    ID3D12CommandSignature* indirectCompute = nullptr;
    ID3D12CommandSignature* indirectRays = nullptr;
    bool Ready() const noexcept {
        return base && point && batch && arguments && extract && pack && compose && commit && indirectCompute && indirectRays;
    }
};

struct DescriptorEdit {
    UINT slot = 0;
    D3D12_CPU_DESCRIPTOR_HANDLE source{};
};
struct DescriptorTable {
    D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
    D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
};
// Allocation must use the verified engine allocator on the SAME currently bound
// heap. Copy only from the private CPU snapshot, never from a shader-visible heap.
struct TableAllocator {
    void* context = nullptr;
    bool (*allocate)(void*, ID3D12DescriptorHeap*, DescriptorTable&) noexcept = nullptr;
    ID3D12Device* device = nullptr;
    UINT step = 0;

    bool Publish(const NativeBindings& native, const DescriptorEdit* edits, UINT count,
                 DescriptorTable& result, D3D12_GPU_VIRTUAL_ADDRESS rayControls = 0) const noexcept {
        if (!device || !step || !allocate || !allocate(context, native.heap, result) || !result.cpu.ptr || !result.gpu.ptr) return false;
        device->CopyDescriptorsSimple(49, result.cpu, native.snapshot, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        for (UINT i = 0; i < count; ++i) {
            if (edits[i].slot >= 49 || !edits[i].source.ptr) return false;
            auto destination = result.cpu; destination.ptr += SIZE_T(edits[i].slot) * step;
            device->CopyDescriptorsSimple(1, destination, edits[i].source, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }
        if (rayControls) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC cb{rayControls, 256};
            device->CreateConstantBufferView(&cb, result.cpu); // b4 is table slot zero.
        }
        return true;
    }
};

struct BatchTables {
    DescriptorTable generate, arguments, extract, rays, pack, compose, commit;
};

class FrameRecorder {
    static constexpr UINT Srv = 5, Uav = 41;
    static UINT Bits(float value) noexcept { UINT bits; std::memcpy(&bits, &value, sizeof(bits)); return bits; }
    static void Compute(ID3D12GraphicsCommandList* list, ID3D12PipelineState* pipeline,
                        D3D12_GPU_VIRTUAL_ADDRESS controls) noexcept {
        list->SetPipelineState(pipeline);
        list->SetComputeRootConstantBufferView(2, controls);
    }
    static DescriptorEdit Read(FrameResources& frame, UINT slot, Surface surface) noexcept {
        return {Srv + slot, frame.View(surface, false)};
    }
    static DescriptorEdit Write(FrameResources& frame, UINT slot, Surface surface) noexcept {
        return {Uav + slot, frame.View(surface, true)};
    }
public:
    // Called immediately AFTER the unchanged native producer dispatch, while
    // its input resources still have the states used by that dispatch.
    static bool ReplayBase(ID3D12GraphicsCommandList* list, FrameResources& frame,
                           const NativeBindings& native, const Pipelines& pipelines,
                           const TableAllocator& allocator, UINT x, UINT y, UINT z) noexcept {
        constexpr UINT64 required = (((1ull << 21) - 1) << Srv) | (7ull << Uav);
        if (!pipelines.Ready() || !native.Has(required) || z != 1 ||
            x != (frame.Width() + 7) / 8 || y != (frame.Height() + 7) / 8) return false;
        const DescriptorEdit edits[] = {Write(frame, 0, Surface::BaseDiffuse), Write(frame, 1, Surface::BaseSpecular), Write(frame, 2, Surface::BaseSelection)};
        DescriptorTable table;
        if (!allocator.Publish(native, edits, 3, table)) return false;
        for (Surface surface : {Surface::BaseDiffuse, Surface::BaseSpecular, Surface::BaseSelection})
            frame.Transition(list, surface, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        native.Apply(list, table.gpu); list->SetPipelineState(pipelines.base);
        list->Dispatch(x, y, z); FrameResources::UavBarrier(list);
        native.Restore(list);
        return true;
    }

    // The caller supplies preallocated storage for every batch table. Validate
    // and allocate ALL tables before recording any writes to the real outputs.
    // Failure leaves the native frame intact, even after private base replay.
    static bool Replace(ID3D12GraphicsCommandList4* list, FrameResources& frame,
                        const NativeBindings& producer, const NativeBindings& ray,
                        const NativeBindings& selected, const Pipelines& pipelines,
                        const TableAllocator& allocator, ID3D12StateObject* rayState,
                        const D3D12_DISPATCH_RAYS_DESC& nativeRays,
                        D3D12_GPU_VIRTUAL_ADDRESS rayRecord, UINT frameTag,
                        BatchTables* tables, UINT tableCapacity) noexcept {
        constexpr UINT64 selectedRequired = (((1ull << 17) - 1) << Srv) | (3ull << Uav);
        constexpr UINT64 producerRequired = (1ull << Srv) | (1ull << (Srv + 3)) | (3ull << (Srv + 15));
        const UINT receivers = frame.Width() * frame.Height();
        const UINT batches = (receivers + BatchCapacity::Receivers - 1) / BatchCapacity::Receivers;
        if (!list || !pipelines.Ready() || !rayState || !rayRecord || !tables || tableCapacity < batches ||
            !producer.Has(producerRequired) || !ray.Has(1ull << (Srv + 6)) || !selected.Has(selectedRequired) ||
            producer.heap != selected.heap || ray.heap != selected.heap || nativeRays.Depth != 1 ||
            nativeRays.Width != frame.Width() || nativeRays.Height != frame.Height()) return false;

        // All four are immutable upload slices owned by this recording.
        const UINT zero = 0, rayLayout[] = {0, 2, 0, 0};
        const UINT limits[] = {BatchCapacity::AtlasWidth, BatchCapacity::AtlasHeight, BatchCapacity::Tasks, 0};
        auto rayArguments = nativeRays; rayArguments.RayGenerationShaderRecord = {rayRecord, 32};
        const auto zeroAddress = frame.StoreConstants(&zero, sizeof(zero));
        const auto templateAddress = frame.StoreConstants(&rayArguments, sizeof(rayArguments));
        const auto rayControls = frame.StoreConstants(rayLayout, sizeof(rayLayout));
        const auto argumentControls = frame.StoreConstants(limits, sizeof(limits));
        if (!zeroAddress || !templateAddress || !rayControls || !argumentControls) return false;

        DescriptorTable pointTable;
        const DescriptorEdit pointEdits[] = {Write(frame, 0, Surface::BaseDiffuse), Write(frame, 1, Surface::BaseSpecular)};
        if (!allocator.Publish(selected, pointEdits, 2, pointTable)) return false;
        for (UINT batch = 0; batch < batches; ++batch) {
            auto& t = tables[batch];
            auto source = [&](UINT index) { auto h = producer.snapshot; h.ptr += SIZE_T(Srv + index) * allocator.step; return h; };
            const DescriptorEdit generate[] = {{Srv, source(0)}, {Srv + 1, source(3)}, {Srv + 2, source(16)}, {Srv + 3, source(15)},
                Write(frame, 0, Surface::Work), Write(frame, 1, Surface::Tasks), Write(frame, 2, Surface::Ranges), Write(frame, 3, Surface::Counter)};
            const DescriptorEdit arguments[] = {Read(frame, 0, Surface::Counter), Write(frame, 0, Surface::Work), Write(frame, 1, Surface::Arguments)};
            const DescriptorEdit extract[] = {Read(frame, 14, Surface::Work), Write(frame, 0, Surface::DiffuseAtlas), Write(frame, 1, Surface::SpecularAtlas)};
            const DescriptorEdit rays[] = {Read(frame, 8, Surface::Tasks), Write(frame, 2, Surface::Visibility)};
            const DescriptorEdit pack[] = {Read(frame, 0, Surface::Work), Read(frame, 1, Surface::DiffuseAtlas), Read(frame, 2, Surface::SpecularAtlas),
                Read(frame, 3, Surface::Tasks), Write(frame, 0, Surface::Contributions)};
            const DescriptorEdit compose[] = {Read(frame, 0, Surface::Visibility), Read(frame, 1, Surface::Contributions), Read(frame, 2, Surface::Ranges), Write(frame, 0, Surface::Lighting)};
            const DescriptorEdit commit[] = {Read(frame, 0, Surface::Lighting), Read(frame, 1, Surface::BaseDiffuse), Read(frame, 2, Surface::BaseSpecular)};
            if (!allocator.Publish(producer, generate, 8, t.generate) || !allocator.Publish(producer, arguments, 3, t.arguments) ||
                !allocator.Publish(selected, extract, 3, t.extract) || !allocator.Publish(ray, rays, 2, t.rays, rayControls) ||
                !allocator.Publish(selected, pack, 5, t.pack) || !allocator.Publish(selected, compose, 4, t.compose) ||
                !allocator.Publish(selected, commit, 3, t.commit)) return false;
        }
        FrameResources::UavBarrier(list);
        selected.Apply(list, pointTable.gpu); list->SetPipelineState(pipelines.point);
        list->Dispatch((frame.Width() + 7) / 8, (frame.Height() + 7) / 8, 1);
        FrameResources::UavBarrier(list);
        for (Surface surface : {Surface::BaseDiffuse, Surface::BaseSpecular})
            frame.Transition(list, surface, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        for (UINT batch = 0; batch < batches; ++batch) {
            const UINT first = batch * BatchCapacity::Receivers;
            const UINT count = (std::min)(BatchCapacity::Receivers, receivers - first);
            const UINT generate[] = {first, count, 1024, frameTag, 32, 1, Bits(0.001f), Bits(0.002f),
                BatchCapacity::AtlasWidth, BatchCapacity::AtlasHeight, Bits(1.0f), 0};
            const UINT pack[] = {frame.Width(), frame.Height(), frameTag, BatchCapacity::Tasks};
            const UINT compose[] = {first, count, frameTag, 0};
            const UINT commit[] = {first, count, frame.Width(), frame.Height()};
            const auto generateAddress = frame.StoreConstants(generate, sizeof(generate));
            const auto packAddress = frame.StoreConstants(pack, sizeof(pack));
            const auto composeAddress = frame.StoreConstants(compose, sizeof(compose));
            const auto commitAddress = frame.StoreConstants(commit, sizeof(commit));
            if (!generateAddress || !packAddress || !composeAddress || !commitAddress) { selected.Restore(list); return false; }
            const auto& t = tables[batch];
            frame.CopyStored(list, Surface::Counter, zeroAddress, 4);
            frame.CopyStored(list, Surface::Arguments, templateAddress, IndirectLayout::Bytes);
            for (Surface surface : {Surface::Counter, Surface::Arguments, Surface::Work, Surface::Tasks, Surface::Ranges})
                frame.Transition(list, surface, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
            producer.Apply(list, t.generate.gpu); list->SetPipelineState(pipelines.batch);
            list->SetComputeRootConstantBufferView(3, producer.constants[2]); // native deferred b2 -> batch b1
            list->SetComputeRootConstantBufferView(4, generateAddress);
            list->Dispatch((count + 63) / 64, 1, 1); FrameResources::UavBarrier(list);
            frame.Transition(list, Surface::Counter, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            producer.Apply(list, t.arguments.gpu); Compute(list, pipelines.arguments, argumentControls);
            list->Dispatch(1, 1, 1); FrameResources::UavBarrier(list);
            frame.Transition(list, Surface::Arguments, D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT);
            for (Surface surface : {Surface::Work, Surface::Tasks, Surface::Ranges})
                frame.Transition(list, surface, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            for (Surface surface : {Surface::DiffuseAtlas, Surface::SpecularAtlas, Surface::Visibility, Surface::Contributions, Surface::Lighting})
                frame.Transition(list, surface, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            selected.Apply(list, t.extract.gpu); list->SetPipelineState(pipelines.extract);
            list->ExecuteIndirect(pipelines.indirectCompute, 1, frame.Get(Surface::Arguments), IndirectLayout::Extract, nullptr, 0);
            FrameResources::UavBarrier(list);
            ray.Apply(list, t.rays.gpu); list->SetPipelineState1(rayState);
            list->ExecuteIndirect(pipelines.indirectRays, 1, frame.Get(Surface::Arguments), IndirectLayout::Rays, nullptr, 0);
            FrameResources::UavBarrier(list);
            for (Surface surface : {Surface::DiffuseAtlas, Surface::SpecularAtlas, Surface::Visibility})
                frame.Transition(list, surface, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            selected.Apply(list, t.pack.gpu); Compute(list, pipelines.pack, packAddress);
            list->ExecuteIndirect(pipelines.indirectCompute, 1, frame.Get(Surface::Arguments), IndirectLayout::Pack, nullptr, 0);
            frame.Transition(list, Surface::Contributions, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            selected.Apply(list, t.compose.gpu); Compute(list, pipelines.compose, composeAddress);
            list->Dispatch((count + 63) / 64, 1, 1);
            frame.Transition(list, Surface::Lighting, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
            selected.Apply(list, t.commit.gpu); Compute(list, pipelines.commit, commitAddress);
            list->Dispatch((count + 63) / 64, 1, 1); FrameResources::UavBarrier(list);
        }
        selected.Restore(list);
        return true;
    }
};
}
