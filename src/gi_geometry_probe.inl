// CPU descriptor inventory only. Does not read GPU memory or change AS builds.
namespace gi_geometry_probe {
using Build=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC*,UINT,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC*);
using Copy=void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList4*,D3D12_GPU_VIRTUAL_ADDRESS,D3D12_GPU_VIRTUAL_ADDRESS,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE);
struct Hook {void** table=nullptr;Build build=nullptr;Copy copy=nullptr;};
inline Hook hooks[8]{};
inline SRWLOCK lock=SRWLOCK_INIT;
inline std::atomic<unsigned> builds{0},geometries{0},copies{0};
inline bool Enabled() noexcept {static const bool enabled=[] {char b[4]{};return GetEnvironmentVariableA("CONTROLTOH_GI_GEOMETRY",b,4)==1&&b[0]=='1';}();return enabled;}
inline Hook Lookup(ID3D12GraphicsCommandList4* list) noexcept {
 Hook found{};auto** table=*reinterpret_cast<void***>(list);
 AcquireSRWLockShared(&lock);for(const auto& h:hooks)if(h.table==table){found=h;break;}ReleaseSRWLockShared(&lock);return found;
}
// Per-list TLAS metadata is independent of the startup BLAS text budget.
inline const GUID instanceKey={0x2800a981,0x2131,0x41bd,{0x98,0x11,0x11,0x82,0x43,0x01,0x00,0x01}};
struct InstanceBuild {UINT64 destination=0,source=0,instances=0;UINT count=0,flags=0,layout=0;};
struct InstanceState {UINT64 generation=0;UINT count=0;bool overflow=false;InstanceBuild records[8]{};};
inline std::atomic<UINT64> nextGeneration{1},submissionSerial{0};
inline std::atomic<unsigned> submissionReports{0};
inline InstanceState ReadInstances(ID3D12Object* list) noexcept {
 InstanceState s{};UINT bytes=sizeof(s);if(FAILED(list->GetPrivateData(instanceKey,&bytes,&s))||bytes!=sizeof(s))s={};return s;
}
inline void ResetInstances(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled()&&!gi_reuse::runtime::Enabled())return;gi_reuse::runtime::Reset(list);InstanceState s{};s.generation=nextGeneration.fetch_add(1);
 if(FAILED(list->SetPrivateData(instanceKey,sizeof(s),&s)))Trace("GI_INSTANCE_R280_FAILED reason=reset_state");
}
inline void RecordInstances(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& d) noexcept {
 auto s=ReadInstances(list);if(!s.generation)s.generation=nextGeneration.fetch_add(1);
 if(s.count<8)s.records[s.count++]={d.DestAccelerationStructureData,d.SourceAccelerationStructureData,d.Inputs.InstanceDescs,d.Inputs.NumDescs,unsigned(d.Inputs.Flags),unsigned(d.Inputs.DescsLayout)};
 else s.overflow=true;
 if(FAILED(list->SetPrivateData(instanceKey,sizeof(s),&s)))Trace("GI_INSTANCE_R280_FAILED reason=record_state");
}
inline void RequestInstances() noexcept {
 if(!Enabled()&&!gi_reuse::runtime::Enabled())return;submissionReports.store(0);Trace("GI_INSTANCE_R280_ARM metadata_only=1 submission_report_budget=32");
}
// Called immediately before the native ExecuteCommandLists call. These are CPU
// submission observations, NOT GPU-completed bytes or proven cross-queue order.
inline void BeforeExecute(ID3D12CommandQueue* queue,UINT count,ID3D12CommandList*const* lists) noexcept {
 if(!Enabled()||submissionReports.load()>=32)return;
 const auto serial=submissionSerial.fetch_add(1);
 for(UINT i=0;i<count;++i){
  if(!lists[i])continue;const auto s=ReadInstances(lists[i]);if(!s.count&&!s.overflow)continue;
  const auto report=submissionReports.fetch_add(1);if(report>=32)return;
  Trace("GI_INSTANCE_R280_SUBMIT serial=%llu queue=%p list=%p array_index=%u generation=%llu count=%u overflow=%u gpu_completion_confirmed=0 ordering_verified=0",serial,queue,lists[i],i,s.generation,s.count,unsigned(s.overflow));
  for(UINT j=0;j<s.count&&j<8;++j){const auto& b=s.records[j];
   Trace("GI_INSTANCE_R280_TLAS serial=%llu array_index=%u record=%u dst=%llu src=%llu instances_va=%llu count=%u flags=%u layout=%u instance_bytes_not_read=1",serial,i,j,b.destination,b.source,b.instances,b.count,b.flags,b.layout);
  }
  if(report==31)Trace("GI_INSTANCE_R280_LIMIT kind=submission_reports limit=32 rearm_with_F7=1");
 }
}
inline void Observe(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d) noexcept {
 if(d&&d->Inputs.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL)RecordInstances(list,*d);
 const unsigned serial=builds.fetch_add(1);
 if(serial>=512){if(serial==512)Trace("GI_GEOMETRY_R279_LIMIT kind=build limit=512 inventory_incomplete=1");return;}
 if(!d){Trace("GI_GEOMETRY_R279_NULL serial=%u",serial);return;}
 const auto& in=d->Inputs;
 Trace("GI_GEOMETRY_R279_BUILD serial=%u list=%p present=%llu dst=%llu src=%llu scratch=%llu type=%u flags=%u layout=%u count=%u cpu_descriptor_only=1",serial,list,presentCount.load(),d->DestAccelerationStructureData,d->SourceAccelerationStructureData,d->ScratchAccelerationStructureData,unsigned(in.Type),unsigned(in.Flags),unsigned(in.DescsLayout),in.NumDescs);
 if(in.Type==D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL){
  Trace("GI_GEOMETRY_R279_TLAS serial=%u instances_gpu_va=%llu instance_bytes_not_read=1",serial,in.InstanceDescs);return;
 }
 if(in.Type!=D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)return;
 if(in.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY&&in.DescsLayout!=D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS){Trace("GI_GEOMETRY_R279_UNSUPPORTED serial=%u reason=layout",serial);return;}
 if((in.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY&&!in.pGeometryDescs)||(in.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY_OF_POINTERS&&!in.ppGeometryDescs)){Trace("GI_GEOMETRY_R279_NULL serial=%u reason=geometry_array",serial);return;}
 for(UINT index=0;index<in.NumDescs;++index){
  const unsigned slot=geometries.fetch_add(1);
  if(slot>=4096){if(slot==4096)Trace("GI_GEOMETRY_R279_LIMIT kind=geometry limit=4096 inventory_incomplete=1");break;}
  const auto* g=in.DescsLayout==D3D12_ELEMENTS_LAYOUT_ARRAY?in.pGeometryDescs+index:in.ppGeometryDescs[index];
  if(!g){Trace("GI_GEOMETRY_R279_NULL serial=%u geometry=%u",serial,index);continue;}
  if(g->Type==D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES){const auto& t=g->Triangles;
   Trace("GI_GEOMETRY_R279_TRI serial=%u geometry=%u flags=%u vertex_va=%llu vertex_stride=%llu vertex_count=%u vertex_format=%u index_va=%llu index_count=%u index_format=%u transform_va=%llu",serial,index,unsigned(g->Flags),t.VertexBuffer.StartAddress,t.VertexBuffer.StrideInBytes,t.VertexCount,unsigned(t.VertexFormat),t.IndexBuffer,t.IndexCount,unsigned(t.IndexFormat),t.Transform3x4);
  }else Trace("GI_GEOMETRY_R279_OTHER serial=%u geometry=%u type=%u flags=%u",serial,index,unsigned(g->Type),unsigned(g->Flags));
 }
}
inline void STDMETHODCALLTYPE BuildHook(ID3D12GraphicsCommandList4* list,const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC* d,UINT count,const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC* info){
 const auto h=Lookup(list);gi_reuse::runtime::RecordBuild(list,d);Observe(list,d);h.build(list,d,count,info);
}
inline void STDMETHODCALLTYPE CopyHook(ID3D12GraphicsCommandList4* list,D3D12_GPU_VIRTUAL_ADDRESS dst,D3D12_GPU_VIRTUAL_ADDRESS src,D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE mode){
 const auto h=Lookup(list);gi_reuse::runtime::RecordCopy(list,dst,src,mode);const auto serial=copies.fetch_add(1);
 if(serial<512)Trace("GI_GEOMETRY_R279_COPY serial=%u list=%p present=%llu dst=%llu src=%llu mode=%u cpu_descriptor_only=1",serial,list,presentCount.load(),dst,src,unsigned(mode));
 else if(serial==512)Trace("GI_GEOMETRY_R279_LIMIT kind=copy limit=512 inventory_incomplete=1");
 h.copy(list,dst,src,mode);
}
inline void InstallList(ID3D12GraphicsCommandList* list) noexcept {
 if(!Enabled()&&!gi_reuse::runtime::Enabled())return;
 ID3D12GraphicsCommandList4* ray=nullptr;if(FAILED(list->QueryInterface(IID_PPV_ARGS(&ray))))return;
 auto** table=*reinterpret_cast<void***>(ray);AcquireSRWLockExclusive(&lock);
 Hook* available=nullptr;bool exists=false;for(auto& h:hooks){if(h.table==table)exists=true;if(!h.table&&!available)available=&h;}
 if(!exists&&available){
  *available={table,reinterpret_cast<Build>(table[72]),reinterpret_cast<Copy>(table[74])};
  const bool b=PatchSlot(&table[72],reinterpret_cast<void*>(available->build),reinterpret_cast<void*>(&BuildHook));
  const bool c=PatchSlot(&table[74],reinterpret_cast<void*>(available->copy),reinterpret_cast<void*>(&CopyHook));
  if(gi_reuse::runtime::Enabled()&&(!b||!c))gi_reuse::runtime::fault=true;
  if(!b&&!c)*available={};
  Trace("GI_GEOMETRY_R279_HOOK build=%u copy=%u read_only=1",unsigned(b),unsigned(c));
 }else if(!exists){if(gi_reuse::runtime::Enabled())gi_reuse::runtime::fault=true;Trace("GI_GEOMETRY_R279_LIMIT kind=vtable limit=8 inventory_incomplete=1");}
 ReleaseSRWLockExclusive(&lock);ray->Release();
}
} // namespace gi_geometry_probe
