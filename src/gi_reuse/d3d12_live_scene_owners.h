#pragma once
#include "d3d12_storage_lease.h"
#include "scene_producer_table.h"
#include <mutex>
#include <memory>
#include <chrono>
namespace gi_reuse {
// Native wrapper lifetime registry, separate from the bounded diagnostic cache.
// Entries are observed at verified native creation/release boundaries. COM
// identity is canonicalized: different interface pointers do not invent owners.
// A retained resource proves storage lifetime only, not unchanged geometry bytes,
// alias activation, GPU state, residency, queue ordering or submission completion.
class D3D12LiveSceneOwners {
    using U64=std::uint64_t;
    struct Entry {
        BufferStorageLease storage;
        Microsoft::WRL::ComPtr<IUnknown> identity;
        U64 token=0,address=0,bytes=0;std::set<U64> wrappers;
    };
    std::mutex mutex_;
    PTOwnedScene::Index index_;
    GIValidatedMappingCache mappingCache_;
    std::map<U64,std::shared_ptr<Entry>> wrappers_,tokens_;
    std::map<IUnknown*,U64> identities_;
    U64 serial_=0,bytes_=0;
    bool healthy_=true;
    const U64 budget_;
    std::shared_ptr<Entry> Remove(U64 wrapper) {
        const auto it=wrappers_.find(wrapper);if(it==wrappers_.end())return {};
        auto old=it->second;wrappers_.erase(it);
        old->wrappers.erase(wrapper);
        if(old->wrappers.empty()){index_.Remove(old->token);identities_.erase(old->identity.Get());tokens_.erase(old->token);bytes_-=old->bytes;}
        return old;
    }
public:
    explicit D3D12LiveSceneOwners(U64 budget=2ull*1024*1024*1024):budget_(budget){}
    struct PrepareTimings {double lockMS=0,mappingMS=0,leasesMS=0;GISceneProducerTimings detail;};
    struct LookupDiagnostic {U64 owners=0,bytes=0,budget=0,covering=0;const char* reason="none";};
    struct Identity {U64 token=0,address=0,bytes=0;};
    struct Lease {
        GISceneProducerPlan plan;
        std::vector<BufferStorageLease> buffers;
        std::vector<std::shared_ptr<const GIFrozenGeometry>> frozen;
    };
    bool Created(U64 wrapper,IUnknown* native,U64 address,U64 nativeBytes) noexcept {
        // Keep evicted references outside the lock: final COM Release may enter
        // an interposer. Failed creation observations invalidate the old wrapper.
        std::shared_ptr<Entry> old,candidate;
        try {
            std::unique_lock<std::mutex> guard(mutex_);old=Remove(wrapper);
            if(!healthy_||!wrapper||!native||!address||!nativeBytes)return false;
            candidate=std::make_shared<Entry>();Microsoft::WRL::ComPtr<ID3D12Resource> resource;
            if(FAILED(native->QueryInterface(IID_PPV_ARGS(&resource)))||
               FAILED(native->QueryInterface(IID_PPV_ARGS(&candidate->identity))))return false;
            const auto d=resource->GetDesc();
            if(d.Dimension!=D3D12_RESOURCE_DIMENSION_BUFFER||!d.Width||nativeBytes>d.Width||
               resource->GetGPUVirtualAddress()!=address||d.Width>UINT64_MAX-address||
               !AcquireStorageLease(resource.Get(),candidate->storage))return false;
            const auto found=identities_.find(candidate->identity.Get());
            if(found!=identities_.end()){
                const auto existing=tokens_.at(found->second);
                if(existing->address!=address||existing->bytes!=d.Width)return false;
                existing->wrappers.insert(wrapper);wrappers_.emplace(wrapper,existing);return true;
            }
            if(tokens_.size()>=8192||bytes_>budget_||d.Width>budget_-bytes_||serial_==UINT64_MAX)return false;
            candidate->token=++serial_;candidate->address=address;candidate->bytes=d.Width;candidate->wrappers.insert(wrapper);
            // Any allocation exception disables admission rather than exposing
            // an index with only half of its owner transaction installed.
            index_.Insert({candidate->token,address,d.Width});
            tokens_.emplace(candidate->token,candidate);identities_.emplace(candidate->identity.Get(),candidate->token);
            wrappers_.emplace(wrapper,candidate);bytes_+=d.Width;return true;
        }catch(...){std::lock_guard<std::mutex> guard(mutex_);healthy_=false;return false;}
    }
    // Remove before native release. Already acquired dispatch leases retain the
    // resource; future address lookups cannot bind its retired generation.
    void Retire(U64 wrapper) noexcept {
        std::shared_ptr<Entry> old;
        try {std::lock_guard<std::mutex> guard(mutex_);old=Remove(wrapper);}
        catch(...){std::lock_guard<std::mutex> guard(mutex_);healthy_=false;}
    }
    bool Identify(U64 address,U64 bytes,Identity& out,LookupDiagnostic* diagnostic=nullptr) noexcept {
        out={};try {std::lock_guard<std::mutex> guard(mutex_);
            if(diagnostic){*diagnostic={};diagnostic->owners=tokens_.size();diagnostic->bytes=bytes_;diagnostic->budget=budget_;}
            if(!healthy_){if(diagnostic)diagnostic->reason="registry_unhealthy";return false;}
            const auto token=index_.Find(address,bytes);if(!token){
                if(diagnostic){for(const auto& item:tokens_){const auto& e=*item.second;
                    if(address>=e.address&&address-e.address<=e.bytes&&bytes<=e.bytes-(address-e.address))++diagnostic->covering;}
                    diagnostic->reason=diagnostic->covering>1?"overlapping_owners":"missing_owner";}
                return false;
            }
            const auto& e=*tokens_.at(token);out={token,address,e.bytes-(address-e.address)};return true;
        }catch(...){return false;}
    }
    bool Geometry(const TriangleMetadata& t,PTOwnedScene::Triangle& out) noexcept {
        out={};try {std::lock_guard<std::mutex> guard(mutex_);
            if(!healthy_||GeometryMetadata::Validate(t)!=GeometryReject::None||t.transform)return false;
            const auto index=index_.Find(t.indices,GeometryMetadata::IndexBytes(t));
            const auto position=index_.Find(t.vertices,GeometryMetadata::VertexBytes(t));
            if(!index||!position)return false;
            out={t.indices,t.vertices,t.stride,index,position,GeometryMetadata::IndexSize(t),t.indexCount,t.vertexCount};
            if(!index_.Observe(out)){out={};return false;}return true;
        }catch(...){return false;}
    }
    // Exact tokens are acquired while the matching metadata index is locked.
    // The scene catalog must separately keep its ordered cut stable throughout
    // this call. No later current-address lookup can repair missing witnesses.
    template<std::size_t Capacity> GISceneProducerError Prepare(
        const OrderedASMetadata<Capacity>& history,U64 cut,U64 epoch,SceneASReference top,
        const SceneByteView& instances,const std::vector<SceneASReference>& perInstance,
        const std::vector<GISceneGeometry>& geometry,const NativeGITable& native,
        const std::vector<PTNativeBinding::Binding>& bindings,U64 pipeline,
        const PTShaderTableAdapter::Exports& exports,Lease& output,GISceneProducerDiagnostic* diagnostic=nullptr,PrepareTimings* timings=nullptr) noexcept {
        Lease next;
        using Clock=std::chrono::steady_clock;
        const auto started=Clock::now();if(timings)*timings={};
        try {std::lock_guard<std::mutex> guard(mutex_);
            const auto locked=Clock::now();
            if(timings)timings->lockMS=std::chrono::duration<double,std::milli>(locked-started).count();
            if(!healthy_)return GISceneProducerError::Identity;
            const auto result=PrepareGISceneProducer(history,cut,epoch,top,instances,perInstance,geometry,index_,native,bindings,pipeline,exports,next.plan,diagnostic,&mappingCache_,timings?&timings->detail:nullptr);
            const auto mapped=Clock::now();
            if(timings)timings->mappingMS=std::chrono::duration<double,std::milli>(mapped-locked).count();
            if(result!=GISceneProducerError::None)return result;
            for(const auto& g:geometry)if(g.frozen)next.frozen.push_back(g.frozen);
            auto tokens=next.plan.allocationTokens;tokens.push_back(top.token);
            for(const auto& bottom:next.plan.closure.bottoms)tokens.push_back(bottom.token);
            std::sort(tokens.begin(),tokens.end());tokens.erase(std::unique(tokens.begin(),tokens.end()),tokens.end());
            next.buffers.reserve(tokens.size());
            for(const auto token:tokens){const auto found=tokens_.find(token);if(found==tokens_.end())return GISceneProducerError::Identity;
                next.buffers.push_back(found->second->storage);}
            const auto matches=[&](const SceneASReference& ref){return index_.Find(ref.address,256)==ref.token;};
            if(!matches(top))return GISceneProducerError::Identity;
            for(const auto& bottom:next.plan.closure.bottoms)if(!matches(bottom))return GISceneProducerError::Identity;
            if(timings)timings->leasesMS=std::chrono::duration<double,std::milli>(Clock::now()-mapped).count();
        }catch(...){return GISceneProducerError::Allocation;}
        output=std::move(next);return GISceneProducerError::None;
    }
    bool Acquire(U64 token,BufferStorageLease& output) noexcept {
        BufferStorageLease next;
        {std::lock_guard<std::mutex> guard(mutex_);if(!healthy_)return false;
            const auto found=tokens_.find(token);if(found==tokens_.end())return false;next=found->second->storage;}
        output=std::move(next);return true;
    }
    bool Inspect(U64 token,BufferStorageLease& output,U64& wrapper) noexcept {
        wrapper=0;std::lock_guard<std::mutex> guard(mutex_);if(!healthy_)return false;
        const auto found=tokens_.find(token);if(found==tokens_.end())return false;
        if(found->second->wrappers.empty())return false;
        wrapper=*found->second->wrappers.begin();output=found->second->storage;return true;
    }
    struct HitSource {U64 token=0;BufferStorageLease storage;D3D12_RESOURCE_STATES state{};};
private:
    struct NativeBufferSample {IUnknown* resource=nullptr;U64 address=0,bytes=0;UINT state=0;};
    static bool ReadNativeBuffer(U64 wrapper,NativeBufferSample* out)noexcept {
        if(!wrapper||!out)return false;
        __try {const auto* p=reinterpret_cast<const unsigned char*>(wrapper);
            out->resource=*reinterpret_cast<IUnknown*const*>(p);out->state=*reinterpret_cast<const UINT*>(p+0x20);
            out->bytes=*reinterpret_cast<const U64*>(p+0x30);out->address=*reinterpret_cast<const U64*>(p+0x48);
            return out->resource&&out->bytes&&out->address;
        }__except(EXCEPTION_EXECUTE_HANDLER){*out={};return false;}
    }
public:
    // Original-ray-only acquisition, from the creation/release registry. Keep
    // the registry lock while reading wrapper metadata: Retire runs BEFORE
    // native wrapper destruction. No raw wrapper escapes this transaction.
    // These leases retain backing storage, not content. Caller must immediately
    // record private copies under the original native source-use ordering.
    bool AcquireHitSources(const std::vector<PTNativeBinding::Binding>& bindings,
                           std::vector<HitSource>& out,const char** failure=nullptr)noexcept {
        out.clear();if(failure)*failure="input";
        if(bindings.empty()||bindings.size()>65536)return false;
        std::vector<HitSource> next;
        try {
            std::lock_guard<std::mutex> guard(mutex_);
            if(!healthy_){if(failure)*failure="registry_unhealthy";return false;}
            std::map<U64,U64> selected;
            for(const auto& b:bindings){
                if(b.type>1||(b.constants[1]!=2&&b.constants[1]!=4)||
                   !b.constants[8]||!b.constants[9])return false;
                const U64 addresses[]={b.indices,b.attributes};const U64 sizes[]={b.constants[8],b.constants[9]};
                for(unsigned kind=0;kind<2;++kind){const auto token=index_.Find(addresses[kind],sizes[kind]);
                    if(!token){if(failure)*failure="source_range_owner";return false;}
                    const auto& allocation=*tokens_.at(token);
                    selected[token]=(std::max)(selected[token],addresses[kind]-allocation.address+sizes[kind]);
                }
            }
            next.reserve(selected.size());
            for(const auto& selectedRange:selected){const auto token=selectedRange.first;const auto found=tokens_.find(token);
                if(found==tokens_.end()||found->second->wrappers.empty()){if(failure)*failure="retired_source";return false;}
                const auto& entry=*found->second;UINT state=0;bool sampled=false;
                for(const auto wrapper:entry.wrappers){NativeBufferSample sample;
                    if(!ReadNativeBuffer(wrapper,&sample)||sample.address!=entry.address||
                       sample.bytes>entry.bytes||sample.bytes<selectedRange.second||
                       (sample.resource!=static_cast<IUnknown*>(entry.storage.resource.Get())&&sample.resource!=entry.identity.Get())){
                        if(failure)*failure="native_wrapper_identity";return false;
                    }
                    if((sample.state!=D3D12_RESOURCE_STATE_COMMON&&!(sample.state&D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE))||
                       (sample.state&~unsigned(D3D12_RESOURCE_STATE_GENERIC_READ))||(sampled&&state!=sample.state)){
                        if(failure)*failure="native_source_state";return false;
                    }
                    state=sample.state;sampled=true;
                }
                next.push_back({token,entry.storage,static_cast<D3D12_RESOURCE_STATES>(state)});
            }
        }catch(...){if(failure)*failure="allocation";return false;}
        out=std::move(next);if(failure)*failure="none";return true;
    }
    U64 RetainedBytes() {std::lock_guard<std::mutex> guard(mutex_);return bytes_;}
};
}
