// Portable control-flow fixtures. Native calls, D3D, shader reflection and SEH
// are mocks; actual production prepare/draw ABI/capture orchestration is run.
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <vector>
#include <string>
#include <utility>
#include <functional>
using DWORD=uint32_t; using UINT=unsigned; using UINT64=uint64_t;
using SIZE_T=size_t; using HRESULT=int;
static_assert(sizeof(DWORD)==4 && sizeof(HRESULT)==4 && sizeof(void*)==8,
              "Fixtures require Windows DWORD/HRESULT widths and 64-bit pointers");
constexpr HRESULT S_OK=0,S_FALSE=1,E_INVALIDARG=-1,E_FAIL=-2,E_UNEXPECTED=-3,E_OUTOFMEMORY=-4;
#define HRESULT_FROM_WIN32(x) (-int(x))
using HANDLE=void*;
static unsigned workerCreates=0, inputReads=0, guideCopies=0, guideStops=0;
static std::vector<std::string> aaEvents;
static bool workerOkay=true, inputOkay=true, acquireOkay=true, cameraOkay=true, guideCopyOkay=true, guideCommands=true;
static std::atomic<unsigned long long> aaCount{0};
#define FAILED(x) ((x)<0)
#define SUCCEEDED(x) ((x)>=0)
#define IID_PPV_ARGS(x) (x)
#define __try try
#define __except(x) catch(...)
#define EXCEPTION_EXECUTE_HANDLER 1
static DWORD GetExceptionCode(){return 0xE0000001;}
static DWORD lastError=0;
static DWORD GetLastError(){return lastError;}
static void SetLastError(DWORD v){lastError=v;}
static void* verifiedRenderer=nullptr;
static std::array<const unsigned char*,4096> tlsSlots{};
static bool tlsArrayPresent=true;
static uint64_t __readgsqword(unsigned x){if(x!=0x58) std::abort(); return tlsArrayPresent?reinterpret_cast<uint64_t>(tlsSlots.data()):0;}
#include "rr_albedo_prepare.h"
#include "rr_albedo_draw_abi.h"

