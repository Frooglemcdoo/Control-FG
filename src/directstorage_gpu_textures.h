#pragma once
#include <windows.h>
#include <d3d12.h>
#include <dxgi.h>
#include <climits>
#include <dstorage.h>
#include <wrl/client.h>
#include <cstdint>
#include <limits>

namespace control_storage {
// Single submission/completion owner. The native adapter must provide exclusive
// access to COMMON-state subresources and preserve engine objects until publication.
// No CPU payload destination, mapping, memcpy, or decompression is used here.
class GpuTextureReader {
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
public:
    static constexpr unsigned BatchCount=4,RequestsPerBatch=32;
    struct Input {
        IDStorageFile* file=nullptr;ID3D12Resource* texture=nullptr;
        UINT64 offset=0;UINT32 storedBytes=0,decodedBytes=0;
        UINT subresource=0;
        DSTORAGE_COMPRESSION_FORMAT compression=DSTORAGE_COMPRESSION_FORMAT_NONE;
    };
    struct Token {unsigned slot=BatchCount;UINT64 serial=0;};
    struct Completion {HRESULT result=E_PENDING;bool publish=false;unsigned requests=0;};
private:
    struct Batch {
        Ptr<IDStorageFile> files[RequestsPerBatch];Ptr<ID3D12Resource> textures[RequestsPerBatch];
        UINT subresources[RequestsPerBatch]{};
        HANDLE event=nullptr;UINT64 serial=0;unsigned count=0;bool pending=false,discard=false;
    } batches[BatchCount];
    Ptr<ID3D12Device> device;Ptr<IDStorageQueue2> queue;Ptr<IDStorageStatusArray> status;
    UINT64 nextSerial=1;bool failed=false,allowCpu=false,ready=false;
    DSTORAGE_COMPRESSION_SUPPORT compressionSupport{};
    bool Matches(Token token)const {return token.slot<BatchCount&&batches[token.slot].pending&&batches[token.slot].serial==token.serial;}
    static bool SameObject(IUnknown* a,IUnknown* b){
        Ptr<IUnknown> x,y;return a&&b&&SUCCEEDED(a->QueryInterface(IID_PPV_ARGS(&x)))&&SUCCEEDED(b->QueryInterface(IID_PPV_ARGS(&y)))&&x.Get()==y.Get();
    }
public:
    GpuTextureReader()=default;
    GpuTextureReader(const GpuTextureReader&)=delete;
    GpuTextureReader& operator=(const GpuTextureReader&)=delete;
    // allowCpuDecompression is explicit test/compatibility policy, false by default.
    HRESULT Initialize(IDStorageFactory* factory,ID3D12Device* target,bool allowCpuDecompression=false){
        if(!factory||!target||queue)return E_INVALIDARG;
        device=target;allowCpu=allowCpuDecompression;
        DSTORAGE_QUEUE_DESC desc{};desc.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;
        desc.Device=target;desc.Capacity=512;desc.Priority=DSTORAGE_PRIORITY_NORMAL;desc.Name="ControlFG GPU texture batches";
        HRESULT hr=factory->CreateQueue(&desc,IID_PPV_ARGS(&queue));if(FAILED(hr))return hr;
        hr=factory->CreateStatusArray(BatchCount,"ControlFG GPU texture completion",IID_PPV_ARGS(&status));if(FAILED(hr))return hr;
        compressionSupport=queue->GetCompressionSupport(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE);
        for(auto& b:batches){b.event=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!b.event)return HRESULT_FROM_WIN32(GetLastError());}
        ready=true;return S_OK;
    }
    DSTORAGE_COMPRESSION_SUPPORT CompressionSupport()const{return compressionSupport;}
    bool UsesGpuDecompression()const {
        return (compressionSupport&(DSTORAGE_COMPRESSION_SUPPORT_GPU_OPTIMIZED|DSTORAGE_COMPRESSION_SUPPORT_GPU_FALLBACK))!=0&&
            (compressionSupport&DSTORAGE_COMPRESSION_SUPPORT_CPU_FALLBACK)==0;
    }
    HRESULT Validate(const Input& in,DSTORAGE_REQUEST& out)const {
        if(!device||!in.file||!in.texture||!in.storedBytes||!in.decodedBytes||in.decodedBytes>32u*1024u*1024u)return E_INVALIDARG;
        if(in.compression!=DSTORAGE_COMPRESSION_FORMAT_NONE&&in.compression!=DSTORAGE_COMPRESSION_FORMAT_GDEFLATE)return E_NOTIMPL;
        if(in.compression==DSTORAGE_COMPRESSION_FORMAT_GDEFLATE&&!allowCpu&&!UsesGpuDecompression())return DXGI_ERROR_UNSUPPORTED;
        if(in.compression==DSTORAGE_COMPRESSION_FORMAT_NONE&&in.storedBytes!=in.decodedBytes)return E_INVALIDARG;
        BY_HANDLE_FILE_INFORMATION info{};HRESULT hr=in.file->GetFileInformation(&info);if(FAILED(hr))return hr;
        const UINT64 length=(UINT64(info.nFileSizeHigh)<<32)|info.nFileSizeLow;
        if(in.offset>length||in.storedBytes>length-in.offset)return E_INVALIDARG;
        Ptr<ID3D12Device> owner;hr=in.texture->GetDevice(IID_PPV_ARGS(&owner));if(FAILED(hr))return hr;
        if(!SameObject(owner.Get(),device.Get()))return E_INVALIDARG;
        const auto desc=in.texture->GetDesc();
        if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D||desc.DepthOrArraySize!=1||desc.SampleDesc.Count!=1||
            !desc.MipLevels||in.subresource>=desc.MipLevels||desc.Width>UINT_MAX)return E_INVALIDARG;
        D3D12_FEATURE_DATA_FORMAT_INFO format{desc.Format,0};
        hr=device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_INFO,&format,sizeof(format));if(FAILED(hr)||format.PlaneCount!=1)return E_INVALIDARG;
        D3D12_HEAP_PROPERTIES heap{};D3D12_HEAP_FLAGS flags{};
        hr=in.texture->GetHeapProperties(&heap,&flags);if(FAILED(hr)||heap.Type!=D3D12_HEAP_TYPE_DEFAULT)return E_INVALIDARG;
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT rows=0;UINT64 rowBytes=0,total=0;
        device->GetCopyableFootprints(&desc,in.subresource,1,0,&footprint,&rows,&rowBytes,&total);
        if(total!=in.decodedBytes||total>UINT_MAX||!rows||!rowBytes)return E_INVALIDARG;
        out={};out.Options.SourceType=DSTORAGE_REQUEST_SOURCE_FILE;
        out.Options.DestinationType=DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;out.Options.CompressionFormat=in.compression;
        out.Source.File={in.file,in.offset,in.storedBytes};
        out.Destination.Texture={in.texture,in.subresource,{0,0,0,footprint.Footprint.Width,footprint.Footprint.Height,1}};
        out.UncompressedSize=in.decodedBytes;return S_OK;
    }
    HRESULT Submit(const Input* inputs,unsigned count,Token& token){
        token={};if(!ready||!queue||!status||failed||!inputs||!count||count>RequestsPerBatch)return E_INVALIDARG;
        unsigned slot=0;for(;slot<BatchCount&&batches[slot].pending;++slot){}if(slot==BatchCount)return HRESULT_FROM_WIN32(ERROR_BUSY);
        DSTORAGE_REQUEST requests[RequestsPerBatch]{};
        // All validation precedes any submission. No partially accepted batches.
        for(unsigned i=0;i<count;++i){
            HRESULT hr=Validate(inputs[i],requests[i]);if(FAILED(hr))return hr;
            for(unsigned j=0;j<i;++j)if(SameObject(inputs[i].texture,inputs[j].texture)&&inputs[i].subresource==inputs[j].subresource)return E_INVALIDARG;
            for(const auto& b:batches)if(b.pending)for(unsigned j=0;j<b.count;++j)
                if(SameObject(inputs[i].texture,b.textures[j].Get())&&inputs[i].subresource==b.subresources[j])return HRESULT_FROM_WIN32(ERROR_BUSY);
        }
        auto& b=batches[slot];ResetEvent(b.event);b.serial=nextSerial++;b.count=count;b.discard=false;b.pending=true;
        for(unsigned i=0;i<count;++i){b.files[i]=inputs[i].file;b.textures[i]=inputs[i].texture;b.subresources[i]=inputs[i].subresource;queue->EnqueueRequest(&requests[i]);}
        queue->EnqueueStatus(status.Get(),slot);queue->EnqueueSetEvent(b.event);queue->Submit();token={slot,b.serial};return S_OK;
    }
    bool Discard(Token token){if(!Matches(token))return false;batches[token.slot].discard=true;return true;}
    // The engine adapter can register these handles with its completion worker.
    // Manual-reset events may be waited on but not reset by the adapter.
    HANDLE CompletionEvent(Token token)const{return Matches(token)?batches[token.slot].event:nullptr;}
    bool Poll(Token token,Completion& result){
        if(!Matches(token)||!status->IsComplete(token.slot))return false;
        auto& b=batches[token.slot];
        // Status precedes SetEvent. Wait for the trailing event before slot reuse,
        // otherwise a delayed SetEvent could target the next batch's generation.
        if(WaitForSingleObject(b.event,0)!=WAIT_OBJECT_0)return false;
        result.result=status->GetHResult(token.slot);result.publish=SUCCEEDED(result.result)&&!b.discard;result.requests=b.count;
        if(FAILED(result.result))failed=true;
        for(unsigned i=0;i<b.count;++i){b.files[i].Reset();b.textures[i].Reset();}
        b.count=0;b.pending=false;return true;
    }
    bool Close(){
        for(const auto& b:batches)if(b.pending)return false;
        ready=false;if(queue)queue->Close();queue.Reset();status.Reset();device.Reset();
        for(auto& b:batches)if(b.event){CloseHandle(b.event);b.event=nullptr;}return true;
    }
    // Caller must retain this object while any token is pending; never unload from DllMain.
};
}
