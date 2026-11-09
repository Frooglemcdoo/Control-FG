// Controlled boundaries around the exact HookAA body; these do not implement FG.
static std::atomic<unsigned long long> presentCount{91},beginCount{0},hudCount{0},hudRenderCount{0};
static std::atomic<unsigned long long>lastAaObservedPresent{91},baselineSampleCount{0},transitionSampleCount{0},sampleCount{0},aaFailures{0};
static bool aaResult=true;
static void RememberAaFrame(unsigned long long,unsigned long long,unsigned long long){}
static void ExtendPathTrace(unsigned long long,unsigned){}
static void CaptureCamera(unsigned long long,const char*){}
static void CaptureTextureArguments(unsigned long long,void**,void**,const char*){}
static void MarkControlDLSSReady(unsigned long long,bool){aaEvents.push_back("mark-ready");}
static bool ReadNgxReset(int*out,unsigned*status,DWORD*fault){*out=0;*status=1;*fault=0;aaEvents.push_back("reset-read");return true;}
static void ConfigureRRPhase7OptionsForAA(unsigned long long,unsigned long long,bool,const CameraSnapshot&){aaEvents.push_back("rr-options");}
static void SubmitSLCommonConstantsForAA(unsigned long long,unsigned long long,bool,const CameraSnapshot&,bool,int){aaEvents.push_back("sl-constants");}
static void SubmitSLDepthMotionTagsForAA(unsigned long long,unsigned long long,void**,size_t){aaEvents.push_back("sl-depth-motion");}
static void CaptureSemantic(unsigned long long,void**,const char*){aaEvents.push_back("capture-semantic");}
static bool originalAA(void*,void*,void*,void*,void*,void*,void*,void*,void*,bool,double,double,float,float,float){aaEvents.push_back("original-aa");return aaResult;}
#define _countof(a) (sizeof(a)/sizeof((a)[0]))
#include "aa_under_test.inc"