struct D3D12_RESOURCE_DESC {int Dimension=0; UINT64 Width=0; UINT Height=0,DepthOrArraySize=0,MipLevels=0;struct{UINT Count=0;} SampleDesc;int Layout=0;};
struct D3D12_HEAP_PROPERTIES{int Type=0;UINT CreationNodeMask=0,VisibleNodeMask=0;};
struct D3D12_PLACED_SUBRESOURCE_FOOTPRINT{UINT64 Offset=0;struct{UINT RowPitch=0;}Footprint;};
struct D3D12_RANGE{SIZE_T Begin,End;};
enum{D3D12_HEAP_TYPE_READBACK,D3D12_RESOURCE_DIMENSION_BUFFER,D3D12_TEXTURE_LAYOUT_ROW_MAJOR,D3D12_HEAP_FLAG_NONE,D3D12_RESOURCE_STATE_COPY_DEST};
struct ID3D12Device;
struct ID3D12Resource{ID3D12Device*device=nullptr; D3D12_RESOURCE_DESC desc{}; D3D12_RESOURCE_DESC GetDesc(){return desc;} HRESULT GetDevice(ID3D12Device**out){*out=device;return S_OK;} HRESULT Map(int,D3D12_RANGE*,void**out){*out=this;return S_OK;}void Unmap(int,D3D12_RANGE*){}};
static ID3D12Resource fakeReadback;
struct ID3D12Device{
 void GetCopyableFootprints(const D3D12_RESOURCE_DESC*d,UINT,UINT,UINT64,D3D12_PLACED_SUBRESOURCE_FOOTPRINT*f,UINT*rows,UINT64*rowBytes,UINT64*bytes){*rows=d->Height;*rowBytes=d->Width*8;*bytes=*rows**rowBytes;f->Footprint.RowPitch=UINT(*rowBytes);}
 HRESULT CreateCommittedResource(D3D12_HEAP_PROPERTIES*,int,D3D12_RESOURCE_DESC*,int,void*,ID3D12Resource**out){*out=&fakeReadback;return S_OK;}
};
struct ID3D12CommandQueue{};
static ID3D12Device deviceA,deviceB;
static ID3D12CommandQueue queueA,queueB;
static ID3D12Resource fakeTarget;
struct CameraSnapshot{void*renderer=nullptr;void*view=nullptr;unsigned long long engineFrame=0; float viewToWorld[12]{1,0,0,0,1,0,0,0,1,0,0,0};};
struct RRGuideInputSnapshot{unsigned long long engineFrame=0,presentToken=0; DWORD fault=0; CameraSnapshot camera{};struct{ID3D12Resource*resource=nullptr; D3D12_RESOURCE_DESC desc{};}gbuffer1;};
struct RRGuideJob{unsigned width=64,height=32,preparationAttempt=0;bool retireOnly=false; ID3D12Device*device=&deviceA;ID3D12CommandQueue*queue=&queueA; RRGuideInputSnapshot input{}; ID3D12Resource*textures[3]{};};
enum class RRGuideStage{Waiting,Preparing,Prepared,Copied,Pending,Writing,Finished,Stopped,Other};
static unsigned int rrGuideAttempts=0;
static unsigned long long rrGuideLastAttempt=0;
static RRGuideStage rrGuideStage=RRGuideStage::Prepared;
static RRGuideJob*rrGuideJob=nullptr;
static std::atomic<bool>rrGuideInactive{false};
static bool guideLock=true;
struct RRGuideEntry{bool locked=guideLock;};
constexpr uint64_t kRRGuideMaxPixels=64*1024*1024;
static struct{long long QuadPart=1000000;}frequency;
static long long ticks=0;
static long long RRGuideQpc(){return ++ticks;}
static double RRGuideElapsedMs(long long start){return double(ticks-start)/1000.;}
static void Log(const char*,...){}
template<class T>static void RRGuideRelease(T*&p){p=nullptr;}
static unsigned long long liveFrame=100;
static bool frameReadable=true,cameraReadable=true;
static CameraSnapshot liveCamera{};
static bool ReadEngineFrameSafe(unsigned long long*out,DWORD*fault){*fault=0;*out=liveFrame;return frameReadable;}
static bool ReadCamera(CameraSnapshot*out,DWORD*fault,const char**){*fault=0;*out=liveCamera;return cameraReadable;}
static bool RRGuideReadRendererContext(RRGuideInputSnapshot*out,const char**){out->engineFrame=liveFrame;return true;}
static ID3D12Device* acquiredDevice=&deviceA;
static ID3D12CommandQueue* acquiredQueue=&queueA;
static HRESULT RRGuideAcquireDeviceQueue(RRGuideJob*out,const RRGuideInputSnapshot*,const char**reason,DWORD*){out->device=acquiredDevice;out->queue=acquiredQueue;*reason="mock_device_failure";return acquireOkay?S_OK:E_FAIL;}
static bool RRGuideReadInputs(RRGuideInputSnapshot*out,const char**reason){++inputReads;out->engineFrame=liveFrame;out->presentToken=91;out->camera=liveCamera;out->gbuffer1.resource=&fakeTarget;out->gbuffer1.desc.Width=64;out->gbuffer1.desc.Height=32;*reason="mock_inputs_failure";return inputOkay;}
static bool RRGuideCameraValid(const CameraSnapshot&){return cameraOkay;}
static bool RRGuideCopyInputs(RRGuideInputSnapshot*,ID3D12Resource*,ID3D12Resource*,const char**reason,bool*commands,DWORD*){aaEvents.push_back("guide-copy");++guideCopies;*commands=guideCommands;*reason="mock_source_preflight_or_recording_failure";return guideCopyOkay;}
static DWORD RRGuidePrepareWorker(void*) noexcept {return 0;}
static HANDLE CreateThread(void*,size_t,DWORD(*)(void*)noexcept,void*,DWORD,DWORD*){++workerCreates;return workerOkay?reinterpret_cast<HANDLE>(uintptr_t(1)):nullptr;}
static void CloseHandle(HANDLE){}
static constexpr const char*kRRPrimaryGuideShaderSha256="mock_fixture_shader";
static bool RRAlbedoCapturePending() noexcept;
static bool RRAlbedoAllowGuideCapture(unsigned long long) noexcept;
static bool RRAlbedoCopyForGuide(RRGuideJob*,const char**) noexcept;
#include "scheduler_under_test.inc"

