#pragma once
#include "pt_native_root_layout.h"
// The hook adapter identifies the exact reviewed shader before insertion. The
// record joins that pipeline object to its verified native global root; pointer
// reuse cannot turn an unrelated PSO into an admitted native pass.
namespace PTNativePass {
template<class T> struct Pipeline {
    Microsoft::WRL::ComPtr<T> object;
    Microsoft::WRL::ComPtr<IUnknown> identity;
    std::shared_ptr<const PTNativeRoot::Layout> root;
};
template<class T> class Pipelines {
    std::mutex mutex;
    std::map<IUnknown*,std::shared_ptr<const Pipeline<T>>> entries;
public:
    bool Insert(IUnknown* created,const std::shared_ptr<const PTNativeRoot::Layout>& root){
        if(!created||!root||root->Type()!=PTNativeRoot::Kind::compute)return false;
        auto pending=std::make_shared<Pipeline<T>>();pending->root=root;
        if(FAILED(created->QueryInterface(IID_PPV_ARGS(&pending->object)))||FAILED(created->QueryInterface(IID_PPV_ARGS(&pending->identity))))return false;
        std::lock_guard<std::mutex> guard(mutex);if(entries.size()>=64)return false;
        auto* key=pending->identity.Get();return entries.emplace(key,std::move(pending)).second;
    }
    std::shared_ptr<const Pipeline<T>> Find(T* object){
        if(!object)return {};Microsoft::WRL::ComPtr<IUnknown> identity;
        if(FAILED(object->QueryInterface(IID_PPV_ARGS(&identity))))return {};
        std::lock_guard<std::mutex> guard(mutex);const auto found=entries.find(identity.Get());return found==entries.end()?nullptr:found->second;
    }
};
}
