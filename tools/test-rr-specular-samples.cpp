#include "../src/rr_specular_sample_patch.h"
#include "../src/native_lighting_live_state.h"
#include <cassert>
#include <cstdio>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif
// Execute the actual emitted instruction relay against synthetic native memory.
// Return flags in the upper DWORD and the producer's R9D in the lower DWORD.
static void ExecuteLayout(bool farLayout){
 const std::size_t size=farLayout?0x82000000ull:0x10000ull;
#ifdef _WIN32
 auto* base=static_cast<unsigned char*>(VirtualAlloc(nullptr,size,MEM_RESERVE,PAGE_NOACCESS));
#else
 auto* base=static_cast<unsigned char*>(mmap(nullptr,size,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0));
 assert(base!=MAP_FAILED);
#endif
 assert(base);
 auto* memory=base+(farLayout?0x7ff00000ull:0x1000ull);
 auto* relay=base+128;
 auto* optionPage=base+(farLayout?0x80700000ull:0x3000ull);
 auto* statePage=base+(farLayout?0x81800000ull:0x5000ull);
 for(auto* page:{base,memory,optionPage,statePage}){
#ifdef _WIN32
  assert(VirtualAlloc(page,4096,MEM_COMMIT,PAGE_READWRITE)==page);
#else
  assert(mprotect(page,4096,PROT_READ|PROT_WRITE)==0);
#endif
 }
 auto* option=reinterpret_cast<unsigned*>(optionPage);
 auto* state=reinterpret_cast<control_rr::SpecularSampleObservation*>(statePage);
 auto* observedRax=reinterpret_cast<std::uint64_t*>(statePage+32);
 *state={};
 // Set known arithmetic flags, then seed RAX without changing those flags.
 memory[0]=0x31;memory[1]=0xc0;memory[2]=0x48;memory[3]=0xb8;
 const std::uint64_t sentinel=0x123456789abcdef0ull;
 std::memcpy(memory+4,&sentinel,8);memory[12]=0xf9;
 const std::uintptr_t site=reinterpret_cast<std::uintptr_t>(memory+13);
 const std::uintptr_t native=reinterpret_cast<std::uintptr_t>(option);
 if(farLayout)assert(native-reinterpret_cast<std::uintptr_t>(relay)>0x80000000ull);
 unsigned char original[7]={0x44,0x8b,0x0d,0,0,0,0};
 const auto displacement=static_cast<std::int32_t>(native-site-7);std::memcpy(original+3,&displacement,4);
 control_rr::SpecularSampleCode code;
 assert(control_rr::BuildSpecularSampleCode(original,site,native,reinterpret_cast<std::uintptr_t>(relay),reinterpret_cast<std::uintptr_t>(state),code));
 std::memcpy(memory+13,code.replacement.data(),7);
 const unsigned char tail[]={0x9c,0x58,0x48,0xc1,0xe0,0x20,0x4c,0x09,0xc8,0xc3};
 // Record restored RAX before the return-value packing overwrites it.
 memory[20]=0x48;memory[21]=0x89;memory[22]=0x05;
 const auto raxDisp=static_cast<std::int32_t>(reinterpret_cast<std::intptr_t>(observedRax)-reinterpret_cast<std::intptr_t>(memory+27));
 std::memcpy(memory+23,&raxDisp,4);
 std::memcpy(memory+27,tail,sizeof(tail));std::memcpy(relay,code.relay.data(),code.relay.size());
 control_rr::SpecularSampleCode rejected;
 assert(!control_rr::BuildSpecularSampleCode(original,site,native+4,reinterpret_cast<std::uintptr_t>(relay),reinterpret_cast<std::uintptr_t>(state),rejected));
 assert(!control_rr::BuildSpecularSampleCode(original,site,native,site+0x100000000ull,reinterpret_cast<std::uintptr_t>(state),rejected));
 original[0]=0x45;
 assert(!control_rr::BuildSpecularSampleCode(original,site,native,reinterpret_cast<std::uintptr_t>(relay),reinterpret_cast<std::uintptr_t>(state),rejected));
 for(auto* page:{base,memory}){
#ifdef _WIN32
  DWORD previous=0;assert(VirtualProtect(page,4096,PAGE_EXECUTE_READ,&previous));
  assert(FlushInstructionCache(GetCurrentProcess(),page,4096));
#else
  assert(mprotect(page,4096,PROT_READ|PROT_EXEC)==0);
#endif
 }
 using Producer=std::uint64_t(*)();auto producer=reinterpret_cast<Producer>(memory);
 for(unsigned n:{0u,1u,2u,8u,16u,17u,0xffffffffu}){
  for(unsigned request:{0u,1u,2u,4u,8u,12u,16u,17u,0xffffffffu}){
   *option=n;*state={request,0,0,0};const auto result=producer();
   const unsigned expected=n>=1&&n<=16&&request>=1&&request<=16?request:n;
   assert(static_cast<unsigned>(result)==expected);
   assert(*observedRax==sentinel);
   assert(((result>>32)&0x8c5)==0x45); // CF/PF/ZF preserved, OF/SF clear.
   assert(*option==n&&state->requested==request&&state->nativeCount==n&&state->effectiveCount==expected&&state->seen==1);
  }
 }
#ifdef _WIN32
 assert(VirtualFree(base,0,MEM_RELEASE));
#else
 assert(munmap(base,size)==0);
#endif
}
int main(){
 ExecuteLayout(false);ExecuteLayout(true);
 using namespace native_lighting;
 LiveSelection live;
 for(auto mode:{Mode::RtSpotSpec1,Mode::RtSpotSpec2,Mode::RtSpotSpec4,Mode::RtSpotSpec8,Mode::RtSpotSpec12,Mode::RtSpotSpec16}){
  specularSamplesReady.store(false);
  assert(!ModeAvailable(mode,kRtSpotReadyBit));
  assert(ModeAvailable(Mode::RtSpotWorldAlbedo,kRtSpotReadyBit));
  specularSamplesReady.store(true);
  assert(ModeAvailable(mode,kRtSpotReadyBit)&&!ModeAvailable(mode,0));
  assert(IsSpecularSampleMode(mode)&&IsRtSpotMode(mode)&&UsesWorldAlbedo(mode)&&PreservesGiFiltering(mode));
  assert(!PreservesSpecularFiltering(mode)&&!UsesRawSpotVisibility(mode)&&!UsesPerspectiveGuide(mode));
  assert(GiSampleRequest(mode,6)==6&&RequiredVariant(mode)==kRtSpotReadyBit);
  const auto selection=live.Select(mode);assert(LiveSelection::GetMode(selection)==mode&&LiveSelection::Observed(selection)==0);
 }
 assert(SpecularSampleRequest(Mode::RtSpotWorldAlbedo)==0);
 assert(SpecularSampleRequest(Mode::Native)==0);
 puts("PASS: executed specular relay; 126 near/far combinations; RAX/flags/options preserved; option and state beyond relay RIP range; invalid values pass through; exact-site/range rejection; mode policies");
}