enum class RRGuideImageSemantic{NativeAlbedoTarget0,NativeAlbedoTarget1};
enum class RRGuideImageFormat{Rgba16Float};
struct RRGuideExportImageView{RRGuideImageSemantic semantic{};RRGuideImageFormat format{};unsigned width=0,height=0;const unsigned char*data=nullptr;unsigned rowPitch=0;unsigned long long sourceFrame=0;};
struct RRGuideExportShaderView{const unsigned char*data=nullptr;size_t bytes=0;unsigned long long sourceFrame=0;unsigned renderTargetCount=0;};
struct RRGuideExportView{unsigned long long engineFrame=0;RRGuideExportImageView*additionalImages=nullptr;unsigned additionalImageCount=0,albedoSourceBatches=0,albedoAcceptedBatches=0,albedoReplayedBatches=0,albedoDrawRanges=0,albedoSkippedRanges=0;RRGuideExportShaderView*shaders=nullptr;size_t shaderCount=0;};
namespace RRAlbedoShader{
 struct Result{unsigned targetCount=1,selectedIndex=0;};
 struct Shader{std::vector<unsigned char>bytes;struct{unsigned targetCount=1;}contract;};
 struct Session{std::vector<Shader>shaders;bool ValidatePair(uintptr_t,uintptr_t,Result&r,const char**){r=Result{};return true;} const std::vector<Shader>&PixelShaders()const{return shaders;}};
}
struct RRAlbedoNativeAPI{};
struct RRAlbedoNativeTarget{ID3D12Resource*resource=nullptr;};
struct RRAlbedoNativeBindings{bool workerContext=false;};
static bool workerMode=false,bindingsOkay=true,bindOkay=true,restoreOkay=true,copyOkay=true;
static unsigned nativeCreates=0,nativeClears=0,nativeBinds=0,nativeDraws=0,nativeRestores=0,nativeCopies=0,originalJoins=0;
static bool RRAlbedoNativeCreate(RRAlbedoNativeAPI*,unsigned w,unsigned h,RRAlbedoNativeTarget*out,const char**,DWORD*){++nativeCreates;fakeTarget.device=&deviceA;fakeTarget.desc.Width=w;fakeTarget.desc.Height=h;out->resource=&fakeTarget;return true;}
static bool RRAlbedoNativePrepareClear(RRAlbedoNativeAPI*,RRAlbedoNativeTarget*,DWORD*){++nativeClears;return true;}
static bool RRAlbedoNativeReadBindings(RRAlbedoNativeAPI*,RRAlbedoNativeBindings*out,const char**,DWORD*){out->workerContext=workerMode;return bindingsOkay;}
static bool RRAlbedoNativeBind(RRAlbedoNativeAPI*,RRAlbedoNativeBindings*,RRAlbedoNativeTarget**,unsigned,bool*changed,DWORD*){++nativeBinds;*changed=true;return bindOkay;}
static bool RRAlbedoNativeRestore(RRAlbedoNativeAPI*,RRAlbedoNativeBindings*,DWORD*){++nativeRestores;return restoreOkay;}
static bool RRAlbedoNativeCopyAfterJoined(RRAlbedoNativeAPI*,RRAlbedoNativeTarget**,unsigned,ID3D12Resource**,D3D12_PLACED_SUBRESOURCE_FOOTPRINT*,RRGuideInputSnapshot*,const char**,bool*commands,DWORD*){++nativeCopies;*commands=true;return copyOkay;}
// Exact production body; only its four dependency includes are removed by run.py.
#include "capture_under_test.inc"
#include "aa_mocks.inc"

static unsigned tests=0;
static void check(bool good,const char*message){if(!good){std::fprintf(stderr,"FAIL %s\n",message);std::exit(1);}}
static void test(const char*name,const std::function<void()>&fn){fn();++tests;std::printf("PASS %s\n",name);}
template<class T>static void put(void*p,size_t offset,T v){std::memcpy(static_cast<unsigned char*>(p)+offset,&v,sizeof v);}
static uintptr_t addr(const void*p){return reinterpret_cast<uintptr_t>(p);}

