#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <bcrypt.h>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <vector>
#include "../src/native_lighting_report.h"
static bool PTReflectionSha256(const unsigned char* data,size_t size,char* out) {
    unsigned char digest[32]{};
    if(size>MAXDWORD || BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,const_cast<PUCHAR>(data),static_cast<ULONG>(size),digest,32)<0) return false;
    for(unsigned i=0;i<32;++i) sprintf_s(out+i*2,3,"%02x",digest[i]);
    return true;
}
namespace native_lighting {
inline SessionReport report;
inline SRWLOCK reportLock=SRWLOCK_INIT;
inline std::atomic<bool> reportHealthy{false};
inline void Trace(const char*,...) {}
inline bool PatchSlot(void**,void*,void*) noexcept { return false; }
#include "../src/rt_shadow_capture.h"
}
static unsigned roots=0,states=0,srvs=0;
static unsigned char data[256];
static int object;
static HRESULT STDMETHODCALLTYPE FakeRoot(ID3D12Device*,UINT node,const void* bytes,SIZE_T size,REFIID,void** out) {
    assert(node==7 && bytes==data && size==256);++roots;*out=&object;return S_OK;
}
static HRESULT STDMETHODCALLTYPE FakeState(ID3D12Device5*,const D3D12_STATE_OBJECT_DESC* desc,REFIID,void** out) {
    assert(desc && desc->NumSubobjects==1);++states;*out=&object;return S_OK;
}
static void STDMETHODCALLTYPE FakeSrv(ID3D12Device*,ID3D12Resource* resource,const D3D12_SHADER_RESOURCE_VIEW_DESC* desc,D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    assert(resource==nullptr && desc->RaytracingAccelerationStructure.Location==4096 && handle.ptr==1234);++srvs;
}
int main() {
    using namespace native_lighting;
    namespace capture=rt_shadow_capture;
    for(unsigned i=0;i<256;++i)data[i]=static_cast<unsigned char>(i);
    assert(report.OpenDefault());
    capture::originalRoot=&FakeRoot;capture::originalState=&FakeState;capture::originalSrv=&FakeSrv;
    void* out=nullptr;
    assert(capture::RootHook(nullptr,7,data,256,__uuidof(ID3D12RootSignature),&out)==S_OK && out==&object);
    D3D12_DXIL_LIBRARY_DESC library{};library.DXILLibrary={data,256};
    D3D12_STATE_SUBOBJECT subobject{D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY,&library};
    D3D12_STATE_OBJECT_DESC state{D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE,1,&subobject};
    assert(capture::StateHook(nullptr,&state,__uuidof(ID3D12StateObject),&out)==S_OK && out==&object);
    D3D12_SHADER_RESOURCE_VIEW_DESC srv{};srv.ViewDimension=D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;srv.RaytracingAccelerationStructure.Location=4096;
    capture::SrvHook(nullptr,nullptr,&srv,{1234});
    assert(roots==1 && states==1 && srvs==1);
    assert(capture::capturedCount==1 && capture::capturedBytes==256 && reportHealthy);
    capture::Blob("direct_cs",data,capture::kBlobLimit+1); // Rejected before reading.
    assert(capture::capturedCount==1);
    HANDLE file=CreateFileW(report.Path(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    assert(file!=INVALID_HANDLE_VALUE);
    char bytes[4096]{};DWORD read=0;assert(ReadFile(file,bytes,sizeof(bytes)-1,&read,nullptr));CloseHandle(file);
    assert(strstr(bytes,"rt_blob_end sha=") && strstr(bytes,"offset=128 hex=80818283"));
    assert(CopyFileW(report.Path(),L"rt-shadow-observer-test.log",FALSE));
    puts("PASS: unchanged root/state/SRV arguments and return values; bounded deduplicated capture persisted");
}
