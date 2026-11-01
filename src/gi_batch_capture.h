#pragma once
#include "gi_batch_capture_policy.h"
// ROI readbacks of physical native array slices. No lighting shader edits.
struct GIBatchCapture {
 struct Batch {
  ID3D12Resource* sources[3]{};
  D3D12_RESOURCE_DESC descriptions[3]{};
  D3D12_RESOURCE_STATES states[3]{};
  unsigned pixelBytes[3]{}, x[2]{}, y=0;
  UINT64 offset[3]{}, sliceBytes[3]{};
  UINT rowPitch[3]{};
  std::uintptr_t commandList=0;
 } batches[gi_batch_capture::MaxBatches];
 gi_batch_capture::Ledger ledger{};
 ID3D12Resource* readback=nullptr;
 UINT64 used=0,present=0;
 bool enabled=false,commandsStarted=false;
};
static bool GIBatchCaptureEnabled() noexcept {
 static const bool enabled=[] {char v[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_CAPTURE",v,sizeof(v))==1&&v[0]=='1';}();
 return enabled;
}
static void GIBatchCaptureRelease(GIBatchCapture& capture) noexcept {
 for(auto& b:capture.batches)for(auto*& r:b.sources)RRGuideRelease(r);
 RRGuideRelease(capture.readback);
 capture={};
}
static HRESULT GIBatchCapturePrepare(GIBatchCapture& c,ID3D12Device* device) noexcept {
 c.enabled=GIBatchCaptureEnabled();if(!c.enabled)return S_OK;
 D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
 D3D12_RESOURCE_DESC d{};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=gi_batch_capture::Capacity;
 d.Height=d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
 return device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&d,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&c.readback));
}
static unsigned GIBatchCapturePixelBytes(DXGI_FORMAT format) noexcept {
 switch(format){
 case DXGI_FORMAT_R16_UINT:return 2;
 case DXGI_FORMAT_R32_UINT:return 4;
 case DXGI_FORMAT_R32G32_UINT:case DXGI_FORMAT_R16G16B16A16_FLOAT:return 8;
 case DXGI_FORMAT_R32G32B32A32_UINT:case DXGI_FORMAT_R32G32B32A32_FLOAT:return 16;
 default:return 0;
 }
}
static bool GIBatchCaptureExport(const GIBatchCapture& c,const std::wstring& directory) {
 if(!c.enabled)return true;
 if(c.ledger.failed||!c.ledger.sealed||!c.ledger.recorded||!c.readback||!c.used)return false;
 void* data=nullptr;D3D12_RANGE range{0,static_cast<SIZE_T>(c.used)};
 if(FAILED(c.readback->Map(0,&range,&data))||!data)return false;
 bool okay=false;
 try {
  RRGuideExportDetail::File raw(directory+L"\\gi-hit-slices.bin");
  okay=raw.Write(data,static_cast<size_t>(c.used))&&raw.Finish();
 }catch(...){okay=false;}
 D3D12_RANGE written{0,0};c.readback->Unmap(0,&written);
 if(!okay)return false;
 std::ostringstream out;
 out<<"{\"schema\":\"gi-hit-slices-r271\",\"frame\":"<<c.ledger.frame
    <<",\"expected_batches\":"<<c.ledger.expected<<",\"recorded_batches\":"<<c.ledger.recorded
    <<",\"rays_per_batch\":"<<c.ledger.rays<<",\"gpu_fence_completed\":true,\"all_batches_recorded\":true"
    <<",\"physical_array_slices\":true,\"srv_view_mapping_verified\":false,\"light_operands_captured\":false"
    <<",\"file\":\"gi-hit-slices.bin\",\"bytes\":"<<c.used<<",\"region_width\":128,\"region_height\":128,\"batches\":[";
 for(unsigned n=0;n<c.ledger.recorded;++n){const auto& b=c.batches[n];if(n)out<<',';
  out<<"{\"batch\":"<<n<<",\"command_list\":"<<b.commandList<<",\"roi_x\":["<<b.x[0]<<','<<b.x[1]<<"],\"roi_y\":"<<b.y<<",\"resources\":[";
  for(unsigned i=0;i<3;++i){if(i)out<<',';const auto& d=b.descriptions[i];
   out<<"{\"kind\":\""<<(i==0?"material":i==1?"normal_uvx":"position_uvy")<<"\",\"identity\":"<<reinterpret_cast<std::uintptr_t>(b.sources[i])
      <<",\"format\":"<<unsigned(d.Format)<<",\"width\":"<<d.Width<<",\"height\":"<<d.Height<<",\"slices\":"<<d.DepthOrArraySize
      <<",\"offset\":"<<b.offset[i]<<",\"slice_region_bytes\":"<<b.sliceBytes[i]<<",\"row_pitch\":"<<b.rowPitch[i]
      <<",\"pixel_bytes\":"<<b.pixelBytes[i]<<",\"state_before_and_after\":"<<unsigned(b.states[i])<<'}';
  }out<<"]}";
 }
 out<<"],\"layout\":\"resource, physical slice, ROI; row pitch retained\"}\n";
 RRGuideExportDetail::File metadata(directory+L"\\gi-hit-slices.json");const auto bytes=out.str();
 return metadata.Write(bytes.data(),bytes.size())&&metadata.Finish();
}