struct Fixture{
 std::vector<unsigned char>module=std::vector<unsigned char>(0x912000);
 std::array<unsigned char,0x700>renderer{};
 std::array<unsigned char,0x50>manager{};
 std::array<unsigned char,24>prepareCallback{},joinCallback{},tls{};
 std::array<uintptr_t,4>primary{},camera{},other{};
 std::array<unsigned char,0xA00>material{};
 std::array<unsigned char,0x250>mainTechnique{},albedoTechnique{};
 std::array<unsigned char,0x80>mainShader{},albedoShader{};
 control_rr_albedo::OpaqueBatch batch{};
 uintptr_t resources=0xA000,instance=0xB000;
 RRGuideJob job{};
 Fixture(){
  rrAlbedoCapture=nullptr;rrAlbedoStage=RRAlbedoStage::Waiting;rrGuideStage=RRGuideStage::Prepared;rrGuideInactive=false;guideLock=true;
  aaCount=0; aaEvents.clear(); aaResult=true; aaFailures=0; lastAaObservedPresent=presentCount.load(); baselineSampleCount=transitionSampleCount=sampleCount=0; rrGuideAttempts=0; rrGuideLastAttempt=0;
  workerCreates=inputReads=guideCopies=guideStops=0;
  workerOkay=inputOkay=acquireOkay=cameraOkay=guideCopyOkay=guideCommands=true;
  acquiredDevice=&deviceA;acquiredQueue=&queueA;
  verifiedRenderer=module.data();tlsSlots.fill(nullptr);tlsArrayPresent=true;
  put(module.data(),0x80283C,DWORD(3));put(module.data(),0x7EC152,(unsigned char)1);
  primary[0]=camera[0]=other[0]=addr(module.data())+0x62E1F0;
  put(tls.data(),8,primary.data());tlsSlots[3]=tls.data();
  put(renderer.data(),0x80,addr(manager.data()));put(renderer.data(),0x678,camera.data());
  put(prepareCallback.data(),0,addr(module.data())+control_rr_albedo::kPreparationVtable);put(prepareCallback.data(),8,addr(renderer.data()));
  put(joinCallback.data(),0,addr(module.data())+0x631C08);put(joinCallback.data(),8,addr(renderer.data()));put(joinCallback.data(),16,primary.data());
  static const char name[]="StandardMaterial.rfx";
  put(material.data(),0x9A0,addr(mainTechnique.data()));put(material.data(),0x9D8,addr(albedoTechnique.data()));
  put(mainTechnique.data(),0x100,addr(name));put(mainTechnique.data(),4,uint32_t(0x02000000));put(mainTechnique.data(),0x240,uint32_t(1024));
  put(albedoTechnique.data(),4,uint32_t(0x02000002));put(albedoTechnique.data(),0x240,uint32_t(128));
  put(mainShader.data(),4,uint32_t(0x02000000));put(albedoShader.data(),4,uint32_t(0x02000002));put(albedoShader.data(),0x10,uintptr_t(1));put(albedoShader.data(),0x50,uintptr_t(2));
  batch.shader=addr(mainShader.data());batch.mesh=1;batch.material=addr(material.data());batch.instanceCount=1;
  put(manager.data(),0x30,addr(&batch));put(manager.data(),0x38,uint32_t(1));put(manager.data(),0x40,addr(&instance));put(manager.data(),0x48,uint32_t(1));
  liveFrame=100;frameReadable=true;cameraReadable=true;liveCamera={renderer.data(),camera.data(),100};
  job.input.engineFrame=100;job.input.camera=liveCamera;rrGuideJob=&job;
  workerMode=false;bindingsOkay=bindOkay=restoreOkay=copyOkay=true;
  nativeCreates=nativeClears=nativeBinds=nativeDraws=nativeRestores=nativeCopies=originalJoins=0;
  rrAlbedoSerialJoinView=nullptr;rrAlbedoWaitStart=0;lastError=123;
  rrAlbedoOriginalDraw=[](void*,int,int){++nativeDraws;};
  rrAlbedoOriginalJoin=[](void*,void*){++originalJoins;};
  current=this;
  rrAlbedoLookupShader=[](void*tech,unsigned){return tech==current->mainTechnique.data()?static_cast<void*>(current->mainShader.data()):static_cast<void*>(current->albedoShader.data());};
 }
 ~Fixture(){if(rrGuideJob&&rrGuideJob!=&job)delete rrGuideJob;delete rrAlbedoCapture;rrAlbedoCapture=nullptr;rrGuideJob=nullptr;verifiedRenderer=nullptr;current=nullptr;}
 static Fixture*current;
 bool readPrep(control_rr_albedo::PreparationState&out){const char*reason=nullptr;DWORD fault=999;return control_rr_albedo::ReadPreparationState(addr(module.data()),prepareCallback.data(),&resources,&out,&reason,&fault);}
 void prepare(){RRAlbedoPrepareCapture(prepareCallback.data(),&resources);check(rrAlbedoStage==RRAlbedoStage::Recording,"valid preparation reaches recording");}
 void schedulerFrame(unsigned long long n=1754){liveFrame=n;liveCamera.engineFrame=n;job.input.engineFrame=n;job.input.camera=liveCamera;aaCount=240;}
 void replay(){rrAlbedoSerialJoinView=primary.data();RRAlbedoReplayRange(manager.data(),0,1);}
 void joined(){prepare();replay();RRAlbedoHookJoin(joinCallback.data(),&resources);check(rrAlbedoStage==RRAlbedoStage::Joined,"valid join");}
};
Fixture*Fixture::current=nullptr;

