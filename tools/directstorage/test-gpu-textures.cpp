#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dstorage.h>
#include <wrl/client.h>
#include <vector>
#include <cstdio>
#include <cstring>
#include "../../src/directstorage_gpu_textures.h"
using Microsoft::WRL::ComPtr;
using Reader=control_storage::GpuTextureReader;
static void Check(bool value,const char* name){if(!value){printf("FAIL %s last_error=%lu\n",name,GetLastError());fflush(stdout);ExitProcess(1);}}
static void Hr(HRESULT hr,const char* name){if(FAILED(hr)){printf("FAIL %s hr=%08lx\n",name,static_cast<unsigned long>(hr));fflush(stdout);ExitProcess(2);}}
static void Wait(HANDLE event){Check(WaitForSingleObject(event,30000)==WAIT_OBJECT_0,"completion timeout");}
static ComPtr<ID3D12Resource> Resource(ID3D12Device* device,const D3D12_RESOURCE_DESC& desc,D3D12_HEAP_TYPE type,D3D12_RESOURCE_STATES state){
 D3D12_HEAP_PROPERTIES heap{};heap.Type=type;ComPtr<ID3D12Resource> result;
 Hr(device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&desc,state,nullptr,IID_PPV_ARGS(&result)),"resource");return result;
}
struct Texture {
 ComPtr<ID3D12Resource> resource;D3D12_RESOURCE_DESC desc{};
 std::vector<std::vector<unsigned char>> payloads;
};
int wmain(int argc,wchar_t** argv){
 const bool forceCpu=argc==2&&wcscmp(argv[1],L"--force-cpu")==0;
 if(forceCpu){DSTORAGE_CONFIGURATION config{};config.DisableGpuDecompression=TRUE;Hr(DStorageSetConfiguration(&config),"force CPU fixture configuration");}
 ComPtr<IDXGIFactory6> dxgi;Hr(CreateDXGIFactory2(0,IID_PPV_ARGS(&dxgi)),"DXGI");
 ComPtr<IDXGIAdapter1> adapter;ComPtr<ID3D12Device> device;
 for(UINT i=0;dxgi->EnumAdapterByGpuPreference(i,DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,IID_PPV_ARGS(&adapter))!=DXGI_ERROR_NOT_FOUND;++i){
  DXGI_ADAPTER_DESC1 desc{};adapter->GetDesc1(&desc);
  if(!(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device))))break;adapter.Reset();
 }
 if(!device){Hr(dxgi->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP");Hr(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_12_0,IID_PPV_ARGS(&device)),"device");}
 DXGI_ADAPTER_DESC1 adapterDesc{};Hr(adapter->GetDesc1(&adapterDesc),"adapter");
 ComPtr<IDStorageFactory> factory;Hr(DStorageGetFactory(IID_PPV_ARGS(&factory)),"factory");
 ComPtr<IDStorageCompressionCodec> codec;Hr(DStorageCreateCompressionCodec(DSTORAGE_COMPRESSION_FORMAT_GDEFLATE,0,IID_PPV_ARGS(&codec)),"codec");
 Reader strict,reader;Hr(strict.Initialize(factory.Get(),device.Get()),"strict reader");Hr(reader.Initialize(factory.Get(),device.Get(),true),"explicit compatibility reader");
 printf("adapter_vendor=%x software_adapter=%u force_cpu=%u compression_support=%x gpu_decompression=%u native_game_integration=0\n",adapterDesc.VendorId,unsigned((adapterDesc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)!=0),unsigned(forceCpu),unsigned(reader.CompressionSupport()),unsigned(reader.UsesGpuDecompression()));
 if(forceCpu)Check(!strict.UsesGpuDecompression(),"forced CPU must be reported");
 Texture textures[2];std::vector<Reader::Input> requests;std::vector<unsigned char> archive(713,0x5a);
 for(unsigned t=0;t<2;++t){
  auto& f=textures[t];f.desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;f.desc.Width=t?128:127;f.desc.Height=t?64:65;f.desc.DepthOrArraySize=1;f.desc.MipLevels=8;
  f.desc.Format=t?DXGI_FORMAT_BC7_UNORM:DXGI_FORMAT_R8G8B8A8_UNORM;f.desc.SampleDesc.Count=1;f.desc.Layout=D3D12_TEXTURE_LAYOUT_UNKNOWN;
  f.resource=Resource(device.Get(),f.desc,D3D12_HEAP_TYPE_DEFAULT,D3D12_RESOURCE_STATE_COMMON);
  for(UINT mip=0;mip<f.desc.MipLevels;++mip){
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp{};UINT rows=0;UINT64 rowBytes=0,total=0;device->GetCopyableFootprints(&f.desc,mip,1,0,&fp,&rows,&rowBytes,&total);
   std::vector<unsigned char> payload(static_cast<size_t>(total),0);
   for(UINT row=0;row<rows;++row)for(UINT64 col=0;col<rowBytes;++col)payload[size_t(row)*fp.Footprint.RowPitch+size_t(col)]=static_cast<unsigned char>((t*31+mip*17+row*7+col*3)%251);
   f.payloads.push_back(payload);Reader::Input in{};in.texture=f.resource.Get();in.offset=archive.size();in.subresource=mip;in.decodedBytes=static_cast<UINT32>(total);
   if(mip&1){
    in.compression=DSTORAGE_COMPRESSION_FORMAT_GDEFLATE;std::vector<unsigned char> compressed(codec->CompressBufferBound(payload.size()));size_t written=0;
    Hr(codec->CompressBuffer(payload.data(),payload.size(),DSTORAGE_COMPRESSION_DEFAULT,compressed.data(),compressed.size(),&written),"offline fixture compression");
    in.storedBytes=static_cast<UINT32>(written);archive.insert(archive.end(),compressed.begin(),compressed.begin()+written);
   }else{in.storedBytes=in.decodedBytes;archive.insert(archive.end(),payload.begin(),payload.end());}
   archive.insert(archive.end(),13,0x77);requests.push_back(in);
  }
 }
 wchar_t temp[MAX_PATH]{},path[MAX_PATH]{};Check(GetTempPathW(MAX_PATH,temp)&&GetTempFileNameW(temp,L"cfg",0,path),"temporary path");
 HANDLE file=CreateFileW(path,GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);Check(file!=INVALID_HANDLE_VALUE,"fixture file");DWORD written=0;
 Check(WriteFile(file,archive.data(),static_cast<DWORD>(archive.size()),&written,nullptr)&&written==archive.size(),"fixture write");CloseHandle(file);
 ComPtr<IDStorageFile> source;Hr(factory->OpenFile(path,IID_PPV_ARGS(&source)),"source");for(auto& in:requests)in.file=source.Get();
 DSTORAGE_REQUEST validated{};
 Hr(strict.Validate(requests[0],validated),"raw GPU input");Check(validated.Options.DestinationType==DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION,"must target texture");
 const auto compressedHr=strict.Validate(requests[1],validated);Check(strict.UsesGpuDecompression()?SUCCEEDED(compressedHr):compressedHr==DXGI_ERROR_UNSUPPORTED,"explicit CPU fallback gate");
 Reader::Token bad{};auto malformed=requests[0];++malformed.decodedBytes;Check(FAILED(reader.Submit(&malformed,1,bad))&&bad.serial==0,"wrong footprint rejects before submit");
 malformed=requests[0];malformed.offset=archive.size();Check(FAILED(reader.Submit(&malformed,1,bad)),"EOF rejects");
 Reader::Input overlap[]={requests[0],requests[0]};Check(FAILED(reader.Submit(overlap,2,bad)),"duplicate subresource rejects");
 Reader::Token first{},second{};Hr(reader.Submit(requests.data(),8,first),"first mip batch");Hr(reader.Submit(requests.data()+8,8,second),"second mip batch");
 Check(!reader.Close(),"pending destinations remain owned");Check(FAILED(reader.Submit(requests.data(),1,bad)),"pending destination cannot be overwritten");
 Check(reader.Discard(second),"discard publication only");source.Reset(); // Backend owns source until all I/O has finished.
 Wait(reader.CompletionEvent(first));Wait(reader.CompletionEvent(second));Reader::Completion result{};
 Check(reader.Poll(first,result)&&SUCCEEDED(result.result)&&result.publish&&result.requests==8,"successful batch publication");
 Check(reader.Poll(second,result)&&SUCCEEDED(result.result)&&!result.publish&&result.requests==8,"discard retires after completion");
 Check(!reader.Poll(first,result)&&!reader.Discard(second),"stale tokens rejected");
 Hr(factory->OpenFile(path,IID_PPV_ARGS(&source)),"reopen source");requests[0].file=source.Get();Reader::Token reused{};
 Hr(reader.Submit(requests.data(),1,reused),"slot reuse");Check(reused.serial!=first.serial&&!reader.Poll(first,result),"generation isolation");
 Wait(reader.CompletionEvent(reused));Check(reader.Poll(reused,result)&&result.publish&&SUCCEEDED(result.result),"reused slot completion");
 // Readback exists only in this validator, never in the runtime transport.
 ComPtr<ID3D12CommandQueue> copy;D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;Hr(device->CreateCommandQueue(&q,IID_PPV_ARGS(&copy)),"validation queue");
 ComPtr<ID3D12Fence> fence;Hr(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"validation fence");HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);Check(event!=nullptr,"validation event");
 for(unsigned t=0;t<2;++t){
  auto& f=textures[t];D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp[8]{};UINT rows[8]{};UINT64 rowBytes[8]{},total=0;
  device->GetCopyableFootprints(&f.desc,0,8,0,fp,rows,rowBytes,&total);
  D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;desc.Width=total;desc.Height=1;desc.DepthOrArraySize=1;desc.MipLevels=1;desc.SampleDesc.Count=1;desc.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  auto readback=Resource(device.Get(),desc,D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_STATE_COPY_DEST);
  ComPtr<ID3D12CommandAllocator> allocator;Hr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"allocator");
  ComPtr<ID3D12GraphicsCommandList> list;Hr(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");
  D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;barrier.Transition={f.resource.Get(),D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE};list->ResourceBarrier(1,&barrier);
  for(UINT mip=0;mip<8;++mip){D3D12_TEXTURE_COPY_LOCATION dst{},src{};dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=fp[mip];src.pResource=f.resource.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=mip;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
  Hr(list->Close(),"close list");ID3D12CommandList* lists[]={list.Get()};copy->ExecuteCommandLists(1,lists);Hr(copy->Signal(fence.Get(),t+1),"validation signal");Hr(fence->SetEventOnCompletion(t+1,event),"validation event signal");Wait(event);
  void* mapped=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(total)};Hr(readback->Map(0,&range,&mapped),"validation readback");
  for(UINT mip=0;mip<8;++mip)for(UINT row=0;row<rows[mip];++row)Check(memcmp(static_cast<unsigned char*>(mapped)+fp[mip].Offset+size_t(row)*fp[mip].Footprint.RowPitch,f.payloads[mip].data()+size_t(row)*fp[mip].Footprint.RowPitch,static_cast<size_t>(rowBytes[mip]))==0,"texture mip bytes");
  D3D12_RANGE noWrites{};readback->Unmap(0,&noWrites);
 }
 Check(reader.Close()&&strict.Close(),"retired shutdown");source.Reset();CloseHandle(event);Check(DeleteFileW(path)!=FALSE,"source handles released");
 puts("PASS R239 GPU texture batches: RGBA/BC7 mip bytes, GDeflate/raw, fallback policy, batch events, deferred discard, ownership, bounds, overlap and generation isolation; native_game_integration=0");return 0;
}
