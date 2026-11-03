#pragma once
#include "d3d12_storage_lease.h"
#include "scene_producer_table.h"
#include <mutex>
#include <memory>
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
        U64 token=0,address=0,bytes=0;std::size_t wrappers=0;
    };
    std::mutex mutex_;
    PTOwnedScene::Index index_;
    std::map<U64,std::shared_ptr<Entry>> wrappers_,tokens_;
    std::map<IUnknown*,U64> identities_;
    U64 serial_=0,bytes_=0;
    bool healthy_=true;
    const U64 budget_;
    std::shared_ptr<Entry> Remove(U64 wrapper) {
        const auto it=wrappers_.find(wrapper);if(it==wrappers_.end())return {};
        auto old=it->second;wrappers_.erase(it);
        if(!--old->wrappers){index_.Remove(old->token);identities_.erase(old->identity.Get());tokens_.erase(old->token);bytes_-=old->bytes;}
        return old;
    }
public:
    explicit D3D12LiveSceneOwners(U64 budget=2ull*1024*1024*1024):budget_(budget){}
    struct Identity {U64 token=0,address=0,bytes=0;};
    struct Lease {
        GISceneProducerPlan plan;
        std::vector<BufferStorageLease> buffers;
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
                wrappers_.emplace(wrapper,existing);++existing->wrappers;return true;
            }
            if(tokens_.size()>=8192||bytes_>budget_||d.Width>budget_-bytes_||serial_==UINT64_MAX)return false;
            candidate->token=++serial_;candidate->address=address;candidate->bytes=d.Width;candidate->wrappers=1;
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
    bool Identify(U64 address,U64 bytes,Identity& out) noexcept {
        out={};try {std::lock_guard<std::mutex> guard(mutex_);if(!healthy_)return false;
            const auto token=index_.Find(address,bytes);if(!token)return false;
            const auto& e=*tokens_.at(token);out={token,address,e.bytes-(address-e.address)};return true;
        }catch(...){return false;}
    }
    bool Geometry(const TriangleMetadata& t,PTOwnedScene::Triangle& out) noexcept {
        out={};try {std::lock_guard<std::mutex> guard(mutex_);
            if(!healthy_||GeometryMetadata::Validate(t)!=GeometryReject::None||t.transform)return false;
            const auto index=index_.Find(t.indices,GeometryMetadata::IndexBytes(t));
            const auto position=index_.Find(t.vertices,GeometryMetadata::VertexBytes(t));
            if(!index||!position)return false;
            out={t.indices,t.vertices,t.stride,index,position,GeometryMetadata::IndexSize(t),t.indexCount,t.vertexCount};return true;
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
        const PTShaderTableAdapter::Exports& exports,Lease& output) noexcept {
        Lease next;
        try {std::lock_guard<std::mutex> guard(mutex_);
            if(!healthy_)return GISceneProducerError::Identity;
            const auto result=PrepareGISceneProducer(history,cut,epoch,top,instances,perInstance,geometry,index_,native,bindings,pipeline,exports,next.plan);
            if(result!=GISceneProducerError::None)return result;
            auto tokens=next.plan.allocationTokens;tokens.push_back(top.token);
            for(const auto& bottom:next.plan.closure.bottoms)tokens.push_back(bottom.token);
            std::sort(tokens.begin(),tokens.end());tokens.erase(std::unique(tokens.begin(),tokens.end()),tokens.end());
            next.buffers.reserve(tokens.size());
            for(const auto token:tokens){const auto found=tokens_.find(token);if(found==tokens_.end())return GISceneProducerError::Identity;
                next.buffers.push_back(found->second->storage);}
            const auto matches=[&](const SceneASReference& ref){return index_.Find(ref.address,256)==ref.token;};
            if(!matches(top))return GISceneProducerError::Identity;
            for(const auto& bottom:next.plan.closure.bottoms)if(!matches(bottom))return GISceneProducerError::Identity;
        }catch(...){return GISceneProducerError::Allocation;}
        output=std::move(next);return GISceneProducerError::None;
    }
    bool Acquire(U64 token,BufferStorageLease& output) noexcept {
        BufferStorageLease next;
        {std::lock_guard<std::mutex> guard(mutex_);if(!healthy_)return false;
            const auto found=tokens_.find(token);if(found==tokens_.end())return false;next=found->second->storage;}
        output=std::move(next);return true;
    }
    U64 RetainedBytes() {std::lock_guard<std::mutex> guard(mutex_);return bytes_;}
};
}