int main(){
 test("equal primary and camera identities remain supported",[]{Fixture f;liveCamera.view=f.primary.data();f.job.input.camera=liveCamera;f.joined();const char*reason=nullptr;check(rrAlbedoCapture->primaryView==rrAlbedoCapture->cameraView&&RRAlbedoCopyForGuide(&f.job,&reason)&&nativeCopies==1,"equal identity valid branch");});
 test("distinct primary and camera captured",[]{Fixture f;f.prepare();check(rrAlbedoCapture->primaryView==f.primary.data()&&rrAlbedoCapture->cameraView==f.camera.data()&&f.primary.data()!=f.camera.data(),"separate identities");check(nativeCreates==1&&nativeClears==1,"one native target prepared");});
 test("serial replay accepts primary distinct from camera",[]{Fixture f;f.prepare();f.replay();check(nativeDraws==1&&nativeBinds==1&&rrAlbedoCapture->ranges==1,"serial replay reached draw");});
 test("worker replay accepts captured primary",[]{Fixture f;f.prepare();workerMode=true;RRAlbedoReplayRange(f.manager.data(),0,1);check(nativeDraws==1,"worker replay reached draw");});
 test("serial camera identity rejected as primary",[]{Fixture f;f.prepare();rrAlbedoSerialJoinView=f.camera.data();RRAlbedoReplayRange(f.manager.data(),0,1);check(!nativeBinds&&rrAlbedoCapture->rejectedRanges==1,"camera cannot substitute primary");});
 test("serial mismatched primary rejected",[]{Fixture f;f.prepare();rrAlbedoSerialJoinView=f.other.data();RRAlbedoReplayRange(f.manager.data(),0,1);check(!nativeBinds,"other serial view rejected");});
 test("worker mismatched primary rejected",[]{Fixture f;f.prepare();workerMode=true;put(f.tls.data(),8,f.other.data());RRAlbedoReplayRange(f.manager.data(),0,1);check(!nativeBinds&&rrAlbedoCapture->rejectedRanges==1,"other worker view rejected");});
 test("worker invalid view vtable rejected",[]{Fixture f;f.prepare();workerMode=true;f.primary[0]=1;RRAlbedoReplayRange(f.manager.data(),0,1);check(!nativeBinds,"invalid worker vtable rejected");});
 test("worker missing TLS rejected",[]{Fixture f;f.prepare();workerMode=true;tlsSlots[3]=nullptr;RRAlbedoReplayRange(f.manager.data(),0,1);check(!nativeBinds,"missing worker TLS rejected");});
 test("replay manager mismatch rejected",[]{Fixture f;f.prepare();RRAlbedoReplayRange(f.renderer.data(),0,1);check(!nativeBinds,"manager gate");});
 test("replay frame mismatch rejected",[]{Fixture f;f.prepare();++liveFrame;f.replay();check(!nativeBinds,"frame gate");});
 test("replay unreadable frame rejected",[]{Fixture f;f.prepare();frameReadable=false;f.replay();check(!nativeBinds,"frame read gate");});
 test("replay prior draw fault rejected",[]{Fixture f;f.prepare();rrAlbedoCapture->drawFault=true;f.replay();check(!nativeBinds,"draw fault gate");});
 test("replay invalid draw range rejected",[]{Fixture f;f.prepare();rrAlbedoSerialJoinView=f.primary.data();RRAlbedoReplayRange(f.manager.data(),1,2);check(!nativeBinds,"range gate");});
 test("replay binding read failure rejected",[]{Fixture f;f.prepare();bindingsOkay=false;f.replay();check(!nativeBinds,"binding read gate");});
 test("replay restores last error",[]{Fixture f;f.prepare();f.replay();check(lastError==123,"last error preserved");});
 test("join primary completes with separate camera",[]{Fixture f;f.joined();check(originalJoins==1,"original join forwarded");});
 test("join camera identity rejected",[]{Fixture f;f.prepare();f.replay();put(f.joinCallback.data(),16,f.camera.data());RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Recording&&originalJoins==1,"join camera mismatch");});
 test("join different valid primary rejected",[]{Fixture f;f.prepare();f.replay();put(f.joinCallback.data(),16,f.other.data());RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Recording,"join primary mismatch");});
 test("join invalid callback vtable rejected",[]{Fixture f;f.prepare();f.replay();put(f.joinCallback.data(),0,uintptr_t(1));RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Recording,"join callback gate");});
 test("join renderer mismatch rejected",[]{Fixture f;f.prepare();f.replay();std::array<unsigned char,0x100>renderer2{};put(renderer2.data(),0x80,addr(f.manager.data()));put(f.joinCallback.data(),8,addr(renderer2.data()));RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Recording,"join renderer gate");});
 test("join manager mismatch rejected",[]{Fixture f;f.prepare();f.replay();put(f.renderer.data(),0x80,uintptr_t(0x1234));RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Recording,"join manager gate");});
 test("join frame mismatch fails capture",[]{Fixture f;f.prepare();f.replay();++liveFrame;RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Failed,"join frame gate");});
 test("join without completed replay fails capture",[]{Fixture f;f.prepare();RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Failed,"join replay gate");});
 test("join scopes serial primary during original callback",[]{Fixture f;f.prepare();rrAlbedoSerialJoinView=f.other.data();rrAlbedoOriginalJoin=[](void*,void*){check(rrAlbedoSerialJoinView==Fixture::current->primary.data(),"scoped original primary");Fixture::current->replay();++originalJoins;};RRAlbedoHookJoin(f.joinCallback.data(),&f.resources);check(rrAlbedoStage==RRAlbedoStage::Joined&&rrAlbedoSerialJoinView==f.other.data(),"scope restored");});
 test("copy accepts paired camera distinct from primary",[]{Fixture f;f.joined();const char*reason=nullptr;check(RRAlbedoCopyForGuide(&f.job,&reason)&&nativeCopies==1&&rrAlbedoStage==RRAlbedoStage::Copied,"paired copy");});
 test("copy primary substituted for camera rejected",[]{Fixture f;f.joined();f.job.input.camera.view=f.primary.data();const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy camera identity gate");});
 test("copy different camera rejected",[]{Fixture f;f.joined();f.job.input.camera.view=f.other.data();const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy changed camera");});
 test("copy frame mismatch rejected",[]{Fixture f;f.joined();++f.job.input.engineFrame;const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy frame gate");});
 test("copy width mismatch rejected",[]{Fixture f;f.joined();++f.job.width;const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy width gate");});
 test("copy height mismatch rejected",[]{Fixture f;f.joined();++f.job.height;const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy height gate");});
 test("copy device mismatch rejected",[]{Fixture f;f.joined();f.job.device=&deviceB;const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy device gate");});
 test("copy queue mismatch rejected",[]{Fixture f;f.joined();f.job.queue=&queueB;const char*reason=nullptr;check(!RRAlbedoCopyForGuide(&f.job,&reason)&&!nativeCopies,"copy queue gate");});
 test("invalid primary TLS blocks preparation allocation",[]{Fixture f;tlsSlots[3]=nullptr;RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates&&rrAlbedoStage==RRAlbedoStage::Waiting,"invalid TLS before owner/native allocations");});
 test("invalid primary vtable blocks preparation allocation",[]{Fixture f;f.primary[0]=1;RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates&&rrAlbedoStage==RRAlbedoStage::Waiting,"invalid vtable before owner/native allocations");});
 test("null primary blocks preparation allocation",[]{Fixture f;put(f.tls.data(),8,static_cast<void*>(nullptr));RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"null primary before allocation");});
 test("persistent camera frame mismatch blocks allocation",[]{Fixture f;++liveCamera.engineFrame;RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"camera frame gate");});
 test("persistent camera renderer mismatch blocks allocation",[]{Fixture f;liveCamera.renderer=f.manager.data();RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"camera renderer gate");});
 test("preparation wireframe blocks allocation",[]{Fixture f;put(f.module.data(),0x802A33,(unsigned char)1);RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"wireframe gate");});
 test("preparation secondary wireframe blocks allocation",[]{Fixture f;put(f.module.data(),0x802A6A,(unsigned char)1);RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"altered depth gate");});
 test("preparation disabled primary blocks allocation",[]{Fixture f;put(f.module.data(),0x7EC152,(unsigned char)0);RRAlbedoPrepareCapture(f.prepareCallback.data(),&f.resources);check(!rrAlbedoCapture&&!nativeCreates,"primary enable gate");});
 test("primary read null output safe",[]{Fixture f;DWORD fault=999;check(!control_rr_albedo::ReadPrimaryView(addr(f.module.data()),nullptr,&fault)&&fault==0,"null output");});
 test("primary read null module clears output",[]{Fixture f;void*out=f.other.data();DWORD fault=999;check(!control_rr_albedo::ReadPrimaryView(0,&out,&fault)&&!out&&fault==0,"null module");});
 test("primary read absent TLS array clears output",[]{Fixture f;tlsArrayPresent=false;void*out=f.other.data();DWORD fault=999;check(!control_rr_albedo::ReadPrimaryView(addr(f.module.data()),&out,&fault)&&!out&&fault==0,"absent TLS array");});
 test("primary TLS index upper bound accepted",[]{Fixture f;put(f.module.data(),0x80283C,DWORD(4095));tlsSlots[4095]=f.tls.data();void*out=nullptr;check(control_rr_albedo::ReadPrimaryView(addr(f.module.data()),&out,nullptr)&&out==f.primary.data(),"TLS index 4095");});
 test("primary TLS index out of bounds rejected",[]{Fixture f;put(f.module.data(),0x80283C,DWORD(4096));void*out=f.other.data();check(!control_rr_albedo::ReadPrimaryView(addr(f.module.data()),&out,nullptr)&&!out,"TLS index 4096");});
 test("failed primary prep clears partially populated state",[]{Fixture f;tlsSlots[3]=nullptr;control_rr_albedo::PreparationState out;check(!f.readPrep(out)&&!out.renderer&&!out.manager&&!out.resourceTable&&!out.primaryView&&!out.parallel,"failed prep zeroed");});
 test("parallel flag preserved with primary view",[]{Fixture f;put(f.module.data(),0x911830,(unsigned char)1);control_rr_albedo::PreparationState out;check(f.readPrep(out)&&out.parallel&&out.primaryView==f.primary.data(),"parallel preparation");});
 #include "scheduler_cases.inc"
 std::printf("RESULT passed=%u failed=0\n",tests);
}
