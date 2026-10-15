#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <memory>
#include <mutex>
#include <cstdint>
// Lease storage ONLY. The caller must retain Buffer through both recorded-list
// references and GPU completion. Primary Assets already has that dual lifetime
// through Recording + Output::dependencies/retirement. Never recycle by frame ID.
namespace PTUploadPool {
class Buffer {
    struct Storage {
        Microsoft::WRL::ComPtr<IUnknown> device;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        UINT64 bytes=0;
    };
    struct Pool {
        std::mutex mutex;
        std::array<std::unique_ptr<Storage>,16> idle;
        std::size_t count=0;
        UINT64 bytes=0,created=0,reused=0;
    };
    static Pool& State(){
        // Process lifetime avoids static-destruction ordering against late
        // quarantined GPU owners. Idle storage is capped at 32 MiB / 16 buffers.
        static Pool* pool=new Pool;return *pool;
    }
    std::unique_ptr<Storage> storage;
    Buffer()=default;
public:
    Buffer(const Buffer&)=delete;Buffer& operator=(const Buffer&)=delete;
    struct Stats {UINT64 created,reused,idleBytes;};
    static Stats Statistics(){auto& p=State();std::lock_guard<std::mutex> lock(p.mutex);return {p.created,p.reused,p.bytes};}
    ID3D12Resource* Resource()const{return storage?storage->resource.Get():nullptr;}
    static HRESULT Acquire(ID3D12Device* device,UINT64 bytes,std::shared_ptr<Buffer>& out){
        out.reset();if(!device||!bytes||bytes>32ull*1024*1024)return E_INVALIDARG;
        const UINT64 capacity=(bytes+65535)&~UINT64(65535);
        Microsoft::WRL::ComPtr<IUnknown> identity;
        HRESULT hr=device->QueryInterface(IID_PPV_ARGS(&identity));if(FAILED(hr))return hr;
        auto result=std::shared_ptr<Buffer>(new Buffer);
        auto& p=State();
        {
            std::lock_guard<std::mutex> lock(p.mutex);std::size_t best=p.idle.size();
            for(std::size_t i=0;i<p.count;++i)if(p.idle[i]->device.Get()==identity.Get()&&p.idle[i]->bytes==capacity){best=i;break;}
            if(best<p.count){
                result->storage=std::move(p.idle[best]);p.bytes-=capacity;++p.reused;
                for(std::size_t i=best+1;i<p.count;++i)p.idle[i-1]=std::move(p.idle[i]);
                --p.count;
            }
        }
        if(!result->storage){
            auto s=std::make_unique<Storage>();s->device=identity;s->bytes=capacity;
            D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=capacity;desc.Height=1;
            desc.DepthOrArraySize=desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;
            hr=device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&s->resource));
            if(FAILED(hr))return hr;
            result->storage=std::move(s);std::lock_guard<std::mutex> lock(p.mutex);++p.created;
        }
        out=std::move(result);return S_OK;
    }
    ~Buffer(){
        if(!storage)return;
        // Only idle resources are eviction candidates. Last-use ownership still
        // waits for BOTH recorded references and GPU completion before arriving here.
        // Destroy retired COM resources after dropping the pool mutex.
        std::array<std::unique_ptr<Storage>,16> evicted;std::size_t retired=0;
        auto& p=State();std::lock_guard<std::mutex> lock(p.mutex);
        while(p.count&&(p.count==p.idle.size()||storage->bytes>32ull*1024*1024-p.bytes)){
            p.bytes-=p.idle[0]->bytes;evicted[retired++]=std::move(p.idle[0]);
            for(std::size_t i=1;i<p.count;++i)p.idle[i-1]=std::move(p.idle[i]);
            --p.count;
        }
        p.bytes+=storage->bytes;p.idle[p.count++]=std::move(storage);
    }
};
}
